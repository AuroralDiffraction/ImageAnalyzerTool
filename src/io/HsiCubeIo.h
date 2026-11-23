#pragma once

#include "core/Hsi.h"
#include "core/HsiColour.h"
#include "core/HsiCorrection.h"

#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

namespace iat::hsi {

// How cube values are stored in an exported file.
enum class SampleType {
    Float32,  // exact (default)
    UInt16,   // rounded and clipped to 0..65535; for raw counts (DN)
};

enum class Interleave { Bsq, Bil, Bip };

// A cube read from a file.
struct CubeFile {
    QString error;                        // empty on success
    Cube cube;
    std::optional<MosaicLayout> layout;   // if the file records one
    QStringList notes;
    bool ok() const { return error.isEmpty(); }
};

// ENVI (.hdr + binary data), the usual exchange format for hyperspectral
// cubes: ENVI, Spectral Python, MATLAB's hypercube, GDAL / QGIS and most
// camera software read it. `path` may name the .hdr or the data file; the
// data goes to <base>.img, the header to <base>.hdr. Wavelengths (nm), band
// names, units and history are written to the header.
bool writeEnvi(const Cube& cube, const QString& path, SampleType type, Interleave interleave, QString* error,
               const std::optional<MosaicLayout>& layout = std::nullopt);
// Reads an ENVI cube from its .hdr or its data file (any of the common data
// types, byte orders and interleaves).
CubeFile readEnvi(const QString& path);
// The header and data file names that writeEnvi uses for `path`.
QString enviHeaderPath(const QString& path);
QString enviDataPath(const QString& path);

// MATLAB .mat (-v7): `cube` (rows × cols × bands, single or uint16),
// `wavelengths` (bands × 1, if known), `mosaicLayout` (4 × 4), `units` and
// `history`. hsi_reconstruct.m's variable names, so the scripts can load it.
bool writeMat(const Cube& cube, const QString& path, SampleType type, QString* error,
              const std::optional<MosaicLayout>& layout = std::nullopt);
// Reads the variable `cube`, or else the first 3-D array, from a .mat file.
// Band wavelengths come from `wavelengths` / `bandCenters` when they have
// one value per band.
CubeFile readMat(const QString& path);

// Multi-page TIFF: one page per band, 32-bit float or 16-bit unsigned,
// uncompressed. Each page's description names the band (and wavelength).
bool writeMultiPageTiff(const Cube& cube, const QString& path, SampleType type, QString* error);

// A cube from .hdr / .img / .mat, by extension. Not for raw mosaics.
CubeFile readCube(const QString& path);
bool isCubeFile(const QString& path);  // .hdr (and data files next to a .hdr)

// ---------------------------------------------------------------------------
// Spectral response (reconstruction matrix) files
// ---------------------------------------------------------------------------

struct ResponseFile {
    QString error;                // empty on success
    SpectralResponse response;    // QE per channel (fraction), if the file has it
    QVector<double> rgbMatrix;    // 3 × B channel → linear sRGB, if the file has only that
    QStringList notes;
    bool ok() const { return error.isEmpty(); }
    bool hasResponse() const { return !response.isNull(); }
};

// Loads the spectral response of the channels:
//  - .mat with a wavelength vector (`lambda`, `wavelength`, `wavelengths`)
//    and a QE matrix (`QE_percent` in percent, or `QE`; nL × B or B × nL);
//    or, without those, a 3 × B colour matrix `reconRGB`;
//  - .csv / .txt (comma, semicolon or tab): a header row with "Band1" …
//    "BandN" columns, matched by name so their order does not matter, and
//    wavelengths in the first column (percent if any value exceeds 1.5).
// Negative values (calibration noise) are clipped to 0, missing ones set to 0.
ResponseFile loadResponse(const QString& path);
bool isResponseCandidate(const QString& path);  // .csv / .txt

// ---------------------------------------------------------------------------
// Spectral correction matrix files
// ---------------------------------------------------------------------------

// Loads a correction matrix supplied with the camera (V × B):
//  - .csv / .txt: one row per corrected band, B numbers per row; with B + 1
//    numbers, the first is the band's centre wavelength (nm). Lines that do
//    not start with a number are skipped.
//  - .mat: the variable `correction`, `correctionMatrix` or `C` (or else the
//    first 2-D numeric array) with V rows; centres from `wavelengths` or
//    `centres` when they have V values.
// `inBands` is the number of measured channels (16); the file must match it.
struct CorrectionFile {
    QString error;
    SpectralCorrection correction;
    bool ok() const { return error.isEmpty(); }
};
CorrectionFile loadCorrectionMatrix(const QString& path, int inBands);

// ---------------------------------------------------------------------------
// Sensor profile
// ---------------------------------------------------------------------------

// Everything that describes how to read one sensor, saved together as JSON
// (.iatsensor): the mosaic layout, bit depth, black level and extraction
// method, the spectral correction settings, exposure times, and the paths
// of the spectral response, correction matrix and reference frames. Paths,
// not contents: the private calibration files stay where they are.
struct SensorProfile {
    QString name;
    MosaicLayout layout = MosaicLayout::defaultLayout();
    int bitDepth = 14;
    double blackLevel = 0.0;
    bool clipNegative = true;
    int method = 0;                 // ExtractMethod
    QString responsePath;
    bool spectralCorrection = false;
    double correctionFwhm = 0.0;    // 0: measured widths
    double correctionRegularisation = 0.01;
    QString correctionMatrixPath;   // empty: computed from the response
    QString darkPath;
    QString whitePath;
    double sceneExposure = 0.0;
    double whiteExposure = 0.0;
    double whiteReflectance = 1.0;
};
bool writeSensorProfile(const SensorProfile& profile, const QString& path, QString* error);
std::optional<SensorProfile> readSensorProfile(const QString& path, QString* error);

// ---------------------------------------------------------------------------
// Spectra
// ---------------------------------------------------------------------------

struct SpectrumSeries {
    QString name;
    QVector<double> mean;
    QVector<double> std;   // may be empty
    qint64 pixels = 0;
};

// CSV with one row per band: band, wavelength (if known), then mean (and
// std) of every series.
bool writeSpectraCsv(const QString& path, const QVector<double>& wavelengths, const QList<SpectrumSeries>& series,
                     QString* error);

} // namespace iat::hsi
