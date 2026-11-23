#pragma once

#include "core/Hsi.h"

#include <QColor>
#include <QImage>
#include <QRect>
#include <QString>
#include <QPointF>
#include <QPolygonF>
#include <QVector>

#include <array>
#include <utility>

#include <memory>
#include <optional>

namespace iat::hsi {

// ---------------------------------------------------------------------------
// Spectra of areas
// ---------------------------------------------------------------------------

struct RegionStats {
    QVector<double> mean;  // per band
    QVector<double> std;   // per band (normalised by N)
    qint64 count = 0;      // pixels used
    bool isValid() const { return count > 0; }
};

// Mean and standard deviation of every band over a rectangle (cube pixels;
// clipped to the cube). Non-finite values are left out.
RegionStats regionStats(const Cube& cube, const QRect& area);
// Same, over the pixels where mask[y * cols + x] is non-zero.
RegionStats maskStats(const Cube& cube, const QVector<quint8>& mask);

// ---------------------------------------------------------------------------
// Similarity and segmentation
// ---------------------------------------------------------------------------

// Spectral angle (radians, 0..π) between every pixel's spectrum and
// `reference`. Insensitive to brightness: a spectrum and a scaled copy of it
// have angle 0. NaN where the pixel's spectrum is all zero.
Image2D spectralAngleMap(const Cube& cube, const QVector<double>& reference);

struct Clustering {
    int k = 0;
    QVector<int> labels;                 // per pixel (row-major), 0..k-1
    QVector<QVector<double>> centres;    // mean spectrum of every class
    QVector<qint64> sizes;               // pixels per class
    bool isValid() const { return k > 0 && !labels.isEmpty(); }
};

// k-means clustering of the pixel spectra (k-means++ start, fixed seed, so
// the result is reproducible). With `normalize`, every spectrum is divided
// by its length first, so classes follow spectral shape rather than
// brightness (like the spectral angle).
Clustering kMeans(const Cube& cube, int k, int iterations = 30, bool normalize = true, quint32 seed = 1);

// Every pixel matched to the most similar of several reference spectra (the
// mean spectra of areas drawn on the image) by spectral angle: the
// supervised spectral angle mapper (SAM) classification of ENVI and others.
struct AreaMatch {
    int k = 0;                  // number of references
    QVector<int> best;          // per pixel: index of the closest reference, -1 if none within maxAngle
    QVector<float> angle;       // per pixel: angle to the closest reference (radians)
    QVector<qint64> counts;     // pixels matched to each reference
    double maxAngle = 0.0;      // radians
    bool isValid() const { return k > 0 && !best.isEmpty(); }
};
AreaMatch matchToReferences(const Cube& cube, const QVector<QVector<double>>& references, double maxAngle);
// Matched pixels in their reference's colour; with `shade`, darker the
// further from the reference (full colour at angle 0, 35 % at maxAngle).
// Unmatched pixels show `background` (a grey picture of the scene; black if null).
QImage renderAreaMatch(const AreaMatch& match, int cols, int rows, const QVector<QColor>& colours, bool shade,
                       const QImage& background);

// ---------------------------------------------------------------------------
// Principal components
// ---------------------------------------------------------------------------

struct Pca {
    QVector<double> mean;        // per band
    QVector<double> variances;   // per component, descending
    QVector<double> vectors;     // B × B, component i in column i
    int bands = 0;
    bool isValid() const { return bands > 0; }
    // Fraction of the total variance in each component.
    QVector<double> explained() const;
};
Pca principalComponents(const Cube& cube);
// Component `index` (0-based) of every pixel.
Image2D pcaComponent(const Cube& cube, const Pca& pca, int index);

// Pixels whose (x[i], y[i]) lies inside `polygon` (e.g. a lasso drawn on a
// score plot of two principal components): 1 inside, 0 outside or NaN.
QVector<quint8> selectInPolygon(const Image2D& x, const Image2D& y, const QPolygonF& polygon);

// ---------------------------------------------------------------------------
// Band maths and spectral indices
// ---------------------------------------------------------------------------

// A formula over the bands, evaluated at every pixel, e.g.
//   (b7 - b11) / (b7 + b11)            bands by number (1-based)
//   (R531 - R570) / (R531 + R570)      bands by wavelength: the nearest band
// with + − × / ^, parentheses, numbers and the functions sqrt, abs, log, exp.
class BandMath {
public:
    // Parses `formula` for a cube with `bands` bands and these centre
    // wavelengths (may be empty). Returns null and sets `error` if the
    // formula is not valid or refers to a band or wavelength the cube does
    // not have (a wavelength more than one band spacing outside the range).
    static std::shared_ptr<BandMath> parse(const QString& formula, int bands, const QVector<double>& wavelengths,
                                           QString* error);
    virtual ~BandMath() = default;
    Image2D evaluate(const Cube& cube) const;
    // The bands (0-based) the formula uses.
    QVector<int> bandsUsed() const;
    // The formula with wavelengths replaced by band numbers.
    QString resolved() const { return m_resolved; }

