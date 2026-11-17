// Unit tests for the image processing pipeline (core/Isp.h): the tables,
// every stage and every algorithm, on small synthetic images.

#include "core/Isp.h"

#include <QtTest>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>

using namespace iat;
using isp::Pipeline;
using isp::StageId;

namespace {

// Reproducible pseudo-random numbers in [0, 1).
struct Rng {
    quint32 s = 12345u;
    double operator()()
    {
        s = s * 1664525u + 1013904223u;
        return double(s >> 8) / double(1u << 24);
    }
    double gauss()  // roughly normal, σ = 1
    {
        double a = 0.0;
        for (int i = 0; i < 12; ++i)
            a += (*this)();
        return a - 6.0;
    }
};

Image2D flat(int w, int h, double level, double noise, quint32 seed = 1)
{
    Image2D img(w, h);
    Rng rng{seed};
    for (qsizetype i = 0; i < img.pixelCount(); ++i)
        img.data()[i] = level + noise * rng.gauss();
    return img;
}

// Left half dark, right half bright, with noise.
Image2D stepEdge(int w, int h, double lo, double hi, double noise)
{
    Image2D img = flat(w, h, 0.0, noise, 7);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            img.data()[qsizetype(y) * w + x] += x < w / 2 ? lo : hi;
    return img;
}

// A 14-bit scene with a dark part, a bright part and a smooth ramp.
Image2D scene(int w = 96, int h = 80)
{
    Image2D img(w, h);
    Rng rng{99};
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            double v = 80.0 + 3.0 * x + 2.0 * y;                 // dim ramp
            if (x > 2 * w / 3 && y < h / 2)
                v = 9000.0 + 60.0 * (x - 2 * w / 3) + 20.0 * y;  // bright window
            if ((x / 4 + y / 4) % 2 == 0)
                v *= 1.05;                                       // texture
            img.data()[qsizetype(y) * w + x] = std::round(std::clamp(v + 4.0 * rng.gauss(), 0.0, 16383.0));
        }
    }
    return img;
}

double stdDev(const Image2D& img, int x0, int x1)
{
    double s = 0.0, s2 = 0.0;
    qsizetype n = 0;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = x0; x < x1; ++x) {
            const double v = img.at(x, y);
            s += v;
            s2 += v * v;
            ++n;
        }
    }
    const double m = s / double(n);
    return std::sqrt(std::max(0.0, s2 / double(n) - m * m));
}

double meanOf(const Image2D& img, int x0, int x1)
{
    double s = 0.0;
    qsizetype n = 0;
    for (int y = 0; y < img.height(); ++y)
        for (int x = x0; x < x1; ++x, ++n)
            s += img.at(x, y);
    return s / double(n);
}

std::pair<double, double> minMax(const Image2D& img)
{
    const auto [mn, mx] = std::minmax_element(img.constData(), img.constData() + img.pixelCount());
    return {*mn, *mx};
}

Pipeline only(StageId id, const char* algorithm)
{
    Pipeline p;
    p.setEnabled(id, true);
    if (!p.setAlgorithm(id, algorithm))
        qFatal("unknown algorithm %s", algorithm);
    return p;
}

} // namespace

