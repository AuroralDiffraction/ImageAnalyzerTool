// Unit tests for the hyperspectral code (core/Hsi*.h, io/HsiCubeIo.h), on
// synthetic data only: mosaics whose values encode band and position, and a
// synthetic spectral response with Gaussian channels.
//
// Optional comparison with MATLAB results on real sensor data: set
// IAT_LOCAL_HSI_DATA to a folder holding
//   <name>.tif / <name>.tiff        a raw 4 × 4 mosaic
//   <name>_cube.mat                 hsi_reconstruct.m's output for it (cube, M, reconRGB)
//   reconstruction_matrix.mat       the spectral response (lambda, QE_percent)
// Those files are private and must never be added to the repository; without
// the variable the test is skipped.

#include "core/Hsi.h"
#include "core/HsiAnalysis.h"
#include "core/HsiColour.h"
#include "core/HsiCorrection.h"
#include "io/HsiCubeIo.h"
#include "io/ImageFileReader.h"
#include "io/MatFileReader.h"
#include "io/MatFileWriter.h"
#include "io/TiffReader.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include <cmath>
#include <cstring>
#include <functional>

using namespace iat;
using namespace iat::hsi;

namespace {

// Raw mosaic in which every pixel holds 1000 · channel + 10 · super-pixel row + super-pixel column.
Image2D codedMosaic(int superRows, int superCols, const MosaicLayout& layout)
{
    Image2D img(superCols * kPattern, superRows * kPattern);
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            img.data()[y * img.width() + x] = 1000.0 * layout.at(y % kPattern, x % kPattern) + 10.0 * (y / kPattern) + (x / kPattern);
    return img;
}

// Raw mosaic of a scene with value f(x, y) in channel ch: f(x, y, ch).
template <typename F>
Image2D sceneMosaic(int rows, int cols, const MosaicLayout& layout, F f)
{
    Image2D img(cols, rows);
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x)
            img.data()[y * cols + x] = f(x, y, layout.at(y % kPattern, x % kPattern));
    return img;
}

// 16 Gaussian channels, peaks 480 … 615 nm (9 nm apart), σ 8 nm, on 450–650 nm.
SpectralResponse syntheticResponse()
{
    SpectralResponse r;
    for (int l = 450; l <= 650; ++l)
        r.lambda << l;
    for (int b = 0; b < kBands; ++b) {
        QVector<double> q;
        const double peak = 480.0 + 9.0 * b;
        for (double l : r.lambda)
            q << 0.2 * std::exp(-0.5 * std::pow((l - peak) / 8.0, 2.0)) + 0.01;
        r.qe << q;
    }
    return r;
}

// Channel values of a spectrum L(λ): s = A L dλ.
QVector<double> sense(const SpectralResponse& r, const std::function<double(double)>& L)
{
    QVector<double> s(r.bands(), 0.0);
    for (int b = 0; b < r.bands(); ++b)
        for (int i = 0; i < r.samples(); ++i)
            s[b] += r.qe[b][i] * L(r.lambda[i]);
    return s;
}

Cube uniformCube(int rows, int cols, const QVector<double>& s)
{
    Cube c(rows, cols, s.size());
    for (int b = 0; b < s.size(); ++b)
        std::fill(c.band(b), c.band(b) + c.planeSize(), float(s[b]));
    return c;
}

struct Rng {
    quint32 s = 12345u;
    double gauss()
    {
        double a = 0.0;
        for (int i = 0; i < 12; ++i) {
            s = s * 1664525u + 1013904223u;
            a += double(s >> 8) / double(1u << 24);
        }
        return a - 6.0;
    }
};

double rmsDiff(const Cube& a, const Cube& b)
{
    double s = 0.0;
    for (qsizetype i = 0; i < a.data().size(); ++i)
        s += std::pow(double(a.data()[i]) - b.data()[i], 2.0);
    return std::sqrt(s / double(a.data().size()));
}

} // namespace

class TestHsi : public QObject {
    Q_OBJECT
private slots:
    void mosaicPresets();
    void layout();
    void extractSubsample();
    void extractRegistered();
    void extractFullResolution();
    void extractScalesMsbAligned();
    void extractRejectsBadInput();
    void enviRoundTrip_data();
    void enviRoundTrip();
    void matRoundTrip();
    void multiPageTiff();
    void responseFromCsv();
    void responseFromMat();
    void colourNeutralAndRed();
    void colourPercentileMatchesMatlab();
    void spectralAngle();
    void clustering();
    void matchToAreas();
    void fixedStretch();
    void pca();
    void bandMath();
    void denoise();
    void reflectanceAndUpscale();
    void processChainOnSharedCube();
    void spectraCsv();
    void referenceFrames();
    void spectralCorrection();
    void correctionMatrixFile();
    void sensorProfile();
    void preprocessing();
    void polygonSelection();
    void localMatlabParity();
};

void TestHsi::layout()
{
    const MosaicLayout m = MosaicLayout::defaultLayout();
    QVERIFY(m.isValid());
    QCOMPARE(m.toString(), QStringLiteral("1 8 9 16; 2 7 10 15; 3 6 11 14; 4 5 12 13"));
    QCOMPARE(m.at(0, 3), 16);
    QCOMPARE(m.at(3, 1), 5);
    int r, c;
    m.positionOf(7, r, c);
    QCOMPARE(r, 1);
    QCOMPARE(c, 1);
    QVERIFY(MosaicLayout::fromString(m.toString()) == m);
    QVERIFY(!MosaicLayout::fromString(QStringLiteral("1 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15")).has_value());
    QVERIFY(!MosaicLayout::fromString(QStringLiteral("1 2 3")).has_value());
    MosaicLayout bad = m;
    bad.set(0, 0, 8);
    QVERIFY(!bad.isValid());
}

void TestHsi::extractSubsample()
{
    // extract_hsi_channels.m: cube(:, :, map(r, c)) = img(r:4:end, c:4:end).
    const MosaicLayout m = MosaicLayout::defaultLayout();
    ExtractOptions o;
    o.bitDepth = 0;
    const ExtractResult r = extract(codedMosaic(5, 6, m), o);
    QVERIFY(r.ok());
    QCOMPARE(r.cube.rows(), 5);
    QCOMPARE(r.cube.cols(), 6);
    QCOMPARE(r.cube.bands(), 16);
    for (int b = 0; b < 16; ++b)
        for (int i = 0; i < 5; ++i)
            for (int j = 0; j < 6; ++j)
                QCOMPARE(r.cube.at(j, i, b), float(1000 * (b + 1) + 10 * i + j));
    QVERIFY(r.cube.history.last().contains(QLatin1String("subsample")));

    // A size that is not a multiple of 4 is cropped, with a note.
    Image2D odd(27, 22, 5.0);
    const ExtractResult c = extract(odd, o);
    QVERIFY(c.ok());
    QCOMPARE(c.cube.cols(), 6);
    QCOMPARE(c.cube.rows(), 5);
    QVERIFY(!c.notes.isEmpty());
}

