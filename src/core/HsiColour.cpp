#include "core/HsiColour.h"

#include "core/HsiLinalg.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace iat::hsi {

namespace {

// CIE 1931 2-degree colour-matching functions, 380-780 nm in 5 nm steps
// (x̄, ȳ, z̄), the table used by hsi_reconstruct.m.
constexpr double kCie1931[81][3] = {
    {0.001368, 0.000039, 0.006450},  // 380
    {0.002236, 0.000064, 0.010550},  // 385
    {0.004243, 0.000120, 0.020050},  // 390
    {0.007650, 0.000217, 0.036210},  // 395
    {0.014310, 0.000396, 0.067850},  // 400
    {0.023190, 0.000640, 0.110200},  // 405
    {0.043510, 0.001210, 0.207400},  // 410
    {0.077630, 0.002180, 0.371300},  // 415
    {0.134380, 0.004000, 0.645600},  // 420
    {0.214770, 0.007300, 1.039050},  // 425
    {0.283900, 0.011600, 1.385600},  // 430
    {0.328500, 0.016840, 1.622960},  // 435
    {0.348280, 0.023000, 1.747060},  // 440
    {0.348060, 0.029800, 1.782600},  // 445
    {0.336200, 0.038000, 1.772110},  // 450
    {0.318700, 0.048000, 1.744100},  // 455
    {0.290800, 0.060000, 1.669200},  // 460
    {0.251100, 0.073900, 1.528100},  // 465
    {0.195360, 0.090980, 1.287640},  // 470
    {0.142100, 0.112600, 1.041900},  // 475
    {0.095640, 0.139020, 0.812950},  // 480
    {0.057950, 0.169300, 0.616200},  // 485
    {0.032010, 0.208020, 0.465180},  // 490
    {0.014700, 0.258600, 0.353300},  // 495
    {0.004900, 0.323000, 0.272000},  // 500
    {0.002400, 0.407300, 0.212300},  // 505
    {0.009300, 0.503000, 0.158200},  // 510
    {0.029100, 0.608200, 0.111700},  // 515
    {0.063270, 0.710000, 0.078250},  // 520
    {0.109600, 0.793200, 0.057250},  // 525
    {0.165500, 0.862000, 0.042160},  // 530
    {0.225750, 0.914850, 0.029840},  // 535
    {0.290400, 0.954000, 0.020300},  // 540
    {0.359700, 0.980300, 0.013400},  // 545
    {0.433450, 0.994950, 0.008750},  // 550
    {0.512050, 1.000000, 0.005750},  // 555
    {0.594500, 0.995000, 0.003900},  // 560
    {0.678400, 0.978600, 0.002750},  // 565
    {0.762100, 0.952000, 0.002100},  // 570
    {0.842500, 0.915400, 0.001800},  // 575
    {0.916300, 0.870000, 0.001650},  // 580
    {0.978600, 0.816300, 0.001400},  // 585
    {1.026300, 0.757000, 0.001100},  // 590
    {1.056700, 0.694900, 0.001000},  // 595
    {1.062200, 0.631000, 0.000800},  // 600
    {1.045600, 0.566800, 0.000600},  // 605
    {1.002600, 0.503000, 0.000340},  // 610
    {0.938400, 0.441200, 0.000240},  // 615
    {0.854450, 0.381000, 0.000190},  // 620
    {0.751400, 0.321000, 0.000100},  // 625
    {0.642400, 0.265000, 0.000050},  // 630
    {0.541900, 0.217000, 0.000030},  // 635
    {0.447900, 0.175000, 0.000020},  // 640
    {0.360800, 0.138200, 0.000010},  // 645
    {0.283500, 0.107000, 0.000000},  // 650
    {0.218700, 0.081600, 0.000000},  // 655
    {0.164900, 0.061000, 0.000000},  // 660
    {0.121200, 0.044580, 0.000000},  // 665
    {0.087400, 0.032000, 0.000000},  // 670
    {0.063600, 0.023200, 0.000000},  // 675
    {0.046770, 0.017000, 0.000000},  // 680
    {0.032900, 0.011920, 0.000000},  // 685
    {0.022700, 0.008210, 0.000000},  // 690
    {0.015840, 0.005723, 0.000000},  // 695
    {0.011359, 0.004102, 0.000000},  // 700
    {0.008111, 0.002929, 0.000000},  // 705
    {0.005790, 0.002091, 0.000000},  // 710
    {0.004109, 0.001484, 0.000000},  // 715
    {0.002899, 0.001047, 0.000000},  // 720
    {0.002049, 0.000740, 0.000000},  // 725
    {0.001440, 0.000520, 0.000000},  // 730
    {0.001000, 0.000361, 0.000000},  // 735
    {0.000690, 0.000249, 0.000000},  // 740
    {0.000476, 0.000172, 0.000000},  // 745
    {0.000332, 0.000120, 0.000000},  // 750
    {0.000235, 0.000085, 0.000000},  // 755
    {0.000166, 0.000060, 0.000000},  // 760
    {0.000117, 0.000042, 0.000000},  // 765
    {0.000083, 0.000030, 0.000000},  // 770
    {0.000059, 0.000021, 0.000000},  // 775
    {0.000042, 0.000015, 0.000000},  // 780
};

// MATLAB's pchip end slope (pchipend).
double pchipEnd(double h1, double h2, double del1, double del2)
{
    double d = ((2.0 * h1 + h2) * del1 - h1 * del2) / (h1 + h2);
    auto sign = [](double v) { return (v > 0) - (v < 0); };
    if (sign(d) != sign(del1))
        d = 0.0;
    else if (sign(del1) != sign(del2) && std::abs(d) > std::abs(3.0 * del1))
        d = 3.0 * del1;
    return d;
}

// Shape-preserving piecewise cubic interpolation as MATLAB's
// interp1(x, y, xi, 'pchip', 0): zero outside [x(1), x(end)].
std::vector<double> pchip(const std::vector<double>& x, const std::vector<double>& y, const QVector<double>& xi)
{
    const int n = int(x.size());
    std::vector<double> h(n - 1), del(n - 1), d(n, 0.0);
    for (int k = 0; k < n - 1; ++k) {
        h[k] = x[k + 1] - x[k];
        del[k] = (y[k + 1] - y[k]) / h[k];
    }
    for (int k = 1; k < n - 1; ++k) {
        if ((del[k - 1] > 0 && del[k] > 0) || (del[k - 1] < 0 && del[k] < 0)) {
            const double w1 = 2.0 * h[k] + h[k - 1], w2 = h[k] + 2.0 * h[k - 1];
            d[k] = (w1 + w2) / (w1 / del[k - 1] + w2 / del[k]);
        }
    }
    d[0] = pchipEnd(h[0], h[1], del[0], del[1]);
    d[n - 1] = pchipEnd(h[n - 2], h[n - 3], del[n - 2], del[n - 3]);
    std::vector<double> out(size_t(xi.size()), 0.0);
    for (qsizetype i = 0; i < xi.size(); ++i) {
        const double v = xi[i];
        if (!(v >= x.front() && v <= x.back()))
            continue;
        int k = int(std::upper_bound(x.begin(), x.end(), v) - x.begin()) - 1;
        k = std::clamp(k, 0, n - 2);
        const double s = v - x[k];
        const double c = (3.0 * del[k] - 2.0 * d[k] - d[k + 1]) / h[k];
        const double b = (d[k] - 2.0 * del[k] + d[k + 1]) / (h[k] * h[k]);
        out[size_t(i)] = y[k] + s * (d[k] + s * (c + s * b));
    }
    return out;
}

// MATLAB gradient() of a vector: central differences, one-sided at the ends.
QVector<double> gradientOf(const QVector<double>& x)
{
    const qsizetype n = x.size();
    QVector<double> g(n, 1.0);
    if (n < 2)
        return g;
    g[0] = x[1] - x[0];
    g[n - 1] = x[n - 1] - x[n - 2];
    for (qsizetype i = 1; i + 1 < n; ++i)
        g[i] = (x[i + 1] - x[i - 1]) / 2.0;
    return g;
}

// Background-subtracted QE (QE − mean over channels), as used for the peaks.
QVector<QVector<double>> peakCurves(const SpectralResponse& r)
{
    QVector<QVector<double>> out = r.qe;
    for (int i = 0; i < r.samples(); ++i) {
        double m = 0.0;
        for (const auto& c : r.qe)
            m += c[i];
        m /= std::max(1, r.bands());
        for (auto& c : out)
            c[i] -= m;
    }
    return out;
}

} // namespace

