#pragma once

#include "core/ImageStack.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVector>

namespace iat {

struct MatVariable {
    QString name;
    QString matlabClass;     // "double", "uint16", "logical", "struct", ...
    QVector<qint64> dims;    // MATLAB size(), e.g. {1028, 1408, 20}
    bool supported = false;  // true if `data` holds the array
    QString note;            // why the variable could not be loaded, if it could not
    ImageStack data;         // [rows, cols, frames]

    // "1028×1408×20"
    QString sizeString() const;
    // A loaded array with at least 2 rows and 2 columns, i.e. something that
    // can be shown as one or more images.
    bool isImageStack() const;
};

struct MatFileContents {
    QString error;           // empty on success
    QString headerText;      // "MATLAB 5.0 MAT-file, Platform: ..."
    QList<MatVariable> variables;

    bool ok() const { return error.isEmpty(); }
    QList<MatVariable> imageStacks() const;
};

// Reader for MATLAB Level 5 MAT-files, i.e. files saved with -v6 or -v7
// (MATLAB's default), including compressed variables.
//
// Supports real, dense numeric and logical arrays with up to three
// (non-singleton) dimensions. Other variables (structs, cells, strings,
// sparse, complex, objects) are listed but not loaded.
//
// Files saved with -v7.3 are HDF5 files and are reported as unsupported.
class MatFileReader {
public:
    static MatFileContents read(const QString& path);
    static MatFileContents readFromBytes(const QByteArray& bytes);
};

} // namespace iat
