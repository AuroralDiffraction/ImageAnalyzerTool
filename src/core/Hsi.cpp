#include "core/Hsi.h"

#include "core/HsiLinalg.h"

#include <QCoreApplication>
#include <QRegularExpression>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <limits>
#include <thread>
#include <vector>

namespace iat::hsi {

namespace {

// Runs f(i) for i in [0, n) on several threads.
void parallelFor(int n, const std::function<void(int)>& f)
{
    const int threads = std::clamp(int(std::thread::hardware_concurrency()), 1, std::max(1, n));
    if (threads <= 1 || n <= 1) {
        for (int i = 0; i < n; ++i)
            f(i);
        return;
    }
    std::vector<std::thread> pool;
    std::atomic<int> next{0};
    for (int t = 0; t < threads; ++t)
        pool.emplace_back([&] {
            for (int i = next++; i < n; i = next++)
                f(i);
        });
    for (auto& th : pool)
        th.join();
}

// Box mean of a rows × cols plane over a (2r+1)² window, clamped at the
// edges (the window shrinks), via an integral image.
std::vector<double> boxMean(const std::vector<double>& in, int rows, int cols, int r)
{
    std::vector<double> integral(size_t(rows + 1) * (cols + 1), 0.0);
    for (int y = 0; y < rows; ++y) {
        double rowSum = 0.0;
        for (int x = 0; x < cols; ++x) {
            rowSum += in[size_t(y) * cols + x];
            integral[size_t(y + 1) * (cols + 1) + x + 1] = integral[size_t(y) * (cols + 1) + x + 1] + rowSum;
        }
    }
    std::vector<double> out(in.size());
    for (int y = 0; y < rows; ++y) {
        const int y0 = std::max(0, y - r), y1 = std::min(rows - 1, y + r);
        for (int x = 0; x < cols; ++x) {
            const int x0 = std::max(0, x - r), x1 = std::min(cols - 1, x + r);
            const double s = integral[size_t(y1 + 1) * (cols + 1) + x1 + 1] - integral[size_t(y0) * (cols + 1) + x1 + 1]
                             - integral[size_t(y1 + 1) * (cols + 1) + x0] + integral[size_t(y0) * (cols + 1) + x0];
            out[size_t(y) * cols + x] = s / double((y1 - y0 + 1) * (x1 - x0 + 1));
        }
    }
    return out;
}

// Keys cubic convolution kernel (a = −0.5, as MATLAB's imresize 'bicubic').
double cubicWeight(double t)
{
    t = std::abs(t);
    if (t <= 1.0)
        return (1.5 * t - 2.5) * t * t + 1.0;
    if (t < 2.0)
        return ((-0.5 * t + 2.5) * t - 4.0) * t + 2.0;
    return 0.0;
}

} // namespace

// ---------------------------------------------------------------------------
// MosaicLayout
// ---------------------------------------------------------------------------

MosaicLayout MosaicLayout::preset(Preset p)
{
    MosaicLayout m;
    for (int r = 0; r < kPattern; ++r)
        for (int c = 0; c < kPattern; ++c) {
            // Position along the numbering path: lines are rows (or columns
            // for the vertical presets); serpentine reverses every other line.
            const bool vertical = p == Preset::RowByRowVertical || p == Preset::SerpentineVertical;
            const bool serpentine = p == Preset::Serpentine || p == Preset::SerpentineVertical;
            const int line = vertical ? c : r;
            int along = vertical ? r : c;
            if (serpentine && line % 2 == 1)
                along = kPattern - 1 - along;
            m.set(r, c, line * kPattern + along + 1);
        }
    return m;
}

std::optional<MosaicLayout::Preset> MosaicLayout::presetOf(const MosaicLayout& layout)
{
    for (Preset p : {Preset::RowByRow, Preset::RowByRowVertical, Preset::Serpentine, Preset::SerpentineVertical})
        if (preset(p) == layout)
            return p;
    return std::nullopt;
}

bool MosaicLayout::isValid() const
{
    std::array<bool, kBands> seen{};
    for (int ch : channel) {
        if (ch < 1 || ch > kBands || seen[size_t(ch - 1)])
            return false;
        seen[size_t(ch - 1)] = true;
    }
    return true;
}

void MosaicLayout::positionOf(int ch, int& row, int& col) const
{
    row = col = -1;
    for (int i = 0; i < kBands; ++i)
        if (channel[size_t(i)] == ch) {
            row = i / kPattern;
            col = i % kPattern;
            return;
        }
}

QString MosaicLayout::toString() const
{
    QStringList rows;
    for (int r = 0; r < kPattern; ++r) {
        QStringList cells;
        for (int c = 0; c < kPattern; ++c)
            cells << QString::number(at(r, c));
        rows << cells.join(QLatin1Char(' '));
    }
    return rows.join(QStringLiteral("; "));
}

std::optional<MosaicLayout> MosaicLayout::fromString(const QString& text)
{
    static const QRegularExpression sep(QStringLiteral("[^0-9]+"));
    const QStringList parts = text.split(sep, Qt::SkipEmptyParts);
    if (parts.size() != kBands)
        return std::nullopt;
    MosaicLayout m;
    for (int i = 0; i < kBands; ++i)
        m.channel[size_t(i)] = parts[i].toInt();
    if (!m.isValid())
        return std::nullopt;
    return m;
}

// ---------------------------------------------------------------------------
// Cube
// ---------------------------------------------------------------------------

Cube::Cube(int rows, int cols, int bands, float fill)
    : m_rows(std::max(0, rows))
    , m_cols(std::max(0, cols))
    , m_bands(rows > 0 && cols > 0 ? std::max(0, bands) : 0)
    , m_data(qsizetype(m_rows) * m_cols * m_bands, fill)
{
}

QVector<double> Cube::spectrum(int x, int y) const
{
    QVector<double> s(m_bands);
    if (x < 0 || y < 0 || x >= m_cols || y >= m_rows)
        return {};
    for (int b = 0; b < m_bands; ++b)
        s[b] = at(x, y, b);
    return s;
}

Image2D Cube::bandImage(int b) const
{
    Image2D img(m_cols, m_rows);
    if (b < 0 || b >= m_bands)
        return {};
    const float* src = band(b);
    double* dst = img.data();
    for (qsizetype i = 0; i < planeSize(); ++i)
        dst[i] = src[i];
    return img;
}

Image2D Cube::meanImage() const
{
    Image2D img(m_cols, m_rows);
    if (isNull())
        return {};
    double* dst = img.data();
    for (int b = 0; b < m_bands; ++b) {
        const float* src = band(b);
        for (qsizetype i = 0; i < planeSize(); ++i)
            dst[i] += src[i];
    }
    for (qsizetype i = 0; i < planeSize(); ++i)
        dst[i] /= m_bands;
    return img;
}

QString Cube::bandLabel(int b) const
{
    const QString base = QCoreApplication::translate("iat::hsi", "Band %1").arg(b + 1);
    if (hasWavelengths())
        return QStringLiteral("%1 (%2 nm)").arg(base).arg(wavelengths[b], 0, 'f', 0);
    return base;
}

int Cube::nearestBand(double nm) const
{
    if (!hasWavelengths())
        return -1;
    int best = 0;
    for (int b = 1; b < m_bands; ++b)
        if (std::abs(wavelengths[b] - nm) < std::abs(wavelengths[best] - nm))
            best = b;
    return best;
}

Cube Cube::withSameMetadata(int rows, int cols) const
{
    Cube c(rows, cols, m_bands);
    c.wavelengths = wavelengths;
    c.units = units;
    c.history = history;
    return c;
}

// ---------------------------------------------------------------------------
// Extraction
// ---------------------------------------------------------------------------

QString methodName(ExtractMethod method)
{
    switch (method) {
    case ExtractMethod::Subsample:  return QStringLiteral("subsample");
    case ExtractMethod::Registered: return QStringLiteral("subsample + registration to super-pixel centre");
    case ExtractMethod::Bilinear:   return QStringLiteral("full-resolution bilinear demosaic");
    case ExtractMethod::Ppid:       return QStringLiteral("full-resolution PPID demosaic");
    }
    return {};
}

float sampleBilinear(const float* plane, int rows, int cols, double x, double y)
{
    x = std::clamp(x, 0.0, double(cols - 1));
    y = std::clamp(y, 0.0, double(rows - 1));
    const int x0 = std::min(int(x), cols - 1), y0 = std::min(int(y), rows - 1);
    const int x1 = std::min(x0 + 1, cols - 1), y1 = std::min(y0 + 1, rows - 1);
    const double fx = x - x0, fy = y - y0;
    const double top = plane[qsizetype(y0) * cols + x0] * (1 - fx) + plane[qsizetype(y0) * cols + x1] * fx;
    const double bottom = plane[qsizetype(y1) * cols + x0] * (1 - fx) + plane[qsizetype(y1) * cols + x1] * fx;
    return float(top * (1 - fy) + bottom * fy);
}

double msbScale(const Image2D& raw, int bitDepth)
{
    if (bitDepth <= 0 || bitDepth >= 16 || raw.isNull())
        return 1.0;
    double maxValue = -std::numeric_limits<double>::infinity();
    for (qsizetype i = 0; i < raw.pixelCount(); ++i)
        if (std::isfinite(raw.constData()[i]))
            maxValue = std::max(maxValue, raw.constData()[i]);
    return maxValue > std::ldexp(1.0, bitDepth) - 1.0 ? std::ldexp(1.0, -(16 - bitDepth)) : 1.0;
}

ExtractResult extract(const Image2D& rawIn, const ExtractOptions& o)
{
    ExtractResult result;
    if (!o.layout.isValid()) {
        result.error = QCoreApplication::translate("iat::hsi", "The mosaic layout must contain every channel 1–16 exactly once.");
        return result;
    }
    const int nR = rawIn.height() / kPattern * kPattern;
    const int nC = rawIn.width() / kPattern * kPattern;
    if (nR < kPattern || nC < kPattern) {
        result.error = QCoreApplication::translate("iat::hsi", "The image is too small for a 4 × 4 mosaic.");
        return result;
    }
    if (nR != rawIn.height() || nC != rawIn.width())
        result.notes << QCoreApplication::translate("iat::hsi", "Cropped from %1 × %2 to %3 × %4 (a multiple of 4).")
                            .arg(rawIn.height()).arg(rawIn.width()).arg(nR).arg(nC);

    // Scale (MSB-aligned data), black level, clipping: as hsi_reconstruct.m.
    double maxValue = -std::numeric_limits<double>::infinity();
    for (int y = 0; y < nR; ++y)
        for (int x = 0; x < nC; ++x)
            if (std::isfinite(rawIn.at(x, y)))
                maxValue = std::max(maxValue, rawIn.at(x, y));
    double scale = 1.0;
    if (o.bitDepth > 0 && o.bitDepth < 16 && maxValue > std::ldexp(1.0, o.bitDepth) - 1.0) {
        scale = std::ldexp(1.0, -(16 - o.bitDepth));
        result.notes << QCoreApplication::translate("iat::hsi", "Values exceed %1: treated as MSB-aligned and shifted down.")
                            .arg(std::ldexp(1.0, o.bitDepth) - 1.0, 0, 'f', 0);
    }
    std::vector<float> raw(size_t(nR) * nC);
    for (int y = 0; y < nR; ++y)
        for (int x = 0; x < nC; ++x) {
            double v = rawIn.at(x, y) * scale - o.blackLevel;
            if (o.clipNegative && v < 0.0)
                v = 0.0;
            raw[size_t(y) * nC + x] = float(v);
        }

    const int h = nR / kPattern, w = nC / kPattern;
    // The 16 sub-sampled planes, in channel order.
    Cube sub(h, w, kBands);
    for (int r = 0; r < kPattern; ++r)
        for (int c = 0; c < kPattern; ++c) {
            float* dst = sub.band(o.layout.at(r, c) - 1);
            for (int i = 0; i < h; ++i)
                for (int j = 0; j < w; ++j)
                    dst[qsizetype(i) * w + j] = raw[size_t(kPattern * i + r) * nC + kPattern * j + c];
        }

    Cube cube;
    switch (o.method) {
    case ExtractMethod::Subsample:
        cube = sub;
        break;
    case ExtractMethod::Registered: {
        cube = Cube(h, w, kBands);
        parallelFor(kBands, [&](int b) {
            int r, c;
            o.layout.positionOf(b + 1, r, c);
            const double dy = (1.5 - r) / kPattern, dx = (1.5 - c) / kPattern;
            const float* src = sub.band(b);
            float* dst = cube.band(b);
            for (int i = 0; i < h; ++i)
                for (int j = 0; j < w; ++j)
                    dst[qsizetype(i) * w + j] = sampleBilinear(src, h, w, j + dx, i + dy);
        });
        break;
    }
    case ExtractMethod::Bilinear:
    case ExtractMethod::Ppid: {
        cube = Cube(nR, nC, kBands);
        std::vector<float> ppi;
        if (o.method == ExtractMethod::Ppid) {
            // Pseudo-panchromatic image: the raw mosaic filtered with
            // [1 2 2 2 1]ᵀ[1 2 2 2 1] / 64, in which each channel of a 4 × 4
            // mosaic has the same total weight (Mihoubi et al. 2017).
            static const double k[5] = {1, 2, 2, 2, 1};
            std::vector<float> tmp(raw.size());
            for (int y = 0; y < nR; ++y)
                for (int x = 0; x < nC; ++x) {
                    double s = 0.0;
                    for (int t = -2; t <= 2; ++t) {
                        int xx = x + t;
                        xx = xx < 0 ? xx + kPattern : (xx >= nC ? xx - kPattern : xx);  // whole periods: same channel
                        s += k[t + 2] * raw[size_t(y) * nC + xx];
                    }
                    tmp[size_t(y) * nC + x] = float(s / 8.0);
                }
            ppi.resize(raw.size());
            for (int y = 0; y < nR; ++y)
                for (int x = 0; x < nC; ++x) {
                    double s = 0.0;
                    for (int t = -2; t <= 2; ++t) {
                        int yy = y + t;
                        yy = yy < 0 ? yy + kPattern : (yy >= nR ? yy - kPattern : yy);
                        s += k[t + 2] * tmp[size_t(yy) * nC + x];
                    }
                    ppi[size_t(y) * nC + x] = float(s / 8.0);
                }
        }
        parallelFor(kBands, [&](int b) {
            int r, c;
            o.layout.positionOf(b + 1, r, c);
            // The band's samples, minus the PPI at the same places for PPID.
            std::vector<float> lattice(size_t(h) * w);
            const float* s = sub.band(b);
            for (int i = 0; i < h; ++i)
                for (int j = 0; j < w; ++j) {
                    float v = s[qsizetype(i) * w + j];
                    if (!ppi.empty())
                        v -= ppi[size_t(kPattern * i + r) * nC + kPattern * j + c];
                    lattice[size_t(i) * w + j] = v;
                }
            float* dst = cube.band(b);
            for (int y = 0; y < nR; ++y) {
                const double u = double(y - r) / kPattern;
                for (int x = 0; x < nC; ++x) {
                    float v = sampleBilinear(lattice.data(), h, w, double(x - c) / kPattern, u);
                    if (!ppi.empty())
                        v += ppi[size_t(y) * nC + x];
                    dst[qsizetype(y) * nC + x] = v;
                }
            }
        });
        break;
    }
    }
    cube.history << QStringLiteral("extracted from %1 × %2 raw mosaic, layout [%3], black level %4, %5")
                        .arg(nR).arg(nC).arg(o.layout.toString()).arg(o.blackLevel).arg(methodName(o.method));
    result.cube = cube;
    return result;
}

// ---------------------------------------------------------------------------
// Processing
// ---------------------------------------------------------------------------

Cube toReflectance(const Cube& cube, const QRect& whiteRef, double whiteReflectance)
{
    const QRect area = whiteRef.intersected(QRect(0, 0, cube.cols(), cube.rows()));
    if (cube.isNull() || area.isEmpty())
        return cube;
    Cube out = cube;
    out.detach();  // before the threads write to it
    for (int b = 0; b < cube.bands(); ++b) {
        double sum = 0.0;
        qint64 n = 0;
        const float* src = cube.band(b);
        for (int y = area.top(); y <= area.bottom(); ++y)
            for (int x = area.left(); x <= area.right(); ++x) {
                const double v = src[qsizetype(y) * cube.cols() + x];
                if (std::isfinite(v)) {
                    sum += v;
                    ++n;
                }
            }
        const double mean = n > 0 ? sum / n : 0.0;
        const float f = mean > 0.0 ? float(whiteReflectance / mean) : 0.0f;
        float* dst = out.band(b);
        for (qsizetype i = 0; i < cube.planeSize(); ++i)
            dst[i] = src[i] * f;
    }
    out.units = QStringLiteral("reflectance");
    out.history << QStringLiteral("reflectance: divided by the mean of rows %1-%2, columns %3-%4 (reference reflectance %5)")
                       .arg(area.top() + 1).arg(area.bottom() + 1).arg(area.left() + 1).arg(area.right() + 1)
                       .arg(whiteReflectance);
    return out;
}

Cube gaussianBlur(const Cube& cube, double sigma)
{
    if (cube.isNull() || !(sigma > 0.0))
        return cube;
    const int radius = std::max(1, int(std::ceil(3.0 * sigma)));
    std::vector<double> k(static_cast<size_t>(2 * radius + 1));
    double sum = 0.0;
    for (int i = -radius; i <= radius; ++i)
        sum += k[size_t(i + radius)] = std::exp(-0.5 * i * i / (sigma * sigma));
    for (double& v : k)
        v /= sum;
    Cube out = cube;
    out.detach();  // before the threads write to it
    const int rows = cube.rows(), cols = cube.cols();
    parallelFor(cube.bands(), [&](int b) {
        const float* src = cube.band(b);
        std::vector<float> tmp(size_t(rows) * cols);
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < cols; ++x) {
                double s = 0.0;
                for (int t = -radius; t <= radius; ++t)
                    s += k[size_t(t + radius)] * src[qsizetype(y) * cols + std::clamp(x + t, 0, cols - 1)];
                tmp[size_t(y) * cols + x] = float(s);
            }
        float* dst = out.band(b);
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < cols; ++x) {
                double s = 0.0;
                for (int t = -radius; t <= radius; ++t)
                    s += k[size_t(t + radius)] * tmp[size_t(std::clamp(y + t, 0, rows - 1)) * cols + x];
                dst[qsizetype(y) * cols + x] = float(s);
            }
    });
    out.history << QStringLiteral("anti-aliasing: Gaussian low-pass, sigma %1 px").arg(sigma);
    return out;
}