QVector<double> SpectralResponse::peaks() const
{
    QVector<double> p;
    for (const auto& c : peakCurves(*this))
        p << lambda[int(std::max_element(c.begin(), c.end()) - c.begin())];
    return p;
}

QVector<double> SpectralResponse::fwhm() const
{
    QVector<double> out;
    for (const auto& c : peakCurves(*this)) {
        const int k = int(std::max_element(c.begin(), c.end()) - c.begin());
        const double half = c[k] / 2.0;
        int lo = k, hi = k;
        while (lo > 0 && c[lo - 1] >= half)
            --lo;
        while (hi + 1 < c.size() && c[hi + 1] >= half)
            ++hi;
        // Linear interpolation of the half-maximum crossings.
        double left = lambda[lo], right = lambda[hi];
        if (lo > 0 && c[lo] != c[lo - 1])
            left = lambda[lo - 1] + (half - c[lo - 1]) / (c[lo] - c[lo - 1]) * (lambda[lo] - lambda[lo - 1]);
        if (hi + 1 < c.size() && c[hi] != c[hi + 1])
            right = lambda[hi] + (c[hi] - half) / (c[hi] - c[hi + 1]) * (lambda[hi + 1] - lambda[hi]);
        out << right - left;
    }
    return out;
}

QVector<double> cie1931(const QVector<double>& lambda)
{
    std::vector<double> x(81);
    for (int i = 0; i < 81; ++i)
        x[size_t(i)] = 380.0 + 5.0 * i;
    QVector<double> out(lambda.size() * 3, 0.0);
    for (int c = 0; c < 3; ++c) {
        std::vector<double> y(81);
        for (int i = 0; i < 81; ++i)
            y[size_t(i)] = kCie1931[i][c];
        const std::vector<double> v = pchip(x, y, lambda);
        for (qsizetype i = 0; i < lambda.size(); ++i)
            out[i * 3 + c] = std::max(0.0, v[size_t(i)]);
    }
    return out;
}

