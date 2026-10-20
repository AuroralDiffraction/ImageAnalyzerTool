#include "io/MatFileReader.h"

#include <QFile>
#include <QStringList>
#include <QSysInfo>
#include <QtEndian>

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

// Format reference: MathWorks, "MAT-File Format" (matfile_format.pdf),
// Level 5 MAT-files.

namespace iat {

namespace {

enum MiType : quint32 {
    miINT8 = 1, miUINT8 = 2, miINT16 = 3, miUINT16 = 4, miINT32 = 5, miUINT32 = 6,
    miSINGLE = 7, miDOUBLE = 9, miINT64 = 12, miUINT64 = 13,
    miMATRIX = 14, miCOMPRESSED = 15, miUTF8 = 16, miUTF16 = 17, miUTF32 = 18
};

enum MxClass : int {
    mxCELL = 1, mxSTRUCT = 2, mxOBJECT = 3, mxCHAR = 4, mxSPARSE = 5,
    mxDOUBLE = 6, mxSINGLE = 7, mxINT8 = 8, mxUINT8 = 9, mxINT16 = 10, mxUINT16 = 11,
    mxINT32 = 12, mxUINT32 = 13, mxINT64 = 14, mxUINT64 = 15, mxFUNCTION = 16, mxOPAQUE = 17
};

constexpr quint32 kFlagComplex = 0x0800;
constexpr quint32 kFlagLogical = 0x0200;

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Bounds-checked access to a byte buffer in the file's byte order.
class ByteReader {
public:
    ByteReader(const QByteArray& bytes, bool swap) : m_bytes(bytes), m_swap(swap) {}

    qsizetype size() const { return m_bytes.size(); }
    bool swapped() const { return m_swap; }