class TestIsp : public QObject {
    Q_OBJECT

private slots:
    void tablesAreConsistent()
    {
        const auto& stages = isp::stages();
        QCOMPARE(int(stages.size()), 10);
        std::set<QString> keys;
        for (std::size_t i = 0; i < stages.size(); ++i) {
            const isp::StageSpec& s = stages[i];
            QCOMPARE(isp::stageIndex(s.id), int(i));
            QVERIFY(keys.insert(QString::fromLatin1(s.key)).second);
            QVERIFY(std::strlen(s.name) > 0 && std::strlen(s.description) > 0);
            QVERIFY(!s.algorithms.empty());
            std::set<QString> algIds;
            for (const isp::AlgorithmSpec& a : s.algorithms) {
                QVERIFY(algIds.insert(QString::fromLatin1(a.id)).second);
                QVERIFY(std::strlen(a.name) > 0 && std::strlen(a.description) > 0);
                std::set<QString> paramIds;
                for (const isp::ParamSpec& p : a.params) {
                    QVERIFY(paramIds.insert(QString::fromLatin1(p.id)).second);
                    QVERIFY2(p.max > p.min && p.def >= p.min && p.def <= p.max, p.id);
                    if (p.logScale)
                        QVERIFY2(p.min > 0.0, p.id);
                    if (p.kind == isp::ParamKind::Choice)
                        QCOMPARE(int(p.choices.size()), int(p.max) + 1);
                }
            }
        }
        // The order of a camera ISP: sensor corrections, then noise, then tone.
        QVERIFY(isp::stageIndex(StageId::BlackLevel) < isp::stageIndex(StageId::DefectPixel));
        QVERIFY(isp::stageIndex(StageId::DefectLine) < isp::stageIndex(StageId::Denoise));
        QVERIFY(isp::stageIndex(StageId::Denoise) < isp::stageIndex(StageId::ToneMap));
        QVERIFY(isp::stageIndex(StageId::ToneMap) < isp::stageIndex(StageId::Sharpen));
        QCOMPARE(isp::stageIndex(StageId::Output), 9);
        // Monochrome only: no colour stages.
        for (const isp::StageSpec& s : stages)
            QVERIFY(QString::fromLatin1(s.key) != QLatin1String("white_balance"));
    }

    void defaultPipelineChangesNothing()
    {
        const Image2D img = scene();
        Pipeline p;
        QVERIFY(!p.isActive());
        QVERIFY(p.summary(false).isEmpty());
        const isp::Result r = isp::run(img, p);
        QVERIFY(!r.displayReferred);
        QCOMPARE(r.image.constData(), img.constData());  // the very same data
        QCOMPARE(r.notes.size(), 10);
    }

    void settings()
    {
        Pipeline p;
        QVERIFY(!p.isActive());

        QVERIFY(p.setAlgorithm(StageId::Denoise, "guided"));
        QVERIFY(!p.setAlgorithm(StageId::Denoise, "no such thing"));
        QVERIFY(p.setValue(StageId::Denoise, "radius", 100.0));
        QCOMPARE(p.value(StageId::Denoise, "radius"), 16.0);  // clamped
        QVERIFY(!p.setValue(StageId::Denoise, "sigma", 1.0));  // belongs to another algorithm
        QVERIFY(std::isnan(p.value(StageId::Denoise, "sigma")));
        p.setAlgorithm(StageId::Denoise, "gaussian");
        p.setAlgorithm(StageId::Denoise, "guided");
        QCOMPARE(p.value(StageId::Denoise, "radius"), 16.0);  // kept while another algorithm was selected

        p.setEnabled(StageId::Denoise, true);
        p.setEnabled(StageId::ToneMap, true);
        p.setAlgorithm(StageId::ToneMap, "clahe");
        QVERIFY(p.isActive() && p.isDisplayReferred());
        QCOMPARE(p.summary(false), QStringLiteral("Denoise: Guided filter; Tone mapping: CLAHE"));

        // Denoising keeps the value scale, tone mapping changes it.
        Pipeline q;
        q.setEnabled(StageId::Denoise, true);
        QVERIFY(q.sameScaleAs(Pipeline()));
        QVERIFY(!p.sameScaleAs(q));
        Pipeline p2 = p;
        p2.setValue(StageId::ToneMap, "clip", 4.0);
        QVERIFY(!p.sameScaleAs(p2));
        p2 = p;
        p2.setValue(StageId::Denoise, "radius", 3.0);
        QVERIFY(p.sameScaleAs(p2) && p != p2);

        p.resetStage(StageId::Denoise);
        QVERIFY(!p.isEnabled(StageId::Denoise));
        QCOMPARE(p.config(StageId::Denoise), Pipeline().config(StageId::Denoise));
    }

