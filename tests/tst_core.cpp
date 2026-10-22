// Unit tests for the MAT-file reader, the TIFF / PNG / JPEG readers and the
// processing functions.

#include "core/Histogram.h"
#include "core/Processing.h"
#include "io/ImageFileReader.h"
#include "io/MatFileReader.h"
#include "io/MatFileWriter.h"
#include "io/TiffReader.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QTemporaryDir>
#include <QtTest>

#include <cmath>

using namespace iat;

namespace {

QString fixture(const char* name)
{
    return QStringLiteral(IAT_TEST_DATA_DIR "/") + QLatin1String(name);
}

QString formatFixture(const char* name)
{
    return QStringLiteral(IAT_TEST_DATA_DIR "/formats/") + QLatin1String(name);
}

// MATLAB rgb2gray weights.
constexpr double kR = 0.298936021293775;
constexpr double kG = 0.587043074451121;
constexpr double kB = 0.114020904255103;

ImageFrame readImage(const char* name)
{
    const ImageFrame f = ImageFileReader::read(formatFixture(name));
    if (!f.ok())
        qWarning() << name << f.error;
    return f;
}

MatVariable onlyStack(const QString& path)
{
    const MatFileContents c = MatFileReader::read(path);
    if (!c.ok())
        qWarning() << c.error;
    const QList<MatVariable> s = c.imageStacks();
    return s.size() == 1 ? s.first() : MatVariable();
}

} // namespace

class TestCore : public QObject {
    Q_OBJECT

private slots:
    void readsCompressedUint16()
    {
        const MatVariable v = onlyStack(fixture("light_small.mat"));
        QCOMPARE(v.name, QStringLiteral("frames"));
        QCOMPARE(v.matlabClass, QStringLiteral("uint16"));
        QCOMPARE(v.dims, (QVector<qint64>{6, 8, 4}));
        QCOMPARE(v.data.rows(), 6);
        QCOMPARE(v.data.cols(), 8);
        QCOMPARE(v.data.frameCount(), 4);
        for (int z = 0; z < 4; ++z) {
            const Image2D f = v.data.frame(z);
            QCOMPARE(f.width(), 8);
            QCOMPARE(f.height(), 6);
            for (int r = 0; r < 6; ++r)
                for (int c = 0; c < 8; ++c)
                    QCOMPARE(f.at(c, r), 1000.0 + 100 * r + 10 * c + z);
        }
    }

    void readsUncompressedDoubleAndMeansOverFrames()
    {
        const MatVariable v = onlyStack(fixture("dark_small.mat"));
        QCOMPARE(v.matlabClass, QStringLiteral("double"));
        QCOMPARE(v.data.frameCount(), 3);
        const Image2D mean = processing::meanOverFrames(v.data);
        QCOMPARE(mean.width(), 8);
        QCOMPARE(mean.height(), 6);
        for (int r = 0; r < 6; ++r)
            for (int c = 0; c < 8; ++c)
                QCOMPARE(mean.at(c, r), r + 0.5 * c + 10.0);  // mean of z*10 for z = 0,1,2
    }

    void readsDoubleStoredAsUint16()
    {
        const MatVariable v = onlyStack(fixture("double_as_uint16.mat"));
        QCOMPARE(v.name, QStringLiteral("img"));
        QCOMPARE(v.matlabClass, QStringLiteral("double"));
        QCOMPARE(v.data.elementType(), ElementType::UInt16);
        QCOMPARE(v.data.frame(3).at(7, 5), 5000.0 + 8 * 5 + 7 + 300);
        QCOMPARE(v.data.frame(0).at(0, 0), 5000.0);
    }

