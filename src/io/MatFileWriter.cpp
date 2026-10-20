#include "io/MatFileWriter.h"

#include <QDateTime>
#include <QLocale>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSysInfo>

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <utility>
#include <vector>

// Format reference: MathWorks, "MAT-File Format", Level 5 MAT-files.

namespace iat {

namespace {

constexpr quint32 miINT8 = 1, miUINT8 = 2, miINT16 = 3, miUINT16 = 4, miINT32 = 5, miUINT32 = 6,
                  miSINGLE = 7, miDOUBLE = 9, miINT64 = 12, miUINT64 = 13, miMATRIX = 14,
                  miCOMPRESSED = 15;

constexpr int mxCHAR = 4, mxDOUBLE = 6, mxSINGLE = 7, mxINT8 = 8, mxUINT8 = 9, mxINT16 = 10,
              mxUINT16 = 11, mxINT32 = 12, mxUINT32 = 13, mxINT64 = 14, mxUINT64 = 15;

constexpr quint32 kFlagLogical = 0x0200;
constexpr qint64 kMaxVariableBytes = std::numeric_limits<qint32>::max();  // 2 GB, as MATLAB -v7

qint64 padded8(qint64 n) { return (n + 7) & ~qint64(7); }

// Bytes taken by a data element holding `bytes` bytes of data.
qint64 elementBytes(qint64 bytes) { return bytes <= 4 ? 8 : 8 + padded8(bytes); }

void put32(char*& p, quint32 v)
{
    std::memcpy(p, &v, 4);
    p += 4;
}

// Writes a data element (tag + data + padding); small data elements (<= 4
// bytes) use the compact format, as MATLAB does.
void putElement(char*& p, quint32 type, qint64 bytes, const std::function<void(char*)>& fill)
{
    if (bytes <= 4) {
        put32(p, (quint32(bytes) << 16) | type);
        std::memset(p, 0, 4);
        if (bytes > 0)
            fill(p);
        p += 4;
        return;
    }
    put32(p, type);
    put32(p, quint32(bytes));
    fill(p);
    p += bytes;
    const qint64 pad = padded8(bytes) - bytes;
    std::memset(p, 0, std::size_t(pad));
    p += pad;
}

quint32 miTypeFor(ElementType t)
{
    switch (t) {
    case ElementType::Int8:   return miINT8;
    case ElementType::UInt8:  return miUINT8;
    case ElementType::Int16:  return miINT16;
    case ElementType::UInt16: return miUINT16;
    case ElementType::Int32:  return miINT32;
    case ElementType::UInt32: return miUINT32;
    case ElementType::Int64:  return miINT64;
    case ElementType::UInt64: return miUINT64;
    case ElementType::Single: return miSINGLE;
    case ElementType::Double: return miDOUBLE;
    }
    return miDOUBLE;
}

int mxClassFor(const QString& cls)
{
    if (cls == QLatin1String("single")) return mxSINGLE;
    if (cls == QLatin1String("int8"))   return mxINT8;
    if (cls == QLatin1String("uint8") || cls == QLatin1String("logical")) return mxUINT8;
    if (cls == QLatin1String("int16"))  return mxINT16;
    if (cls == QLatin1String("uint16")) return mxUINT16;
    if (cls == QLatin1String("int32"))  return mxINT32;
    if (cls == QLatin1String("uint32")) return mxUINT32;
    if (cls == QLatin1String("int64"))  return mxINT64;
    if (cls == QLatin1String("uint64")) return mxUINT64;
    return mxDOUBLE;
}

bool isValidName(const QString& name)
{
    static const QRegularExpression re(QStringLiteral("^[A-Za-z][A-Za-z0-9_]{0,62}$"));
    return re.match(name).hasMatch();
}

QString platformName()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("PCWIN64");
#elif defined(Q_OS_MACOS)
    return QSysInfo::currentCpuArchitecture() == QLatin1String("arm64") ? QStringLiteral("MACA64")
                                                                         : QStringLiteral("MACI64");
#else
    return QStringLiteral("GLNXA64");
#endif
}

QByteArray header()
{
    QByteArray h(128, ' ');
    const QString created = QLocale::c().toString(QDateTime::currentDateTime(), QStringLiteral("ddd MMM dd HH:mm:ss yyyy"));
    const QByteArray text = QStringLiteral("MATLAB 5.0 MAT-file, Platform: %1, Created on: %2, by ImageAnalyzerTool")
                                .arg(platformName(), created)
                                .toLatin1()
                                .left(116);
    std::memcpy(h.data(), text.constData(), std::size_t(text.size()));
    std::memset(h.data() + 116, 0, 8);  // no subsystem data
    const quint16 version = 0x0100;
    const quint16 endian = quint16(('M' << 8) | 'I');  // reads back as "IM" on little-endian machines
    std::memcpy(h.data() + 124, &version, 2);
    std::memcpy(h.data() + 126, &endian, 2);
    return h;
}

} // namespace