    void blackLevel()
    {
        const Image2D img = flat(40, 30, 500.0, 3.0);
        Pipeline p = only(StageId::BlackLevel, "fixed");
        p.setValue(StageId::BlackLevel, "offset", 64.0);
        isp::Result r = isp::run(img, p);
        for (qsizetype i = 0; i < img.pixelCount(); ++i)
            QCOMPARE(r.image.constData()[i], img.constData()[i] - 64.0);
        QVERIFY(r.notes[0].contains(QLatin1String("64")));

        // Clip at 0 (the default) or keep the negative values.
        p.setValue(StageId::BlackLevel, "offset", 502.0);
        QVERIFY(minMax(isp::run(img, p).image).first == 0.0);
        p.setValue(StageId::BlackLevel, "clip", 1.0);
        QVERIFY(minMax(isp::run(img, p).image).first < 0.0);

        // Automatic: a low percentile lands near the bottom of the data.
        p = only(StageId::BlackLevel, "auto");
        r = isp::run(img, p);
        QVERIFY(std::abs(meanOf(r.image, 0, 40) - 10.0) < 6.0);
    }

    void defectPixels_data()
    {
        QTest::addColumn<QString>("algorithm");
        QTest::newRow("minmax") << "minmax";
        QTest::newRow("median") << "median";
    }
    void defectPixels()
    {
        QFETCH(QString, algorithm);
        Image2D clean = stepEdge(48, 40, 200.0, 3000.0, 2.0);  // an edge must survive
        Image2D img = clean;
        img.data()[10 * 48 + 10] = 16383.0;  // hot
        img.data()[30 * 48 + 20] = 0.0;      // dead
        img.data()[0] = 16383.0;             // hot, in the corner

        Pipeline p = only(StageId::DefectPixel, algorithm.toLatin1().constData());
        isp::Result r = isp::run(img, p);
        QVERIFY(std::abs(r.image.at(10, 10) - 200.0) < 10.0);
        QVERIFY(std::abs(r.image.at(20, 30) - 200.0) < 10.0);
        QVERIFY(std::abs(r.image.at(0, 0) - 200.0) < 10.0);
        qsizetype changed = 0;
        for (qsizetype i = 0; i < img.pixelCount(); ++i)
            changed += r.image.constData()[i] != img.constData()[i];
        QCOMPARE(changed, qsizetype(3));  // nothing else, and not the edge
        QVERIFY(r.notes[isp::stageIndex(StageId::DefectPixel)].startsWith(QLatin1String("3 ")));

        p.setValue(StageId::DefectPixel, "type", 1.0);  // hot pixels only
        r = isp::run(img, p);
        QCOMPARE(r.image.at(20, 30), 0.0);
        QVERIFY(r.image.at(10, 10) < 300.0);
        p.setValue(StageId::DefectPixel, "type", 2.0);  // dead pixels only
        r = isp::run(img, p);
        QCOMPARE(r.image.at(10, 10), 16383.0);
        QVERIFY(r.image.at(20, 30) > 100.0);
    }

