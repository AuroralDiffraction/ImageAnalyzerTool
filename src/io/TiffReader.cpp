#include "io/TiffReader.h"

#include "core/ElementType.h"

#include <QFile>
#include <QMap>
#include <QSet>
#include <QSysInfo>
#include <QtEndian>

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>

// Format references: Adobe, "TIFF Revision 6.0" (1992); Adobe, "Photoshop TIFF
// Technical Note 3" (floating-point predictor); "BigTIFF" (libtiff.org).

namespace iat {

namespace {

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

enum Tag : quint16 {
    kImageWidth = 256,
    kImageLength = 257,
    kBitsPerSample = 258,
    kCompression = 259,
    kPhotometric = 262,
    kFillOrder = 266,
    kStripOffsets = 273,
    kSamplesPerPixel = 277,
    kRowsPerStrip = 278,
    kStripByteCounts = 279,
    kPlanarConfig = 284,
    kPredictor = 317,
    kTileWidth = 322,
    kTileLength = 323,
    kTileOffsets = 324,
    kTileByteCounts = 325,
    kSampleFormat = 339,
};

enum Compression : quint64 {
    kNone = 1,
    kLzw = 5,
    kJpegOld = 6,
    kJpeg = 7,
    kDeflate = 8,
    kPackBits = 32773,
    kAdobeDeflate = 32946,
};

QString compressionName(quint64 c)
{
    switch (c) {
    case 2: return QStringLiteral("CCITT Group 3 (modified Huffman)");
    case 3: return QStringLiteral("CCITT Group 3 fax");
    case 4: return QStringLiteral("CCITT Group 4 fax");
    case kJpegOld:
    case kJpeg: return QStringLiteral("JPEG");
    case 34712: return QStringLiteral("JPEG 2000");
    case 34887: return QStringLiteral("LERC");
    case 34925: return QStringLiteral("LZMA");
    case 50000: return QStringLiteral("Zstandard");
    case 50001: return QStringLiteral("WebP");
    case 50002: return QStringLiteral("JPEG XL");
    }
    return QStringLiteral("type %1").arg(c);
}

// Bounds-checked reads in the file's byte order.
class Bytes {
public:
    Bytes(const QByteArray& data, bool bigEndian) : m_data(data), m_big(bigEndian) {}

    qint64 size() const { return m_data.size(); }
    bool bigEndian() const { return m_big; }

    void require(qint64 pos, qint64 len) const
    {
        if (pos < 0 || len < 0 || pos > size() || len > size() - pos)
            throw ParseError("the file is truncated or corrupt");
    }
    const uchar* ptr(qint64 pos, qint64 len) const
    {
        require(pos, len);
        return reinterpret_cast<const uchar*>(m_data.constData()) + pos;
    }
    quint16 u16(qint64 pos) const
    {
        const uchar* p = ptr(pos, 2);
        return m_big ? qFromBigEndian<quint16>(p) : qFromLittleEndian<quint16>(p);
    }
    quint32 u32(qint64 pos) const
    {
        const uchar* p = ptr(pos, 4);
        return m_big ? qFromBigEndian<quint32>(p) : qFromLittleEndian<quint32>(p);
    }
    quint64 u64(qint64 pos) const
    {
        const uchar* p = ptr(pos, 8);
        return m_big ? qFromBigEndian<quint64>(p) : qFromLittleEndian<quint64>(p);
    }
    // An offset stored in 4 (classic TIFF) or 8 (BigTIFF) bytes.
    qint64 offset(qint64 pos, bool bigTiff) const
    {
        const quint64 v = bigTiff ? u64(pos) : u32(pos);
        if (v > quint64(std::numeric_limits<qint64>::max()))
            throw ParseError("the file is corrupt");
        return qint64(v);
    }

private:
    const QByteArray& m_data;
    bool m_big;
};

int typeSize(quint16 type)
{
    switch (type) {
    case 1: case 2: case 6: case 7: return 1;                        // BYTE ASCII SBYTE UNDEFINED
    case 3: case 8: return 2;                                         // SHORT SSHORT
    case 4: case 9: case 11: case 13: return 4;                       // LONG SLONG FLOAT IFD
    case 5: case 10: case 12: case 16: case 17: case 18: return 8;    // RATIONAL SRATIONAL DOUBLE LONG8 SLONG8 IFD8
    }
    return 0;
}

struct Entry {
    quint16 type = 0;
    quint64 count = 0;
    qint64 valuePos = 0;  // file position of the first value
};

struct Ifd {
    QMap<quint16, Entry> entries;
    qint64 next = 0;  // offset of the next IFD, 0 if none

