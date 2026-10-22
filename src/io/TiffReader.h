#pragma once

#include <QByteArray>
#include <QString>

namespace iat {

// Decoded first page (image) of a TIFF file.
struct TiffImage {
    QString error;            // empty on success
    int width = 0;            // columns
    int height = 0;           // rows
    int samplesPerPixel = 1;  // 1 = grey, 3 = RGB, 4 = RGBA, ...
    int bitsPerSample = 0;    // 1, 8, 16, 32 or 64
    int sampleFormat = 1;     // 1 = unsigned integer, 2 = signed integer, 3 = floating point
    int photometric = 1;      // 0 = WhiteIsZero, 1 = BlackIsZero, 2 = RGB
    int pageCount = 1;        // number of images (pages) in the file
    // Row-major, samples interleaved (RGBRGB...), host byte order. For 1-bit
    // images every pixel is one byte holding 0 or 1.
    QByteArray pixels;

    bool ok() const { return error.isEmpty(); }
};

// Reader for baseline TIFF and BigTIFF files, as written by cameras, ImageJ /
// Fiji, MATLAB's imwrite, LabVIEW, libtiff, tifffile and most other software.
//
// Supported: little- and big-endian files; strips and tiles; chunky and
// planar sample layout; no compression, PackBits, LZW and Deflate (zlib),
// with or without the horizontal (2) or floating-point (3) predictor;
// 1-bit, 8-, 16-, 32- and 64-bit integers (unsigned or signed) and 32- and
// 64-bit floating point samples; grey and RGB(A) images.
//
// Not supported: JPEG-compressed and other compressed variants, palette
// (indexed colour) images, CMYK / YCbCr / Lab, and packed bit depths such as
// 12-bit. These are reported with an error that says what the file holds.
//
// Only the first page of a multi-page file is decoded; pageCount says how
// many there are.
class TiffReader {
public:
    static TiffImage read(const QString& path);
    static TiffImage readFromBytes(const QByteArray& bytes);
};

} // namespace iat