MatFileWriter::MatFileWriter(int compressionLevel)
    : m_level(std::clamp(compressionLevel, 0, 9))
{
}

bool MatFileWriter::fail(const QString& message)
{
    m_error = message;
    return false;
}

bool MatFileWriter::addMatrix(const QString& name, int mxClass, quint32 extraFlags, const QVector<qint64>& dims,
                              quint32 miType, qint64 dataBytes, const std::function<void(char*)>& fill)
{
    if (!isValidName(name))
        return fail(QStringLiteral("'%1' is not a valid MATLAB variable name.").arg(name));
    for (qint64 d : dims)
        if (d < 0 || d > std::numeric_limits<qint32>::max())
            return fail(QStringLiteral("Variable '%1' has an invalid size.").arg(name));

    const QByteArray latinName = name.toLatin1();
    const qint64 content = 16                                 // array flags
                           + elementBytes(4 * dims.size())    // dimensions
                           + elementBytes(latinName.size())   // name
                           + elementBytes(dataBytes);         // real part
    if (content > kMaxVariableBytes)
        return fail(QStringLiteral("Variable '%1' is larger than 2 GB, the limit of MATLAB -v7 files. "
                                   "Save fewer frames at a time.").arg(name));

    QByteArray matrix;
    try {
        matrix = QByteArray(qsizetype(8 + content), Qt::Uninitialized);
    } catch (const std::bad_alloc&) {
        return fail(QStringLiteral("Not enough memory to save '%1'.").arg(name));
    }

    char* p = matrix.data();
    put32(p, miMATRIX);
    put32(p, quint32(content));
    const quint32 flags[2] = {quint32(mxClass) | extraFlags, 0};
    putElement(p, miUINT32, 8, [&](char* d) { std::memcpy(d, flags, 8); });
    putElement(p, miINT32, 4 * dims.size(), [&](char* d) {
        for (qint64 v : dims) {
            const qint32 i = qint32(v);
            std::memcpy(d, &i, 4);
            d += 4;
        }
    });
    putElement(p, miINT8, latinName.size(), [&](char* d) { std::memcpy(d, latinName.constData(), std::size_t(latinName.size())); });
    putElement(p, miType, dataBytes, fill);
    Q_ASSERT(p == matrix.data() + matrix.size());

    if (m_level == 0) {
        m_elements.append(matrix);
        return true;
    }

    // qCompress output is a 4-byte length followed by a zlib stream, which is
    // exactly what a miCOMPRESSED element holds once the length is removed.
    QByteArray z = qCompress(reinterpret_cast<const uchar*>(matrix.constData()), matrix.size(), m_level);
    matrix = QByteArray();
    if (z.size() <= 4)
        return fail(QStringLiteral("Could not compress '%1' (not enough memory?).").arg(name));
    z.remove(0, 4);
    if (qint64(z.size()) > std::numeric_limits<quint32>::max())
        return fail(QStringLiteral("Variable '%1' is too large.").arg(name));

    QByteArray element(8, Qt::Uninitialized);
    char* t = element.data();
    put32(t, miCOMPRESSED);
    put32(t, quint32(z.size()));
    element.append(z);
    m_elements.append(element);
    return true;
}