    void defectLines()
    {
        // A smooth ramp with noise; one hot row, one dead column.
        Image2D clean(64, 48);
        Rng rng{5};
        for (int y = 0; y < 48; ++y)
            for (int x = 0; x < 64; ++x)
                clean.data()[y * 64 + x] = 1000.0 + 5.0 * x + 3.0 * y + 2.0 * rng.gauss();
        Image2D img = clean;
        for (int x = 0; x < 64; ++x)
            img.data()[17 * 64 + x] += 400.0;
        for (int y = 0; y < 48; ++y)
            img.data()[y * 64 + 40] = 0.0;

        Pipeline p = only(StageId::DefectLine, "replace");
        isp::Result r = isp::run(img, p);
        double worst = 0.0;
        for (qsizetype i = 0; i < img.pixelCount(); ++i)
            worst = std::max(worst, std::abs(r.image.constData()[i] - clean.constData()[i]));
        QVERIFY2(worst < 12.0, qPrintable(QStringLiteral("%1, %2").arg(worst).arg(r.notes[2])));
        QCOMPARE(r.notes[isp::stageIndex(StageId::DefectLine)], QStringLiteral("1 rows and 1 columns replaced"));

        // Rows only: the column stays dead.
        p.setValue(StageId::DefectLine, "dir", 1.0);
        r = isp::run(img, p);
        QCOMPARE(r.image.at(40, 5), 0.0);
        QVERIFY(std::abs(r.image.at(5, 17) - clean.at(5, 17)) < 12.0);

        // A clean image is left alone.
        p.setValue(StageId::DefectLine, "dir", 0.0);
        r = isp::run(clean, p);
        for (qsizetype i = 0; i < clean.pixelCount(); ++i)
            QCOMPARE(r.image.constData()[i], clean.constData()[i]);

        // Structure in the picture is not a defect: a sharp horizontal edge
        // across the whole frame, and a brightness slope up to the border.
        Image2D edge(64, 48);
        for (int y = 0; y < 48; ++y)
            for (int x = 0; x < 64; ++x)
                edge.data()[y * 64 + x] = (y < 20 ? 3000.0 : 500.0) + 8.0 * y + 2.0 * rng.gauss();
        r = isp::run(edge, p);
        QCOMPARE(r.notes[isp::stageIndex(StageId::DefectLine)], QStringLiteral("0 rows and 0 columns replaced"));

        // Defects at the border and side by side: the first row is dead, and
        // columns 30 and 31 are both hot.
        img = clean;
        for (int x = 0; x < 64; ++x)
            img.data()[x] = 0.0;
        for (int y = 0; y < 48; ++y) {
            img.data()[y * 64 + 30] = 16383.0;
            img.data()[y * 64 + 31] = 16383.0;
        }
        r = isp::run(img, p);
        QCOMPARE(r.notes[isp::stageIndex(StageId::DefectLine)], QStringLiteral("1 rows and 2 columns replaced"));
        worst = 0.0;
        for (qsizetype i = 0; i < img.pixelCount(); ++i)
            worst = std::max(worst, std::abs(r.image.constData()[i] - clean.constData()[i]));
        QVERIFY2(worst < 16.0, qPrintable(QString::number(worst)));

        // Destripe: every row has its own offset (σ = 6 DN).
        Image2D striped = clean;
        for (int y = 0; y < 48; ++y) {
            const double offset = 6.0 * rng.gauss();
            for (int x = 0; x < 64; ++x)
                striped.data()[y * 64 + x] += offset;
        }
        p = only(StageId::DefectLine, "destripe");
        p.setValue(StageId::DefectLine, "dir", 1.0);
        r = isp::run(striped, p);
        auto rowJitter = [](const Image2D& im) {
            double acc = 0.0;
            for (int y = 1; y < im.height() - 1; ++y) {
                double a = 0.0, b = 0.0, c = 0.0;
                for (int x = 0; x < im.width(); ++x) {
                    a += im.at(x, y - 1);
                    b += im.at(x, y);
                    c += im.at(x, y + 1);
                }
                acc += std::abs(b - 0.5 * (a + c)) / im.width();
            }
            return acc / (im.height() - 2);
        };
        QVERIFY(rowJitter(striped) > 4.0);
        QVERIFY2(rowJitter(r.image) < 0.3 * rowJitter(striped),
                 qPrintable(QStringLiteral("%1 -> %2").arg(rowJitter(striped)).arg(rowJitter(r.image))));
        // Only whole-row offsets were removed: the picture is still the ramp.
        QVERIFY(std::abs(meanOf(r.image, 0, 64) - meanOf(clean, 0, 64)) < 2.0);
    }

    void digitalGain()
    {
        const Image2D img = flat(32, 32, 250.0, 2.0);
        Pipeline p = only(StageId::Gain, "manual");
        p.setValue(StageId::Gain, "gain", 4.0);
        isp::Result r = isp::run(img, p);
        for (qsizetype i = 0; i < img.pixelCount(); ++i)
            QCOMPARE(r.image.constData()[i], 4.0 * img.constData()[i]);
        p = only(StageId::Gain, "auto");
        p.setValue(StageId::Gain, "target", 2000.0);
        r = isp::run(img, p);
        QVERIFY(std::abs(meanOf(r.image, 0, 32) - 2000.0) < 1e-6);
    }

