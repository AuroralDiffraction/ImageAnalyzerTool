#include "io/ImageFileReader.h"

#include "io/TiffReader.h"

#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QMap>
#include <QRgba64>

#include <algorithm>
#include <cstring>
#include <new>
#include <utility>
#include <vector>

namespace iat {

namespace {

// MATLAB rgb2gray: the luminance row of the inverse NTSC (YIQ) matrix,
// documented as 0.2989 R + 0.5870 G + 0.1140 B.
constexpr double kRed = 0.298936021293775;
constexpr double kGreen = 0.587043074451121;
constexpr double kBlue = 0.114020904255103;

const QString kRgbNote = QStringLiteral("colour image converted to grey with rgb2gray weights (0.2989 R + 0.5870 G + 0.1140 B)");

QString classFor(ElementType type)
{
    switch (type) {
    case ElementType::Int8:   return QStringLiteral("int8");
    case ElementType::UInt8:  return QStringLiteral("uint8");
    case ElementType::Int16:  return QStringLiteral("int16");
    case ElementType::UInt16: return QStringLiteral("uint16");
    case ElementType::Int32:  return QStringLiteral("int32");
    case ElementType::UInt32: return QStringLiteral("uint32");
    case ElementType::Int64:  return QStringLiteral("int64");
    case ElementType::UInt64: return QStringLiteral("uint64");
    case ElementType::Single: return QStringLiteral("single");
    case ElementType::Double: break;
    }
    return QStringLiteral("double");
}

bool isAsciiDigit(QChar c) { return c >= QLatin1Char('0') && c <= QLatin1Char('9'); }

// One channel of row-major, interleaved samples of type T, as a column-major
// (MATLAB order) rows × cols plane of T.
template <typename T>
QByteArray channelColumnMajor(const uchar* src, qsizetype bytesPerLine, int width, int height, int spp, int channel)
{
    QByteArray out(qsizetype(width) * height * qsizetype(sizeof(T)), Qt::Uninitialized);
    char* dst = out.data();
    for (int y = 0; y < height; ++y) {
        const uchar* line = src + qsizetype(y) * bytesPerLine;
        for (int x = 0; x < width; ++x)
            std::memcpy(dst + (qsizetype(x) * height + y) * qsizetype(sizeof(T)),
                        line + (qsizetype(x) * spp + channel) * qsizetype(sizeof(T)), sizeof(T));
    }
    return out;
}

template <typename T>
bool rgbChannelsEqual(const uchar* src, qsizetype bytesPerLine, int width, int height, int spp)
{
    for (int y = 0; y < height; ++y) {
        const uchar* line = src + qsizetype(y) * bytesPerLine;
        for (int x = 0; x < width; ++x) {
            const uchar* p = line + qsizetype(x) * spp * qsizetype(sizeof(T));
            if (std::memcmp(p, p + sizeof(T), sizeof(T)) != 0 || std::memcmp(p, p + 2 * sizeof(T), sizeof(T)) != 0)
                return false;
        }
    }
    return true;
}

template <typename T>
QByteArray rgbToGreyColumnMajor(const uchar* src, qsizetype bytesPerLine, int width, int height, int spp)
{
    QByteArray out(qsizetype(width) * height * qsizetype(sizeof(double)), Qt::Uninitialized);
    double* dst = reinterpret_cast<double*>(out.data());
    for (int y = 0; y < height; ++y) {
        const uchar* line = src + qsizetype(y) * bytesPerLine;
        for (int x = 0; x < width; ++x) {
            const char* p = reinterpret_cast<const char*>(line + qsizetype(x) * spp * qsizetype(sizeof(T)));
            const double r = double(loadAs<T>(p));
            const double g = double(loadAs<T>(p + sizeof(T)));
            const double b = double(loadAs<T>(p + 2 * sizeof(T)));
            dst[qsizetype(x) * height + y] = kRed * r + kGreen * g + kBlue * b;
        }
    }
    return out;
}

ElementType tiffElementType(const TiffImage& t)
{
    if (t.bitsPerSample == 1)
        return ElementType::UInt8;
    if (t.sampleFormat == 3)
        return t.bitsPerSample == 32 ? ElementType::Single : ElementType::Double;
    const bool sign = t.sampleFormat == 2;
    switch (t.bitsPerSample) {
    case 8:  return sign ? ElementType::Int8 : ElementType::UInt8;
    case 16: return sign ? ElementType::Int16 : ElementType::UInt16;
    case 32: return sign ? ElementType::Int32 : ElementType::UInt32;
    default: return sign ? ElementType::Int64 : ElementType::UInt64;
    }
}

ImageFrame fromTiff(const TiffImage& t)
{
    ImageFrame f;
    f.format = QStringLiteral("TIFF");
    const ElementType type = tiffElementType(t);
    const auto* src = reinterpret_cast<const uchar*>(t.pixels.constData());
    const int spp = t.samplesPerPixel;
    const qsizetype bytesPerLine = qsizetype(t.width) * spp * qsizetype(elementSize(type));
    QStringList notes;

    dispatchElementType(type, [&](auto tag) {
        using T = decltype(tag);
        if (spp >= 3 && t.photometric == 2 && !rgbChannelsEqual<T>(src, bytesPerLine, t.width, t.height, spp)) {
            f.data = ImageStack(t.height, t.width, 1, ElementType::Double,
                                rgbToGreyColumnMajor<T>(src, bytesPerLine, t.width, t.height, spp));
            f.matlabClass = QStringLiteral("double");
            notes << kRgbNote;
        } else {
            f.data = ImageStack(t.height, t.width, 1, type,
                                channelColumnMajor<T>(src, bytesPerLine, t.width, t.height, spp, 0));
            f.matlabClass = t.bitsPerSample == 1 ? QStringLiteral("logical") : classFor(type);
        }
    });
    if (spp > 3 || (spp == 2 && t.photometric != 2))
        notes << QStringLiteral("extra channels (alpha) ignored");
    if (t.photometric == 0)
        notes << QStringLiteral("stored as WhiteIsZero; values used as stored");
    if (t.pageCount > 1)
        notes << QStringLiteral("page 1 of %1 (only the first page is read)").arg(t.pageCount);
    f.note = notes.join(QStringLiteral("; "));
    return f;
}

ImageFrame readWithQt(const QString& path)
{
    ImageFrame f;
    QImageReader reader(path);
    reader.setAutoTransform(false);  // pixels as stored, as imread does (no EXIF rotation)
    reader.setDecideFormatFromContent(true);
    const QString format = QString::fromLatin1(reader.format()).toUpper();  // known only before read()
    const QImage img = reader.read();
    f.format = format == QLatin1String("JPG") ? QStringLiteral("JPEG") : format;
    if (img.isNull()) {
        f.error = reader.errorString();
        if (f.format.isEmpty())
            f.format = QFileInfo(path).suffix().toUpper();
        return f;
    }

    const int w = img.width();
    const int h = img.height();
    QStringList notes;
    if (img.format() == QImage::Format_Grayscale8) {
        f.data = ImageStack(h, w, 1, ElementType::UInt8,
                            channelColumnMajor<quint8>(img.constBits(), img.bytesPerLine(), w, h, 1, 0));
        f.matlabClass = QStringLiteral("uint8");
    } else if (img.format() == QImage::Format_Grayscale16) {
        f.data = ImageStack(h, w, 1, ElementType::UInt16,
                            channelColumnMajor<quint16>(img.constBits(), img.bytesPerLine(), w, h, 1, 0));
        f.matlabClass = QStringLiteral("uint16");
    } else {
        // Colour, palette, 1-bit or with alpha. More than 8 bits per channel
        // is read as 16-bit RGBA, everything else as 8-bit RGBA.
        const bool wide = img.depth() > 32 || img.pixelFormat().redSize() > 8;
        if (wide) {
            const QImage c = img.convertToFormat(QImage::Format_RGBA64);
            if (rgbChannelsEqual<quint16>(c.constBits(), c.bytesPerLine(), w, h, 4)) {
                f.data = ImageStack(h, w, 1, ElementType::UInt16,
                                    channelColumnMajor<quint16>(c.constBits(), c.bytesPerLine(), w, h, 4, 0));
                f.matlabClass = QStringLiteral("uint16");
            } else {
                f.data = ImageStack(h, w, 1, ElementType::Double,
                                    rgbToGreyColumnMajor<quint16>(c.constBits(), c.bytesPerLine(), w, h, 4));
                f.matlabClass = QStringLiteral("double");
                notes << kRgbNote;
            }
        } else {
            // Format_RGBA8888 has the byte order R, G, B, A on every platform.
            const QImage c = img.convertToFormat(QImage::Format_RGBA8888);
            if (rgbChannelsEqual<quint8>(c.constBits(), c.bytesPerLine(), w, h, 4)) {
                f.data = ImageStack(h, w, 1, ElementType::UInt8,
                                    channelColumnMajor<quint8>(c.constBits(), c.bytesPerLine(), w, h, 4, 0));
                f.matlabClass = QStringLiteral("uint8");
            } else {
                f.data = ImageStack(h, w, 1, ElementType::Double,
                                    rgbToGreyColumnMajor<quint8>(c.constBits(), c.bytesPerLine(), w, h, 4));
                f.matlabClass = QStringLiteral("double");
                notes << kRgbNote;
            }
        }
        if (img.hasAlphaChannel())
            notes << QStringLiteral("alpha channel ignored");
    }
    f.note = notes.join(QStringLiteral("; "));
    return f;
}

QByteArray toDoubles(const QByteArray& raw, ElementType type)
{
    const qsizetype n = raw.size() / qsizetype(elementSize(type));
    QByteArray out(n * qsizetype(sizeof(double)), Qt::Uninitialized);
    double* dst = reinterpret_cast<double*>(out.data());
    dispatchElementType(type, [&](auto tag) {
        using T = decltype(tag);
        for (qsizetype i = 0; i < n; ++i)
            dst[i] = static_cast<double>(loadAs<T>(raw.constData() + i * qsizetype(sizeof(T))));
    });
    return out;
}

} // namespace

bool ImageFileReader::isImageFile(const QString& path)
{
    const QString s = QFileInfo(path).suffix().toLower();
    return s == QLatin1String("tif") || s == QLatin1String("tiff") || s == QLatin1String("png")
           || s == QLatin1String("jpg") || s == QLatin1String("jpeg");
}

bool ImageFileReader::isMatFile(const QString& path)
{
    return QFileInfo(path).suffix().compare(QLatin1String("mat"), Qt::CaseInsensitive) == 0;
}

QString ImageFileReader::imageFilePatterns()
{
    return QStringLiteral("*.tif *.tiff *.png *.jpg *.jpeg");
}

ImageFrame ImageFileReader::read(const QString& path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QLatin1String("tif") || suffix == QLatin1String("tiff")) {
        const TiffImage t = TiffReader::read(path);
        if (!t.ok()) {
            ImageFrame f;
            f.format = QStringLiteral("TIFF");
            f.error = t.error;
            return f;
        }
        return fromTiff(t);
    }
    return readWithQt(path);
}