Cube guidedDenoise(const Cube& cube, int radius, double eps)
{
    if (cube.isNull() || radius < 1)
        return cube;
    const int rows = cube.rows(), cols = cube.cols();
    const size_t n = size_t(rows) * cols;
    const Image2D guideImage = cube.meanImage();
    std::vector<double> I(guideImage.constData(), guideImage.constData() + n);
    const std::vector<double> meanI = boxMean(I, rows, cols, radius);
    std::vector<double> II(n);
    for (size_t i = 0; i < n; ++i)
        II[i] = I[i] * I[i];
    const std::vector<double> meanII = boxMean(II, rows, cols, radius);
    double gMean = 0.0, gVar = 0.0;
    for (double v : I)
        gMean += v;
    gMean /= double(n);
    for (double v : I)
        gVar += (v - gMean) * (v - gMean);
    gVar /= double(n);
    const double epsAbs = std::max(eps * gVar, 1e-12);

    Cube out = cube;
    out.detach();  // before the threads write to it
    parallelFor(cube.bands(), [&](int b) {
        const float* src = cube.band(b);
        std::vector<double> p(src, src + n), Ip(n);
        for (size_t i = 0; i < n; ++i)
            Ip[i] = I[i] * p[i];
        const std::vector<double> meanP = boxMean(p, rows, cols, radius);
        const std::vector<double> meanIp = boxMean(Ip, rows, cols, radius);
        std::vector<double> a(n), bb(n);
        for (size_t i = 0; i < n; ++i) {
            const double varI = meanII[i] - meanI[i] * meanI[i];
            const double covIp = meanIp[i] - meanI[i] * meanP[i];
            a[i] = covIp / (varI + epsAbs);
            bb[i] = meanP[i] - a[i] * meanI[i];
        }
        const std::vector<double> meanA = boxMean(a, rows, cols, radius);
        const std::vector<double> meanB = boxMean(bb, rows, cols, radius);
        float* dst = out.band(b);
        for (size_t i = 0; i < n; ++i)
            dst[i] = float(meanA[i] * I[i] + meanB[i]);
    });
    out.history << QStringLiteral("denoise: guided filter (guide: mean of all bands), radius %1, eps %2").arg(radius).arg(eps);
    return out;
}