    void denoise_data()
    {
        QTest::addColumn<QString>("algorithm");
        QTest::addColumn<bool>("keepsEdges");
        QTest::newRow("gaussian") << "gaussian" << false;
        QTest::newRow("median") << "median" << true;
        QTest::newRow("bilateral") << "bilateral" << true;
        QTest::newRow("guided") << "guided" << true;
        QTest::newRow("nlm") << "nlm" << true;
    }
    void denoise()
    {
        QFETCH(QString, algorithm);
        QFETCH(bool, keepsEdges);
        const int w = 64, h = 48;
        const Image2D img = stepEdge(w, h, 1000.0, 1400.0, 10.0);
        QVERIFY(std::abs(isp::estimateNoise(img) - 10.0) < 2.5);  // the estimate ignores the edge

        const isp::Result r = isp::run(img, only(StageId::Denoise, algorithm.toLatin1().constData()));
        QVERIFY(!r.displayReferred);
        // Less noise in the flat parts, the same levels.
        const double before = stdDev(img, 4, w / 2 - 6), after = stdDev(r.image, 4, w / 2 - 6);
        QVERIFY2(after < 0.75 * before, qPrintable(QStringLiteral("%1 -> %2").arg(before).arg(after)));
        QVERIFY(std::abs(meanOf(r.image, 4, w / 2 - 6) - 1000.0) < 3.0);
        QVERIFY(std::abs(meanOf(r.image, w / 2 + 6, w - 4) - 1400.0) < 3.0);
        if (keepsEdges) {
            // The step is still one pixel wide: the columns next to it keep their level.
            QVERIFY2(std::abs(meanOf(r.image, w / 2 - 1, w / 2) - 1000.0) < 25.0,
                     qPrintable(QString::number(meanOf(r.image, w / 2 - 1, w / 2))));
            QVERIFY(std::abs(meanOf(r.image, w / 2, w / 2 + 1) - 1400.0) < 25.0);
        }
    }

    void dynamicRange_data()
    {
        QTest::addColumn<QString>("algorithm");
        QTest::newRow("local") << "local";
        QTest::newRow("log") << "log";
        QTest::newRow("gamma") << "gamma";
    }
    void dynamicRange()
    {
        QFETCH(QString, algorithm);
        const Image2D img = scene();
        const isp::Result r = isp::run(img, only(StageId::DynamicRange, algorithm.toLatin1().constData()));
        QVERIFY(!r.displayReferred);  // still DN
        const auto [mn0, mx0] = minMax(img);
        const auto [mn1, mx1] = minMax(r.image);
        QVERIFY(mn1 >= mn0 - 1e-9 && mx1 <= mx0 + 1e-9);
        // Shadows are lifted towards the highlights.
        const double dark0 = meanOf(img, 0, 30), dark1 = meanOf(r.image, 0, 30);
        QVERIFY2(dark1 > 1.5 * dark0, qPrintable(QStringLiteral("%1 -> %2").arg(dark0).arg(dark1)));
    }