double percentileOf(std::vector<double> values, double q)
{
    values.erase(std::remove_if(values.begin(), values.end(), [](double v) { return std::isnan(v); }), values.end());
    if (values.empty())
        return std::numeric_limits<double>::quiet_NaN();
    const qsizetype n = qsizetype(values.size());
    const qsizetype k = std::clamp<qsizetype>(qsizetype(std::llround(q / 100.0 * double(n))), 1, n) - 1;
    std::nth_element(values.begin(), values.begin() + k, values.end());
    return values[size_t(k)];
}

double srgbEncode(double x)
{
    return x > 0.0031308 ? 1.055 * std::pow(x, 1.0 / 2.4) - 0.055 : 12.92 * x;
}

QVector<double> spectralEstimator(const SpectralResponse& r, double sigmaNm, double alphaRel)
{
    const int B = r.bands(), nL = r.samples();
    if (B == 0 || nL == 0 || !(sigmaNm > 0))
        return {};
    // A: B × nL
    QVector<double> A(B * nL);
    for (int b = 0; b < B; ++b)
        for (int i = 0; i < nL; ++i)
            A[b * nL + i] = r.qe[b][i];
    QVector<double> K(nL * nL);
    for (int i = 0; i < nL; ++i)
        for (int j = 0; j < nL; ++j) {
            const double d = r.lambda[i] - r.lambda[j];
            K[i * nL + j] = std::exp(-d * d / (2.0 * sigmaNm * sigmaNm));
        }
    const QVector<double> At = linalg::transpose(A, B, nL);              // nL × B
    const QVector<double> KAt = linalg::multiply(K, At, nL, nL, B);      // nL × B
    QVector<double> G = linalg::multiply(A, KAt, B, nL, B);              // B × B
    double trace = 0.0;
    for (int b = 0; b < B; ++b)
        trace += G[b * B + b];
    const double alpha = alphaRel * trace / B;
    for (int b = 0; b < B; ++b)
        G[b * B + b] += alpha;
    // R = KAᵀ G⁻¹  ⇔  Rᵀ = G⁻ᵀ (KAᵀ)ᵀ = G⁻¹ (KAᵀ)ᵀ (G symmetric)
    const QVector<double> Rt = linalg::solve(G, linalg::transpose(KAt, nL, B), B, nL);  // B × nL
    if (Rt.isEmpty())
        return {};
    return linalg::transpose(Rt, B, nL);                                  // nL × B
}

