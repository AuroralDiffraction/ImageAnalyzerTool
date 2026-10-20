#pragma once

#include "core/Image2D.h"
#include "core/ImageStack.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVector>

#include <functional>

namespace iat {

// Writes MATLAB Level 5 MAT-files: the default -v7 format, which MATLAB,
// Octave, SciPy and MatFileReader all read. Each variable is stored
// zlib-compressed, as MATLAB does, unless the compression level is 0.
//
// Like MATLAB's -v7 format, a single variable can be at most 2 GB.
//
//   MatFileWriter w;
//   w.addImage("dark_mean", image);
//   w.addScalar("img_noise", 9.53);
//   if (!w.save(path)) qWarning() << w.errorString();
class MatFileWriter {
public:
    explicit MatFileWriter(int compressionLevel = 1);  // 0 = none, 1 = fastest … 9 = smallest

    // Image as a rows × cols double array (row y of the image is MATLAB row y+1).
    bool addImage(const QString& name, const Image2D& image);

    // Stack as a rows × cols × frames array, values stored in their original
    // type. `matlabClass` ("double", "uint16", "logical", …) is the class
    // MATLAB gives the variable when it loads it.
    bool addStack(const QString& name, const ImageStack& stack, const QString& matlabClass = QStringLiteral("double"));

    // rows × cols × frames double array. `fillFrame(z, plane)` must write
    // frame z (0-based) into `plane`, rows × cols values in column-major
    // (MATLAB) order.
    bool addDoubleStack(const QString& name, int rows, int cols, int frames,
                        const std::function<void(int, double*)>& fillFrame);

    bool addScalar(const QString& name, double value);
    bool addString(const QString& name, const QString& text);

    // Writes all added variables. The file is replaced only when everything
    // was written successfully.
    bool save(const QString& path);

    QString errorString() const { return m_error; }

private:
    bool addMatrix(const QString& name, int mxClass, quint32 extraFlags, const QVector<qint64>& dims,
                   quint32 miType, qint64 dataBytes, const std::function<void(char*)>& fill);
    bool fail(const QString& message);

    int m_level;
    QList<QByteArray> m_elements;
    QString m_error;
};

} // namespace iat