void TestHsi::extractRegistered()
{
    // A scene that is a linear ramp f = x + 2y in every channel: after
    // registration every band holds the ramp at the super-pixel centre
    // (4j + 1.5) + 2 (4i + 1.5), away from the clamped border.
    const MosaicLayout m = MosaicLayout::defaultLayout();
    ExtractOptions o;
    o.bitDepth = 0;
    o.method = ExtractMethod::Registered;
    const ExtractResult r = extract(sceneMosaic(32, 40, m, [](int x, int y, int) { return x + 2.0 * y; }), o);
    QVERIFY(r.ok());
    QCOMPARE(r.cube.rows(), 8);
    for (int b = 0; b < 16; ++b)
        for (int i = 1; i < 7; ++i)
            for (int j = 1; j < 9; ++j)
                QVERIFY2(std::abs(r.cube.at(j, i, b) - ((4 * j + 1.5) + 2 * (4 * i + 1.5))) < 1e-3,
                         qPrintable(QStringLiteral("band %1 at %2,%3: %4").arg(b).arg(j).arg(i).arg(r.cube.at(j, i, b))));
    // Plain sub-sampling leaves the bands offset from each other.
    o.method = ExtractMethod::Subsample;
    const ExtractResult s = extract(sceneMosaic(32, 40, m, [](int x, int y, int) { return x + 2.0 * y; }), o);
    QVERIFY(s.cube.at(3, 3, 0) != s.cube.at(3, 3, 15));
}

void TestHsi::extractFullResolution()
{
    const MosaicLayout m = MosaicLayout::defaultLayout();
    for (ExtractMethod method : {ExtractMethod::Bilinear, ExtractMethod::Ppid}) {
        ExtractOptions o;
        o.bitDepth = 0;
        o.method = method;
        // Flat scene, different level per channel: every band is flat.
        const ExtractResult flat = extract(sceneMosaic(24, 32, m, [](int, int, int ch) { return 100.0 * ch; }), o);
        QVERIFY(flat.ok());
        QCOMPARE(flat.cube.rows(), 24);
        QCOMPARE(flat.cube.cols(), 32);
        for (int b = 0; b < 16; ++b)
            for (qsizetype i = 0; i < flat.cube.planeSize(); ++i)
                QVERIFY(std::abs(flat.cube.band(b)[i] - 100.0f * (b + 1)) < 1e-2f);
        // Linear ramp in every channel: reproduced exactly away from the border.
        const ExtractResult ramp = extract(sceneMosaic(24, 32, m, [](int x, int y, int) { return 3.0 * x + y; }), o);
        for (int b = 0; b < 16; ++b)
            for (int y = 5; y < 19; ++y)
                for (int x = 5; x < 27; ++x)
                    QVERIFY2(std::abs(ramp.cube.at(x, y, b) - (3.0 * x + y)) < 1e-2,
                             qPrintable(QStringLiteral("%1 band %2 at %3,%4: %5").arg(methodName(method)).arg(b).arg(x).arg(y)
                                            .arg(ramp.cube.at(x, y, b))));
    }
}

void TestHsi::extractScalesMsbAligned()
{
    // 14-bit data stored MSB-aligned (× 4) is shifted down, as hsi_reconstruct.m does.
    const MosaicLayout m = MosaicLayout::defaultLayout();
    Image2D img = codedMosaic(2, 2, m);
    for (qsizetype i = 0; i < img.pixelCount(); ++i)
        img.data()[i] *= 4.0;
    ExtractOptions o;
    const ExtractResult r = extract(img, o);
    QVERIFY(r.ok());
    QCOMPARE(r.cube.at(1, 1, 15), float(16000 + 11));
    QVERIFY(!r.notes.isEmpty());
    // Black level and clipping.
    o.bitDepth = 0;
    o.blackLevel = 5000.0;
    const ExtractResult k = extract(codedMosaic(2, 2, m), o);
    QCOMPARE(k.cube.at(0, 0, 0), 0.0f);
    QCOMPARE(k.cube.at(0, 0, 15), 11000.0f);
    o.clipNegative = false;
    QCOMPARE(extract(codedMosaic(2, 2, m), o).cube.at(0, 0, 0), -4000.0f);
}

void TestHsi::extractRejectsBadInput()
{
    ExtractOptions o;
    o.layout.set(0, 0, 2);  // channel 2 twice
    QVERIFY(!extract(Image2D(8, 8, 1.0), o).ok());
    QVERIFY(!extract(Image2D(3, 8, 1.0), ExtractOptions()).ok());
}

void TestHsi::enviRoundTrip_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("interleave");
    for (int t : {0, 1})
        for (int il : {0, 1, 2})
            QTest::addRow("type%d_il%d", t, il) << t << il;
}

void TestHsi::enviRoundTrip()
{
    QFETCH(int, type);
    QFETCH(int, interleave);
    Cube c(7, 9, 16);
    for (int b = 0; b < 16; ++b)
        for (qsizetype i = 0; i < c.planeSize(); ++i)
            c.band(b)[i] = float(b * 100 + i) + (type == 0 ? 0.25f : 0.0f);
    for (int b = 0; b < 16; ++b)
        c.wavelengths << 480.0 + 9.0 * b;
    c.history << QStringLiteral("test {braces}");
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("cube.hdr"));
    QString error;
    QVERIFY2(writeEnvi(c, path, SampleType(type), Interleave(interleave), &error, MosaicLayout::defaultLayout()),
             qPrintable(error));
    QVERIFY(QFile::exists(dir.filePath(QStringLiteral("cube.img"))));
    QFile hdr(path);
    QVERIFY(hdr.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(hdr.readAll());
    QVERIFY(text.startsWith(QLatin1String("ENVI\n")));
    QVERIFY(text.contains(QLatin1String("wavelength units = Nanometers")));
    QVERIFY(text.contains(QLatin1String("default bands = {")));
    for (const QString& p : {path, dir.filePath(QStringLiteral("cube.img"))}) {
        const CubeFile r = readEnvi(p);
        QVERIFY2(r.ok(), qPrintable(r.error));
        QCOMPARE(r.cube.rows(), 7);
        QCOMPARE(r.cube.cols(), 9);
        QCOMPARE(r.cube.bands(), 16);
        QVERIFY(r.cube.data() == c.data());
        QCOMPARE(r.cube.wavelengths, c.wavelengths);
        QVERIFY(r.layout.has_value() && *r.layout == MosaicLayout::defaultLayout());
    }
    QVERIFY(isCubeFile(path));
    QVERIFY(isCubeFile(dir.filePath(QStringLiteral("cube.img"))));
}