    void toneMapping_data()
    {
        QTest::addColumn<QString>("algorithm");
        QTest::addColumn<bool>("global");
        for (const isp::AlgorithmSpec& a : isp::stage(StageId::ToneMap).algorithms) {
            const QString id = QString::fromLatin1(a.id);
            const bool global = id != QLatin1String("clahe") && id != QLatin1String("local") && id != QLatin1String("fusion");
            QTest::newRow(a.id) << id << global;
        }
    }
    void toneMapping()
    {
        QFETCH(QString, algorithm);
        QFETCH(bool, global);
        const Image2D img = scene();
        const isp::Result r = isp::run(img, only(StageId::ToneMap, algorithm.toLatin1().constData()));
        QVERIFY(r.displayReferred);
        const auto [mn, mx] = minMax(r.image);
        QVERIFY2(mn >= 0.0 && mx <= 255.0, qPrintable(QStringLiteral("%1..%2").arg(mn).arg(mx)));
        QVERIFY2(mx - mn > 100.0, "uses a good part of the display range");
        // The dark part (in the bottom 3 % of the range) is no longer black,
        // except with the two linear mappings, which can only show one end.
        const double dark = meanOf(r.image, 0, 30);
        if (algorithm == QLatin1String("shift") || algorithm == QLatin1String("stretch"))
            QVERIFY(dark < 6.0);
        else
            QVERIFY2(dark > 12.0, qPrintable(QString::number(dark)));

        if (global) {
            // One input value, one output value, in the same order.
            std::vector<std::pair<double, double>> pairs;
            for (qsizetype i = 0; i < img.pixelCount(); ++i)
                pairs.emplace_back(img.constData()[i], r.image.constData()[i]);
            std::sort(pairs.begin(), pairs.end());
            for (std::size_t i = 1; i < pairs.size(); ++i) {
                QVERIFY(pairs[i].second >= pairs[i - 1].second);
                if (pairs[i].first == pairs[i - 1].first)
                    QCOMPARE(pairs[i].second, pairs[i - 1].second);
            }
        }
    }

    void toneMappingValues()
    {
        // A ramp 0, 16, 32, … 16368 (14-bit).
        Image2D ramp(1024, 1);
        for (int x = 0; x < 1024; ++x)
            ramp.data()[x] = 16.0 * x;

        // Bit shift: the top 8 of 14 bits.
        isp::Result r = isp::run(ramp, only(StageId::ToneMap, "shift"));
        for (int x = 0; x < 1024; ++x)
            QCOMPARE(r.image.constData()[x], std::floor(16.0 * x / 64.0));
        Pipeline p = only(StageId::ToneMap, "shift");
        p.setValue(StageId::ToneMap, "bits", 5.0);  // 16-bit input
        r = isp::run(ramp, p);
        QCOMPARE(r.image.constData()[1023], std::floor(16368.0 / 256.0));

        // Percentile stretch without clipping is linear from min to max.
        p = only(StageId::ToneMap, "stretch");
        p.setValue(StageId::ToneMap, "lo", 0.0);
        p.setValue(StageId::ToneMap, "hi", 0.0);
        r = isp::run(ramp, p);
        for (int x = 0; x < 1024; ++x)
            QVERIFY(std::abs(r.image.constData()[x] - 255.0 * x / 1023.0) < 1e-9);
        // Clipping 5 % at each end: the window is narrower, both ends saturate.
        p.setValue(StageId::ToneMap, "lo", 5.0);
        p.setValue(StageId::ToneMap, "hi", 5.0);
        r = isp::run(ramp, p);
        QCOMPARE(r.image.constData()[20], 0.0);
        QCOMPARE(r.image.constData()[1000], 255.0);
        QVERIFY(std::abs(r.image.constData()[512] - 127.5) < 1.0);

        // Gamma 2.2: mid-grey input (18 %) lands near 46 % of the display.
        p = only(StageId::ToneMap, "gamma");
        r = isp::run(ramp, p);
        QVERIFY(std::abs(r.image.constData()[184] / 255.0 - std::pow(184.0 / 1023.0, 1 / 2.2)) < 0.01);

        // Plateau equalisation of a ramp (every value once) is linear.
        r = isp::run(ramp, only(StageId::ToneMap, "he"));
        for (int x = 0; x < 1024; x += 37)
            QVERIFY(std::abs(r.image.constData()[x] - 255.0 * x / 1023.0) < 0.5);

        // Negative values (a calibrated image) are handled like any others.
        Image2D shifted = ramp;
        for (int x = 0; x < 1024; ++x)
            shifted.data()[x] -= 5000.0;
        p = only(StageId::ToneMap, "stretch");
        p.setValue(StageId::ToneMap, "lo", 0.0);
        p.setValue(StageId::ToneMap, "hi", 0.0);
        r = isp::run(shifted, p);
        QVERIFY(std::abs(r.image.constData()[512] - 255.0 * 512 / 1023.0) < 1e-9);
    }

