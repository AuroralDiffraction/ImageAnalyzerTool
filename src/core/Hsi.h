#pragma once

#include "core/Image2D.h"

#include <QRect>
#include <QString>
#include <QStringList>
#include <QVector>

#include <array>
#include <optional>

namespace iat::hsi {

// Snapshot hyperspectral sensor with a 4 × 4 Fabry-Perot filter mosaic:
// every 4 × 4 block of pixels (a super-pixel) holds one pixel of each of the
// 16 spectral channels.
constexpr int kPattern = 4;
constexpr int kBands = kPattern * kPattern;

// Which channel (1..16) sits at each position of the 4 × 4 super-pixel,
// row by row (left → right, top → bottom as seen on the sensor). This is
// `mosaicLayout` / `map` of hsi_reconstruct.m and extract_hsi_channels.m.
struct MosaicLayout {
    std::array<int, kBands> channel{};

    // Ways of numbering the 16 positions of the super-pixel.
    enum class Preset {
        RowByRow,            // [1 2 3 4; 5 6 7 8; 9 10 11 12; 13 14 15 16]
        RowByRowVertical,    // column by column: [1 5 9 13; 2 6 10 14; …]
        Serpentine,          // rows, every other one reversed: [1 2 3 4; 8 7 6 5; …]
        SerpentineVertical,  // columns, every other one reversed: [1 8 9 16; 2 7 10 15; …]
    };
    static MosaicLayout preset(Preset p);
    // The layout used until another is chosen: serpentine (vertical),
    // [1 8 9 16; 2 7 10 15; 3 6 11 14; 4 5 12 13].
    static MosaicLayout defaultLayout() { return preset(Preset::SerpentineVertical); }
    static MosaicLayout rowMajor() { return preset(Preset::RowByRow); }
    // The preset this layout is, if any.
    static std::optional<Preset> presetOf(const MosaicLayout& layout);

    int at(int row, int col) const { return channel[size_t(row * kPattern + col)]; }
    void set(int row, int col, int ch) { channel[size_t(row * kPattern + col)] = ch; }
    // Every channel 1..16 exactly once (the check of hsi_reconstruct.m).
    bool isValid() const;
    // Row and column (0-based) of `ch` (1-based) in the super-pixel.
    void positionOf(int ch, int& row, int& col) const;
    // "1 8 9 16; 2 7 10 15; 3 6 11 14; 4 5 12 13"
    QString toString() const;
    static std::optional<MosaicLayout> fromString(const QString& text);
    bool operator==(const MosaicLayout& o) const { return channel == o.channel; }
};

// A hyperspectral cube: `bands` images of rows × cols values (float), one
// after the other (band-sequential, BSQ). Band b (0-based) is channel b+1.
// Pixel (x, y) is column x, row y, as in Image2D. The data is implicitly
// shared, so copies are cheap and a cube can be handed to a worker thread.
class Cube {
public:
    Cube() = default;
    Cube(int rows, int cols, int bands, float fill = 0.0f);

    bool isNull() const { return m_bands == 0; }
    int rows() const { return m_rows; }
    int cols() const { return m_cols; }
    int bands() const { return m_bands; }
    qsizetype planeSize() const { return qsizetype(m_rows) * m_cols; }

    const float* band(int b) const { return m_data.constData() + b * planeSize(); }
    float* band(int b) { return m_data.data() + b * planeSize(); }
    float at(int x, int y, int b) const { return m_data[b * planeSize() + qsizetype(y) * m_cols + x]; }
    const QVector<float>& data() const { return m_data; }
    // Gives this cube its own copy of shared data. Call it before writing
    // to a copied cube from several threads: band() would otherwise make
    // the copy on each thread at once (copy-on-write is not thread-safe).
    void detach() { m_data.detach(); }

    // The spectrum of pixel (x, y), one value per band.
    QVector<double> spectrum(int x, int y) const;
    // Band b as a double image (width = cols, height = rows).
    Image2D bandImage(int b) const;
    // Mean over the bands at every pixel: the pseudo-panchromatic image.
    Image2D meanImage() const;

    // Centre wavelength of every band in nm, or empty if not known.
    QVector<double> wavelengths;
    // "DN" (raw counts) or "reflectance".
    QString units = QStringLiteral("DN");
    // What was done to the data, one step per entry (English; stored in exports).
    QStringList history;