void TestHsi::matRoundTrip()
{
    Cube c(5, 6, 16);
    for (int b = 0; b < 16; ++b)
        for (qsizetype i = 0; i < c.planeSize(); ++i)
            c.band(b)[i] = float(b * 1000 + i);
    for (int b = 0; b < 16; ++b)
        c.wavelengths << 480.0 + 9.0 * b;
    QTemporaryDir dir;
    for (SampleType t : {SampleType::Float32, SampleType::UInt16}) {
        const QString path = dir.filePath(QStringLiteral("cube%1.mat").arg(int(t)));
        QString error;
        QVERIFY2(writeMat(c, path, t, &error, MosaicLayout::defaultLayout()), qPrintable(error));
        // MATLAB's view: cube is rows × cols × bands, cube(i, j, b) = pixel (j, i) of band b.
        const MatFileContents mat = MatFileReader::read(path);
        QVERIFY(mat.ok());
        const MatVariable* v = nullptr;
        for (const MatVariable& x : mat.variables)
            if (x.name == QLatin1String("cube"))
                v = &x;
        QVERIFY(v && v->supported);
        QCOMPARE(v->matlabClass, t == SampleType::Float32 ? QStringLiteral("single") : QStringLiteral("uint16"));
        QCOMPARE(v->dims, (QVector<qint64>{5, 6, 16}));
        QCOMPARE(v->data.frame(3).at(2, 4), double(c.at(2, 4, 3)));
        const CubeFile r = readMat(path);
        QVERIFY2(r.ok(), qPrintable(r.error));
        QVERIFY(r.cube.data() == c.data());
        QCOMPARE(r.cube.wavelengths, c.wavelengths);
        QVERIFY(r.layout.has_value() && *r.layout == MosaicLayout::defaultLayout());
    }
}

void TestHsi::multiPageTiff()
{
    Cube c(6, 5, 16);
    for (int b = 0; b < 16; ++b)
        for (qsizetype i = 0; i < c.planeSize(); ++i)
            c.band(b)[i] = float(b * 10 + i) + 0.5f;
    QTemporaryDir dir;
    for (SampleType t : {SampleType::Float32, SampleType::UInt16}) {
        const QString path = dir.filePath(QStringLiteral("cube%1.tif").arg(int(t)));
        QString error;
        QVERIFY2(writeMultiPageTiff(c, path, t, &error), qPrintable(error));
        const TiffImage img = TiffReader::read(path);
        QVERIFY2(img.ok(), qPrintable(img.error));
        QCOMPARE(img.pageCount, 16);
        QCOMPARE(img.width, 5);
        QCOMPARE(img.height, 6);
        QCOMPARE(img.bitsPerSample, t == SampleType::Float32 ? 32 : 16);
        if (t == SampleType::Float32) {
            float v;
            std::memcpy(&v, img.pixels.constData() + 7 * 4, 4);
            QCOMPARE(v, c.band(0)[7]);
        } else {
            quint16 v;
            std::memcpy(&v, img.pixels.constData() + 7 * 2, 2);
            QCOMPARE(int(v), int(std::lround(c.band(0)[7])));
        }
    }
}

void TestHsi::responseFromCsv()
{
    // Columns in a shuffled order, values in percent, a negative value
    // (calibration noise) and a missing one.
    const SpectralResponse s = syntheticResponse();
    const MosaicLayout m = MosaicLayout::defaultLayout();
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("qe.csv"));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    QString text = QStringLiteral("Spectral response,,\nWavelength");
    for (int k = 0; k < 16; ++k)
        text += QStringLiteral(",Band%1").arg(m.channel[size_t(k)]);
    text += QLatin1Char('\n');
    for (int i = 0; i < s.samples(); ++i) {
        text += QString::number(s.lambda[i]);
        for (int k = 0; k < 16; ++k) {
            const int b = m.channel[size_t(k)] - 1;
            if (i == 3 && b == 0)
                text += QStringLiteral(",-0.04");
            else if (i == 4 && b == 1)
                text += QStringLiteral(",");
            else
                text += QStringLiteral(",%1").arg(s.qe[b][i] * 100.0, 0, 'g', 12);
        }
        text += QLatin1Char('\n');
    }
    f.write(text.toUtf8());
    f.close();
    const ResponseFile r = loadResponse(path);
    QVERIFY2(r.ok(), qPrintable(r.error));
    QVERIFY(r.hasResponse());
    QCOMPARE(r.response.bands(), 16);
    QCOMPARE(r.response.samples(), 201);
    QCOMPARE(r.response.qe[0][3], 0.0);
    QCOMPARE(r.response.qe[1][4], 0.0);
    QVERIFY(std::abs(r.response.qe[5][100] - s.qe[5][100]) < 1e-12);
    const QVector<double> peaks = r.response.peaks();
    for (int b = 0; b < 16; ++b)
        QCOMPARE(peaks[b], 480.0 + 9.0 * b);
    const QVector<double> w = syntheticResponse().fwhm();
    // About the FWHM of the Gaussian (2.355 σ = 18.8 nm), a little less
    // because the background shared by all channels is subtracted first.
    QVERIFY2(w[8] > 14.0 && w[8] < 19.5, qPrintable(QString::number(w[8])));

    // Not a response file.
    QFile g(dir.filePath(QStringLiteral("other.csv")));
    QVERIFY(g.open(QIODevice::WriteOnly));
    g.write("a,b\n1,2\n");
    g.close();
    QVERIFY(!loadResponse(dir.filePath(QStringLiteral("other.csv"))).ok());
}

void TestHsi::responseFromMat()
{
    // lambda (nL × 1) and QE_percent (nL × 16), as the converted
    // reconstruction matrix, written with the app's own MAT writer.
    const SpectralResponse s = syntheticResponse();
    QTemporaryDir dir;
    MatFileWriter w;
    Image2D lam(1, s.samples());
    Image2D qe(16, s.samples());
    for (int i = 0; i < s.samples(); ++i) {
        lam.data()[i] = s.lambda[i];
        for (int b = 0; b < 16; ++b)
            qe.data()[i * 16 + b] = s.qe[b][i] * 100.0;
    }
    QVERIFY(w.addImage(QStringLiteral("lambda"), lam));
    QVERIFY(w.addImage(QStringLiteral("QE_percent"), qe));
    const QString path = dir.filePath(QStringLiteral("qe.mat"));
    QVERIFY(w.save(path));
    const ResponseFile r = loadResponse(path);
    QVERIFY2(r.ok(), qPrintable(r.error));
    QCOMPARE(r.response.bands(), 16);
    QVERIFY(std::abs(r.response.qe[7][50] - s.qe[7][50]) < 1e-12);

    // A file with only a 3 × 16 colour matrix.
    MatFileWriter w2;
    Image2D rgb(16, 3, 0.5);
    QVERIFY(w2.addImage(QStringLiteral("reconRGB"), rgb));
    const QString p2 = dir.filePath(QStringLiteral("rgb.mat"));
    QVERIFY(w2.save(p2));
    const ResponseFile r2 = loadResponse(p2);
    QVERIFY(r2.ok() && !r2.hasResponse() && r2.rgbMatrix.size() == 48);
}