    void require(qsizetype pos, qsizetype len) const
    {
        if (pos < 0 || len < 0 || pos > size() || len > size() - pos)
            throw ParseError("the file is truncated or corrupt");
    }
    const char* ptr(qsizetype pos, qsizetype len) const
    {
        require(pos, len);
        return m_bytes.constData() + pos;
    }
    quint32 u32(qsizetype pos) const
    {
        quint32 v = loadAs<quint32>(ptr(pos, 4));
        return m_swap ? qbswap(v) : v;
    }
    qint32 i32(qsizetype pos) const { return qint32(u32(pos)); }

private:
    const QByteArray& m_bytes;
    bool m_swap;
};

struct Element {
    quint32 type = 0;
    qsizetype dataPos = 0;
    qsizetype dataSize = 0;
    qsizetype nextPos = 0;  // where the next element's tag starts
};

qsizetype align8(qsizetype v) { return (v + 7) & ~qsizetype(7); }

Element readElement(const ByteReader& r, qsizetype pos)
{
    Element e;
    const quint32 first = r.u32(pos);
    if ((first >> 16) != 0) {
        // Small data element: size and type packed into 4 bytes, data in the next 4.
        e.type = first & 0xFFFF;
        e.dataSize = qsizetype(first >> 16);
        e.dataPos = pos + 4;
        e.nextPos = pos + 8;
        if (e.dataSize > 4)
            throw ParseError("corrupt small data element");
    } else {
        e.type = first;
        e.dataSize = qsizetype(r.u32(pos + 4));
        e.dataPos = pos + 8;
        e.nextPos = e.dataPos + e.dataSize;
        if (e.type != miCOMPRESSED)
            e.nextPos = align8(e.nextPos);
    }
    r.require(e.dataPos, e.dataSize);
    return e;
}

bool storageType(quint32 mi, ElementType* out)
{
    switch (mi) {
    case miINT8:   *out = ElementType::Int8;   return true;
    case miUINT8:  *out = ElementType::UInt8;  return true;
    case miINT16:  *out = ElementType::Int16;  return true;
    case miUINT16: *out = ElementType::UInt16; return true;
    case miINT32:  *out = ElementType::Int32;  return true;
    case miUINT32: *out = ElementType::UInt32; return true;
    case miSINGLE: *out = ElementType::Single; return true;
    case miDOUBLE: *out = ElementType::Double; return true;
    case miINT64:  *out = ElementType::Int64;  return true;
    case miUINT64: *out = ElementType::UInt64; return true;
    default: return false;
    }
}

QString className(int cls, bool logical)
{
    if (logical)
        return QStringLiteral("logical");
    switch (cls) {
    case mxCELL:     return QStringLiteral("cell");
    case mxSTRUCT:   return QStringLiteral("struct");
    case mxOBJECT:   return QStringLiteral("object");
    case mxCHAR:     return QStringLiteral("char");
    case mxSPARSE:   return QStringLiteral("sparse");
    case mxDOUBLE:   return QStringLiteral("double");
    case mxSINGLE:   return QStringLiteral("single");
    case mxINT8:     return QStringLiteral("int8");
    case mxUINT8:    return QStringLiteral("uint8");
    case mxINT16:    return QStringLiteral("int16");
    case mxUINT16:   return QStringLiteral("uint16");
    case mxINT32:    return QStringLiteral("int32");
    case mxUINT32:   return QStringLiteral("uint32");
    case mxINT64:    return QStringLiteral("int64");
    case mxUINT64:   return QStringLiteral("uint64");
    case mxFUNCTION: return QStringLiteral("function_handle");
    case mxOPAQUE:   return QStringLiteral("object");
    default:         return QStringLiteral("unknown");
    }
}

bool isNumericClass(int cls) { return cls >= mxDOUBLE && cls <= mxUINT64; }

QByteArray swapCopy(const char* src, qsizetype bytes, std::size_t elemSize)
{
    QByteArray out(src, bytes);
    if (elemSize <= 1)
        return out;
    char* p = out.data();
    for (qsizetype i = 0; i + qsizetype(elemSize) <= bytes; i += qsizetype(elemSize))
        std::reverse(p + i, p + i + qsizetype(elemSize));
    return out;
}

// Inflates the zlib stream of a miCOMPRESSED element.
QByteArray inflate(const char* data, qsizetype size)
{
    // qUncompress expects a 4-byte big-endian length prefix. Qt treats it as
    // a starting buffer size only and grows the buffer if it is too small.
    const qint64 guess = std::min<qint64>(qint64(size) * 3, 512LL * 1024 * 1024);
    const quint32 hint = quint32(std::max<qint64>(guess, 64));
    QByteArray framed;
    framed.reserve(size + 4);
    char prefix[4];
    qToBigEndian(hint, prefix);
    framed.append(prefix, 4);
    framed.append(data, size);
    QByteArray out = qUncompress(framed);
    if (out.isEmpty())
        throw ParseError("a compressed variable could not be decompressed (corrupt file or not enough memory)");
    return out;
}

// Parses the contents of a miMATRIX element spanning [pos, end).
MatVariable parseMatrix(const ByteReader& r, qsizetype pos, qsizetype end)
{
    MatVariable v;
    if (end - pos < 16) {
        v.note = QStringLiteral("empty array");
        return v;
    }

    // 1. Array flags
    const Element flags = readElement(r, pos);
    if (flags.type != miUINT32 || flags.dataSize != 8)
        throw ParseError("unexpected array flags");
    const quint32 f0 = r.u32(flags.dataPos);
    const int cls = int(f0 & 0xFF);
    const bool complex = (f0 & kFlagComplex) != 0;
    const bool logical = (f0 & kFlagLogical) != 0;
    v.matlabClass = className(cls, logical);

    if (cls == mxOPAQUE || cls == mxFUNCTION) {
        // Objects (string, table, ...): flags are followed directly by the name.
        const Element name = readElement(r, flags.nextPos);
        if (name.type == miINT8 || name.type == miUTF8)
            v.name = QString::fromUtf8(r.ptr(name.dataPos, name.dataSize), name.dataSize);
        v.note = QStringLiteral("MATLAB objects are not supported");
        return v;
    }

    // 2. Dimensions
    const Element dims = readElement(r, flags.nextPos);
    if (dims.type != miINT32 || dims.dataSize % 4 != 0 || dims.dataSize < 8)
        throw ParseError("unexpected dimensions element");
    for (qsizetype i = 0; i < dims.dataSize / 4; ++i) {
        const qint32 d = r.i32(dims.dataPos + 4 * i);
        if (d < 0)
            throw ParseError("negative array dimension");
        v.dims.append(d);
    }

    // 3. Name
    const Element name = readElement(r, dims.nextPos);
    if (name.type != miINT8 && name.type != miUTF8)
        throw ParseError("unexpected array name element");
    v.name = QString::fromUtf8(r.ptr(name.dataPos, name.dataSize), name.dataSize);

    if (!isNumericClass(cls)) {
        v.note = QStringLiteral("%1 arrays are not supported").arg(v.matlabClass);
        return v;
    }
    if (complex) {
        v.note = QStringLiteral("complex arrays are not supported");
        return v;
    }

    qint64 count = 1;
    for (qint64 d : std::as_const(v.dims)) {
        if (d != 0 && count > std::numeric_limits<qint64>::max() / d)
            throw ParseError("array is too large");
        count *= d;
    }
    if (count == 0) {
        v.note = QStringLiteral("empty array");
        return v;
    }

    // MATLAB never stores trailing singleton dimensions beyond the second,
    // but be tolerant: [rows, cols, frames, 1, 1] is still a stack.
    qint64 frames = 1;
    for (int i = 2; i < v.dims.size(); ++i) {
        if (i >= 3 && v.dims[i] != 1) {
            v.note = QStringLiteral("arrays with more than 3 dimensions are not supported");
            return v;
        }
        frames *= v.dims[i];
    }
    constexpr qint64 kMaxInt = std::numeric_limits<int>::max();
    if (v.dims[0] > kMaxInt || v.dims[1] > kMaxInt || frames > kMaxInt) {
        v.note = QStringLiteral("array is too large");
        return v;
    }

    // 4. Real part
    const Element real = readElement(r, name.nextPos);
    ElementType type;
    if (!storageType(real.type, &type))
        throw ParseError("unsupported storage type for numeric data");
    const qint64 bytes = count * qint64(elementSize(type));
    if (real.dataSize != bytes)
        throw ParseError("numeric data size does not match the array dimensions");

    const char* src = r.ptr(real.dataPos, real.dataSize);
    QByteArray raw = r.swapped() ? swapCopy(src, real.dataSize, elementSize(type))
                                 : QByteArray(src, real.dataSize);

    v.data = ImageStack(int(v.dims[0]), int(v.dims[1]), int(frames), type, std::move(raw));
    v.supported = !v.data.isNull();
    if (!v.supported)
        v.note = QStringLiteral("could not create the image stack");
    return v;
}

MatFileContents parse(const QByteArray& bytes)
{
    MatFileContents out;
    if (bytes.size() < 128) {
        out.error = QStringLiteral("The file is too small to be a MAT-file.");
        return out;
    }

    out.headerText = QString::fromLatin1(bytes.constData(), 116).trimmed();
    const bool hdf5Signature = bytes.size() >= 520 && bytes.mid(512, 8) == QByteArray("\x89HDF\r\n\x1a\n", 8);
    if (out.headerText.startsWith(QLatin1String("MATLAB 7.3")) || hdf5Signature) {
        out.error = QStringLiteral(
            "This is a MATLAB v7.3 (HDF5) MAT-file, which this version cannot read yet. "
            "Re-save it in MATLAB with:  save('file.mat', 'variableName', '-v7')");
        return out;
    }
    if (!out.headerText.startsWith(QLatin1String("MATLAB"))) {
        out.error = QStringLiteral("This is not a MATLAB MAT-file (version 5 or 7 format).");
        return out;
    }

    const char e0 = bytes.at(126);
    const char e1 = bytes.at(127);
    const bool hostLittle = QSysInfo::ByteOrder == QSysInfo::LittleEndian;
    bool fileLittle;
    if (e0 == 'I' && e1 == 'M')
        fileLittle = true;
    else if (e0 == 'M' && e1 == 'I')
        fileLittle = false;
    else {
        out.error = QStringLiteral("The MAT-file header has an invalid byte-order marker.");
        return out;
    }
    const bool swap = fileLittle != hostLittle;

    try {
        ByteReader r(bytes, swap);
        qsizetype pos = 128;
        while (pos + 8 <= r.size()) {
            const Element e = readElement(r, pos);
            if (e.type == miCOMPRESSED) {
                const QByteArray inflated = inflate(r.ptr(e.dataPos, e.dataSize), e.dataSize);
                ByteReader ir(inflated, swap);
                const Element inner = readElement(ir, 0);
                if (inner.type == miMATRIX)
                    out.variables.append(parseMatrix(ir, inner.dataPos, inner.dataPos + inner.dataSize));
            } else if (e.type == miMATRIX) {
                out.variables.append(parseMatrix(r, e.dataPos, e.dataPos + e.dataSize));
            }
            pos = e.nextPos;
        }
    } catch (const ParseError& err) {
        out.error = QStringLiteral("Could not read the MAT-file: %1.").arg(QString::fromUtf8(err.what()));
        out.variables.clear();
        return out;
    } catch (const std::bad_alloc&) {
        out.error = QStringLiteral("Not enough memory to read this MAT-file.");
        out.variables.clear();
        return out;
    }

    // MATLAB stores helper data for objects as a nameless variable; hide it.
    out.variables.erase(std::remove_if(out.variables.begin(), out.variables.end(),
                                       [](const MatVariable& v) { return v.name.isEmpty(); }),
                        out.variables.end());
    return out;
}

} // namespace

QString MatVariable::sizeString() const
{
    QStringList parts;
    for (qint64 d : dims)
        parts << QString::number(d);
    return parts.join(QChar(0x00D7));
}

bool MatVariable::isImageStack() const
{
    return supported && !data.isNull() && data.rows() > 1 && data.cols() > 1;
}

QList<MatVariable> MatFileContents::imageStacks() const
{
    QList<MatVariable> result;
    for (const MatVariable& v : variables)
        if (v.isImageStack())
            result.append(v);
    return result;
}

MatFileContents MatFileReader::read(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        MatFileContents out;
        out.error = QStringLiteral("Could not open the file: %1").arg(file.errorString());
        return out;
    }
    QByteArray bytes;
    try {
        bytes = file.readAll();
    } catch (const std::bad_alloc&) {
        MatFileContents out;
        out.error = QStringLiteral("Not enough memory to read this file.");
        return out;
    }
    return readFromBytes(bytes);
}

MatFileContents MatFileReader::readFromBytes(const QByteArray& bytes)
{
    return parse(bytes);
}

} // namespace iat