    void detailAndSharpening_data()
    {
        QTest::addColumn<int>("stage");
        QTest::addColumn<QString>("algorithm");
        QTest::newRow("detail guided") << int(StageId::Detail) << "guided";
        QTest::newRow("detail multiscale") << int(StageId::Detail) << "multiscale";
        QTest::newRow("unsharp mask") << int(StageId::Sharpen) << "usm";
        QTest::newRow("laplacian") << int(StageId::Sharpen) << "laplace";
    }
    void detailAndSharpening()
    {
        QFETCH(int, stage);
        QFETCH(QString, algorithm);
        // Faint texture (ripples of ±5 with a period of 8 px, noise σ = 1) on a slow ramp.
        const int w = 64, h = 48;
        Image2D img(w, h);
        Rng rng{3};
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                img.data()[y * w + x] = 2000.0 + 1.0 * x + 5.0 * std::sin(M_PI * x / 4) * std::sin(M_PI * y / 4)
                                        + rng.gauss();
        auto texture = [&](const Image2D& im) {  // mean |deviation from the ramp|
            double acc = 0.0;
            int n = 0;
            for (int y = 8; y < h - 8; ++y)
                for (int x = 8; x < w - 8; ++x, ++n)
                    acc += std::abs(im.at(x, y) - 2000.0 - 1.0 * x);
            return acc / n;
        };
        Pipeline p = only(StageId(stage), algorithm.toLatin1().constData());
        // (By default the unsharp mask leaves alone what is at noise level
        // and what would overshoot, which is most of so faint a texture.)
        p.setValue(StageId::Sharpen, "limit", 0.0);
        p.setValue(StageId::Sharpen, "thr", 0.0);
        p.setValue(StageId::Sharpen, "sigma", 2.0);
        const isp::Result r = isp::run(img, p);
        QVERIFY2(texture(r.image) > 1.2 * texture(img),
                 qPrintable(QStringLiteral("%1 -> %2").arg(texture(img)).arg(texture(r.image))));
        // The large-scale brightness is unchanged.
        QVERIFY(std::abs(meanOf(r.image, 8, w - 8) - meanOf(img, 8, w - 8)) < 2.0);
    }

    void unsharpMaskOvershoot()
    {
        const Image2D img = stepEdge(64, 32, 1000.0, 2000.0, 0.0);
        Pipeline p = only(StageId::Sharpen, "usm");
        p.setValue(StageId::Sharpen, "amount", 3.0);
        p.setValue(StageId::Sharpen, "sigma", 2.0);
        // Limited (the default): no value outside the original range.
        isp::Result r = isp::run(img, p);
        QVERIFY(minMax(r.image).first >= 1000.0 && minMax(r.image).second <= 2000.0);
        // Allowed: halos on both sides of the edge.
        p.setValue(StageId::Sharpen, "limit", 0.0);
        r = isp::run(img, p);
        QVERIFY(minMax(r.image).first < 900.0 && minMax(r.image).second > 2100.0);
    }

    void outputQuantisation_data()
    {
        QTest::addColumn<QString>("algorithm");
        for (const isp::AlgorithmSpec& a : isp::stage(StageId::Output).algorithms)
            QTest::newRow(a.id) << QString::fromLatin1(a.id);
    }
    void outputQuantisation()
    {
        QFETCH(QString, algorithm);
        // A shallow gradient: 2.5 display levels across 256 pixels.
        Image2D ramp(256, 16);
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 256; ++x)
                ramp.data()[y * 256 + x] = 1000.0 + x;

        // Without tone mapping the stage is skipped, and says so.
        Pipeline p = only(StageId::Output, algorithm.toLatin1().constData());
        isp::Result r = isp::run(ramp, p);
        QVERIFY(!r.displayReferred);
        QCOMPARE(r.image.at(100, 3), ramp.at(100, 3));
        QVERIFY(!r.notes[isp::stageIndex(StageId::Output)].isEmpty());
        QVERIFY(p.summary(false).isEmpty());

