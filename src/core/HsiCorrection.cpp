#include "core/HsiCorrection.h"

#include "core/HsiLinalg.h"

#include <QCoreApplication>

#include <algorithm>
#include <cmath>
#include <limits>

namespace iat::hsi {

namespace {
QString tr(const char* text) { return QCoreApplication::translate("iat::hsi", text); }
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
}

FrameCorrection applyReferenceFrames(const Image2D& raw, const ReferenceFrames& f)
{
    FrameCorrection out;
    if (raw.isNull()) {
        out.error = tr("There is no image to correct.");
        return out;
    }
    const bool hasDark = !f.dark.isNull(), hasWhite = !f.white.isNull();
    if (!hasDark && !hasWhite) {
        out.image = raw;
        return out;
    }
    if ((hasDark && !f.dark.sameSize(raw)) || (hasWhite && !f.white.sameSize(raw))) {
        out.error = tr("The reference frames must have the same size as the image (%1 × %2): dark %3, white %4.")
                        .arg(raw.height()).arg(raw.width())
                        .arg(hasDark ? QStringLiteral("%1 × %2").arg(f.dark.height()).arg(f.dark.width()) : tr("none"))
                        .arg(hasWhite ? QStringLiteral("%1 × %2").arg(f.white.height()).arg(f.white.width()) : tr("none"));
        return out;
    }
    // Exposure: the scene and the white reference are divided by their own
    // exposure times (equal if either is unknown).
    const double exposureRatio = (f.sceneExposure > 0.0 && f.whiteExposure > 0.0) ? f.whiteExposure / f.sceneExposure : 1.0;
    Image2D img(raw.width(), raw.height());
    const double* r = raw.constData();
    const double* d = hasDark ? f.dark.constData() : nullptr;
    const double* w = hasWhite ? f.white.constData() : nullptr;
    double* o = img.data();
    qint64 bad = 0;
    for (qsizetype i = 0; i < raw.pixelCount(); ++i) {
        const double dark = d ? d[i] : 0.0;
        double v = r[i] - dark;
        if (w) {
            const double ref = w[i] - dark;
            if (ref > 0.0 && std::isfinite(ref)) {
                v = v / ref * exposureRatio * f.whiteReflectance;
            } else {
                v = kNaN;
                ++bad;
            }
        }
        o[i] = v;
    }
    out.image = img;
    out.reflectance = hasWhite;
    out.notes << (hasWhite ? (hasDark ? tr("Flat field: (image − dark) / (white − dark).") : tr("Flat field: image / white."))
                           : tr("Dark frame subtracted."));
    if (exposureRatio != 1.0)
        out.notes << tr("Exposure: scene %1 ms, white %2 ms.").arg(f.sceneExposure).arg(f.whiteExposure);
    if (bad > 0)
        out.notes << tr("%1 pixels have no signal in the white frame and are left empty.").arg(bad);
    return out;
}

SpectralCorrection computeCorrection(const SpectralResponse& r, double targetFwhm, double regularisation)
{
    SpectralCorrection c;
    const int B = r.bands(), nL = r.samples();
    if (B == 0 || nL < 3)
        return c;
    const QVector<double> peaks = r.peaks();
    const QVector<double> measured = r.fwhm();
    // A (B × nL), T (B × nL)
    QVector<double> A(B * nL), T(B * nL);
    for (int b = 0; b < B; ++b)
        for (int i = 0; i < nL; ++i)
            A[b * nL + i] = r.qe[b][i];
    c.fwhm.resize(B);
    for (int b = 0; b < B; ++b) {
        const double w = targetFwhm > 0.0 ? targetFwhm : std::max(1.0, measured[b]);
        c.fwhm[b] = w;
        const double sigma = w / 2.3548200450309493;
        double area = 0.0, targetArea = 0.0;
        for (int i = 0; i < nL; ++i) {
            area += A[b * nL + i];
            const double t = std::exp(-0.5 * std::pow((r.lambda[i] - peaks[b]) / sigma, 2.0));
            T[b * nL + i] = t;
            targetArea += t;
        }
        const double scale = targetArea > 0.0 ? area / targetArea : 0.0;
        for (int i = 0; i < nL; ++i)
            T[b * nL + i] *= scale;
    }
    const QVector<double> At = linalg::transpose(A, B, nL);
    QVector<double> G = linalg::multiply(A, At, B, nL, B);  // A Aᵀ
    double trace = 0.0;
    for (int b = 0; b < B; ++b)
        trace += G[b * B + b];
    const double lambda = std::max(0.0, regularisation) * trace / B;
    for (int b = 0; b < B; ++b)
        G[b * B + b] += lambda;
    // C = T Aᵀ G⁻¹  ⇔  Cᵀ = G⁻¹ (T Aᵀ)ᵀ   (G symmetric)
    const QVector<double> TAt = linalg::multiply(T, At, B, nL, B);
    const QVector<double> Ct = linalg::solve(G, linalg::transpose(TAt, B, B), B, B);
    if (Ct.isEmpty())
        return c;
    c.matrix = linalg::transpose(Ct, B, B);
    c.outBands = B;
    c.inBands = B;
    c.centres = peaks;
    // Residual ‖C A − T‖ / ‖T‖.
    const QVector<double> CA = linalg::multiply(c.matrix, A, B, B, nL);
    double num = 0.0, den = 0.0;
    for (qsizetype i = 0; i < CA.size(); ++i) {
        num += std::pow(CA[i] - T[i], 2.0);
        den += T[i] * T[i];
    }
    c.residual = den > 0.0 ? std::sqrt(num / den) : 0.0;
    c.source = targetFwhm > 0.0
                   ? QStringLiteral("computed from the spectral response: Gaussian targets, FWHM %1 nm, regularisation %2")
                         .arg(targetFwhm).arg(regularisation)
                   : QStringLiteral("computed from the spectral response: Gaussian targets of the measured widths, regularisation %1")
                         .arg(regularisation);
    return c;
}

SpectralResponse correctedResponse(const SpectralResponse& r, const SpectralCorrection& c)
{
    SpectralResponse out;
    if (!c.isValid() || c.inBands != r.bands())
        return out;
    out.lambda = r.lambda;
    out.qe = QVector<QVector<double>>(c.outBands, QVector<double>(r.samples(), 0.0));
    for (int v = 0; v < c.outBands; ++v)
        for (int b = 0; b < c.inBands; ++b) {
            const double k = c.matrix[v * c.inBands + b];
            for (int i = 0; i < r.samples(); ++i)
                out.qe[v][i] += k * r.qe[b][i];
        }
    return out;
}

Cube applyCorrection(const Cube& cube, const SpectralCorrection& c)
{
    if (!c.isValid() || cube.bands() != c.inBands)
        return cube;
    const int B = c.inBands, V = c.outBands;
    Cube out(cube.rows(), cube.cols(), V);
    out.units = cube.units;
    out.history = cube.history;
    out.wavelengths = c.centres.size() == V ? c.centres : QVector<double>();
    QVector<double> s(B);
    for (qsizetype p = 0; p < cube.planeSize(); ++p) {
        for (int b = 0; b < B; ++b)
            s[b] = cube.band(b)[p];
        for (int v = 0; v < V; ++v) {
            double acc = 0.0;
            const double* row = c.matrix.constData() + qsizetype(v) * B;
            for (int b = 0; b < B; ++b)
                acc += row[b] * s[b];
            out.band(v)[p] = float(acc);
        }
    }
    out.history << QStringLiteral("spectral correction (%1 → %2 bands): %3").arg(B).arg(V).arg(c.source);
    return out;
}

double noiseGain(const SpectralCorrection& c)
{
    if (!c.isValid())
        return 0.0;
    double total = 0.0;
    for (int i = 0; i < c.outBands; ++i) {
        double sq = 0.0, sum = 0.0;
        for (int j = 0; j < c.inBands; ++j) {
            const double v = c.matrix[qsizetype(i) * c.inBands + j];
            sq += v * v;
            sum += v;
        }
        total += std::abs(sum) > 1e-12 ? std::sqrt(sq) / std::abs(sum) : 0.0;
    }
    return total / c.outBands;
}

} // namespace iat::hsi
