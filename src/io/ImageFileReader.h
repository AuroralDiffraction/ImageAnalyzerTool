#pragma once

#include "core/ImageStack.h"

#include <QString>
#include <QStringList>

namespace iat {

// One image file (TIFF, PNG or JPEG) read as a single X × Y frame.
//
// Values are kept as stored, like MATLAB's imread: an 8-bit PNG gives uint8,
// a 16-bit TIFF uint16, a 32-bit float TIFF single, and so on. Colour images
// whose red, green and blue values are equal everywhere (grey images saved
// as RGB) keep their values. Other colour images are converted to grey with
// MATLAB's rgb2gray weights (0.2989 R + 0.5870 G + 0.1140 B) and become
// double; `note` says so.
struct ImageFrame {
    QString error;        // empty on success
    ImageStack data;      // rows × cols × 1
    QString matlabClass;  // "uint8", "uint16", "single", "double", "logical", ...
    QString format;       // "TIFF", "PNG" or "JPEG"
    QString note;         // conversions or other remarks, e.g. "page 1 of 5"

    bool ok() const { return error.isEmpty(); }
};

// Several image files stacked into one rows × cols × N array, frame k being
// the k-th file in natural file-name order.
struct ImageSequence {
    QString error;        // empty on success
    QStringList paths;    // in frame order
    ImageStack data;
    QString matlabClass;  // common class; "double" if the files have different types
    QString format;       // "TIFF", "PNG", "JPEG" or "image" (mixed)
    QStringList notes;    // remarks, each with the number of files it applies to

    bool ok() const { return error.isEmpty(); }
};

class ImageFileReader {
public:
    static bool isImageFile(const QString& path);  // .tif .tiff .png .jpg .jpeg
    static bool isMatFile(const QString& path);    // .mat
    static bool isSupportedFile(const QString& path) { return isImageFile(path) || isMatFile(path); }
    // "*.mat *.tif *.tiff *.png *.jpg *.jpeg" etc. for file dialogs.
    static QString imageFilePatterns();

    static ImageFrame read(const QString& path);
    // Reads the files, sorted naturally by file name, into one stack. All
    // images must have the same number of rows and columns.
    static ImageSequence readSequence(const QStringList& paths);
};

// Natural file-name order: numbers are compared by value and letters without
// regard to case, so "frame_2.png" comes before "frame_10.png".
bool naturalLess(const QString& a, const QString& b);
// Sorts paths by file name (then by full path) in natural order.
void sortNaturally(QStringList& paths);

// The files from a drop or a file dialog that a panel can load.
struct FileSelection {
    QStringList files;     // the usable files, in the order given
    int ignored = 0;       // files of other types, left out
    QString error;         // why the selection cannot be loaded; empty if it can
    bool isMat = false;    // a single .mat file (otherwise image files)

    bool ok() const { return error.isEmpty(); }
};

// Checks what a panel was given. A panel takes either one .mat file, or one
// or more TIFF / PNG / JPEG images (more than one only if allowMultiple).
// Files of other types are ignored.
FileSelection selectFiles(const QStringList& paths, bool allowMultiple);

// A short name for a set of files, for export file names: the file's base
// name for one file, otherwise the common start of the names without
// trailing digits and separators ("light_001.png", "light_002.png" -> "light").
QString baseNameFor(const QStringList& paths);

} // namespace iat