bool MatFileWriter::addImage(const QString& name, const Image2D& image)
{
    const int rows = image.height();
    const int cols = image.width();
    return addMatrix(name, mxDOUBLE, 0, {rows, cols}, miDOUBLE, qint64(rows) * cols * 8, [&](char* d) {
        for (int c = 0; c < cols; ++c)
            for (int r = 0; r < rows; ++r) {
                const double v = image.at(c, r);
                std::memcpy(d, &v, 8);
                d += 8;
            }
    });
}

bool MatFileWriter::addStack(const QString& name, const ImageStack& stack, const QString& matlabClass)
{
    if (stack.isNull())
        return fail(QStringLiteral("Variable '%1' is empty.").arg(name));
    const QByteArray& raw = stack.rawData();
    const bool logical = matlabClass == QLatin1String("logical") && stack.elementType() == ElementType::UInt8;
    QVector<qint64> dims{stack.rows(), stack.cols()};
    if (stack.frameCount() > 1)
        dims.append(stack.frameCount());
    return addMatrix(name, mxClassFor(matlabClass), logical ? kFlagLogical : 0, dims,
                     miTypeFor(stack.elementType()), raw.size(),
                     [&](char* d) { std::memcpy(d, raw.constData(), std::size_t(raw.size())); });
}

bool MatFileWriter::addDoubleStack(const QString& name, int rows, int cols, int frames,
                                   const std::function<void(int, double*)>& fillFrame)
{
    if (rows <= 0 || cols <= 0 || frames <= 0)
        return fail(QStringLiteral("Variable '%1' is empty.").arg(name));
    const qint64 plane = qint64(rows) * cols;
    QVector<qint64> dims{rows, cols};
    if (frames > 1)
        dims.append(frames);
    std::vector<double> buffer;
    try {
        buffer.resize(std::size_t(plane));
    } catch (const std::bad_alloc&) {
        return fail(QStringLiteral("Not enough memory to save '%1'.").arg(name));
    }
    return addMatrix(name, mxDOUBLE, 0, dims, miDOUBLE, plane * frames * 8, [&](char* d) {
        for (int z = 0; z < frames; ++z) {
            fillFrame(z, buffer.data());
            std::memcpy(d, buffer.data(), std::size_t(plane * 8));
            d += plane * 8;
        }
    });
}

bool MatFileWriter::addScalar(const QString& name, double value)
{
    return addMatrix(name, mxDOUBLE, 0, {1, 1}, miDOUBLE, 8, [&](char* d) { std::memcpy(d, &value, 8); });
}

bool MatFileWriter::addString(const QString& name, const QString& text)
{
    // char arrays hold UTF-16 code units.
    const qint64 n = text.size();
    const QVector<qint64> dims = n == 0 ? QVector<qint64>{0, 0} : QVector<qint64>{1, n};
    return addMatrix(name, mxCHAR, 0, dims, miUINT16, n * 2, [&](char* d) {
        std::memcpy(d, text.utf16(), std::size_t(n * 2));
    });
}

bool MatFileWriter::save(const QString& path)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return fail(QStringLiteral("Could not create %1: %2").arg(path, file.errorString()));
    bool ok = file.write(header()) == 128;
    for (const QByteArray& e : std::as_const(m_elements))
        ok = ok && file.write(e) == e.size();
    if (!ok) {
        file.cancelWriting();
        return fail(QStringLiteral("Could not write %1: %2").arg(path, file.errorString()));
    }
    if (!file.commit())
        return fail(QStringLiteral("Could not save %1: %2").arg(path, file.errorString()));
    return true;
}

} // namespace iat