    bool hasWavelengths() const { return wavelengths.size() == m_bands && m_bands > 0; }
    // "Band 5" or "Band 5 (514 nm)"
    QString bandLabel(int b) const;
    // The band whose centre is closest to `nm`, or -1 without wavelengths.
    int nearestBand(double nm) const;
    // Same metadata (wavelengths, units, history), other size.
    Cube withSameMetadata(int rows, int cols) const;

private:
    int m_rows = 0;
    int m_cols = 0;
    int m_bands = 0;
    QVector<float> m_data;
};

// How the 16 channels are taken out of the raw mosaic.
enum class ExtractMethod {
    // One value per super-pixel and channel: rows/4 × cols/4 × 16, exactly
    // what extract_hsi_channels.m does. Every band is sampled at a slightly
    // different place in the super-pixel (up to ¾ of a cube pixel apart).
    Subsample,
    // As Subsample, but each band is resampled (bilinear) to the centre of
    // the super-pixel, so all bands describe the same point of the scene.
    // This removes false spectra at edges; recommended for analysis.
    Registered,
    // Full sensor resolution: each band interpolated bilinearly from its own
    // samples at their true positions (rows × cols × 16).
    Bilinear,
    // Full resolution, pseudo-panchromatic image difference (Mihoubi et al.,
    // IEEE TCI 2017): a full-resolution image of all channels guides the
    // interpolation of every band, so edges stay sharp and aligned.
    Ppid,
};

QString methodName(ExtractMethod method);  // English (stored in exports)

struct ExtractOptions {
    MosaicLayout layout = MosaicLayout::defaultLayout();
    double blackLevel = 0.0;      // subtracted from every pixel, in DN
    int bitDepth = 14;            // ADC bit depth; 0 = no check for MSB-aligned data
    bool clipNegative = true;     // max(raw − black, 0), as hsi_reconstruct.m does
    ExtractMethod method = ExtractMethod::Subsample;
};

struct ExtractResult {
    Cube cube;
    QString error;   // empty on success
    QStringList notes;
    bool ok() const { return error.isEmpty(); }
};

// Raw mosaic (rows × cols, any size ≥ 4 × 4; cropped to a multiple of 4)
// → cube with bands in channel order 1..16. If the largest value exceeds
// 2^bitDepth − 1, the data is taken to be MSB-aligned in a 16-bit container
// and shifted down, as hsi_reconstruct.m does.
ExtractResult extract(const Image2D& raw, const ExtractOptions& options);
// 2^−(16 − bitDepth) if the image's largest value exceeds 2^bitDepth − 1
// (MSB-aligned data), else 1: the scaling extract() applies.
double msbScale(const Image2D& raw, int bitDepth);

// ---------------------------------------------------------------------------
// Processing. Each function returns a new cube and appends to its history.
// ---------------------------------------------------------------------------

// Reflectance: every band divided by its mean over `whiteRef` (a white
// reference in the scene), times `whiteReflectance`. Use after dark
// subtraction. Pixels outside the image are ignored; returns the input if
// the area is empty.
Cube toReflectance(const Cube& cube, const QRect& whiteRef, double whiteReflectance = 1.0);
// Gaussian low-pass of every band (anti-aliasing); σ in cube pixels.
Cube gaussianBlur(const Cube& cube, double sigma);
// Guided filter (He et al.) of every band with the pseudo-panchromatic image
// as the guide: smooths noise while keeping the edges of all bands in the
// same place. `eps` is relative to the guide's variance.
Cube guidedDenoise(const Cube& cube, int radius, double eps);
// Minimum noise fraction (MNF) denoising, as in ENVI: the bands are
// transformed into components ordered by signal-to-noise ratio (noise
// estimated from differences of horizontally adjacent pixels), the
// components after `keep` are set to zero, and the result transformed back.
Cube mnfDenoise(const Cube& cube, int keep);
// Signal-to-noise ratio of every MNF component (descending), for choosing `keep`.
QVector<double> mnfSnr(const Cube& cube);

enum class Interpolation { Nearest, Bilinear, Bicubic };
// Enlarges every band by `factor` (2, 3, 4, ...). Interpolation only: no new
// detail is created.
Cube upscale(const Cube& cube, int factor, Interpolation method);

// Spectral preprocessing (chemometrics): per-pixel transforms of the
// spectrum that remove brightness, scatter and baseline effects before
// PCA, classification or comparison.
enum class SpectralNormalisation {
    None,
    Snv,         // standard normal variate: (x − mean) / std over the bands
    Msc,         // multiplicative scatter correction against the mean spectrum
    UnitLength,  // divided by its length (L2 norm)
    UnitArea,    // divided by the sum of its absolute values
    UnitMax,     // divided by its largest absolute value
};
// −log10(reflectance), values clipped at 1e-6. Use on reflectance data.
Cube toAbsorbance(const Cube& cube);
Cube normaliseSpectra(const Cube& cube, SpectralNormalisation method);
// Savitzky–Golay smoothing (derivative 0) or derivative (1, 2) along the
// bands: a polynomial of degree `polynomial` fitted over `window` bands
// (odd; shifted inward at the ends). Derivatives are per nm when the band
// wavelengths are known, else per band.
Cube savitzkyGolay(const Cube& cube, int window, int polynomial, int derivative);

// The processing chain of the hyperspectral workspace, in its fixed order:
// reflectance → MNF denoise → guided filter → anti-aliasing → absorbance →
// normalisation → Savitzky–Golay derivative → upscaling.
// Sensor corrections come first (they need the raw statistics), denoising
// before anything that spreads noise, resampling last.
struct ProcessingSettings {
    bool reflectance = false;
    QRect whiteReference;           // in cube pixels
    double whiteReflectance = 1.0;  // reflectance of the reference (e.g. 0.99 for Spectralon)
    bool mnf = false;
    int mnfKeep = 6;
    bool guided = false;
    int guidedRadius = 2;
    double guidedEps = 0.01;
    bool antiAlias = false;
    double antiAliasSigma = 0.7;
    bool absorbance = false;
    SpectralNormalisation normalisation = SpectralNormalisation::None;
    bool smooth = false;   // Savitzky–Golay smoothing (when derivative is 0)
    int derivative = 0;    // 0: none, 1 or 2: Savitzky–Golay derivative
    int sgWindow = 5;
    int sgPolynomial = 2;
    int upscale = 1;
    Interpolation interpolation = Interpolation::Bicubic;

    // True if any step changes the cube.
    bool any() const;
};
Cube process(const Cube& cube, const ProcessingSettings& settings);

// Bilinear sample of a rows × cols float plane at fractional position
// (x, y), clamped to the edges.
float sampleBilinear(const float* plane, int rows, int cols, double x, double y);

} // namespace iat::hsi