ImageSequence ImageFileReader::readSequence(const QStringList& input)
{
    ImageSequence seq;
    QStringList paths = input;
    sortNaturally(paths);
    seq.paths = paths;
    if (paths.isEmpty()) {
        seq.error = QStringLiteral("No image files given.");
        return seq;
    }

    try {
        const int n = int(paths.size());
        int rows = 0;
        int cols = 0;
        ElementType type = ElementType::Double;
        QString cls;
        bool sameClass = true;
        bool sameType = true;
        QString format;
        QByteArray raw;
        QMap<QString, int> noteCounts;
        QStringList noteOrder;

        for (int i = 0; i < n; ++i) {
            const QString& path = paths.at(i);
            const ImageFrame f = read(path);
            if (!f.ok()) {
                seq.error = QStringLiteral("%1: %2").arg(QFileInfo(path).fileName(), f.error);
                return seq;
            }
            const ImageStack& d = f.data;
            if (i == 0) {
                rows = d.rows();
                cols = d.cols();
                type = d.elementType();
                cls = f.matlabClass;
                format = f.format;
                raw.reserve(d.rawData().size() * qsizetype(n));
            } else if (d.rows() != rows || d.cols() != cols) {
                seq.error = QStringLiteral("%1 is %2 × %3 pixels, but %4 is %5 × %6. All images must have the same size.")
                                .arg(QFileInfo(path).fileName())
                                .arg(d.rows())
                                .arg(d.cols())
                                .arg(QFileInfo(paths.first()).fileName())
                                .arg(rows)
                                .arg(cols);
                return seq;
            }
            if (f.matlabClass != cls)
                sameClass = false;
            if (f.format != format)
                format = QStringLiteral("image");
            if (d.elementType() != type) {
                if (sameType && type != ElementType::Double)
                    raw = toDoubles(raw, type);
                sameType = false;
                type = ElementType::Double;
            }
            if (type == ElementType::Double && d.elementType() != ElementType::Double)
                raw.append(toDoubles(d.rawData(), d.elementType()));
            else
                raw.append(d.rawData());

            if (!f.note.isEmpty()) {
                for (const QString& note : f.note.split(QStringLiteral("; "))) {
                    if (!noteCounts.contains(note))
                        noteOrder << note;
                    ++noteCounts[note];
                }
            }
        }

        seq.data = ImageStack(rows, cols, n, type, std::move(raw));
        seq.matlabClass = sameClass ? cls : classFor(type);
        seq.format = format;
        if (!sameType)
            seq.notes << QStringLiteral("files of different types, converted to double");
        for (const QString& note : noteOrder) {
            const int count = noteCounts.value(note);
            seq.notes << (count == n || n == 1 ? note : QStringLiteral("%1 (%2 of %3 files)").arg(note).arg(count).arg(n));
        }
    } catch (const std::bad_alloc&) {
        seq.data = ImageStack();
        seq.error = QStringLiteral("Not enough memory to load %1 images.").arg(paths.size());
    }
    return seq;
}

