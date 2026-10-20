// Unit tests for the MAT-file reader and the processing functions.
//
// Optional: set IAT_EXAMPLE_MAT to the path of a real dark-frame file
// (all_pixel_dark_1.5ms-1.mat) to also check it against values computed
// independently with SciPy.

#include "core/Histogram.h"
#include "core/Processing.h"
#include "io/MatFileReader.h"
#include "io/MatFileWriter.h"

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

    void exampleDarkFile()
    {
        const QString path = qEnvironmentVariable("IAT_EXAMPLE_MAT");
        if (path.isEmpty())
            QSKIP("Set IAT_EXAMPLE_MAT to all_pixel_dark_1.5ms-1.mat to run this test");

        QElapsedTimer timer;
        timer.start();
        const MatVariable v = onlyStack(path);
        const qint64 readMs = timer.restart();
        QCOMPARE(v.name, QStringLiteral("all_pixel"));
        QCOMPARE(v.matlabClass, QStringLiteral("double"));
        QCOMPARE(v.dims, (QVector<qint64>{1028, 1408, 20}));

        // Reference values from SciPy (0-based indices [row, col, frame]).
        QCOMPARE(v.data.frame(3).at(2, 1), 8446.0);
        QCOMPARE(v.data.frame(5).at(20, 10), 8510.0);
        QCOMPARE(v.data.frame(0).at(4, 0), 8380.0);

        const ImageStats f0 = v.data.frame(0).computeStats();
        QCOMPARE(f0.min >= 5311.0, true);
        QVERIFY(std::abs(f0.mean - 8526.105092909887) < 1e-6);

        const Image2D mean = processing::meanOverFrames(v.data);
        const qint64 meanMs = timer.elapsed();
        const ImageStats ms = mean.computeStats();
        QVERIFY(std::abs(ms.mean - 8526.095962585945) < 1e-6);
        QCOMPARE(ms.min, 5353.35);
        QCOMPARE(ms.max, 16383.0);

        // calculateImageStats / calculateImageNoise against NumPy.
        timer.restart();
        const processing::MeanStd s1 = processing::calculateImageStats(v.data.frame(0));
        QVERIFY(std::abs(s1.mean - 8526.105092909887) < 1e-8);
        QVERIFY(std::abs(s1.std - 164.148862251702) < 1e-8);
        const processing::MeanStd sd = processing::calculateImageStats(mean);
        QVERIFY(std::abs(sd.std - 163.889064652385) < 1e-8);
        const processing::MeanStd sc = processing::calculateImageStats(processing::subtract(v.data.frame(0), mean));
        QVERIFY(std::abs(sc.mean - 0.009130323941) < 1e-9);
        QVERIFY(std::abs(sc.std - 9.532488209494) < 1e-8);
        const processing::MeanStd sAll = processing::calculateImageStats(v.data);
        QVERIFY(std::abs(sAll.std - 164.166033086360) < 1e-8);
        const double noise = processing::calculateImageNoise(v.data);
        QVERIFY(std::abs(noise - 9.532098755154) < 1e-9);
        const double calNoise = processing::calculateImageNoise(v.data, mean);
        QVERIFY(std::abs(calNoise - 9.532098755154) < 1e-9);
        const qint64 noiseMs = timer.elapsed();

        qInfo("example file: read %lld ms, mean over frames %lld ms, stats + 2x noise %lld ms",
              readMs, meanMs, noiseMs);
    }
};

QTEST_GUILESS_MAIN(TestCore)
#include "tst_core.moc"