    bool has(quint16 tag) const { return entries.contains(tag); }

    // All values of an integer-valued tag.
    std::vector<quint64> values(const Bytes& b, quint16 tag) const
    {
        const auto it = entries.constFind(tag);
        if (it == entries.constEnd())
            return {};
        const Entry& e = *it;
        const int size = typeSize(e.type);
        if (size == 0 || e.count > quint64(b.size()))
            throw ParseError("the file is corrupt");
        b.require(e.valuePos, qint64(e.count) * size);
        std::vector<quint64> out;
        out.reserve(std::size_t(e.count));
        for (quint64 i = 0; i < e.count; ++i) {
            const qint64 p = e.valuePos + qint64(i) * size;
            switch (e.type) {
            case 1: case 7: out.push_back(*b.ptr(p, 1)); break;
            case 3: out.push_back(b.u16(p)); break;
            case 4: case 13: out.push_back(b.u32(p)); break;
            case 16: case 18: out.push_back(b.u64(p)); break;
            case 6: out.push_back(quint64(std::max<qint64>(0, qint8(*b.ptr(p, 1))))); break;
            case 8: out.push_back(quint64(std::max<qint64>(0, qint16(b.u16(p))))); break;
            case 9: out.push_back(quint64(std::max<qint64>(0, qint32(b.u32(p))))); break;
            case 17: out.push_back(quint64(std::max<qint64>(0, qint64(b.u64(p))))); break;
            default:
                throw ParseError("the file is corrupt (unexpected field type)");
            }
        }
        return out;
    }