bool naturalLess(const QString& a, const QString& b)
{
    qsizetype i = 0;
    qsizetype j = 0;
    while (i < a.size() && j < b.size()) {
        if (isAsciiDigit(a[i]) && isAsciiDigit(b[j])) {
            const qsizetype si = i;
            const qsizetype sj = j;
            while (i < a.size() && isAsciiDigit(a[i]))
                ++i;
            while (j < b.size() && isAsciiDigit(b[j]))
                ++j;
            QStringView na = QStringView(a).mid(si, i - si);
            QStringView nb = QStringView(b).mid(sj, j - sj);
            while (na.size() > 1 && na.front() == QLatin1Char('0'))
                na = na.mid(1);
            while (nb.size() > 1 && nb.front() == QLatin1Char('0'))
                nb = nb.mid(1);
            if (na.size() != nb.size())
                return na.size() < nb.size();
            const int c = na.compare(nb);
            if (c != 0)
                return c < 0;
            continue;
        }
        const QChar la = a[i].toLower();
        const QChar lb = b[j].toLower();
        if (la != lb)
            return la < lb;
        ++i;
        ++j;
    }
    const qsizetype restA = a.size() - i;
    const qsizetype restB = b.size() - j;
    if (restA != restB)
        return restA < restB;
    return a < b;  // equal apart from case or leading zeros: any fixed order
}