QVector<double> xyzMatrix(const SpectralResponse& r, double sigmaNm, double alphaRel)
{
    const QVector<double> R = spectralEstimator(r, sigmaNm, alphaRel);
    if (R.isEmpty())
        return {};
    const int B = r.bands(), nL = r.samples();
    const QVector<double> cmf = cie1931(r.lambda);  // nL × 3
    const QVector<double> dl = gradientOf(r.lambda);
    QVector<double> M(3 * B, 0.0);
    for (int c = 0; c < 3; ++c)
        for (int b = 0; b < B; ++b) {
            double s = 0.0;
            for (int i = 0; i < nL; ++i)
                s += cmf[i * 3 + c] * dl[i] * R[i * B + b];
            M[c * B + b] = s;
        }
    return M;
}

QVector<double> rgbMatrix(const Cube& cube, const QVector<double>& M, const ColourSettings& s,
                          const SpectralResponse& response, QVector<double>* whiteOut)
{
    const int B = cube.bands();
    if (cube.isNull() || M.size() != 3 * B)
        return {};
    const qsizetype N = cube.planeSize();
    auto xyzAt = [&](qsizetype p, double out[3]) {
        out[0] = out[1] = out[2] = 0.0;
        for (int b = 0; b < B; ++b) {
            const double v = cube.band(b)[p];
            out[0] += M[b] * v;
            out[1] += M[B + b] * v;
            out[2] += M[2 * B + b] * v;
        }
    };
    double w[3] = {0, 0, 0};
    switch (s.whiteBalance) {
    case WhiteBalance::GrayWorld: {
        double x[3];
        for (qsizetype p = 0; p < N; ++p) {
            xyzAt(p, x);
            for (int c = 0; c < 3; ++c)
                w[c] += x[c];
        }
        for (double& v : w)
            v /= double(N);
        break;
    }
    case WhiteBalance::WhitePatch: {
        std::vector<double> Y(static_cast<size_t>(N));
        double x[3];
        for (qsizetype p = 0; p < N; ++p) {
            xyzAt(p, x);
            Y[size_t(p)] = x[1];
        }
        const double t = percentileOf(Y, s.whitePercentile);
        qint64 n = 0;
        for (qsizetype p = 0; p < N; ++p)
            if (Y[size_t(p)] >= t) {
                xyzAt(p, x);
                for (int c = 0; c < 3; ++c)
                    w[c] += x[c];
                ++n;
            }
        for (double& v : w)
            v /= double(std::max<qint64>(1, n));
        break;
    }
    case WhiteBalance::Reference: {
        const QRect area = s.reference.intersected(QRect(0, 0, cube.cols(), cube.rows()));
        if (area.isEmpty())
            return {};
        double x[3];
        qint64 n = 0;
        for (int yy = area.top(); yy <= area.bottom(); ++yy)
            for (int xx = area.left(); xx <= area.right(); ++xx) {
                xyzAt(qsizetype(yy) * cube.cols() + xx, x);
                for (int c = 0; c < 3; ++c)
                    w[c] += x[c];
                ++n;
            }
        for (double& v : w)
            v /= double(n);
        break;
    }
    case WhiteBalance::Flat: {
        // Pipeline response to a flat spectrum: M · (A · 1).
        if (response.bands() != B)
            return {};
        for (int b = 0; b < B; ++b) {
            double a = 0.0;
            for (double q : response.qe[b])
                a += q;
            for (int c = 0; c < 3; ++c)
                w[c] += M[c * B + b] * a;
        }
        break;
    }
    }
    if (!(std::abs(w[1]) > 0.0) || !std::isfinite(w[1]))
        return {};
    const double whiteY = w[1];
    for (double& v : w)
        v /= whiteY;
    if (whiteOut)
        *whiteOut = {w[0], w[1], w[2]};

    // Bradford: scene white → D65.
    static const QVector<double> bfd = {0.8951, 0.2664, -0.1614, -0.7502, 1.7135, 0.0367, 0.0389, -0.0685, 1.0296};
    static const double d65[3] = {0.95047, 1.0, 1.08883};
    QVector<double> scale(9, 0.0);
    for (int i = 0; i < 3; ++i) {
        double num = 0.0, den = 0.0;
        for (int j = 0; j < 3; ++j) {
            num += bfd[i * 3 + j] * d65[j];
            den += bfd[i * 3 + j] * w[j];
        }
        scale[i * 3 + i] = num / den;
    }
    const QVector<double> cat = linalg::solve(bfd, linalg::multiply(scale, bfd, 3, 3, 3), 3, 3);  // Mbfd \ (D Mbfd)
    if (cat.isEmpty())
        return {};
    static const QVector<double> xyz2rgb = {3.2404542, -1.5371385, -0.4985314, -0.9692660, 1.8760108, 0.0415560,
                                            0.0556434, -0.2040259, 1.0572252};
    return linalg::multiply(linalg::multiply(xyz2rgb, cat, 3, 3, 3), M, 3, 3, B);
}

