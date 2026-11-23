#pragma once

#include "core/Hsi.h"
#include "core/HsiColour.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace iat::hsi {

// ---------------------------------------------------------------------------
// Reference frames (radiometric calibration in the mosaic domain)
// ---------------------------------------------------------------------------

// Dark and white reference captures of the same sensor, applied pixel by
// pixel to the raw mosaic before the channels are extracted (a flat field):
//
//   dark only          raw − dark                                (DN)
//   white only         raw / white · ρ_white                     (reflectance)
//   dark and white     (raw − dark) / (white − dark) · ρ_white   (reflectance)
//
// With exposure times, the scene and the white reference are each divided
// by their own exposure first, so they may be taken with different settings.
// Per-pixel division also removes vignetting and pixel-to-pixel response
// differences of the filters, which a white area in the scene cannot.
struct ReferenceFrames {
    Image2D dark;                 // empty: not used
    Image2D white;                // empty: not used
    double sceneExposure = 0.0;   // ms; 0: unknown (taken as equal)
    double whiteExposure = 0.0;   // ms; 0: unknown
    double whiteReflectance = 1.0;
    bool any() const { return !dark.isNull() || !white.isNull(); }
};

struct FrameCorrection {
    Image2D image;
    QString error;       // empty on success
    QStringList notes;
    bool reflectance = false;  // true if a white frame was used
    bool ok() const { return error.isEmpty(); }
};

// Applies the reference frames to a raw mosaic. The frames must have the
// same size as the raw image. White pixels at or below the dark level give
// NaN (no information).
FrameCorrection applyReferenceFrames(const Image2D& raw, const ReferenceFrames& frames);

// ---------------------------------------------------------------------------
// Spectral correction
// ---------------------------------------------------------------------------

// A linear map from the B measured channels to V corrected ("virtual")
// bands: corrected = C · measured, C is V × B. Fabry-Perot filters on a
// snapshot mosaic respond outside their main peak (a broadband background,
// harmonics, cross-talk from neighbouring pixels), so the raw channels
// overlap strongly. The matrix is chosen so that the corrected bands
// respond like narrow Gaussian filters at the channel peaks, as in the
// correction step of snapshot-mosaic camera software.
struct SpectralCorrection {
    QVector<double> matrix;     // V × B, row-major
    int outBands = 0;           // V
    int inBands = 0;            // B
    QVector<double> centres;    // nm, per corrected band (may be empty for a loaded matrix)
    QVector<double> fwhm;       // nm, target width per corrected band (computed matrices)
    double residual = 0.0;      // ‖C·A − T‖ / ‖T‖ of a computed matrix (0..1, lower is better)
    QString source;             // "computed …" or the file it came from
    bool isValid() const { return outBands > 0 && inBands > 0 && matrix.size() == qsizetype(outBands) * inBands; }
};

// Computes the correction from the spectral response: target band v is a
// Gaussian at the channel's peak with width `targetFwhm` nm (0: each
// channel's own measured FWHM), scaled to the same area as the channel's
// response so a flat spectrum keeps its level. C = T Aᵀ (A Aᵀ + λ I)⁻¹ with
// λ = regularisation · trace(A Aᵀ) / B (larger: less noise amplification,
// wider bands).
SpectralCorrection computeCorrection(const SpectralResponse& response, double targetFwhm, double regularisation);

// The response of the corrected bands: C · A (V × nL).
SpectralResponse correctedResponse(const SpectralResponse& response, const SpectralCorrection& correction);
// How much the correction amplifies independent per-channel noise relative
// to a flat signal: mean over the output bands of ‖row‖₂ / |Σ row|. 1/√B
// for plain averaging, 1 for no change; strongly overlapping channels give
// large values (noise grows as the bands get narrower).
double noiseGain(const SpectralCorrection& correction);

// Applies the correction to every pixel. Band wavelengths become the
// correction's centres (if it has them).
Cube applyCorrection(const Cube& cube, const SpectralCorrection& correction);

} // namespace iat::hsi