    void readsBigEndianFiles()
    {
        const MatVariable v = onlyStack(fixture("bigendian.mat"));
        QCOMPARE(v.name, QStringLiteral("be"));
        QCOMPARE(v.dims, (QVector<qint64>{3, 4, 2}));
        for (int z = 0; z < 2; ++z)
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 4; ++c)
                    QCOMPARE(v.data.frame(z).at(c, r), r * 1.5 - c + z * 0.25);
    }

    void reads2DArrayAsOneFrame()
    {
        const MatVariable v = onlyStack(fixture("single2d.mat"));
        QCOMPARE(v.matlabClass, QStringLiteral("single"));
        QCOMPARE(v.data.frameCount(), 1);
        QCOMPARE(v.data.frame(0).at(6, 4), (4 * 7 + 6) / 4.0);
    }

    void listsAllVariablesButOnlyLoadsImages()
    {
        const MatFileContents c = MatFileReader::read(fixture("multi.mat"));
        QVERIFY2(c.ok(), qPrintable(c.error));
        QCOMPARE(c.variables.size(), 5);
        const QList<MatVariable> stacks = c.imageStacks();
        QCOMPARE(stacks.size(), 2);
        QStringList names;
        for (const MatVariable& v : stacks)
            names << v.name;
        names.sort();
        QCOMPARE(names, (QStringList{"a", "b"}));
        for (const MatVariable& v : c.variables) {
            if (v.name == QLatin1String("txt"))
                QCOMPARE(v.matlabClass, QStringLiteral("char"));
            if (v.name == QLatin1String("s"))
                QCOMPARE(v.matlabClass, QStringLiteral("struct"));
            if (v.name == QLatin1String("a"))
                QCOMPARE(v.data.frame(1).at(3, 0), double(0 - 3 + 1));  // row 0, col 3, frame 1
        }
    }

    void statisticsIgnoreNaNAndInf()
    {
        const MatVariable v = onlyStack(fixture("nan_values.mat"));
        const ImageStats s = v.data.frame(0).computeStats();
        QCOMPARE(s.finiteCount, qint64(10));
        QCOMPARE(s.nonFiniteCount, qint64(2));
        // Values 0..11 in MATLAB order without index 0 (NaN) and 11 (Inf).
        QCOMPARE(s.min, 1.0);
        QCOMPARE(s.max, 10.0);
        QCOMPARE(s.mean, 5.5);
    }

    void rejectsV73Files()
    {
        const MatFileContents c = MatFileReader::read(fixture("v73_header.mat"));
        QVERIFY(!c.ok());
        QVERIFY(c.error.contains(QLatin1String("7.3")));
        QVERIFY(c.error.contains(QLatin1String("-v7")));
    }

    void rejectsOtherFiles()
    {
        QVERIFY(!MatFileReader::read(fixture("not_a_mat.mat")).ok());
        QVERIFY(!MatFileReader::read(fixture("does_not_exist.mat")).ok());
        QVERIFY(!MatFileReader::readFromBytes(QByteArray(50, 'x')).ok());

        // Truncated copy of a valid file.
        QFile f(fixture("light_small.mat"));
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray bytes = f.readAll();
        QVERIFY(!MatFileReader::readFromBytes(bytes.left(bytes.size() - 20)).ok());
    }

    void subtractAndStats()
    {
        Image2D a(3, 2, 10.0);
        Image2D b(3, 2, 0.0);
        b.data()[4] = 4.5;
        const Image2D d = processing::subtract(a, b);
        QCOMPARE(d.at(1, 1), 5.5);
        QCOMPARE(d.at(0, 0), 10.0);
        const ImageStats s = d.computeStats();
        QCOMPARE(s.min, 5.5);
        QCOMPARE(s.max, 10.0);
        QCOMPARE(s.mean, (5 * 10.0 + 5.5) / 6.0);
        QVERIFY(!s.integerValued);
        QVERIFY(processing::subtract(a, Image2D(2, 2)).isNull());
    }

    void integerHistogramHasWholeNumberBins()
    {
        Image2D img(1000, 1);
        for (int i = 0; i < 1000; ++i)
            img.data()[i] = 5000 + i;  // 1000 distinct integers
        const ImageStats s = img.computeStats();
        QVERIFY(s.integerValued);
        const Histogram h = computeHistogram(img, s, 512);
        QCOMPARE(h.binWidth(), 2.0);
        QCOMPARE(h.binCount(), 500);
        QCOMPARE(h.lo, 4999.5);
        for (qint64 count : h.counts)
            QCOMPARE(count, qint64(2));  // no comb artefacts
    }

    void percentiles()
    {
        Image2D img(101, 1);
        for (int i = 0; i <= 100; ++i)
            img.data()[i] = 100 - i;
        QCOMPARE(processing::percentile(img, 0.0), 0.0);
        QCOMPARE(processing::percentile(img, 0.5), 50.0);
        QCOMPARE(processing::percentile(img, 1.0), 100.0);

        const auto [lo, hi] = processing::percentileRange(img, 0.1, 0.9);
        QCOMPARE(lo, 10.0);
        QCOMPARE(hi, 90.0);
        const auto [lo2, hi2] = processing::percentileRange(Image2D(), 0.1, 0.9);
        QVERIFY(std::isnan(lo2) && std::isnan(hi2));
    }

    // Ports of calculateImageStats.m / calculateImageNoise.m.
    // Reference values from NumPy: mean(), std(ddof=0), sqrt(var(axis=2, ddof=0).mean()).
    void calculateImageStatsMatchesMatlab()
    {
        const MatVariable light = onlyStack(fixture("light_small.mat"));
        const processing::MeanStd f1 = processing::calculateImageStats(light.data.frame(0));
        QCOMPARE(f1.mean, 1285.0);
        QVERIFY(std::abs(f1.std - 172.312700247738) < 1e-9);

        const processing::MeanStd all = processing::calculateImageStats(light.data);
        QCOMPARE(all.mean, 1286.5);
        QVERIFY(std::abs(all.std - 172.316327336288) < 1e-9);

        const MatVariable dark = onlyStack(fixture("dark_small.mat"));
        const processing::MeanStd d = processing::calculateImageStats(processing::meanOverFrames(dark.data));
        QCOMPARE(d.mean, 14.25);
        QVERIFY(std::abs(d.std - 2.056493779876) < 1e-9);

        // Like MATLAB, NaN propagates.
        const MatVariable nan = onlyStack(fixture("nan_values.mat"));
        QVERIFY(std::isnan(processing::calculateImageStats(nan.data.frame(0)).mean));
        QVERIFY(std::isnan(processing::calculateImageStats(Image2D()).mean));
    }

    void calculateImageNoiseMatchesMatlab()
    {
        const MatVariable light = onlyStack(fixture("light_small.mat"));
        const MatVariable dark = onlyStack(fixture("dark_small.mat"));
        // light: per-pixel values z = 0..3 -> var 1.25; dark: 10z, z = 0..2 -> var 200/3
        QVERIFY(std::abs(processing::calculateImageNoise(light.data) - std::sqrt(1.25)) < 1e-12);
        QVERIFY(std::abs(processing::calculateImageNoise(dark.data) - 8.164965809277) < 1e-9);

        // Calibrated stack: subtracting a fixed frame leaves the noise unchanged.
        const Image2D darkMean = processing::meanOverFrames(dark.data);
        QVERIFY(std::abs(processing::calculateImageNoise(light.data, darkMean) - std::sqrt(1.25)) < 1e-12);
        QVERIFY(std::isnan(processing::calculateImageNoise(light.data, Image2D(3, 3))));

        // One frame: var over one sample is 0, as in MATLAB.
        const MatVariable single = onlyStack(fixture("single2d.mat"));
        QCOMPARE(processing::calculateImageNoise(single.data), 0.0);
        QVERIFY(std::isnan(processing::calculateImageNoise(ImageStack())));
    }

    void columnMajorHelpers()
    {
        const MatVariable light = onlyStack(fixture("light_small.mat"));
        const Image2D f2 = light.data.frame(2);
        const std::vector<double> cm = processing::toColumnMajor(f2);
        QCOMPARE(cm.size(), std::size_t(48));
        QCOMPARE(cm[std::size_t(3 * 6 + 4)], f2.at(3, 4));  // col 3, row 4

        std::vector<double> out(48);
        processing::frameMinusOffset(light.data, 2, cm.data(), out.data());
        for (double v : out)
            QCOMPARE(v, 0.0);
        processing::frameMinusOffset(light.data, 2, nullptr, out.data());
        QCOMPARE(out[std::size_t(3 * 6 + 4)], 1000.0 + 100 * 4 + 10 * 3 + 2);
    }

    void matWriterRoundTrip_data()
    {
        QTest::addColumn<int>("level");
        QTest::newRow("compressed") << 1;
        QTest::newRow("uncompressed") << 0;
    }

    void matWriterRoundTrip()
    {
        QFETCH(int, level);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("out.mat"));

        const MatVariable light = onlyStack(fixture("double_as_uint16.mat"));
        Image2D img(5, 3);
        for (int y = 0; y < 3; ++y)
            for (int x = 0; x < 5; ++x)
                img.data()[y * 5 + x] = x * 0.5 - y * 10.25;

        MatFileWriter w(level);
        QVERIFY(w.addImage(QStringLiteral("img"), img));
        QVERIFY(w.addStack(QStringLiteral("stack"), light.data, light.matlabClass));
        QVERIFY(w.addDoubleStack(QStringLiteral("cal"), 6, 8, 4, [&](int z, double* out) {
            processing::frameMinusOffset(light.data, z, nullptr, out);
            for (int i = 0; i < 48; ++i)
                out[i] -= 5000.0;
        }));
        QVERIFY(w.addScalar(QStringLiteral("img_noise"), 9.5));
        QVERIFY(w.addString(QStringLiteral("source_file"), QStringLiteral("C:\\data\\x.mat")));
        QVERIFY(w.addString(QStringLiteral("e"), QString()));
        QVERIFY(!w.addScalar(QStringLiteral("1bad name"), 1.0));
        QVERIFY2(w.save(path), qPrintable(w.errorString()));

        const MatFileContents c = MatFileReader::read(path);
        QVERIFY2(c.ok(), qPrintable(c.error));
        QVERIFY(c.headerText.startsWith(QLatin1String("MATLAB 5.0 MAT-file")));
        QCOMPARE(c.variables.size(), 6);
        QMap<QString, MatVariable> vars;
        for (const MatVariable& v : c.variables)
            vars.insert(v.name, v);

        const MatVariable& vi = vars[QStringLiteral("img")];
        QCOMPARE(vi.dims, (QVector<qint64>{3, 5}));
        QCOMPARE(vi.matlabClass, QStringLiteral("double"));
        for (int y = 0; y < 3; ++y)
            for (int x = 0; x < 5; ++x)
                QCOMPARE(vi.data.frame(0).at(x, y), img.at(x, y));

        const MatVariable& vs = vars[QStringLiteral("stack")];
        QCOMPARE(vs.dims, (QVector<qint64>{6, 8, 4}));
        QCOMPARE(vs.matlabClass, QStringLiteral("double"));         // class kept
        QCOMPARE(vs.data.elementType(), ElementType::UInt16);       // storage kept
        QCOMPARE(vs.data.rawData(), light.data.rawData());

        const MatVariable& vc = vars[QStringLiteral("cal")];
        QCOMPARE(vc.dims, (QVector<qint64>{6, 8, 4}));
        QCOMPARE(vc.data.frame(3).at(7, 5), 8 * 5 + 7 + 300.0);

        QCOMPARE(vars[QStringLiteral("img_noise")].data.frame(0).at(0, 0), 9.5);
        QCOMPARE(vars[QStringLiteral("source_file")].matlabClass, QStringLiteral("char"));
        QCOMPARE(vars[QStringLiteral("source_file")].dims, (QVector<qint64>{1, 13}));
    }

    // ---------------------------------------------------------------- TIFF

    void readsUncompressedTiffStrips()
    {
        const ImageFrame f = readImage("u16_le_strips.tif");
        QVERIFY(f.ok());
        QCOMPARE(f.format, QStringLiteral("TIFF"));
        QCOMPARE(f.matlabClass, QStringLiteral("uint16"));
        QCOMPARE(f.data.elementType(), ElementType::UInt16);
        QCOMPARE(f.data.rows(), 6);
        QCOMPARE(f.data.cols(), 8);
        QCOMPARE(f.data.frameCount(), 1);
        QVERIFY(f.note.isEmpty());
        const Image2D img = f.data.frame(0);
        for (int r = 0; r < 6; ++r)
            for (int c = 0; c < 8; ++c)
                QCOMPARE(img.at(c, r), 1000.0 + 100 * r + 10 * c);
    }

    void tiffCompressionsMatchUncompressed_data()
    {
        QTest::addColumn<QString>("name");
        QTest::newRow("LZW, predictor, big-endian") << QStringLiteral("u16_be_lzw_predictor.tif");
        QTest::newRow("Deflate, predictor, BigTIFF") << QStringLiteral("u16_bigtiff_deflate.tif");
        QTest::newRow("Adobe Deflate, tiles") << QStringLiteral("u16_adobe_deflate_tiles.tif");
    }

    void tiffCompressionsMatchUncompressed()
    {
        QFETCH(QString, name);
        const ImageFrame ref = readImage("u16_random_uncompressed.tif");
        QVERIFY(ref.ok());
        // Reference values from NumPy.
        const Image2D img = ref.data.frame(0);
        QCOMPARE(img.at(0, 0), 53638.0);
        QCOMPARE(img.at(7, 5), 54378.0);
        QCOMPARE(img.at(79, 63), 12943.0);
        double sum = 0;
        for (qsizetype i = 0; i < img.pixelCount(); ++i)
            sum += img.constData()[i];
        QCOMPARE(sum, 167843880.0);

        const ImageFrame f = readImage(qPrintable(name));
        QVERIFY(f.ok());
        QCOMPARE(f.matlabClass, QStringLiteral("uint16"));
        QCOMPARE(f.data.rows(), 64);
        QCOMPARE(f.data.cols(), 80);
        QCOMPARE(f.data.rawData(), ref.data.rawData());
    }

    void readsFloatAndSignedTiff()
    {
        const ImageFrame f32 = readImage("f32_tiles_deflate_fp_predictor.tif");
        QVERIFY(f32.ok());
        QCOMPARE(f32.matlabClass, QStringLiteral("single"));
        QCOMPARE(f32.data.rows(), 20);
        QCOMPARE(f32.data.cols(), 36);
        const ImageFrame f64 = readImage("f64_be.tif");
        QVERIFY(f64.ok());
        QCOMPARE(f64.matlabClass, QStringLiteral("double"));
        for (int r = 0; r < 20; ++r)
            for (int c = 0; c < 36; ++c) {
                QCOMPARE(f32.data.frame(0).at(c, r), r * 0.25 - c * 1.5 + 0.125);
                QCOMPARE(f64.data.frame(0).at(c, r), 3 * (r * 0.25 - c * 1.5 + 0.125));
            }

        const ImageFrame i16 = readImage("i16_packbits.tif");
        QVERIFY(i16.ok());
        QCOMPARE(i16.matlabClass, QStringLiteral("int16"));
        for (int r = 0; r < 6; ++r)
            for (int c = 0; c < 8; ++c)
                QCOMPARE(i16.data.frame(0).at(c, r), double((r - 3) * 100 + c));
    }

    void readsColourMultipageAndBilevelTiff()
    {
        const ImageFrame rgb = readImage("rgb8.tif");
        QVERIFY(rgb.ok());
        QCOMPARE(rgb.matlabClass, QStringLiteral("double"));
        QVERIFY(rgb.note.contains(QLatin1String("rgb2gray")));
        const ImageFrame grey = readImage("rgb8_equal_channels.tif");
        QVERIFY(grey.ok());
        QCOMPARE(grey.matlabClass, QStringLiteral("uint8"));
        QVERIFY(grey.note.isEmpty());
        const ImageFrame planar = readImage("rgb16_planar.tif");
        QVERIFY(planar.ok());
        const ImageFrame pages = readImage("u16_three_pages.tif");
        QVERIFY(pages.ok());
        // Three pages as written by make_fixtures.py; a copy may carry extra
        // pages (e.g. a metadata IFD appended by other software).
        QVERIFY2(pages.note.contains(QLatin1String("page 1 of ")), qPrintable(pages.note));
        const ImageFrame bits = readImage("bilevel.tif");
        QVERIFY(bits.ok());
        QCOMPARE(bits.matlabClass, QStringLiteral("logical"));
        for (int r = 0; r < 6; ++r)
            for (int c = 0; c < 8; ++c) {
                QVERIFY(std::abs(rgb.data.frame(0).at(c, r) - (kR * r * 30 + kG * c * 20 + kB * (r + c) * 10)) < 1e-9);
                QCOMPARE(grey.data.frame(0).at(c, r), double(r * 30 + c));
                const double v = 1000.0 + 100 * r + 10 * c;
                QVERIFY(std::abs(planar.data.frame(0).at(c, r)
                                 - (kR * v + kG * std::floor(v / 2) + kB * std::floor(v / 4))) < 1e-9);
                QCOMPARE(pages.data.frame(0).at(c, r), v);
                QCOMPARE(bits.data.frame(0).at(c, r), (r + c) % 3 == 0 ? 1.0 : 0.0);
            }
    }

    void rejectsUnsupportedTiff()
    {
        const ImageFrame jpeg = readImage("jpeg_compressed.tif");
        QVERIFY(!jpeg.ok());
        QVERIFY2(jpeg.error.contains(QLatin1String("JPEG compression is not supported")), qPrintable(jpeg.error));
        const ImageFrame palette = readImage("palette.tif");
        QVERIFY(!palette.ok());
        QVERIFY(palette.error.contains(QLatin1String("palette"), Qt::CaseInsensitive));
        QVERIFY(!TiffReader::readFromBytes(QByteArray("II*\0garbage")).ok());
        QVERIFY(!TiffReader::readFromBytes(QByteArray(100, 'x')).ok());

        // Truncated copy of a valid file.
        QFile file(formatFixture("u16_random_uncompressed.tif"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray bytes = file.readAll();
        QVERIFY(!TiffReader::readFromBytes(bytes.left(bytes.size() / 2)).ok());
    }

    // ---------------------------------------------------------------- PNG / JPEG

    void readsPngAndJpeg()
    {
        const ImageFrame g8 = readImage("grey8.png");
        QVERIFY(g8.ok());
        QCOMPARE(g8.format, QStringLiteral("PNG"));
        QCOMPARE(g8.matlabClass, QStringLiteral("uint8"));
        const ImageFrame rgb = readImage("rgb8.png");
        QVERIFY(rgb.ok());
        QCOMPARE(rgb.matlabClass, QStringLiteral("double"));
        const ImageFrame rgba16 = readImage("rgba16_equal_channels.png");
        QVERIFY(rgba16.ok());
        QCOMPARE(rgba16.matlabClass, QStringLiteral("uint16"));
        QVERIFY(rgba16.note.contains(QLatin1String("alpha")));
        for (int r = 0; r < 6; ++r)
            for (int c = 0; c < 8; ++c) {
                QCOMPARE(g8.data.frame(0).at(c, r), double(r * 30 + c));
                QVERIFY(std::abs(rgb.data.frame(0).at(c, r) - (kR * r * 30 + kG * c * 20 + kB * (r + c) * 10)) < 1e-9);
                QCOMPARE(rgba16.data.frame(0).at(c, r), 1000.0 + 100 * r + 10 * c);
            }

        // JPEG is lossy, so allow small differences.
        const ImageFrame jpg = readImage("grey.jpg");
        QVERIFY(jpg.ok());
        QCOMPARE(jpg.format, QStringLiteral("JPEG"));
        QCOMPARE(jpg.matlabClass, QStringLiteral("uint8"));
        QCOMPARE(jpg.data.rows(), 16);
        QCOMPARE(jpg.data.cols(), 24);
        QVERIFY(std::abs(jpg.data.frame(0).at(3, 2) - 100) <= 2);
        QVERIFY(std::abs(jpg.data.frame(0).at(20, 13) - 200) <= 2);
        const ImageFrame colour = readImage("colour.jpg");
        QVERIFY(colour.ok());
        QCOMPARE(colour.matlabClass, QStringLiteral("double"));
        QVERIFY(std::abs(colour.data.frame(0).at(5, 5) - (kR * 200 + kG * 50 + kB * 100)) <= 3);

        QVERIFY(!ImageFileReader::read(formatFixture("not_an_image.png")).ok());
        QVERIFY(!ImageFileReader::read(formatFixture("does_not_exist.png")).ok());
        QVERIFY(!ImageFileReader::read(formatFixture("does_not_exist.tif")).ok());
    }

    // ---------------------------------------------------------------- several files

    void readsImageSequenceInNaturalOrder()
    {
        // Same values as light_small.mat, given in the wrong order.
        QStringList pngs;
        for (int k : {3, 1, 4, 2})
            pngs << fixture(qPrintable(QStringLiteral("light_png/light_%1.png").arg(k)));
        const ImageSequence light = ImageFileReader::readSequence(pngs);
        QVERIFY2(light.ok(), qPrintable(light.error));
        QCOMPARE(QFileInfo(light.paths.first()).fileName(), QStringLiteral("light_1.png"));
        QCOMPARE(light.format, QStringLiteral("PNG"));
        QCOMPARE(light.matlabClass, QStringLiteral("uint16"));
        const MatVariable mat = onlyStack(fixture("light_small.mat"));
        QCOMPARE(light.data.frameCount(), 4);
        QCOMPARE(light.data.rawData(), mat.data.rawData());

        // dark_1, dark_2, dark_10: the frames of dark_small.mat, as float32.
        const ImageSequence dark = ImageFileReader::readSequence(
            {fixture("dark_tiff/dark_10.tif"), fixture("dark_tiff/dark_2.tif"), fixture("dark_tiff/dark_1.tif")});
        QVERIFY2(dark.ok(), qPrintable(dark.error));
        QCOMPARE(dark.matlabClass, QStringLiteral("single"));
        QCOMPARE(QFileInfo(dark.paths.last()).fileName(), QStringLiteral("dark_10.tif"));
        const Image2D tiffMean = processing::meanOverFrames(dark.data);
        const Image2D matMean = processing::meanOverFrames(onlyStack(fixture("dark_small.mat")).data);
        for (qsizetype i = 0; i < matMean.pixelCount(); ++i)
            QCOMPARE(tiffMean.constData()[i], matMean.constData()[i]);
    }

    void imageSequenceErrorsAndMixedTypes()
    {
        const ImageSequence wrongSize =
            ImageFileReader::readSequence({fixture("light_png/light_1.png"), formatFixture("grey16_5x7.png")});
        QVERIFY(!wrongSize.ok());
        QVERIFY(wrongSize.error.contains(QLatin1String("same size")));

        const ImageSequence broken =
            ImageFileReader::readSequence({fixture("light_png/light_1.png"), formatFixture("not_an_image.png")});
        QVERIFY(!broken.ok());
        QVERIFY(broken.error.contains(QLatin1String("not_an_image.png")));

        // uint8 + uint16 + int16: stacked as double, values kept.
        const ImageSequence mixed = ImageFileReader::readSequence(
            {formatFixture("grey8.png"), formatFixture("u16_le_strips.tif"), formatFixture("i16_packbits.tif")});
        QVERIFY2(mixed.ok(), qPrintable(mixed.error));
        QCOMPARE(mixed.matlabClass, QStringLiteral("double"));
        QCOMPARE(mixed.data.elementType(), ElementType::Double);
        QCOMPARE(mixed.format, QStringLiteral("image"));
        QVERIFY(mixed.notes.join(' ').contains(QLatin1String("different types")));
        // Natural order: grey8.png, i16_packbits.tif, u16_le_strips.tif
        QCOMPARE(mixed.data.frame(0).at(7, 5), double(5 * 30 + 7));
        QCOMPARE(mixed.data.frame(1).at(7, 5), double((5 - 3) * 100 + 7));
        QCOMPARE(mixed.data.frame(2).at(7, 5), 1000.0 + 500 + 70);
    }

    void naturalSortSelectionAndNames()
    {
        QVERIFY(naturalLess(QStringLiteral("frame_2.png"), QStringLiteral("frame_10.png")));
        QVERIFY(!naturalLess(QStringLiteral("frame_10.png"), QStringLiteral("frame_2.png")));
        QVERIFY(naturalLess(QStringLiteral("Frame_02.png"), QStringLiteral("frame_3.png")));
        QVERIFY(naturalLess(QStringLiteral("a.png"), QStringLiteral("a1.png")));
        QStringList paths{"x/img10.tif", "y/img9.tif", "x/img1.tif", "z/img100.tif"};
        sortNaturally(paths);
        QCOMPARE(paths, (QStringList{"x/img1.tif", "y/img9.tif", "x/img10.tif", "z/img100.tif"}));

        FileSelection s = selectFiles({"a.mat"}, true);
        QVERIFY(s.ok() && s.isMat && s.files.size() == 1);
        s = selectFiles({"a.png", "b.TIF", "c.jpeg", "notes.txt"}, true);
        QVERIFY(s.ok() && !s.isMat);
        QCOMPARE(s.files.size(), 3);
        QCOMPARE(s.ignored, 1);
        QVERIFY(!selectFiles({"a.mat", "b.mat"}, true).ok());
        QVERIFY(!selectFiles({"a.mat", "b.png"}, true).ok());
        QVERIFY(!selectFiles({"a.png", "b.png"}, false).ok());
        QVERIFY(selectFiles({"a.png"}, false).ok());
        QVERIFY(!selectFiles({"notes.txt"}, true).ok());
        QVERIFY(!selectFiles({}, true).ok());

        QCOMPARE(baseNameFor({"d/light_001.png", "d/light_002.png", "d/light_010.png"}), QStringLiteral("light"));
        QCOMPARE(baseNameFor({"d/stack.mat"}), QStringLiteral("stack"));
        QCOMPARE(baseNameFor({"d/1.png", "d/2.png"}), QStringLiteral("images"));
    }

    // examples/: simulated sensor data written by examples/make_examples.py,
    // with reference values computed by NumPy in reference.json. Set
    // IAT_EXAMPLES_DIR to check a set generated elsewhere (e.g. larger, with
    // make_examples.py --rows 1028 --cols 1408 --frames 20 --out <folder>).
    void exampleDataset()
    {
        const QString dir = qEnvironmentVariable("IAT_EXAMPLES_DIR", QStringLiteral(IAT_EXAMPLES_DIR));
        QFile file(dir + QStringLiteral("/reference.json"));
        QVERIFY2(file.open(QIODevice::ReadOnly), "examples/reference.json is missing: run examples/make_examples.py");
        const QJsonObject ref = QJsonDocument::fromJson(file.readAll()).object();
        auto refValue = [&](const QStringList& path) {
            QJsonValue v = ref;
            for (const QString& key : path)
                v = v.toObject().value(key);
            return v.toDouble(std::nan(""));
        };
        auto close = [](double a, double b) { return std::abs(a - b) <= 1e-9 * std::max(1.0, std::abs(b)); };
        const QJsonArray size = ref.value(QStringLiteral("size")).toArray();
        const int rows = size.at(0).toInt();
        const int cols = size.at(1).toInt();
        const int frames = size.at(2).toInt();

        QElapsedTimer timer;
        timer.start();
        const MatVariable light = onlyStack(dir + QStringLiteral("/light_frames.mat"));
        const MatVariable dark = onlyStack(dir + QStringLiteral("/dark_frames.mat"));
        QCOMPARE(light.name, QStringLiteral("all_pixel"));
        QCOMPARE(light.matlabClass, QStringLiteral("double"));
        QCOMPARE(light.data.elementType(), ElementType::UInt16);  // as MATLAB stores whole numbers
        QCOMPARE(light.dims, (QVector<qint64>{rows, cols, frames}));
        QCOMPARE(dark.dims, light.dims);
        QCOMPARE(light.data.frame(0).at(0, 0), refValue({"pixels", "light(1,1,1)"}));
        QCOMPARE(light.data.frame(frames - 1).at(cols - 1, rows - 1), refValue({"pixels", "light(end,end,end)"}));
        QCOMPARE(dark.data.frame(1).at(4, 2), refValue({"pixels", "dark(3,5,2)"}));

        // calculateImageStats / calculateImageNoise against NumPy.
        auto checkStats = [&](const processing::MeanStd& ms, const QStringList& path) {
            return close(ms.mean, refValue(path + QStringList{"mean"})) && close(ms.std, refValue(path + QStringList{"std"}));
        };
        const Image2D lightMean = processing::meanOverFrames(light.data);
        const Image2D darkMean = processing::meanOverFrames(dark.data);
        QVERIFY(checkStats(processing::calculateImageStats(light.data.frame(0)), {"light", "frame1"}));
        QVERIFY(checkStats(processing::calculateImageStats(lightMean), {"light", "mean_image"}));
        QVERIFY(checkStats(processing::calculateImageStats(light.data), {"light", "all_frames"}));
        QVERIFY(close(processing::calculateImageNoise(light.data), refValue({"light", "noise"})));
        QVERIFY(checkStats(processing::calculateImageStats(dark.data.frame(0)), {"dark", "frame1"}));
        QVERIFY(checkStats(processing::calculateImageStats(darkMean), {"dark", "mean_image"}));
        QVERIFY(close(processing::calculateImageNoise(dark.data), refValue({"dark", "noise"})));
        const Image2D calibrated = processing::subtract(light.data.frame(0), darkMean);
        QVERIFY(checkStats(processing::calculateImageStats(calibrated), {"calibrated", "light_frame1_minus_dark_mean"}));
        QVERIFY(checkStats(processing::calculateImageStats(processing::subtract(lightMean, darkMean)),
                           {"calibrated", "light_mean_minus_dark_mean"}));
        QVERIFY(checkStats(processing::calculateImageStats(processing::subtract(light.data.frame(0), dark.data.frame(0))),
                           {"calibrated", "light_frame1_minus_dark_frame1"}));
        QVERIFY(close(processing::calculateImageNoise(light.data, darkMean), refValue({"calibrated", "noise"})));
        const ImageStats ds = dark.data.frame(0).computeStats();
        QCOMPARE(ds.max, 16383.0);  // a saturated hot pixel

        const qint64 matMs = timer.restart();

        // The same frames as TIFF and PNG files: identical values.
        auto files = [&](const QString& folder, const QString& pattern) {
            QStringList out;
            const QDir d(dir + QLatin1Char('/') + folder);
            for (const QString& name : d.entryList({pattern}, QDir::Files))
                out << d.filePath(name);
            return out;
        };
        for (const auto& [name, mat] : {std::pair{QStringLiteral("light"), light}, std::pair{QStringLiteral("dark"), dark}}) {
            for (const auto& [folder, pattern] : {std::pair{QStringLiteral("_tiff"), QStringLiteral("*.tif")},
                                                  std::pair{QStringLiteral("_png"), QStringLiteral("*.png")}}) {
                const ImageSequence seq = ImageFileReader::readSequence(files(name + folder, pattern));
                QVERIFY2(seq.ok(), qPrintable(seq.error));
                QCOMPARE(seq.matlabClass, QStringLiteral("uint16"));
                QCOMPARE(seq.data.frameCount(), frames);
                QVERIFY2(seq.data.rawData() == mat.data.rawData(), qPrintable(name + folder));
            }
            // JPEG: 8-bit (values / 64) and lossy.
            const ImageSequence jpg = ImageFileReader::readSequence(files(name + QStringLiteral("_jpg"), QStringLiteral("*.jpg")));
            QVERIFY2(jpg.ok(), qPrintable(jpg.error));
            QCOMPARE(jpg.matlabClass, QStringLiteral("uint8"));
            QCOMPARE(jpg.data.frameCount(), frames);
            QCOMPARE(jpg.data.rows(), rows);
            const double jpgMean = processing::calculateImageStats(jpg.data.frame(0)).mean;
            QVERIFY(std::abs(jpgMean - refValue({"jpeg_frame1_mean_pillow", name})) < 0.5);
        }

        // calibrated_frame01.tif: light(:, :, 1) - mean(dark, 3) as float32.
        const ImageFrame cal = ImageFileReader::read(dir + QStringLiteral("/calibrated_frame01.tif"));
        QVERIFY2(cal.ok(), qPrintable(cal.error));
        QCOMPARE(cal.matlabClass, QStringLiteral("single"));
        const Image2D calImage = cal.data.frame(0);
        for (qsizetype i = 0; i < calImage.pixelCount(); ++i)
            QCOMPARE(calImage.constData()[i], double(float(calibrated.constData()[i])));

        qInfo("example data %dx%dx%d: .mat files and processing %lld ms, image files %lld ms", rows, cols, frames,
              matMs, qint64(timer.elapsed()));
    }
};

QTEST_GUILESS_MAIN(TestCore)
#include "tst_core.moc"