void TestHsi::colourNeutralAndRed()
{
    const SpectralResponse s = syntheticResponse();
    const QVector<double> M = xyzMatrix(s, 20.0, 0.05);
    QCOMPARE(M.size(), 48);
    // A flat spectrum under grey-world and flat white balance renders grey.
    const Cube flat = uniformCube(4, 4, sense(s, [](double) { return 1.0; }));
    for (WhiteBalance wb : {WhiteBalance::GrayWorld, WhiteBalance::Flat, WhiteBalance::WhitePatch}) {
        ColourSettings cs;
        cs.whiteBalance = wb;
        const QVector<double> rgb = rgbMatrix(flat, M, cs, s);
        QCOMPARE(rgb.size(), 48);
        const QImage img = renderRgb(flat, rgb, 99.0);
        const QColor c = img.pixelColor(1, 1);
        QVERIFY2(std::abs(c.red() - c.green()) <= 1 && std::abs(c.green() - c.blue()) <= 1 && c.red() > 250,
                 qPrintable(QStringLiteral("wb %1: %2 %3 %4").arg(int(wb)).arg(c.red()).arg(c.green()).arg(c.blue())));
    }
    // Left half reddish (long wavelengths), right half flat (the reference white).
    const QVector<double> red = sense(s, [](double l) { return l > 590.0 ? 1.0 : 0.05; });
    const QVector<double> white = sense(s, [](double) { return 1.0; });
    Cube scene(4, 8, 16);
    for (int b = 0; b < 16; ++b)
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 8; ++x)
                scene.band(b)[y * 8 + x] = float(x < 4 ? red[b] : white[b]);
    ColourSettings cs;
    cs.whiteBalance = WhiteBalance::Reference;
    cs.reference = QRect(4, 0, 4, 4);
    QVector<double> whiteXyz;
    const QVector<double> rgb = rgbMatrix(scene, M, cs, s, &whiteXyz);
    QCOMPARE(whiteXyz.size(), 3);
    QCOMPARE(whiteXyz[1], 1.0);
    const QImage img = renderRgb(scene, rgb, 99.0);
    const QColor l = img.pixelColor(1, 1), r = img.pixelColor(6, 1);
    QVERIFY2(l.red() > l.green() + 40 && l.red() > l.blue() + 40,
             qPrintable(QStringLiteral("%1 %2 %3").arg(l.red()).arg(l.green()).arg(l.blue())));
    QVERIFY(std::abs(r.red() - r.green()) <= 1 && std::abs(r.green() - r.blue()) <= 1);
    // Reference area outside the cube: no matrix.
    cs.reference = QRect(100, 100, 3, 3);
    QVERIFY(rgbMatrix(scene, M, cs, s).isEmpty());

    // The spectral estimator recovers a smooth spectrum within the channels' range.
    const QVector<double> R = spectralEstimator(s, 20.0, 0.001);
    QCOMPARE(R.size(), 201 * 16);
    const QVector<double> sv = sense(s, [](double l) { return 1.0 + 0.5 * std::sin(l / 30.0); });
    for (int i = 50; i <= 150; i += 25) {
        double est = 0.0;
        for (int b = 0; b < 16; ++b)
            est += R[i * 16 + b] * sv[b];
        const double truth = 1.0 + 0.5 * std::sin(s.lambda[i] / 30.0);
        QVERIFY2(std::abs(est - truth) < 0.1, qPrintable(QStringLiteral("%1 nm: %2 vs %3").arg(s.lambda[i]).arg(est).arg(truth)));
    }
}

void TestHsi::colourPercentileMatchesMatlab()
{
    // pct(x, q) = x(max(1, min(n, round(q/100 · n)))) on sorted x.
    std::vector<double> x;
    for (int i = 1; i <= 10; ++i)
        x.push_back(11 - i);
    QCOMPARE(percentileOf(x, 99.0), 10.0);
    QCOMPARE(percentileOf(x, 50.0), 5.0);
    QCOMPARE(percentileOf(x, 0.0), 1.0);
    QCOMPARE(percentileOf(x, 14.0), 1.0);  // round(1.4) = 1
    QCOMPARE(percentileOf(x, 15.0), 2.0);  // round(1.5) = 2
    QVERIFY(std::abs(srgbEncode(0.5) - 0.735356983) < 1e-6);
    // CIE table: ȳ(555) = 1, zero outside 380–780 nm.
    const QVector<double> cmf = cie1931({555.0, 300.0, 800.0});
    QCOMPARE(cmf[1], 1.0);
    QCOMPARE(cmf[3], 0.0);
    QCOMPARE(cmf[7], 0.0);
}

void TestHsi::spectralAngle()
{
    Cube c(1, 3, 3);
    const float px[3][3] = {{1, 2, 3}, {2, 4, 6}, {3, -1.5f, 0}};  // same, scaled, orthogonal
    for (int x = 0; x < 3; ++x)
        for (int b = 0; b < 3; ++b)
            c.band(b)[x] = px[x][b];
    const Image2D a = spectralAngleMap(c, {1, 2, 3});
    QVERIFY(std::abs(a.at(0, 0)) < 1e-6);
    QVERIFY(std::abs(a.at(1, 0)) < 1e-6);
    QVERIFY(std::abs(a.at(2, 0) - 1.5707963267948966) < 1e-6);
}

void TestHsi::clustering()
{
    // Two materials with different spectral shapes, the second at two brightnesses.
    Cube c(10, 20, 16);
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 20; ++x)
            for (int b = 0; b < 16; ++b) {
                const double v = x < 8 ? 100.0 + 10.0 * b : (300.0 - 15.0 * b) * (y < 5 ? 1.0 : 2.0);
                c.band(b)[y * 20 + x] = float(v);
            }
    const Clustering k = kMeans(c, 2, 30, true);
    QVERIFY(k.isValid());
    QCOMPARE(k.sizes[0] + k.sizes[1], qint64(200));
    const int left = k.labels[0];
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 20; ++x)
            QCOMPARE(k.labels[y * 20 + x], x < 8 ? left : 1 - left);
    QCOMPARE(k.sizes[left], qint64(80));
    QVERIFY(!renderClasses(k, 20, 10).isNull());
}

void TestHsi::matchToAreas()
{
    // Left: material A, middle: A at half brightness, right: material B.
    Cube c(4, 12, 16);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 12; ++x)
            for (int b = 0; b < 16; ++b)
                c.band(b)[y * 12 + x] = float(x < 4 ? 100.0 + 10.0 * b : x < 8 ? 50.0 + 5.0 * b : 300.0 - 15.0 * b);
    QVector<double> a, bRef;
    for (int b = 0; b < 16; ++b) {
        a << 100.0 + 10.0 * b;
        bRef << 300.0 - 15.0 * b;
    }
    const AreaMatch m = matchToReferences(c, {a, bRef}, 5.0 / 57.29577951308232);
    QVERIFY(m.isValid());
    QCOMPARE(m.counts, (QVector<qint64>{32, 16}));  // brightness does not matter
    QCOMPARE(m.best[0], 0);
    QCOMPARE(m.best[5], 0);
    QCOMPARE(m.best[10], 1);
    QVERIFY(std::abs(m.angle[0]) < 1e-3f);
    // A tight angle leaves pixels of another shape unmatched.
    QVector<double> other(16, 1.0);
    const AreaMatch strict = matchToReferences(c, {other}, 1.0 / 57.29577951308232);
    QCOMPARE(strict.counts[0], qint64(0));
    QCOMPARE(strict.best[0], -1);
    const QImage img = renderAreaMatch(m, 12, 4, {QColor(255, 0, 0), QColor(0, 0, 255)}, true, QImage());
    QCOMPARE(img.pixelColor(1, 1), QColor(255, 0, 0));
    QCOMPARE(img.pixelColor(10, 1), QColor(0, 0, 255));
    QCOMPARE(renderAreaMatch(strict, 12, 4, {QColor(255, 0, 0)}, true, QImage()).pixelColor(1, 1), QColor(0, 0, 0));
}