namespace {

struct MnfModel {
    QVector<double> mean;      // B
    QVector<double> lower;     // L, noise covariance = L Lᵀ (B × B)
    QVector<double> values;    // eigenvalues of the whitened data covariance, descending
    QVector<double> vectors;   // B × B, eigenvector i in column i
    bool ok = false;
};

MnfModel fitMnf(const Cube& cube)
{
    MnfModel m;
    const int B = cube.bands(), rows = cube.rows(), cols = cube.cols();
    if (B < 2 || cols < 2 || rows < 1)
        return m;
    // Use at most about 250 000 pixels for the statistics.
    const qsizetype total = cube.planeSize();
    const int step = std::max(1, int(std::sqrt(double(total) / 250000.0)));
    QVector<double> mean(B, 0.0), cov(B * B, 0.0), ncov(B * B, 0.0);
    qint64 count = 0, ncount = 0;
    QVector<double> v(B), d(B);
    for (int y = 0; y < rows; y += step)
        for (int x = 0; x + 1 < cols; x += step) {
            bool finite = true;
            for (int b = 0; b < B; ++b) {
                v[b] = cube.at(x, y, b);
                d[b] = v[b] - cube.at(x + 1, y, b);
                finite = finite && std::isfinite(v[b]) && std::isfinite(d[b]);
            }
            if (!finite)
                continue;
            for (int b = 0; b < B; ++b)
                mean[b] += v[b];
            ++count;
            for (int i = 0; i < B; ++i)
                for (int j = 0; j <= i; ++j)
                    ncov[i * B + j] += d[i] * d[j];
            ++ncount;
        }
    if (count < B * 4)
        return m;
    for (double& x : mean)
        x /= count;
    for (int y = 0; y < rows; y += step)
        for (int x = 0; x + 1 < cols; x += step) {
            bool finite = true;
            for (int b = 0; b < B; ++b) {
                v[b] = double(cube.at(x, y, b)) - mean[b];
                finite = finite && std::isfinite(v[b]) && std::isfinite(double(cube.at(x + 1, y, b)));
            }
            if (!finite)
                continue;
            for (int i = 0; i < B; ++i)
                for (int j = 0; j <= i; ++j)
                    cov[i * B + j] += v[i] * v[j];
        }
    for (int i = 0; i < B; ++i)
        for (int j = 0; j <= i; ++j) {
            cov[i * B + j] /= double(count);
            ncov[i * B + j] /= 2.0 * double(ncount);  // var(a − b) = 2 σ² for independent noise
            cov[j * B + i] = cov[i * B + j];
            ncov[j * B + i] = ncov[i * B + j];
        }
    m.lower = linalg::cholesky(ncov, B);
    if (m.lower.isEmpty())
        return m;
    // C = L⁻¹ Σ L⁻ᵀ
    const QVector<double> linv = linalg::inverse(m.lower, B);
    if (linv.isEmpty())
        return m;
    const QVector<double> c = linalg::multiply(linalg::multiply(linv, cov, B, B, B), linalg::transpose(linv, B, B), B, B, B);
    linalg::symmetricEigen(c, B, m.values, m.vectors);
    m.mean = mean;
    m.ok = true;
    return m;
}

} // namespace