    struct Node;
private:
    std::shared_ptr<Node> m_root;
    QString m_resolved;
};

struct IndexPreset {
    QString name;       // English
    QString formula;    // with R<nm> references
    QString description;
};
// Spectral indices for vegetation and colour. Those whose wavelengths the
// sensor does not cover are rejected by BandMath::parse.
QVector<IndexPreset> indexPresets();

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

enum class Colormap { Grey, Viridis, Turbo, Diverging };
QColor colormapColor(Colormap map, double t);  // t in 0..1

// Grey or colour-mapped 8-bit picture of an image, linear from `low` to
// `high`. NaN pixels are black.
QImage renderScalar(const Image2D& image, double low, double high, Colormap map = Colormap::Grey);
// `lowFraction` / `highFraction` percentiles (e.g. 0.005 / 0.995) of the finite pixels.
std::pair<double, double> autoRange(const Image2D& image, double lowFraction = 0.005, double highFraction = 0.995);

// Display range of one channel (low, high).
using Range = std::pair<double, double>;
// Three images on red, green and blue. With `useGiven`, each is stretched
// over ranges[i]; otherwise between its own 0.5 and 99.5 % percentiles,
// which are written to `ranges` (so a second picture can use the same
// stretch, e.g. for A/B).
QImage renderComposite(const Image2D& r, const Image2D& g, const Image2D& b, std::array<Range, 3>& ranges, bool useGiven);
// False colour: bands r, g, b (0-based) on the red, green and blue
// channels, each stretched between its own 0.5 and 99.5 % percentiles.
QImage renderFalseColour(const Cube& cube, int r, int g, int b);
// The first three principal components on red, green and blue.
QImage renderPcaComposite(const Cube& cube, const Pca& pca);
// Three bands for a natural-looking picture: about 610, 550 and 490 nm, or
// bands 15, 9 and 2 when the wavelengths are not known.
void pseudoTrueColourBands(const Cube& cube, int& r, int& g, int& b);
// The three bands that are least correlated with each other (the most
// colourful false-colour composite).
void maxContrastBands(const Cube& cube, int& r, int& g, int& b);
// All bands side by side in a grid (4 × 4 for 16 bands) with one shared
// stretch, `gap` pixels apart.
// `range`, if given: used as the stretch when `useGiven`, else the
// stretch used is written to it.
QImage renderBandGallery(const Cube& cube, int gap = 2, Range* range = nullptr, bool useGiven = false);
// Where pixel (x, y) of a band appears in the gallery (top-left of each
// band's tile plus (x, y)), for every band.
QVector<QPointF> galleryPositions(const Cube& cube, double x, double y, int gap = 2);
// One colour per class, for class maps and spectrum plots.
QColor classColor(int index);
QImage renderClasses(const Clustering& clustering, int cols, int rows);
// `base` (grey) with the pixels where mask is non-zero tinted in `color`.
QImage renderMaskOverlay(const QImage& base, const QVector<quint8>& mask, const QColor& color, double opacity = 0.55);

} // namespace iat::hsi