void TestHsi::fixedStretch()
{
    // A/B renders the "before" picture with the stretch of the "after" one.
    Image2D a(4, 1), b(4, 1);
    for (int x = 0; x < 4; ++x) {
        a.data()[x] = 10.0 * x;
        b.data()[x] = 5.0 * x;  // half the contrast
    }
    std::array<Range, 3> ranges;
    const QImage after = renderComposite(a, a, a, ranges, false);
    const QImage before = renderComposite(b, b, b, ranges, true);
    QCOMPARE(after.pixelColor(3, 0).red(), 255);
    QVERIFY(std::abs(before.pixelColor(3, 0).red() - 128) <= 2);  // not stretched up to full contrast
    Cube c(2, 2, 3, 1.0f);
    const QVector<double> m = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    double scale = 0.0;
    renderRgb(c, m, 99.0, &scale);
    QCOMPARE(scale, 1.0);
    double fixed = 4.0;
    const QImage dim = renderRgb(c, m, 99.0, &fixed);
    QVERIFY(dim.pixelColor(0, 0).red() < 160);  // 0.25 linear, sRGB-encoded
    const QVector<QPointF> g = galleryPositions(c, 0.5, 1.5, 2);
    QCOMPARE(g.size(), 3);
    QCOMPARE(g[1], QPointF(2 + 2 + 0.5, 1.5));
}

void TestHsi::pca()
{
    // All variation along one spectral direction: one component explains it.
    Cube c(8, 8, 16);
    for (qsizetype p = 0; p < c.planeSize(); ++p)
        for (int b = 0; b < 16; ++b)
            c.band(b)[p] = float(50.0 + p * (1.0 + 0.1 * b));
    const Pca pc = principalComponents(c);
    QVERIFY(pc.isValid());
    QVERIFY(pc.explained()[0] > 0.999);
    const Image2D comp = pcaComponent(c, pc, 0);
    QVERIFY(comp.at(7, 7) > comp.at(0, 0));  // sign convention: increases with the data
    QVERIFY(!renderPcaComposite(c, pc).isNull());
    int r, g, b;
    maxContrastBands(c, r, g, b);
    QVERIFY(r > g && g > b);
}

void TestHsi::bandMath()
{
    Cube c(2, 2, 16);
    for (int b = 0; b < 16; ++b)
        std::fill(c.band(b), c.band(b) + 4, float(b + 1));
    QString error;
    auto f = BandMath::parse(QStringLiteral("(b3 - b1) / (b3 + b1) * 2 ^ 2 + sqrt(b16) - -1"), 16, {}, &error);
    QVERIFY2(f, qPrintable(error));
    QCOMPARE(f->evaluate(c).at(1, 1), (3.0 - 1.0) / (3.0 + 1.0) * 4.0 + 4.0 + 1.0);
    QCOMPARE(f->bandsUsed(), (QVector<int>{0, 2, 15}));

    QVERIFY(!BandMath::parse(QStringLiteral("b17"), 16, {}, &error));
    QVERIFY(error.contains(QLatin1String("17")));
    QVERIFY(!BandMath::parse(QStringLiteral("R531"), 16, {}, &error));  // needs wavelengths
    QVERIFY(!BandMath::parse(QStringLiteral("(b1"), 16, {}, &error));
    QVERIFY(!BandMath::parse(QStringLiteral("foo(b1)"), 16, {}, &error));
    QVERIFY(!BandMath::parse(QStringLiteral(""), 16, {}, &error));
    QVERIFY(!BandMath::parse(QStringLiteral("b1 b2"), 16, {}, &error));

    // Wavelengths: a synthetic grid, 480–600 nm every 8 nm.
    QVector<double> wl(16);
    for (int b = 0; b < 16; ++b)
        wl[b] = 480.0 + 8.0 * b;
    c.wavelengths = wl;
    auto pri = BandMath::parse(indexPresets()[0].formula, 16, wl, &error);
    QVERIFY2(pri, qPrintable(error));
    QCOMPARE(pri->resolved(), QStringLiteral("(b7 - b12) / (b7 + b12)"));  // 528 and 568 nm
    QCOMPARE(pri->evaluate(c).at(0, 0), (7.0 - 12.0) / (7.0 + 12.0));
    // NDVI needs 800 nm: rejected for this sensor.
    QVERIFY(!BandMath::parse(QStringLiteral("(R800 - R670) / (R800 + R670)"), 16, wl, &error));
    QVERIFY(error.contains(QLatin1String("800")));
    // Division by zero gives NaN, not a crash.
    auto z = BandMath::parse(QStringLiteral("b1 / (b1 - b1)"), 16, wl, &error);
    QVERIFY(std::isnan(z->evaluate(c).at(0, 0)));
}

void TestHsi::denoise()
{
    // A rank-2 scene (two materials mixed in varying proportion) plus noise.
    const int rows = 48, cols = 48;
    Cube truth(rows, cols, 16), noisy(rows, cols, 16);
    Rng rng;
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x) {
            const double a = 0.5 + 0.5 * std::sin(x / 6.0) * std::cos(y / 9.0);
            for (int b = 0; b < 16; ++b) {
                const double v = 200.0 + 400.0 * a * (1.0 + 0.05 * b) + 300.0 * (1.0 - a) * (1.5 - 0.06 * b);
                truth.band(b)[y * cols + x] = float(v);
                noisy.band(b)[y * cols + x] = float(v + 10.0 * rng.gauss());
            }
        }
    const double before = rmsDiff(noisy, truth);
    const Cube mnf = mnfDenoise(noisy, 2);
    QVERIFY2(rmsDiff(mnf, truth) < before * 0.6, qPrintable(QStringLiteral("%1 vs %2").arg(rmsDiff(mnf, truth)).arg(before)));
    QVERIFY(mnf.history.last().contains(QLatin1String("MNF")));
    const QVector<double> snr = mnfSnr(noisy);
    QCOMPARE(snr.size(), 16);
    QVERIFY(snr[0] > 100.0 * snr[5]);
    const Cube guided = guidedDenoise(noisy, 2, 0.01);
    QVERIFY(rmsDiff(guided, truth) < before);
    const Cube blurred = gaussianBlur(noisy, 1.0);
    QVERIFY(rmsDiff(blurred, truth) < before);
    QCOMPARE(mnfDenoise(noisy, 16).data(), noisy.data());  // keeping everything changes nothing
}