QVector<double> mnfSnr(const Cube& cube)
{
    const MnfModel m = fitMnf(cube);
    QVector<double> snr;
    for (double v : m.values)
        snr << std::max(0.0, v - 1.0);
    return snr;
}

Cube mnfDenoise(const Cube& cube, int keep)
{
    const int B = cube.bands();
    if (cube.isNull() || keep >= B || keep < 1)
        return cube;
    const MnfModel m = fitMnf(cube);
    if (!m.ok)
        return cube;
    // X̂ = (X − μ) T + μ with T = L⁻ᵀ U P Uᵀ Lᵀ (P keeps the first `keep` components).
    const QVector<double> linvT = linalg::transpose(linalg::inverse(m.lower, B), B, B);
    QVector<double> upu(B * B, 0.0);
    for (int i = 0; i < B; ++i)
        for (int j = 0; j < B; ++j) {
            double s = 0.0;
            for (int k = 0; k < keep; ++k)
                s += m.vectors[i * B + k] * m.vectors[j * B + k];
            upu[i * B + j] = s;
        }
    const QVector<double> T = linalg::multiply(linalg::multiply(linvT, upu, B, B, B), linalg::transpose(m.lower, B, B), B, B, B);

    Cube out = cube;
    out.detach();  // before the threads write to it
    const qsizetype n = cube.planeSize();
    const int chunks = 64;
    parallelFor(chunks, [&](int chunk) {
        const qsizetype begin = n * chunk / chunks, end = n * (chunk + 1) / chunks;
        QVector<double> x(B);
        for (qsizetype p = begin; p < end; ++p) {
            for (int b = 0; b < B; ++b)
                x[b] = double(cube.band(b)[p]) - m.mean[b];
            for (int j = 0; j < B; ++j) {
                double s = m.mean[j];
                for (int i = 0; i < B; ++i)
                    s += x[i] * T[i * B + j];
                out.band(j)[p] = float(s);
            }
        }
    });
    out.history << QStringLiteral("denoise: MNF, kept %1 of %2 components").arg(keep).arg(B);
    return out;
}