void sortNaturally(QStringList& paths)
{
    std::vector<std::pair<QString, QString>> keyed;  // file name, path
    keyed.reserve(std::size_t(paths.size()));
    for (const QString& p : paths)
        keyed.emplace_back(QFileInfo(p).fileName(), p);
    std::stable_sort(keyed.begin(), keyed.end(), [](const auto& x, const auto& y) {
        if (naturalLess(x.first, y.first))
            return true;
        if (naturalLess(y.first, x.first))
            return false;
        return naturalLess(x.second, y.second);
    });
    for (qsizetype i = 0; i < paths.size(); ++i)
        paths[i] = keyed[std::size_t(i)].second;
}

FileSelection selectFiles(const QStringList& paths, bool allowMultiple)
{
    FileSelection s;
    int mats = 0;
    int images = 0;
    for (const QString& p : paths) {
        if (ImageFileReader::isMatFile(p)) {
            ++mats;
            s.files << p;
        } else if (ImageFileReader::isImageFile(p)) {
            ++images;
            s.files << p;
        } else {
            ++s.ignored;
        }
    }
    s.isMat = mats > 0;

    if (s.files.isEmpty()) {
        if (paths.size() == 1)
            s.error = QStringLiteral("%1 is not a supported file. Use a .mat file, or TIFF, PNG or JPG images.")
                          .arg(QFileInfo(paths.first()).fileName());
        else
            s.error = QStringLiteral("No supported file. Use a .mat file, or TIFF, PNG or JPG images.");
    } else if (mats > 1 || (mats == 1 && images > 0)) {
        s.error = QStringLiteral("Several files can only be loaded together when they are all TIFF, PNG or JPG "
                                 "images. Load one .mat file at a time.");
    } else if (images > 1 && !allowMultiple) {
        s.error = QStringLiteral("Load one file here. Several images can be combined in the Light frames and "
                                 "Dark frames panels.");
    }
    return s;
}

QString baseNameFor(const QStringList& paths)
{
    if (paths.isEmpty())
        return {};
    QString prefix = QFileInfo(paths.first()).completeBaseName();
    if (paths.size() == 1)
        return prefix;
    for (qsizetype i = 1; i < paths.size(); ++i) {
        const QString name = QFileInfo(paths.at(i)).completeBaseName();
        qsizetype k = 0;
        while (k < prefix.size() && k < name.size() && prefix[k] == name[k])
            ++k;
        prefix.truncate(k);
    }
    while (!prefix.isEmpty()
           && (isAsciiDigit(prefix.back()) || QStringLiteral("_- .").contains(prefix.back())))
        prefix.chop(1);
    return prefix.isEmpty() ? QStringLiteral("images") : prefix;
}

} // namespace iat