QImage renderRgb(const Cube& cube, const QVector<double>& m, double exposurePercentile, double* fixedScale)
{
    const int B = cube.bands();
    if (cube.isNull() || m.size() != 3 * B)
        return {};
    const qsizetype N = cube.planeSize();
    std::vector<float> rgb(size_t(N) * 3);
    std::vector<double> peak(static_cast<size_t>(N));
    for (qsizetype p = 0; p < N; ++p) {
        double c[3] = {0, 0, 0};
        for (int b = 0; b < B; ++b) {
            const double v = cube.band(b)[p];
            c[0] += m[b] * v;
            c[1] += m[B + b] * v;
            c[2] += m[2 * B + b] * v;
        }
        for (int k = 0; k < 3; ++k)
            rgb[size_t(p) * 3 + k] = float(c[k]);
        peak[size_t(p)] = std::max({c[0], c[1], c[2]});
    }
    double scale = fixedScale && *fixedScale > 0.0 ? *fixedScale : percentileOf(peak, exposurePercentile);
    if (!(scale > 0.0) || !std::isfinite(scale))
        scale = 1.0;
    if (fixedScale)
        *fixedScale = scale;
    // 4096-entry lookup table for the transfer curve.
    static const std::vector<uchar> lut = [] {
        std::vector<uchar> t(4097);
        for (int i = 0; i <= 4096; ++i)
            t[size_t(i)] = uchar(std::lround(255.0 * srgbEncode(i / 4096.0)));
        return t;
    }();
    QImage img(cube.cols(), cube.rows(), QImage::Format_RGB888);
    for (int y = 0; y < cube.rows(); ++y) {
        uchar* line = img.scanLine(y);
        for (int x = 0; x < cube.cols(); ++x) {
            const qsizetype p = qsizetype(y) * cube.cols() + x;
            for (int k = 0; k < 3; ++k) {
                const double v = std::clamp(double(rgb[size_t(p) * 3 + k]) / scale, 0.0, 1.0);
                line[x * 3 + k] = std::isfinite(v) ? lut[size_t(std::lround(v * 4096.0))] : 0;
            }
        }
    }
    return img;
}

} // namespace iat::hsi