Cube upscale(const Cube& cube, int factor, Interpolation method)
{
    if (cube.isNull() || factor <= 1)
        return cube;
    const int rows = cube.rows(), cols = cube.cols();
    const int R = rows * factor, C = cols * factor;
    Cube out = cube.withSameMetadata(R, C);
    parallelFor(cube.bands(), [&](int b) {
        const float* src = cube.band(b);
        float* dst = out.band(b);
        for (int y = 0; y < R; ++y) {
            const double sy = (y + 0.5) / factor - 0.5;
            for (int x = 0; x < C; ++x) {
                const double sx = (x + 0.5) / factor - 0.5;
                float v;
                if (method == Interpolation::Nearest) {
                    v = src[qsizetype(std::clamp(y / factor, 0, rows - 1)) * cols + std::clamp(x / factor, 0, cols - 1)];
                } else if (method == Interpolation::Bilinear) {
                    v = sampleBilinear(src, rows, cols, sx, sy);
                } else {
                    const int x0 = int(std::floor(sx)), y0 = int(std::floor(sy));
                    double s = 0.0, wsum = 0.0;
                    for (int j = -1; j <= 2; ++j) {
                        const double wy = cubicWeight(sy - (y0 + j));
                        const int yy = std::clamp(y0 + j, 0, rows - 1);
                        for (int i = -1; i <= 2; ++i) {
                            const double wgt = wy * cubicWeight(sx - (x0 + i));
                            s += wgt * src[qsizetype(yy) * cols + std::clamp(x0 + i, 0, cols - 1)];
                            wsum += wgt;
                        }
                    }
                    v = float(s / wsum);
                }
                dst[qsizetype(y) * C + x] = v;
            }
        }
    });
    static const char* names[] = {"nearest", "bilinear", "bicubic"};
    out.history << QStringLiteral("upscaled %1x (%2)").arg(factor).arg(QLatin1String(names[int(method)]));
    return out;
}

