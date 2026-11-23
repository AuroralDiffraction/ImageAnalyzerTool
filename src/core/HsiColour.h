#pragma once

#include "core/Hsi.h"

#include <QImage>
#include <QRect>
#include <QVector>

namespace iat::hsi {

// Spectral response (quantum efficiency) of every channel: the forward model
// s = A · L of hsi_reconstruct.m, A = QEᵀ.
struct SpectralResponse {
    QVector<double> lambda;           // wavelengths in nm, ascending (nL)
    QVector<QVector<double>> qe;      // qe[b][i]: channel b+1 at lambda[i], as a fraction (0..1)

    int bands() const { return int(qe.size()); }
    int samples() const { return int(lambda.size()); }
    bool isNull() const { return qe.isEmpty() || lambda.isEmpty(); }

    // Filter peak of every channel, after subtracting the broadband
    // background shared by all channels (as hsi_reconstruct.m prints them).
    QVector<double> peaks() const;
    // Width at half maximum of every channel's background-subtracted peak, in nm.
    QVector<double> fwhm() const;
};

enum class WhiteBalance {
    GrayWorld,   // the scene average is grey (hsi_reconstruct.m's default)
    WhitePatch,  // the brightest pixels (whitePercentile of Y) are white
    Reference,   // the mean of a reference area (a white or grey target) is white
    Flat,        // no balancing: an equal-energy illuminant
};

struct ColourSettings {
    double sigmaNm = 20.0;      // correlation length of the smoothness prior [nm]
    double alphaRel = 0.05;     // regularisation, relative to the mean channel energy
    WhiteBalance whiteBalance = WhiteBalance::GrayWorld;
    double whitePercentile = 99.0;
    QRect reference;            // in cube pixels, for WhiteBalance::Reference
    double exposurePercentile = 99.0;  // this percentile of max(R, G, B) maps to 1.0
};

// The spectral estimator R (nL × B, row-major) of the Wiener / Tikhonov
// inverse with a Gaussian smoothness prior:
//   L̂ = K Aᵀ (A K Aᵀ + α I)⁻¹ s,   α = alphaRel · trace(A K Aᵀ) / B
// Empty if the matrix is singular.
QVector<double> spectralEstimator(const SpectralResponse& response, double sigmaNm, double alphaRel);

// 3 × B matrix (row-major) from channel values to CIE XYZ: cmfᵀ (dλ ∘ R).
QVector<double> xyzMatrix(const SpectralResponse& response, double sigmaNm, double alphaRel);

// 3 × B matrix (row-major) from channel values to linear sRGB, including the
// white balance (Bradford adaptation from the scene white to D65) for this
// cube: reconRGB of hsi_reconstruct.m. `xyz` is xyzMatrix(); `response` is
// needed for WhiteBalance::Flat only. Empty on failure. `whiteXyz`, if
// given, receives the scene white (Y = 1).
QVector<double> rgbMatrix(const Cube& cube, const QVector<double>& xyz, const ColourSettings& settings,
                          const SpectralResponse& response, QVector<double>* whiteXyz = nullptr);

// Applies a 3 × B matrix to the cube, scales so that `exposurePercentile`
// of max(R, G, B) is 1, clips to 0..1 and applies the sRGB transfer curve.
// `scale`, if given: a value > 0 is used as that scale (to render another
// cube with the same exposure, e.g. for A/B); otherwise the scale used is
// written to it.
QImage renderRgb(const Cube& cube, const QVector<double>& rgb3xB, double exposurePercentile, double* scale = nullptr);

// CIE 1931 2° colour-matching functions at `lambda` (nL × 3, row-major;
// zero outside 380–780 nm), interpolated from the 5 nm table.
QVector<double> cie1931(const QVector<double>& lambda);

// Nearest-rank percentile as in hsi_reconstruct.m: sorted x at
// round(q/100 · n) (1-based, clamped). NaNs are ignored.
double percentileOf(std::vector<double> values, double q);

// sRGB transfer curve (linear 0..1 → encoded 0..1).
double srgbEncode(double linear);

} // namespace iat::hsi