void TestHsi::reflectanceAndUpscale()
{
    Cube c(4, 6, 16);
    for (int b = 0; b < 16; ++b)
        for (qsizetype i = 0; i < c.planeSize(); ++i)
            c.band(b)[i] = float((b + 1) * (i % 6 < 3 ? 100 : 50));
    const Cube r = toReflectance(c, QRect(0, 0, 3, 4), 0.9);
    QCOMPARE(r.units, QStringLiteral("reflectance"));
    QVERIFY(std::abs(r.at(1, 1, 5) - 0.9f) < 1e-6f);
    QVERIFY(std::abs(r.at(4, 1, 5) - 0.45f) < 1e-6f);
    QCOMPARE(toReflectance(c, QRect(), 1.0).data(), c.data());
    const RegionStats st = regionStats(c, QRect(0, 0, 3, 4));
    QCOMPARE(st.count, qint64(12));
    QCOMPARE(st.mean[2], 300.0);
    QCOMPARE(st.std[2], 0.0);
    QVector<quint8> mask(c.planeSize(), 0);
    mask[5] = 1;
    QCOMPARE(maskStats(c, mask).mean[0], 50.0);

    for (Interpolation m : {Interpolation::Nearest, Interpolation::Bilinear, Interpolation::Bicubic}) {
        const Cube u = upscale(uniformCube(3, 5, QVector<double>(16, 7.0)), 3, m);
        QCOMPARE(u.rows(), 9);
        QCOMPARE(u.cols(), 15);
        QVERIFY(std::abs(u.at(8, 4, 3) - 7.0f) < 1e-4f);
    }
}

void TestHsi::processChainOnSharedCube()
{
    // Every combination of steps, on a cube that shares its data with
    // another (as the workspace's unprocessed cube does). The filters write
    // from several threads; they must not touch the shared input (this used
    // to crash with guided denoise followed by anti-aliasing).
    Image2D raw(96, 80);
    for (qsizetype i = 0; i < raw.pixelCount(); ++i)
        raw.data()[i] = double((i * 37) % 997);
    for (ExtractMethod m : {ExtractMethod::Subsample, ExtractMethod::Ppid}) {
        ExtractOptions o;
        o.bitDepth = 0;
        o.method = m;
        const Cube base = extract(raw, o).cube;
        const QVector<float> before = base.data();
        for (int mask = 0; mask < 32; ++mask) {
            ProcessingSettings s;
            s.reflectance = mask & 1;
            s.whiteReference = QRect(1, 1, 4, 4);
            s.mnf = mask & 2;
            s.mnfKeep = 4;
            s.guided = mask & 4;
            s.antiAlias = mask & 8;
            s.upscale = (mask & 16) ? 2 : 1;
            for (int repeat = 0; repeat < 3; ++repeat) {
                const Cube shared = base;
                const Cube out = process(shared, s);
                QCOMPARE(out.rows(), base.rows() * s.upscale);
                QVERIFY(base.data() == before);
            }
        }
    }
}

void TestHsi::spectraCsv()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("s.csv"));
    QString error;
    QVERIFY(writeSpectraCsv(path, {500, 510}, {{QStringLiteral("ROI 1"), {1, 2}, {0.1, 0.2}, 12}}, &error));
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QString t = QString::fromUtf8(f.readAll());
    QCOMPARE(t, QStringLiteral("band,wavelength_nm,\"ROI 1 mean\",\"ROI 1 std\"\n1,500,1,0.1\n2,510,2,0.2\npixels,,12,\n"));
}

void TestHsi::referenceFrames()
{
    // Pixel response varies (vignetting): raw = gain · (ρ · L) + dark.
    Image2D raw(8, 4), dark(8, 4), white(8, 4);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 8; ++x) {
            const double gain = 0.5 + 0.1 * x, d = 100.0 + y;
            const double rho = x < 4 ? 0.2 : 0.6;
            raw.data()[y * 8 + x] = gain * rho * 1000.0 + d;
            white.data()[y * 8 + x] = gain * 1.0 * 1000.0 + d;
            dark.data()[y * 8 + x] = d;
        }
    ReferenceFrames f;
    f.dark = dark;
    const FrameCorrection dOnly = applyReferenceFrames(raw, f);
    QVERIFY(dOnly.ok() && !dOnly.reflectance);
    QCOMPARE(dOnly.image.at(2, 1), 0.7 * 0.2 * 1000.0);
    f.white = white;
    const FrameCorrection fr = applyReferenceFrames(raw, f);
    QVERIFY(fr.ok() && fr.reflectance);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 8; ++x)
            QVERIFY(std::abs(fr.image.at(x, y) - (x < 4 ? 0.2 : 0.6)) < 1e-12);  // gain and dark removed
    // White reference taken with half the exposure, reference reflectance 0.99.
    f.white = Image2D(8, 4);
    for (qsizetype i = 0; i < white.pixelCount(); ++i)
        f.white.data()[i] = (white.constData()[i] - dark.constData()[i]) / 2.0 + dark.constData()[i];
    f.sceneExposure = 10.0;
    f.whiteExposure = 5.0;
    f.whiteReflectance = 0.99;
    QVERIFY(std::abs(applyReferenceFrames(raw, f).image.at(6, 2) - 0.6 * 0.99) < 1e-12);
    // A dead white pixel gives NaN; mismatched sizes are refused.
    f.white.data()[0] = f.dark.constData()[0];
    QVERIFY(std::isnan(applyReferenceFrames(raw, f).image.at(0, 0)));
    f.dark = Image2D(4, 4);
    QVERIFY(!applyReferenceFrames(raw, f).ok());
}

void TestHsi::spectralCorrection()
{
    // Channels with a broad background and a second-order bump: overlapping.
    SpectralResponse r;
    for (int l = 450; l <= 650; ++l)
        r.lambda << l;
    for (int b = 0; b < 16; ++b) {
        QVector<double> q;
        const double peak = 480.0 + 9.0 * b;
        for (double l : r.lambda)
            q << 0.2 * std::exp(-0.5 * std::pow((l - peak) / 8.0, 2.0)) + 0.05 * std::exp(-0.5 * std::pow((l - 640.0 + 3.0 * b) / 25.0, 2.0)) + 0.03;
        r.qe << q;
    }
    const SpectralCorrection c = computeCorrection(r, 8.0, 1e-4);
    QVERIFY(c.isValid());
    QCOMPARE(c.outBands, 16);
    QCOMPARE(c.centres, r.peaks());
    // Narrower targets than the channels can reach leave a larger residual;
    // targets of the channels' own widths are matched closely.
    const SpectralCorrection own = computeCorrection(r, 0.0, 1e-4);
    QVERIFY2(own.residual < 0.2 && own.residual < c.residual, qPrintable(QStringLiteral("%1 %2").arg(own.residual).arg(c.residual)));
    // Narrower targets and less regularisation cost more noise.
    QVERIFY(noiseGain(c) > noiseGain(own) && noiseGain(own) > noiseGain(computeCorrection(r, 0.0, 1.0)));
    SpectralCorrection identity;
    identity.inBands = identity.outBands = 3;
    identity.matrix = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    QVERIFY(std::abs(noiseGain(identity) - 1.0) < 1e-12);
    // The corrected responses are narrower: less of their energy lies further than 20 nm from the peak.
    const SpectralResponse cr = correctedResponse(r, c);
    auto outside = [](const SpectralResponse& s, int b, double peak) {
        double in = 0.0, out = 0.0;
        for (int i = 0; i < s.samples(); ++i)
            (std::abs(s.lambda[i] - peak) <= 20.0 ? in : out) += std::abs(s.qe[b][i]);
        return out / (in + out);
    };
    QVERIFY(outside(cr, 7, c.centres[7]) < 0.5 * outside(r, 7, c.centres[7]));
    // On a cube: corrected = C · measured, wavelengths = centres.
    Cube cube(2, 2, 16);
    QVector<double> s(16);
    for (int b = 0; b < 16; ++b) {
        s[b] = 10.0 + b * b;
        std::fill(cube.band(b), cube.band(b) + 4, float(s[b]));
    }
    const Cube out = applyCorrection(cube, c);
    double expect = 0.0;
    for (int b = 0; b < 16; ++b)
        expect += c.matrix[5 * 16 + b] * s[b];
    QVERIFY(std::abs(out.at(1, 1, 5) - expect) < 1e-3 * std::abs(expect) + 1e-4);
    QCOMPARE(out.wavelengths, c.centres);
    QVERIFY(out.history.last().contains(QLatin1String("spectral correction")));
    // A flat spectrum keeps roughly its level per band (targets have the response's area).
    const QVector<double> flatS = sense(r, [](double) { return 1.0; });
    double flatOut = 0.0;
    for (int b = 0; b < 16; ++b)
        flatOut += c.matrix[8 * 16 + b] * flatS[b];
    QVERIFY(std::abs(flatOut / flatS[8] - 1.0) < 0.35);
    // The measured widths as targets.
    QVERIFY(computeCorrection(r, 0.0, 0.01).isValid());
}