Cube toAbsorbance(const Cube& cube)
{
    if (cube.isNull())
        return cube;
    Cube out = cube;
    out.detach();
    for (int b = 0; b < out.bands(); ++b) {
        float* d = out.band(b);
        for (qsizetype i = 0; i < out.planeSize(); ++i)
            d[i] = std::isfinite(d[i]) ? float(-std::log10(std::max(double(d[i]), 1e-6))) : d[i];
    }
    out.units = QStringLiteral("absorbance");
    out.history << QStringLiteral("absorbance: -log10(value)");
    return out;
}

Cube normaliseSpectra(const Cube& cube, SpectralNormalisation method)
{
    if (cube.isNull() || method == SpectralNormalisation::None)
        return cube;
    const int B = cube.bands();
    Cube out = cube;
    out.detach();
    const qsizetype N = cube.planeSize();
    // MSC reference: the mean spectrum.
    QVector<double> ref(B, 0.0);
    double refMean = 0.0;
    if (method == SpectralNormalisation::Msc) {
        QVector<qint64> n(B, 0);
        for (int b = 0; b < B; ++b) {
            const float* s = cube.band(b);
            for (qsizetype p = 0; p < N; ++p)
                if (std::isfinite(s[p])) {
                    ref[b] += s[p];
                    ++n[b];
                }
            ref[b] = n[b] ? ref[b] / double(n[b]) : 0.0;
            refMean += ref[b];
        }
        refMean /= B;
    }
    double refVar = 0.0;
    for (double r : ref)
        refVar += (r - refMean) * (r - refMean);
    const int chunks = 64;
    parallelFor(chunks, [&](int chunk) {
        const qsizetype begin = N * chunk / chunks, end = N * (chunk + 1) / chunks;
        QVector<double> x(B);
        for (qsizetype p = begin; p < end; ++p) {
            bool finite = true;
            for (int b = 0; b < B; ++b) {
                x[b] = cube.band(b)[p];
                finite = finite && std::isfinite(x[b]);
            }
            if (!finite)
                continue;
            double offset = 0.0, scale = 1.0;
            switch (method) {
            case SpectralNormalisation::Snv: {
                double m = 0.0;
                for (double v : x)
                    m += v;
                m /= B;
                double var = 0.0;
                for (double v : x)
                    var += (v - m) * (v - m);
                offset = m;
                scale = B > 1 ? std::sqrt(var / (B - 1)) : 1.0;
                break;
            }
            case SpectralNormalisation::Msc: {
                // x ≈ a + b·ref → (x − a) / b
                double m = 0.0, cov = 0.0;
                for (double v : x)
                    m += v;
                m /= B;
                for (int b = 0; b < B; ++b)
                    cov += (ref[b] - refMean) * (x[b] - m);
                const double slope = refVar > 0.0 ? cov / refVar : 0.0;
                offset = m - slope * refMean;
                scale = slope;
                break;
            }
            case SpectralNormalisation::UnitLength: {
                double s = 0.0;
                for (double v : x)
                    s += v * v;
                scale = std::sqrt(s);
                break;
            }
            case SpectralNormalisation::UnitArea: {
                double s = 0.0;
                for (double v : x)
                    s += std::abs(v);
                scale = s;
                break;
            }
            case SpectralNormalisation::UnitMax: {
                double s = 0.0;
                for (double v : x)
                    s = std::max(s, std::abs(v));
                scale = s;
                break;
            }
            case SpectralNormalisation::None:
                break;
            }
            const bool ok = scale != 0.0 && std::isfinite(scale);
            for (int b = 0; b < B; ++b)
                out.band(b)[p] = ok ? float((x[b] - offset) / scale) : std::numeric_limits<float>::quiet_NaN();
        }
    });
    static const char* names[] = {"", "SNV", "MSC (mean spectrum as reference)", "unit length", "unit area", "unit maximum"};
    out.units = QStringLiteral("normalised");
    out.history << QStringLiteral("normalisation: %1").arg(QLatin1String(names[int(method)]));
    return out;
}