    quint64 value(const Bytes& b, quint16 tag, quint64 fallback) const
    {
        const std::vector<quint64> v = values(b, tag);
        return v.empty() ? fallback : v.front();
    }
};

Ifd readIfd(const Bytes& b, qint64 pos, bool bigTiff)
{
    Ifd ifd;
    const qint64 count = bigTiff ? qint64(std::min<quint64>(b.u64(pos), quint64(b.size()))) : b.u16(pos);
    const qint64 entrySize = bigTiff ? 20 : 12;
    const qint64 first = pos + (bigTiff ? 8 : 2);
    b.require(first, count * entrySize);
    const int inlineBytes = bigTiff ? 8 : 4;
    for (qint64 i = 0; i < count; ++i) {
        const qint64 p = first + i * entrySize;
        Entry e;
        const quint16 tag = b.u16(p);
        e.type = b.u16(p + 2);
        e.count = bigTiff ? b.u64(p + 4) : b.u32(p + 4);
        const qint64 field = p + (bigTiff ? 12 : 8);
        const int size = typeSize(e.type);
        if (size == 0)
            continue;  // unknown field type: ignore the entry, as TIFF readers should
        if (e.count <= quint64(inlineBytes / size))
            e.valuePos = field;
        else
            e.valuePos = b.offset(field, bigTiff);
        ifd.entries.insert(tag, e);
    }
    ifd.next = b.offset(first + count * entrySize, bigTiff);
    return ifd;
}

qint64 nextIfd(const Bytes& b, qint64 pos, bool bigTiff)
{
    const qint64 count = bigTiff ? qint64(std::min<quint64>(b.u64(pos), quint64(b.size()))) : b.u16(pos);
    return b.offset(pos + (bigTiff ? 8 + count * 20 : 2 + count * 12), bigTiff);
}

// ---------------------------------------------------------------------------
// Decompression
// ---------------------------------------------------------------------------

QByteArray packBitsDecode(const uchar* in, qint64 n, qint64 expected)
{
    QByteArray out;
    out.reserve(expected);
    qint64 i = 0;
    while (i < n && out.size() < expected) {
        const int c = qint8(in[i++]);
        if (c >= 0) {
            const qint64 len = c + 1;
            if (len > n - i)
                throw ParseError("corrupt PackBits data");
            out.append(reinterpret_cast<const char*>(in + i), len);
            i += len;
        } else if (c != -128) {
            if (i >= n)
                throw ParseError("corrupt PackBits data");
            out.append(qsizetype(1 - c), char(in[i++]));
        }
    }
    return out;
}

// TIFF LZW: codes of 9 to 12 bits, most significant bit first, with the
// code width growing one code early ("early change"), Clear = 256, EOI = 257.
QByteArray lzwDecode(const uchar* in, qint64 n, qint64 expected)
{
    if (n >= 2 && in[0] == 0 && (in[1] & 0x1))
        throw ParseError("old-style LZW compression (from before TIFF 6.0) is not supported");

    constexpr int kClear = 256;
    constexpr int kEoi = 257;
    constexpr quint16 kRoot = 0xFFFF;
    std::vector<quint16> prefix(4096, kRoot);
    std::vector<quint16> length(4096, 1);
    std::vector<uchar> suffix(4096), first(4096);
    for (int i = 0; i < 256; ++i)
        suffix[std::size_t(i)] = first[std::size_t(i)] = uchar(i);

    QByteArray out;
    out.reserve(expected);
    auto emitCode = [&](int code) {
        const qsizetype start = out.size();
        const int len = length[std::size_t(code)];
        out.resize(start + len);
        char* p = out.data() + start + len;
        for (int c = code;; c = prefix[std::size_t(c)]) {
            *--p = char(suffix[std::size_t(c)]);
            if (prefix[std::size_t(c)] == kRoot)
                break;
        }
    };
    quint64 bits = 0;
    int bitCount = 0;
    qint64 pos = 0;
    int width = 9;
    int next = 258;
    int old = -1;
    while (out.size() < expected) {
        while (bitCount < width && pos < n) {
            bits = (bits << 8) | in[pos++];
            bitCount += 8;
        }
        if (bitCount < width)
            break;  // end of the data without an EOI code
        const int code = int((bits >> (bitCount - width)) & ((1u << width) - 1));
        bitCount -= width;

        if (code == kEoi)
            break;
        if (code == kClear) {
            width = 9;
            next = 258;
            old = -1;
            continue;
        }
        if (old < 0) {
            if (code > 255)
                throw ParseError("corrupt LZW data");
            emitCode(code);
            old = code;
            continue;
        }
        if (code < next) {
            emitCode(code);
            if (next < 4096) {
                prefix[std::size_t(next)] = quint16(old);
                suffix[std::size_t(next)] = first[std::size_t(code)];
                first[std::size_t(next)] = first[std::size_t(old)];
                length[std::size_t(next)] = quint16(length[std::size_t(old)] + 1);
                ++next;
            }
        } else if (code == next && next < 4096) {
            prefix[std::size_t(next)] = quint16(old);
            suffix[std::size_t(next)] = first[std::size_t(old)];
            first[std::size_t(next)] = first[std::size_t(old)];
            length[std::size_t(next)] = quint16(length[std::size_t(old)] + 1);
            ++next;
            emitCode(code);
        } else {
            throw ParseError("corrupt LZW data");
        }
        old = code;
        if (next >= (1 << width) - 1 && width < 12)
            ++width;
    }
    return out;
}

QByteArray inflate(const uchar* in, qint64 n, qint64 expected)
{
    // qUncompress expects a 4-byte big-endian length prefix; Qt uses it as
    // the starting buffer size and grows the buffer if needed.
    QByteArray framed;
    framed.reserve(n + 4);
    char prefix[4];
    qToBigEndian(quint32(std::clamp<qint64>(expected, 1, 0x7FFFFFFF)), prefix);
    framed.append(prefix, 4);
    framed.append(reinterpret_cast<const char*>(in), n);
    QByteArray out = qUncompress(framed);
    if (out.isEmpty() && expected > 0)
        throw ParseError("Deflate-compressed image data could not be decompressed (corrupt file?)");
    return out;
}

// ---------------------------------------------------------------------------
// Predictors
// ---------------------------------------------------------------------------

template <typename T>
void horizontalAccumulate(uchar* row, qint64 samples, int stride)
{
    for (qint64 i = stride; i < samples; ++i) {
        const T a = loadAs<T>(reinterpret_cast<const char*>(row + (i - stride) * qint64(sizeof(T))));
        const T b = loadAs<T>(reinterpret_cast<const char*>(row + i * qint64(sizeof(T))));
        const T s = T(a + b);
        std::memcpy(row + i * qint64(sizeof(T)), &s, sizeof(T));
    }
}

// Floating-point predictor: undo the byte-wise differencing, then gather
// the byte planes (most significant byte first) back into host-order values.
void floatAccumulate(uchar* row, qint64 samples, int bytes, int stride, std::vector<uchar>& tmp)
{
    const qint64 n = samples * bytes;
    for (qint64 i = stride; i < n; ++i)
        row[i] = uchar(row[i] + row[i - stride]);
    tmp.assign(row, row + n);
    const bool little = QSysInfo::ByteOrder == QSysInfo::LittleEndian;
    for (qint64 k = 0; k < samples; ++k)
        for (int j = 0; j < bytes; ++j)
            row[k * bytes + j] = tmp[std::size_t((little ? bytes - 1 - j : j) * samples + k)];
}

void swapSamples(uchar* p, qint64 count, int bytes)
{
    for (qint64 i = 0; i < count; ++i)
        std::reverse(p + i * bytes, p + (i + 1) * bytes);
}

// ---------------------------------------------------------------------------
// Decoding
// ---------------------------------------------------------------------------

TiffImage decode(const QByteArray& data)
{
    if (data.size() < 8)
        throw ParseError("not a TIFF file");
    bool big = false;
    if (data.startsWith("II"))
        big = false;
    else if (data.startsWith("MM"))
        big = true;
    else
        throw ParseError("not a TIFF file");
    const Bytes b(data, big);

    const quint16 magic = b.u16(2);
    bool bigTiff = false;
    qint64 firstIfd = 0;
    if (magic == 42) {
        firstIfd = b.u32(4);
    } else if (magic == 43) {
        bigTiff = true;
        if (b.u16(4) != 8)
            throw ParseError("unsupported BigTIFF variant");
        firstIfd = b.offset(8, true);
    } else {
        throw ParseError("not a TIFF file");
    }

    const Ifd ifd = readIfd(b, firstIfd, bigTiff);

    TiffImage img;
    // Count the pages (images) without decoding them.
    {
        QSet<qint64> seen{firstIfd};
        qint64 next = ifd.next;
        try {
            while (next != 0 && !seen.contains(next) && img.pageCount < 100000) {
                seen.insert(next);
                next = nextIfd(b, next, bigTiff);
                ++img.pageCount;
            }
        } catch (const ParseError&) {
            // A broken link to a later page does not matter for the first one.
        }
    }

    const quint64 width = ifd.value(b, kImageWidth, 0);
    const quint64 height = ifd.value(b, kImageLength, 0);
    if (width == 0 || height == 0)
        throw ParseError("the image has no width or height");
    if (width > 1000000 || height > 1000000)
        throw ParseError("the image is too large");
    img.width = int(width);
    img.height = int(height);

    const quint64 spp = ifd.value(b, kSamplesPerPixel, 1);
    if (spp < 1 || spp > 16)
        throw ParseError("unsupported number of samples per pixel");
    img.samplesPerPixel = int(spp);

    const std::vector<quint64> bitsList = ifd.has(kBitsPerSample) ? ifd.values(b, kBitsPerSample)
                                                                  : std::vector<quint64>{1};
    if (bitsList.empty())
        throw ParseError("the file is corrupt (no bit depth)");
    for (quint64 v : bitsList)
        if (v != bitsList.front())
            throw ParseError("channels with different bit depths are not supported");
    const quint64 bits = bitsList.front();

    std::vector<quint64> formats = ifd.values(b, kSampleFormat);
    quint64 format = formats.empty() ? 1 : formats.front();
    for (quint64 v : formats)
        if (v != format)
            throw ParseError("channels with different sample formats are not supported");
    if (format == 4)
        format = 1;  // "undefined": treat as unsigned integers

    const quint64 compression = ifd.value(b, kCompression, kNone);
    const quint64 photometric = ifd.value(b, kPhotometric, spp >= 3 ? 2 : 1);
    const quint64 planar = ifd.value(b, kPlanarConfig, 1);
    const quint64 predictor = ifd.value(b, kPredictor, 1);

    if (photometric == 3)
        throw ParseError("palette (indexed colour) TIFF images are not supported; save the image as greyscale");
    if (photometric == 4)
        throw ParseError("transparency-mask TIFF images are not supported");
    if (photometric > 2) {
        const char* name = photometric == 5 ? "CMYK" : photometric == 6 ? "YCbCr" : photometric == 8 ? "CIE Lab" : nullptr;
        throw ParseError(name ? QStringLiteral("%1 TIFF images are not supported; save the image as greyscale or RGB")
                                    .arg(QLatin1String(name)).toStdString()
                              : "this TIFF colour space is not supported; save the image as greyscale or RGB");
    }
    if (photometric == 2 && spp < 3)
        throw ParseError("the file is corrupt (RGB image with fewer than 3 samples per pixel)");
    if (bits != 1 && bits != 8 && bits != 16 && bits != 32 && bits != 64)
        throw ParseError(QStringLiteral("%1-bit samples are not supported (only 1, 8, 16, 32 and 64 bits)")
                             .arg(bits).toStdString());
    if (bits == 1 && spp != 1)
        throw ParseError("1-bit images with more than one channel are not supported");
    if (format != 1 && format != 2 && format != 3)
        throw ParseError("complex-valued TIFF images are not supported");
    if (format == 3 && bits != 32 && bits != 64)
        throw ParseError(QStringLiteral("%1-bit floating-point samples are not supported (only 32 and 64 bits)")
                             .arg(bits).toStdString());
    if (compression != kNone && compression != kLzw && compression != kDeflate && compression != kAdobeDeflate
        && compression != kPackBits)
        throw ParseError(QStringLiteral("%1 compression is not supported. Save the image uncompressed, or with "
                                        "LZW or Deflate (ZIP) compression.")
                             .arg(compressionName(compression)).toStdString());
    if (predictor != 1 && predictor != 2 && predictor != 3)
        throw ParseError("unsupported predictor");
    if ((predictor == 2 && (format == 3 || bits == 1)) || (predictor == 3 && format != 3))
        throw ParseError("the file is corrupt (predictor does not match the sample format)");
    if (ifd.value(b, kFillOrder, 1) == 2)
        throw ParseError("bit-reversed (FillOrder 2) TIFF files are not supported");
    if (planar != 1 && planar != 2)
        throw ParseError("unsupported planar configuration");

    img.bitsPerSample = int(bits);
    img.sampleFormat = int(format);
    img.photometric = int(photometric);

    const int bytesPerSample = bits == 1 ? 1 : int(bits / 8);  // in the output
    const int chunkSpp = planar == 2 ? 1 : int(spp);           // samples per pixel in one strip / tile
    const int planes = planar == 2 ? int(spp) : 1;
    const qint64 outPixelBytes = qint64(spp) * bytesPerSample;
    const qint64 outBytes = qint64(width) * qint64(height) * outPixelBytes;
    if (outBytes > (qint64(1) << 34))
        throw ParseError("the image is too large");

    // Strips or tiles.
    const bool tiled = ifd.has(kTileWidth);
    qint64 blockW = 0;
    qint64 blockH = 0;
    std::vector<quint64> offsets;
    std::vector<quint64> counts;
    if (tiled) {
        blockW = qint64(ifd.value(b, kTileWidth, 0));
        blockH = qint64(ifd.value(b, kTileLength, 0));
        if (blockW <= 0 || blockH <= 0 || blockW > 65536 || blockH > 65536)
            throw ParseError("the file is corrupt (bad tile size)");
        offsets = ifd.values(b, kTileOffsets);
        counts = ifd.values(b, kTileByteCounts);
    } else {
        blockW = qint64(width);
        quint64 rps = ifd.value(b, kRowsPerStrip, height);
        if (rps == 0 || rps > height)
            rps = height;
        blockH = qint64(rps);
        offsets = ifd.values(b, kStripOffsets);
        counts = ifd.values(b, kStripByteCounts);
    }
    const qint64 across = (qint64(width) + blockW - 1) / blockW;
    const qint64 down = (qint64(height) + blockH - 1) / blockH;
    const qint64 blocks = across * down * planes;
    if (qint64(offsets.size()) < blocks)
        throw ParseError("the file is corrupt (missing strip or tile offsets)");
    if (!counts.empty() && qint64(counts.size()) < blocks)
        throw ParseError("the file is corrupt (missing strip or tile sizes)");
    if (counts.empty() && compression != kNone)
        throw ParseError("the file is corrupt (no strip or tile sizes)");

    const qint64 blockRowBytes = bits == 1 ? (blockW * chunkSpp + 7) / 8 : blockW * chunkSpp * (bits / 8);
    const bool swap = bits > 8 && predictor != 3 && (big != (QSysInfo::ByteOrder == QSysInfo::BigEndian));

    img.pixels = QByteArray(outBytes, Qt::Uninitialized);
    uchar* out = reinterpret_cast<uchar*>(img.pixels.data());
    std::vector<uchar> tmp;

    for (int plane = 0; plane < planes; ++plane) {
        for (qint64 by = 0; by < down; ++by) {
            for (qint64 bx = 0; bx < across; ++bx) {
                const qint64 index = qint64(plane) * across * down + by * across + bx;
                const qint64 y0 = by * blockH;
                const qint64 x0 = bx * blockW;
                // Strips at the bottom may hold fewer rows; tiles are always full size.
                const qint64 rows = tiled ? blockH : std::min(blockH, qint64(height) - y0);
                const qint64 expected = blockRowBytes * rows;

                const quint64 offset = offsets[std::size_t(index)];
                quint64 count = counts.empty() ? quint64(expected) : counts[std::size_t(index)];
                if (offset > quint64(b.size()))
                    throw ParseError("the image data is truncated");
                count = std::min<quint64>(count, quint64(b.size()) - offset);
                const uchar* src = b.ptr(qint64(offset), qint64(count));

                QByteArray block;
                switch (compression) {
                case kNone:
                    block = QByteArray::fromRawData(reinterpret_cast<const char*>(src), qsizetype(count));
                    break;
                case kPackBits:
                    block = packBitsDecode(src, qint64(count), expected);
                    break;
                case kLzw:
                    block = lzwDecode(src, qint64(count), expected);
                    break;
                default:
                    block = inflate(src, qint64(count), expected);
                    break;
                }
                if (block.size() < expected)
                    throw ParseError("the image data is truncated");
                if (compression == kNone)
                    block.detach();  // writable copy of this block only
                uchar* blockData = reinterpret_cast<uchar*>(block.data());

                const qint64 validW = std::min(blockW, qint64(width) - x0);
                for (qint64 r = 0; r < rows; ++r) {
                    const qint64 y = y0 + r;
                    uchar* row = blockData + r * blockRowBytes;
                    const qint64 samples = blockW * chunkSpp;
                    if (bits > 1) {
                        const int sb = int(bits / 8);
                        if (swap)
                            swapSamples(row, samples, sb);
                        if (predictor == 2) {
                            switch (sb) {
                            case 1: horizontalAccumulate<quint8>(row, samples, chunkSpp); break;
                            case 2: horizontalAccumulate<quint16>(row, samples, chunkSpp); break;
                            case 4: horizontalAccumulate<quint32>(row, samples, chunkSpp); break;
                            default: horizontalAccumulate<quint64>(row, samples, chunkSpp); break;
                            }
                        } else if (predictor == 3) {
                            floatAccumulate(row, samples, sb, chunkSpp, tmp);
                        }
                    }
                    if (y >= qint64(height))
                        continue;  // padding rows of a tile at the bottom edge

                    uchar* dst = out + (y * qint64(width) + x0) * outPixelBytes;
                    if (bits == 1) {
                        for (qint64 x = 0; x < validW; ++x)
                            dst[x] = uchar((row[x >> 3] >> (7 - (x & 7))) & 1);
                    } else if (planar == 1) {
                        std::memcpy(dst, row, std::size_t(validW * outPixelBytes));
                    } else {
                        for (qint64 x = 0; x < validW; ++x)
                            std::memcpy(dst + x * outPixelBytes + plane * bytesPerSample, row + x * bytesPerSample,
                                        std::size_t(bytesPerSample));
                    }
                }
            }
        }
    }
    return img;
}

} // namespace

TiffImage TiffReader::readFromBytes(const QByteArray& bytes)
{
    try {
        return decode(bytes);
    } catch (const ParseError& e) {
        TiffImage img;
        img.error = QString::fromStdString(e.what());
        img.error[0] = img.error[0].toUpper();
        return img;
    } catch (const std::bad_alloc&) {
        TiffImage img;
        img.error = QStringLiteral("Not enough memory to read the image");
        return img;
    }
}

TiffImage TiffReader::read(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        TiffImage img;
        img.error = file.errorString();
        return img;
    }
    return readFromBytes(file.readAll());
}

} // namespace iat