void TestHsi::correctionMatrixFile()
{
    QTemporaryDir dir;
    const QString csv = dir.filePath(QStringLiteral("corr.csv"));
    QFile f(csv);
    QVERIFY(f.open(QIODevice::WriteOnly));
    QString t = QStringLiteral("# correction matrix\ncentre,b1,b2,b3\n");
    t += QStringLiteral("500, 1, -0.2, 0\n510, -0.1, 1.1, -0.1\n");
    f.write(t.toUtf8());
    f.close();
    const CorrectionFile c = loadCorrectionMatrix(csv, 3);
    QVERIFY2(c.ok(), qPrintable(c.error));
    QCOMPARE(c.correction.outBands, 2);
    QCOMPARE(c.correction.centres, (QVector<double>{500, 510}));
    QCOMPARE(c.correction.matrix[4], 1.1);
    QVERIFY(!loadCorrectionMatrix(csv, 5).ok());  // rows of 4 fit neither 5 nor 5 + 1
    // .mat with variable C.
    MatFileWriter w;
    Image2D m(3, 2);  // 2 rows × 3 columns
    for (int i = 0; i < 6; ++i)
        m.data()[i] = i;
    QVERIFY(w.addImage(QStringLiteral("C"), m));
    const QString mat = dir.filePath(QStringLiteral("corr.mat"));
    QVERIFY(w.save(mat));
    const CorrectionFile cm = loadCorrectionMatrix(mat, 3);
    QVERIFY2(cm.ok(), qPrintable(cm.error));
    QCOMPARE(cm.correction.matrix, (QVector<double>{0, 1, 2, 3, 4, 5}));
}

void TestHsi::sensorProfile()
{
    QTemporaryDir dir;
    SensorProfile p;
    p.name = QStringLiteral("FP sensor");
    p.layout = MosaicLayout::rowMajor();
    p.bitDepth = 12;
    p.method = 3;
    p.responsePath = QStringLiteral("C:/data/qe.mat");
    p.spectralCorrection = true;
    p.correctionFwhm = 9.5;
    p.darkPath = QStringLiteral("dark.tif");
    p.whiteExposure = 4.5;
    p.whiteReflectance = 0.99;
    QString error;
    const QString path = dir.filePath(QStringLiteral("s.iatsensor"));
    QVERIFY(writeSensorProfile(p, path, &error));
    const auto q = readSensorProfile(path, &error);
    QVERIFY2(q.has_value(), qPrintable(error));
    QCOMPARE(q->name, p.name);
    QVERIFY(q->layout == p.layout);
    QCOMPARE(q->bitDepth, 12);
    QCOMPARE(q->method, 3);
    QCOMPARE(q->responsePath, p.responsePath);
    QVERIFY(q->spectralCorrection);
    QCOMPARE(q->correctionFwhm, 9.5);
    QCOMPARE(q->darkPath, p.darkPath);
    QCOMPARE(q->whiteExposure, 4.5);
    QCOMPARE(q->whiteReflectance, 0.99);
    QFile junk(dir.filePath(QStringLiteral("j.iatsensor")));
    QVERIFY(junk.open(QIODevice::WriteOnly));
    junk.write("{}");
    junk.close();
    QVERIFY(!readSensorProfile(dir.filePath(QStringLiteral("j.iatsensor")), &error).has_value());
}

void TestHsi::preprocessing()
{
    // Two pixels with the same spectral shape, one scaled and offset.
    Cube c(1, 2, 8);
    for (int b = 0; b < 8; ++b) {
        const double base = 0.1 + 0.05 * b + 0.02 * (b % 3);
        c.band(b)[0] = float(base);
        c.band(b)[1] = float(3.0 * base + 0.2);
    }
    const Cube snv = normaliseSpectra(c, SpectralNormalisation::Snv);
    double m = 0.0, v = 0.0;
    for (int b = 0; b < 8; ++b)
        m += snv.at(0, 0, b);
    m /= 8;
    for (int b = 0; b < 8; ++b)
        v += std::pow(snv.at(0, 0, b) - m, 2.0);
    QVERIFY(std::abs(m) < 1e-6 && std::abs(std::sqrt(v / 7) - 1.0) < 1e-5);
    for (int b = 0; b < 8; ++b)
        QVERIFY(std::abs(snv.at(0, 0, b) - snv.at(1, 0, b)) < 1e-5);  // offset and scale removed
    const Cube msc = normaliseSpectra(c, SpectralNormalisation::Msc);
    for (int b = 0; b < 8; ++b)
        QVERIFY(std::abs(msc.at(0, 0, b) - msc.at(1, 0, b)) < 1e-5);
    const Cube l2 = normaliseSpectra(c, SpectralNormalisation::UnitLength);
    double n = 0.0;
    for (int b = 0; b < 8; ++b)
        n += l2.at(0, 0, b) * l2.at(0, 0, b);
    QVERIFY(std::abs(n - 1.0) < 1e-5);
    QCOMPARE(normaliseSpectra(c, SpectralNormalisation::UnitMax).at(0, 0, 7), 1.0f);
    QCOMPARE(snv.units, QStringLiteral("normalised"));
    // Savitzky–Golay: the derivative of a straight line is its slope (per nm with wavelengths).
    Cube line(1, 1, 9);
    for (int b = 0; b < 9; ++b) {
        line.band(b)[0] = float(2.0 + 0.5 * (480 + 10 * b));
        line.wavelengths << 480.0 + 10.0 * b;
    }
    const Cube d1 = savitzkyGolay(line, 5, 2, 1);
    for (int b = 0; b < 9; ++b)
        QVERIFY2(std::abs(d1.at(0, 0, b) - 0.5f) < 1e-3f, qPrintable(QString::number(d1.at(0, 0, b))));
    const Cube d2 = savitzkyGolay(line, 5, 2, 2);
    QVERIFY(std::abs(d2.at(0, 0, 4)) < 1e-4f);
    const Cube smooth = savitzkyGolay(line, 5, 2, 0);
    QVERIFY(std::abs(smooth.at(0, 0, 0) - line.at(0, 0, 0)) < 1e-3f);  // a polynomial is kept
    // Absorbance.
    Cube r(1, 1, 2);
    r.band(0)[0] = 0.1f;
    r.band(1)[0] = 1.0f;
    const Cube a = toAbsorbance(r);
    QVERIFY(std::abs(a.at(0, 0, 0) - 1.0f) < 1e-6f && std::abs(a.at(0, 0, 1)) < 1e-6f);
    // In the chain.
    ProcessingSettings ps;
    ps.normalisation = SpectralNormalisation::Snv;
    ps.derivative = 1;
    QVERIFY(ps.any());
    const Cube chained = process(line, ps);
    QVERIFY(chained.history.size() >= 2);
}