        p.setEnabled(StageId::ToneMap, true);
        p.setAlgorithm(StageId::ToneMap, "stretch");
        p.setValue(StageId::ToneMap, "lo", 0.0);
        p.setValue(StageId::ToneMap, "hi", 0.0);
        for (int choice = 0; choice < 3; ++choice) {
            const int bits = choice == 0 ? 8 : choice == 1 ? 6 : 4;
            p.setValue(StageId::Output, "bits", choice);
            r = isp::run(ramp, p);
            QVERIFY(r.displayReferred);
            std::set<double> levels;
            for (qsizetype i = 0; i < r.image.pixelCount(); ++i) {
                const double v = r.image.constData()[i];
                QCOMPARE(v, std::round(v));
                QVERIFY(v >= 0.0 && v <= 255.0);
                levels.insert(v);
            }
            QVERIFY(int(levels.size()) <= (1 << bits));
            QVERIFY(levels.count(0.0) && levels.count(255.0));
            if (bits == 4) {
                QCOMPARE(int(levels.size()), 16);
                // Dither keeps the local average: with 16 levels (0, 17, …)
                // columns 102 to 109 all round to 102, but dithered their
                // mean still follows the ramp (105.5).
                const double error = std::abs(meanOf(r.image, 102, 110) - 105.5);
                if (algorithm == QLatin1String("round"))
                    QCOMPARE(error, 3.5);
                else
                    QVERIFY2(error < 2.5, qPrintable(QString::number(error)));
            }
        }
    }

    void nonFinitePixelsAreKept()
    {
        Image2D img = scene();
        img.data()[5] = std::nan("");
        img.data()[777] = std::numeric_limits<double>::infinity();
        Pipeline p = isp::presetPipeline("hdr");
        p.setEnabled(StageId::Sharpen, true);
        const isp::Result r = isp::run(img, p);
        QVERIFY(std::isnan(r.image.constData()[5]));
        QVERIFY(std::isinf(r.image.constData()[777]));
        qsizetype bad = 0;
        for (qsizetype i = 0; i < r.image.pixelCount(); ++i)
            bad += !std::isfinite(r.image.constData()[i]);
        QCOMPARE(bad, qsizetype(2));  // they did not spread

        // Nothing but NaN: returned as it is.
        Image2D nan(8, 8, std::nan(""));
        QVERIFY(std::isnan(isp::run(nan, p).image.at(3, 3)));
    }

    void everyAlgorithmOnTinyAndConstantImages()
    {
        // The test files of the UI are 8 × 6 pixels; nothing may crash or
        // produce NaN, whatever the size or content.
        const Image2D tiny = scene(8, 6);
        const Image2D constant(20, 14, 1234.0);
        const Image2D one(1, 1, 7.0);
        for (const Image2D& img : {tiny, constant, one}) {
            for (const isp::StageSpec& s : isp::stages()) {
                for (const isp::AlgorithmSpec& a : s.algorithms) {
                    Pipeline p = only(s.id, a.id);
                    p.setEnabled(StageId::ToneMap, true);
                    const isp::Result r = isp::run(img, p);
                    QCOMPARE(r.image.width(), img.width());
                    QCOMPARE(r.image.height(), img.height());
                    for (qsizetype i = 0; i < r.image.pixelCount(); ++i)
                        QVERIFY2(std::isfinite(r.image.constData()[i]), a.id);
                }
            }
        }
    }

    void presets()
    {
        QVERIFY(isp::presets().size() >= 4);
        const Image2D img = scene();
        for (const isp::Preset& preset : isp::presets()) {
            const Pipeline p = isp::presetPipeline(preset.id);
            QVERIFY2(p.isActive(), preset.id);
            const isp::Result r = isp::run(img, p);
            QCOMPARE(r.displayReferred, p.isDisplayReferred());
            QVERIFY(r.milliseconds >= 0.0);
        }
        QVERIFY(!isp::presetPipeline("cleanup").isDisplayReferred());
        QVERIFY(!isp::presetPipeline("no such preset").isActive());
    }
};

QTEST_GUILESS_MAIN(TestIsp)
#include "tst_isp.moc"