Cube savitzkyGolay(const Cube& cube, int window, int polynomial, int derivative)
{
    const int B = cube.bands();
    if (cube.isNull() || B < 3)
        return cube;
    window = std::clamp(window | 1, 3, (B % 2) ? B : B - 1);
    polynomial = std::clamp(polynomial, std::max(derivative, 1), window - 1);
    derivative = std::clamp(derivative, 0, polynomial);
    const int P = polynomial + 1;
    QVector<double> x(B);
    for (int b = 0; b < B; ++b)
        x[b] = cube.hasWavelengths() ? cube.wavelengths[b] : double(b);
    // Weights per output band: d! × row d of (XᵀX)⁻¹ Xᵀ, X_jk = (x_j − x_b)^k.
    QVector<QVector<double>> weights(B);
    QVector<int> start(B);
    double fact = 1.0;
    for (int k = 2; k <= derivative; ++k)
        fact *= k;
    for (int b = 0; b < B; ++b) {
        const int s = std::clamp(b - window / 2, 0, B - window);
        start[b] = s;
        QVector<double> X(window * P);
        for (int j = 0; j < window; ++j)
            for (int k = 0; k < P; ++k)
                X[j * P + k] = std::pow(x[s + j] - x[b], k);
        const QVector<double> Xt = linalg::transpose(X, window, P);
        const QVector<double> XtX = linalg::multiply(Xt, X, P, window, P);
        const QVector<double> H = linalg::solve(XtX, Xt, P, window);  // P × window
        QVector<double> w(window, 0.0);
        if (!H.isEmpty())
            for (int j = 0; j < window; ++j)
                w[j] = fact * H[derivative * window + j];
        weights[b] = w;
    }
    Cube out = cube;
    out.detach();
    const qsizetype N = cube.planeSize();
    const int chunks = 64;
    parallelFor(chunks, [&](int chunk) {
        const qsizetype begin = N * chunk / chunks, end = N * (chunk + 1) / chunks;
        QVector<double> v(B);
        for (qsizetype p = begin; p < end; ++p) {
            for (int b = 0; b < B; ++b)
                v[b] = cube.band(b)[p];
            for (int b = 0; b < B; ++b) {
                double acc = 0.0;
                for (int j = 0; j < window; ++j)
                    acc += weights[b][j] * v[start[b] + j];
                out.band(b)[p] = float(acc);
            }
        }
    });
    if (derivative > 0)
        out.units = QStringLiteral("derivative");
    out.history << QStringLiteral("Savitzky-Golay: window %1, polynomial %2, derivative %3%4")
                       .arg(window).arg(polynomial).arg(derivative)
                       .arg(derivative > 0 ? (cube.hasWavelengths() ? QStringLiteral(" (per nm)") : QStringLiteral(" (per band)")) : QString());
    return out;
}

bool ProcessingSettings::any() const
{
    return (reflectance && !whiteReference.isEmpty()) || mnf || guided || antiAlias || absorbance
           || normalisation != SpectralNormalisation::None || smooth || derivative > 0 || upscale > 1;
}

Cube process(const Cube& input, const ProcessingSettings& s)
{
    Cube c = input;
    if (s.reflectance && !s.whiteReference.isEmpty())
        c = toReflectance(c, s.whiteReference, s.whiteReflectance);
    if (s.mnf)
        c = mnfDenoise(c, s.mnfKeep);
    if (s.guided)
        c = guidedDenoise(c, s.guidedRadius, s.guidedEps);
    if (s.antiAlias)
        c = gaussianBlur(c, s.antiAliasSigma);
    if (s.absorbance)
        c = toAbsorbance(c);
    if (s.normalisation != SpectralNormalisation::None)
        c = normaliseSpectra(c, s.normalisation);
    if (s.smooth || s.derivative > 0)
        c = savitzkyGolay(c, s.sgWindow, s.sgPolynomial, s.derivative);
    if (s.upscale > 1)
        c = upscale(c, s.upscale, s.interpolation);
    return c;
}

} // namespace iat::hsi