void TestHsi::polygonSelection()
{
    Image2D x(3, 1), y(3, 1);
    x.data()[0] = 0.5; y.data()[0] = 0.5;
    x.data()[1] = 2.0; y.data()[1] = 2.0;
    x.data()[2] = std::nan(""); y.data()[2] = 0.5;
    const QPolygonF square({QPointF(0, 0), QPointF(1, 0), QPointF(1, 1), QPointF(0, 1)});
    QCOMPARE(selectInPolygon(x, y, square), (QVector<quint8>{1, 0, 0}));
}

void TestHsi::localMatlabParity()
{
    const QString dirPath = qEnvironmentVariable("IAT_LOCAL_HSI_DATA");
    if (dirPath.isEmpty())
        QSKIP("IAT_LOCAL_HSI_DATA is not set (private sensor data; never committed)");
    const QDir dir(dirPath);
    int compared = 0;
    for (const QString& tif : dir.entryList({QStringLiteral("*.tif"), QStringLiteral("*.tiff")}, QDir::Files)) {
        const QString base = QFileInfo(tif).completeBaseName();
        const QString matPath = dir.filePath(base + QStringLiteral("_cube.mat"));
        if (!QFile::exists(matPath))
            continue;
        const ImageFrame raw = ImageFileReader::read(dir.filePath(tif));
        QVERIFY2(raw.ok(), qPrintable(raw.error));
        const MatFileContents saved = MatFileReader::read(matPath);
        QVERIFY2(saved.ok(), qPrintable(saved.error));
        auto var = [&](const char* name) -> const MatVariable* {
            for (const MatVariable& v : saved.variables)
                if (v.name == QLatin1String(name) && v.supported)
                    return &v;
            return nullptr;
        };
        const MatVariable* cubeVar = var("cube");
        QVERIFY(cubeVar);
        ExtractOptions o;  // hsi_reconstruct.m's defaults: bit depth 14, black level 0
        const ExtractResult r = extract(raw.data.frame(0), o);
        QVERIFY2(r.ok(), qPrintable(r.error));
        QCOMPARE(r.cube.rows(), cubeVar->data.rows());
        QCOMPARE(r.cube.cols(), cubeVar->data.cols());
        for (int b = 0; b < 16; ++b) {
            const Image2D f = cubeVar->data.frame(b);
            for (qsizetype i = 0; i < f.pixelCount(); ++i)
                QCOMPARE(double(r.cube.band(b)[i]), f.constData()[i]);
        }
        std::printf("  %s: cube identical to MATLAB (%d x %d x 16)\n", qPrintable(tif), r.cube.rows(), r.cube.cols());

        const QString qePath = dir.filePath(QStringLiteral("reconstruction_matrix.mat"));
        if (QFile::exists(qePath) && var("M") && var("reconRGB")) {
            const ResponseFile rf = loadResponse(qePath);
            QVERIFY2(rf.ok(), qPrintable(rf.error));
            const QVector<double> M = xyzMatrix(rf.response, 20.0, 0.05);
            const Image2D savedM = var("M")->data.frame(0);
            double worst = 0.0, scale = 0.0;
            for (int c = 0; c < 3; ++c)
                for (int b = 0; b < 16; ++b) {
                    worst = std::max(worst, std::abs(M[c * 16 + b] - savedM.at(b, c)));
                    scale = std::max(scale, std::abs(savedM.at(b, c)));
                }
            std::printf("  XYZ matrix M: largest difference %.3g (of %.3g)\n", worst, scale);
            QVERIFY(worst < 1e-9 * scale);
            ColourSettings cs;  // grey world, as the saved run
            const QVector<double> rgb = rgbMatrix(r.cube, M, cs, rf.response);
            const Image2D savedRgb = var("reconRGB")->data.frame(0);
            worst = scale = 0.0;
            for (int c = 0; c < 3; ++c)
                for (int b = 0; b < 16; ++b) {
                    worst = std::max(worst, std::abs(rgb[c * 16 + b] - savedRgb.at(b, c)));
                    scale = std::max(scale, std::abs(savedRgb.at(b, c)));
                }
            std::printf("  reconRGB: largest difference %.3g (of %.3g)\n", worst, scale);
            QVERIFY(worst < 1e-9 * scale);
        }
        ++compared;
    }
    if (compared == 0)
        QSKIP("no <name>.tif + <name>_cube.mat pair in IAT_LOCAL_HSI_DATA");
}

void TestHsi::mosaicPresets()
{
    using P = MosaicLayout::Preset;
    QCOMPARE(MosaicLayout::preset(P::RowByRow).toString(), QStringLiteral("1 2 3 4; 5 6 7 8; 9 10 11 12; 13 14 15 16"));
    QCOMPARE(MosaicLayout::preset(P::RowByRowVertical).toString(), QStringLiteral("1 5 9 13; 2 6 10 14; 3 7 11 15; 4 8 12 16"));
    QCOMPARE(MosaicLayout::preset(P::Serpentine).toString(), QStringLiteral("1 2 3 4; 8 7 6 5; 9 10 11 12; 16 15 14 13"));
    QCOMPARE(MosaicLayout::preset(P::SerpentineVertical).toString(), QStringLiteral("1 8 9 16; 2 7 10 15; 3 6 11 14; 4 5 12 13"));
    QVERIFY(MosaicLayout::defaultLayout() == MosaicLayout::preset(P::SerpentineVertical));
    for (P p : {P::RowByRow, P::RowByRowVertical, P::Serpentine, P::SerpentineVertical}) {
        QVERIFY(MosaicLayout::preset(p).isValid());
        QCOMPARE(MosaicLayout::presetOf(MosaicLayout::preset(p)), std::optional<P>(p));
    }
    MosaicLayout other = MosaicLayout::rowMajor();
    std::swap(other.channel[0], other.channel[5]);
    QVERIFY(!MosaicLayout::presetOf(other));
}

QTEST_GUILESS_MAIN(TestHsi)
#include "tst_hsi.moc"
