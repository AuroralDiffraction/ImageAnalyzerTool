#include "io/HsiCubeIo.h"

#include "io/MatFileReader.h"
#include "io/MatFileWriter.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <limits>
#include <map>

namespace iat::hsi {

namespace {

QString tr(const char* text) { return QCoreApplication::translate("iat::hsi", text); }

quint16 toUInt16(float v)
{
    if (!std::isfinite(v) || v <= 0.0f)
        return 0;
    return quint16(std::min(65535L, std::lround(v)));
}

QString baseOf(const QString& path)
{
    const QFileInfo fi(path);
    const QString suffix = fi.suffix().toLower();
    static const QStringList known = {QStringLiteral("hdr"), QStringLiteral("img"), QStringLiteral("dat"),
                                      QStringLiteral("raw"), QStringLiteral("bsq"), QStringLiteral("bil"),
                                      QStringLiteral("bip")};
    if (known.contains(suffix))
        return fi.path() + QLatin1Char('/') + fi.completeBaseName();
    return path;
}

QString braceList(const QStringList& items)
{
    return QStringLiteral("{") + items.join(QStringLiteral(", ")) + QStringLiteral("}");
}

QString numberList(const QVector<double>& v, int decimals)
{
    QStringList items;
    for (double x : v)
        items << QString::number(x, 'f', decimals);
    return braceList(items);
}

// key → value (braces removed, lower-case keys) of an ENVI header.
std::map<QString, QString> parseEnviHeader(const QString& text)
{
    std::map<QString, QString> out;
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("\r\n|\r|\n")));
    QString key, value;
    int depth = 0;
    for (int i = 1; i < lines.size(); ++i) {  // line 0 is "ENVI"
        const QString& line = lines[i];
        if (depth > 0) {
            value += QLatin1Char('\n') + line;
            depth += int(line.count(QLatin1Char('{'))) - int(line.count(QLatin1Char('}')));
        } else {
            const int eq = line.indexOf(QLatin1Char('='));
            if (eq < 0)
                continue;
            key = line.left(eq).trimmed().toLower();
            value = line.mid(eq + 1).trimmed();
            depth = int(value.count(QLatin1Char('{'))) - int(value.count(QLatin1Char('}')));
        }
        if (depth <= 0) {
            QString v = value.trimmed();
            if (v.startsWith(QLatin1Char('{')) && v.endsWith(QLatin1Char('}')))
                v = v.mid(1, v.size() - 2).trimmed();
            out[key] = v;
            depth = 0;
        }
    }
    return out;
}

QStringList splitList(const QString& v)
{
    QStringList items;
    for (const QString& s : v.split(QLatin1Char(',')))
        items << s.trimmed();
    return items;
}

} // namespace

// ---------------------------------------------------------------------------
// ENVI
// ---------------------------------------------------------------------------

QString enviHeaderPath(const QString& path) { return baseOf(path) + QStringLiteral(".hdr"); }
QString enviDataPath(const QString& path) { return baseOf(path) + QStringLiteral(".img"); }

bool writeEnvi(const Cube& cube, const QString& path, SampleType type, Interleave il, QString* error,
               const std::optional<MosaicLayout>& layout)
{
    if (cube.isNull()) {
        if (error)
            *error = tr("There is no cube to save.");
        return false;
    }
    const int rows = cube.rows(), cols = cube.cols(), B = cube.bands();
    const int bytes = type == SampleType::Float32 ? 4 : 2;
    QByteArray data;
    data.resize(qsizetype(rows) * cols * B * bytes);
    char* out = data.data();
    auto put = [&](qsizetype index, float v) {
        if (type == SampleType::Float32)
            qToLittleEndian(v, out + index * 4);
        else
            qToLittleEndian(toUInt16(v), out + index * 2);
    };
    for (int b = 0; b < B; ++b) {
        const float* src = cube.band(b);
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < cols; ++x) {
                qsizetype index = 0;
                switch (il) {
                case Interleave::Bsq: index = (qsizetype(b) * rows + y) * cols + x; break;
                case Interleave::Bil: index = (qsizetype(y) * B + b) * cols + x; break;
                case Interleave::Bip: index = (qsizetype(y) * cols + x) * B + b; break;
                }
                put(index, src[qsizetype(y) * cols + x]);
            }
    }

    QStringList bandNames;
    for (int b = 0; b < B; ++b)
        bandNames << (cube.hasWavelengths() ? QStringLiteral("Band %1 (%2 nm)").arg(b + 1).arg(cube.wavelengths[b], 0, 'f', 1)
                                            : QStringLiteral("Band %1").arg(b + 1));
    QString hdr;
    QTextStream s(&hdr);
    s << "ENVI\n";
    QStringList desc = {QStringLiteral("Hyperspectral cube written by ImageAnalyzerTool")};
    desc << cube.history;
    if (layout)
        desc << QStringLiteral("mosaic layout [%1]").arg(layout->toString());
    QString d = desc.join(QStringLiteral("; "));
    d.replace(QLatin1Char('{'), QLatin1Char('(')).replace(QLatin1Char('}'), QLatin1Char(')'));
    s << "description = {" << d << "}\n";
    s << "samples = " << cols << "\n";
    s << "lines = " << rows << "\n";
    s << "bands = " << B << "\n";
    s << "header offset = 0\n";
    s << "file type = ENVI Standard\n";
    s << "data type = " << (type == SampleType::Float32 ? 4 : 12) << "\n";
    s << "interleave = " << (il == Interleave::Bsq ? "bsq" : il == Interleave::Bil ? "bil" : "bip") << "\n";
    s << "byte order = 0\n";
    s << "band names = " << braceList(bandNames) << "\n";
    if (cube.hasWavelengths()) {
        s << "wavelength units = Nanometers\n";
        s << "wavelength = " << numberList(cube.wavelengths, 2) << "\n";
    }
    if (B >= 3) {
        int r, g, bl;
        // Default bands for an RGB view: about 610 / 550 / 490 nm (1-based in ENVI).
        if (cube.hasWavelengths()) {
            r = cube.nearestBand(610.0);
            g = cube.nearestBand(550.0);
            bl = cube.nearestBand(490.0);
        } else {
            r = std::min(14, B - 1);
            g = std::min(8, B - 1);
            bl = std::min(1, B - 1);
        }
        s << "default bands = {" << r + 1 << ", " << g + 1 << ", " << bl + 1 << "}\n";
    }
    if (cube.units == QLatin1String("reflectance"))
        s << "reflectance scale factor = 1.0\n";
    s << "data units = " << cube.units << "\n";
    s.flush();

    QSaveFile dataFile(enviDataPath(path));
    if (!dataFile.open(QIODevice::WriteOnly) || dataFile.write(data) != data.size() || !dataFile.commit()) {
        if (error)
            *error = tr("Could not write %1: %2").arg(enviDataPath(path), dataFile.errorString());
        return false;
    }
    QSaveFile hdrFile(enviHeaderPath(path));
    if (!hdrFile.open(QIODevice::WriteOnly) || hdrFile.write(hdr.toUtf8()) < 0 || !hdrFile.commit()) {
        if (error)
            *error = tr("Could not write %1: %2").arg(enviHeaderPath(path), hdrFile.errorString());
        return false;
    }
    return true;
}

CubeFile readEnvi(const QString& path)
{
    CubeFile result;
    // Header: <base>.hdr or <file>.hdr.
    QString hdrPath = enviHeaderPath(path);
    if (!QFileInfo::exists(hdrPath) && QFileInfo::exists(path + QStringLiteral(".hdr")))
        hdrPath = path + QStringLiteral(".hdr");
    QFile hf(hdrPath);
    if (!hf.open(QIODevice::ReadOnly)) {
        result.error = tr("Could not open the ENVI header %1.").arg(hdrPath);
        return result;
    }
    const QString text = QString::fromUtf8(hf.readAll());
    if (!text.trimmed().startsWith(QLatin1String("ENVI"))) {
        result.error = tr("%1 is not an ENVI header (it does not start with “ENVI”).").arg(QFileInfo(hdrPath).fileName());
        return result;
    }
    auto h = parseEnviHeader(text);
    const int cols = h[QStringLiteral("samples")].toInt();
    const int rows = h[QStringLiteral("lines")].toInt();
    const int B = h[QStringLiteral("bands")].toInt();
    const qint64 offset = h[QStringLiteral("header offset")].toLongLong();
    const int dtype = h[QStringLiteral("data type")].toInt();
    const QString il = h[QStringLiteral("interleave")].toLower();
    const bool bigEndian = h[QStringLiteral("byte order")].toInt() == 1;
    if (rows <= 0 || cols <= 0 || B <= 0) {
        result.error = tr("The ENVI header does not give the size (samples, lines, bands).");
        return result;
    }
    int bytes = 0;
    switch (dtype) {
    case 1: bytes = 1; break;
    case 2: case 12: bytes = 2; break;
    case 3: case 4: case 13: bytes = 4; break;
    case 5: case 14: case 15: bytes = 8; break;
    default:
        result.error = tr("ENVI data type %1 is not supported (complex data?).").arg(dtype);
        return result;
    }

    // Data file: the path itself if it is not the header, else <base> + a usual extension.
    QString dataPath;
    if (QFileInfo(path).suffix().toLower() != QLatin1String("hdr") && QFileInfo::exists(path)) {
        dataPath = path;
    } else {
        const QString base = baseOf(hdrPath);
        for (const char* ext : {".img", ".dat", ".raw", ".bsq", ".bil", ".bip", ""})
            if (QFileInfo::exists(base + QLatin1String(ext)) && QFileInfo(base + QLatin1String(ext)).isFile()) {
                dataPath = base + QLatin1String(ext);
                break;
            }
        if (dataPath.isEmpty() && hdrPath.endsWith(QLatin1String(".hdr")) && QFileInfo::exists(hdrPath.chopped(4))
            && QFileInfo(hdrPath.chopped(4)).isFile())
            dataPath = hdrPath.chopped(4);
    }
    if (dataPath.isEmpty()) {
        result.error = tr("The data file for %1 was not found (expected .img, .dat, .raw or no extension).")
                           .arg(QFileInfo(hdrPath).fileName());
        return result;
    }
    QFile df(dataPath);
    if (!df.open(QIODevice::ReadOnly)) {
        result.error = tr("Could not open %1: %2").arg(dataPath, df.errorString());
        return result;
    }
    const qint64 need = qint64(rows) * cols * B * bytes;
    df.seek(offset);
    const QByteArray raw = df.read(need);
    if (raw.size() != need) {
        result.error = tr("%1 is too short: %2 bytes expected after the header offset, %3 found.")
                           .arg(QFileInfo(dataPath).fileName()).arg(need).arg(raw.size());
        return result;
    }
    auto value = [&](qsizetype index) -> float {
        const char* p = raw.constData() + index * bytes;
        char tmp[8];
        std::memcpy(tmp, p, size_t(bytes));
        if (bigEndian != (QSysInfo::ByteOrder == QSysInfo::BigEndian))
            std::reverse(tmp, tmp + bytes);
        switch (dtype) {
        case 1: return float(quint8(tmp[0]));
        case 2: { qint16 v; std::memcpy(&v, tmp, 2); return float(v); }
        case 12: { quint16 v; std::memcpy(&v, tmp, 2); return float(v); }
        case 3: { qint32 v; std::memcpy(&v, tmp, 4); return float(v); }
        case 13: { quint32 v; std::memcpy(&v, tmp, 4); return float(v); }
        case 4: { float v; std::memcpy(&v, tmp, 4); return v; }
        case 5: { double v; std::memcpy(&v, tmp, 8); return float(v); }
        case 14: { qint64 v; std::memcpy(&v, tmp, 8); return float(v); }
        case 15: { quint64 v; std::memcpy(&v, tmp, 8); return float(v); }
        }
        return 0.0f;
    };
    Cube cube(rows, cols, B);
    for (int b = 0; b < B; ++b) {
        float* dst = cube.band(b);
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < cols; ++x) {
                qsizetype index;
                if (il == QLatin1String("bil"))
                    index = (qsizetype(y) * B + b) * cols + x;
                else if (il == QLatin1String("bip"))
                    index = (qsizetype(y) * cols + x) * B + b;
                else
                    index = (qsizetype(b) * rows + y) * cols + x;
                dst[qsizetype(y) * cols + x] = value(index);
            }
    }
    const QString wl = h[QStringLiteral("wavelength")];
    if (!wl.isEmpty()) {
        QVector<double> w;
        for (const QString& item : splitList(wl))
            w << item.toDouble();
        const QString units = h[QStringLiteral("wavelength units")].toLower();
        if (units.startsWith(QLatin1String("micro")) || units == QLatin1String("um"))
            for (double& x : w)
                x *= 1000.0;
        if (w.size() == B)
            cube.wavelengths = w;
    }
    const QString dataUnits = h[QStringLiteral("data units")];
    if (dataUnits.compare(QLatin1String("reflectance"), Qt::CaseInsensitive) == 0)
        cube.units = QStringLiteral("reflectance");
    const QString desc = h[QStringLiteral("description")];
    static const QRegularExpression layoutRe(QStringLiteral("mosaic layout \\[([0-9 ;]+)\\]"));
    const auto m = layoutRe.match(desc);
    if (m.hasMatch())
        result.layout = MosaicLayout::fromString(m.captured(1));
    cube.history << QStringLiteral("read from ENVI file %1").arg(QFileInfo(dataPath).fileName());
    result.cube = cube;
    return result;
}

// ---------------------------------------------------------------------------
// MAT
// ---------------------------------------------------------------------------

bool writeMat(const Cube& cube, const QString& path, SampleType type, QString* error,
              const std::optional<MosaicLayout>& layout)
{
    if (cube.isNull()) {
        if (error)
            *error = tr("There is no cube to save.");
        return false;
    }
    const int rows = cube.rows(), cols = cube.cols(), B = cube.bands();
    const int bytes = type == SampleType::Float32 ? 4 : 2;
    QByteArray data;
    data.resize(qsizetype(rows) * cols * B * bytes);
    char* out = data.data();
    for (int b = 0; b < B; ++b) {
        const float* src = cube.band(b);
        for (int x = 0; x < cols; ++x)
            for (int y = 0; y < rows; ++y) {
                const qsizetype index = (qsizetype(b) * cols + x) * rows + y;  // column-major
                const float v = src[qsizetype(y) * cols + x];
                if (type == SampleType::Float32)
                    std::memcpy(out + index * 4, &v, 4);
                else {
                    const quint16 u = toUInt16(v);
                    std::memcpy(out + index * 2, &u, 2);
                }
            }
    }
    MatFileWriter w;
    const ImageStack stack(rows, cols, B, type == SampleType::Float32 ? ElementType::Single : ElementType::UInt16, data);
    bool ok = w.addStack(QStringLiteral("cube"), stack,
                         type == SampleType::Float32 ? QStringLiteral("single") : QStringLiteral("uint16"));
    if (cube.hasWavelengths()) {
        Image2D wl(1, B);
        for (int b = 0; b < B; ++b)
            wl.data()[b] = cube.wavelengths[b];
        ok = ok && w.addImage(QStringLiteral("wavelengths"), wl);
    }
    if (layout) {
        Image2D m(kPattern, kPattern);
        for (int r = 0; r < kPattern; ++r)
            for (int c = 0; c < kPattern; ++c)
                m.data()[r * kPattern + c] = layout->at(r, c);
        ok = ok && w.addImage(QStringLiteral("mosaicLayout"), m);
    }
    ok = ok && w.addString(QStringLiteral("units"), cube.units);
    ok = ok && w.addString(QStringLiteral("history"), cube.history.join(QLatin1Char('\n')));
    ok = ok && w.save(path);
    if (!ok && error)
        *error = w.errorString();
    return ok;
}

CubeFile readMat(const QString& path)
{
    CubeFile result;
    const MatFileContents c = MatFileReader::read(path);
    if (!c.ok()) {
        result.error = c.error;
        return result;
    }
    const MatVariable* cubeVar = nullptr;
    for (const MatVariable& v : c.variables)
        if (v.supported && v.name == QLatin1String("cube") && v.data.frameCount() > 1)
            cubeVar = &v;
    if (!cubeVar)
        for (const MatVariable& v : c.variables)
            if (v.supported && v.isImageStack() && v.data.frameCount() > 1) {
                cubeVar = &v;
                break;
            }
    if (!cubeVar) {
        result.error = tr("%1 has no 3-D array (rows × columns × bands).").arg(QFileInfo(path).fileName());
        return result;
    }
    const ImageStack& s = cubeVar->data;
    Cube cube(s.rows(), s.cols(), s.frameCount());
    for (int b = 0; b < s.frameCount(); ++b) {
        const Image2D f = s.frame(b);
        float* dst = cube.band(b);
        for (qsizetype i = 0; i < f.pixelCount(); ++i)
            dst[i] = float(f.constData()[i]);
    }
    for (const MatVariable& v : c.variables) {
        if (!v.supported || v.data.isNull())
            continue;
        const qint64 n = qint64(v.data.rows()) * v.data.cols() * v.data.frameCount();
        if ((v.name == QLatin1String("wavelengths") || v.name == QLatin1String("bandCenters")
             || v.name == QLatin1String("wavelength"))
            && n == cube.bands() && (v.data.rows() == 1 || v.data.cols() == 1)) {
            const Image2D f = v.data.frame(0);
            cube.wavelengths = QVector<double>(f.constData(), f.constData() + n);
        }
        if (v.name == QLatin1String("mosaicLayout") && v.data.rows() == kPattern && v.data.cols() == kPattern) {
            const Image2D f = v.data.frame(0);
            MosaicLayout m;
            for (int r = 0; r < kPattern; ++r)
                for (int col = 0; col < kPattern; ++col)
                    m.set(r, col, int(std::lround(f.at(col, r))));
            if (m.isValid())
                result.layout = m;
        }
    }
    if (cubeVar->name != QLatin1String("cube"))
        result.notes << tr("Variable “%1” used as the cube.").arg(cubeVar->name);
    cube.history << QStringLiteral("read from %1, variable %2").arg(QFileInfo(path).fileName(), cubeVar->name);
    result.cube = cube;
    return result;
}

// ---------------------------------------------------------------------------
// Multi-page TIFF
// ---------------------------------------------------------------------------

bool writeMultiPageTiff(const Cube& cube, const QString& path, SampleType type, QString* error)
{
    if (cube.isNull()) {
        if (error)
            *error = tr("There is no cube to save.");
        return false;
    }
    const int rows = cube.rows(), cols = cube.cols(), B = cube.bands();
    const int bps = type == SampleType::Float32 ? 32 : 16;
    const qint64 planeBytes = qint64(rows) * cols * (bps / 8);
    if (planeBytes * B > qint64(std::numeric_limits<quint32>::max()) - 1024 * qint64(B)) {
        if (error)
            *error = tr("The cube is too large for a classic TIFF file (4 GB). Save it as ENVI instead.");
        return false;
    }
    QByteArray file;
    auto u16 = [&](quint16 v) { char b[2]; qToLittleEndian(v, b); file.append(b, 2); };
    auto u32 = [&](quint32 v) { char b[4]; qToLittleEndian(v, b); file.append(b, 4); };
    auto patch32 = [&](qsizetype at, quint32 v) { qToLittleEndian(v, file.data() + at); };
    file.append("II", 2);
    u16(42);
    const qsizetype firstIfdPos = file.size();
    u32(0);

    qsizetype prevNextPos = firstIfdPos;
    for (int b = 0; b < B; ++b) {
        // Description text, then pixel data, then the IFD.
        QByteArray desc = (cube.hasWavelengths() ? QStringLiteral("Band %1, %2 nm").arg(b + 1).arg(cube.wavelengths[b], 0, 'f', 1)
                                                 : QStringLiteral("Band %1").arg(b + 1)).toLatin1();
        desc.append('\0');
        const quint32 descPos = quint32(file.size());
        file.append(desc);
        if (file.size() % 2)
            file.append('\0');
        const quint32 dataPos = quint32(file.size());
        const float* src = cube.band(b);
        for (qsizetype i = 0; i < cube.planeSize(); ++i) {
            if (type == SampleType::Float32) {
                char buf[4];
                qToLittleEndian(src[i], buf);
                file.append(buf, 4);
            } else {
                u16(toUInt16(src[i]));
            }
        }
        if (file.size() % 2)
            file.append('\0');
        const quint32 ifdPos = quint32(file.size());
        patch32(prevNextPos, ifdPos);
        struct Entry { quint16 tag, type; quint32 count, value; };
        const Entry entries[] = {
            {256, 4, 1, quint32(cols)},           // ImageWidth
            {257, 4, 1, quint32(rows)},           // ImageLength
            {258, 3, 1, quint32(bps)},            // BitsPerSample
            {259, 3, 1, 1},                       // Compression: none
            {262, 3, 1, 1},                       // Photometric: BlackIsZero
            {270, 2, quint32(desc.size()), descPos},  // ImageDescription
            {273, 4, 1, dataPos},                 // StripOffsets
            {277, 3, 1, 1},                       // SamplesPerPixel
            {278, 4, 1, quint32(rows)},           // RowsPerStrip
            {279, 4, 1, quint32(planeBytes)},     // StripByteCounts
            {284, 3, 1, 1},                       // PlanarConfiguration: chunky
            {339, 3, 1, quint32(type == SampleType::Float32 ? 3 : 1)},  // SampleFormat
        };
        u16(quint16(std::size(entries)));
        for (const Entry& e : entries) {
            u16(e.tag);
            u16(e.type);
            u32(e.count);
            if (e.type == 3 && e.count == 1) {
                u16(quint16(e.value));
                u16(0);
            } else {
                u32(e.value);
            }
        }
        prevNextPos = file.size();
        u32(0);  // next IFD, patched for the following page
    }
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(file) != file.size() || !f.commit()) {
        if (error)
            *error = tr("Could not write %1: %2").arg(path, f.errorString());
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

bool isCubeFile(const QString& path)
{
    const QString s = QFileInfo(path).suffix().toLower();
    if (s == QLatin1String("hdr"))
        return true;
    if (s == QLatin1String("img") || s == QLatin1String("dat") || s == QLatin1String("raw") || s == QLatin1String("bsq")
        || s == QLatin1String("bil") || s == QLatin1String("bip"))
        return QFileInfo::exists(enviHeaderPath(path)) || QFileInfo::exists(path + QStringLiteral(".hdr"));
    return false;
}

CubeFile readCube(const QString& path)
{
    if (QFileInfo(path).suffix().compare(QLatin1String("mat"), Qt::CaseInsensitive) == 0)
        return readMat(path);
    return readEnvi(path);
}

// ---------------------------------------------------------------------------
// Spectral response
// ---------------------------------------------------------------------------

bool isResponseCandidate(const QString& path)
{
    const QString s = QFileInfo(path).suffix().toLower();
    return s == QLatin1String("csv") || s == QLatin1String("txt");
}

namespace {

void cleanResponse(SpectralResponse& r, bool percent)
{
    for (auto& c : r.qe)
        for (double& v : c) {
            if (!std::isfinite(v))
                v = 0.0;
            if (percent)
                v /= 100.0;
            v = std::max(0.0, v);
        }
}

ResponseFile responseFromMat(const QString& path)
{
    ResponseFile result;
    const MatFileContents c = MatFileReader::read(path);
    if (!c.ok()) {
        result.error = c.error;
        return result;
    }
    auto find = [&](std::initializer_list<const char*> names) -> const MatVariable* {
        for (const char* n : names)
            for (const MatVariable& v : c.variables)
                if (v.supported && !v.data.isNull() && v.name == QLatin1String(n))
                    return &v;
        return nullptr;
    };
    const MatVariable* lam = find({"lambda", "wavelength", "wavelengths", "lambda_nm"});
    const MatVariable* qe = find({"QE_percent", "QE", "qe", "QE_fraction"});
    if (lam && qe && qe->data.frameCount() == 1) {
        const Image2D l = lam->data.frame(0);
        const int nL = int(l.pixelCount());
        const Image2D q = qe->data.frame(0);  // width = cols, height = rows
        bool samplesInRows;
        if (q.height() == nL)
            samplesInRows = true;
        else if (q.width() == nL)
            samplesInRows = false;
        else {
            result.error = tr("%1: %2 has %3 × %4 values, but there are %5 wavelengths.")
                               .arg(QFileInfo(path).fileName(), qe->name).arg(q.height()).arg(q.width()).arg(nL);
            return result;
        }
        const int B = samplesInRows ? q.width() : q.height();
        result.response.lambda = QVector<double>(l.constData(), l.constData() + nL);
        result.response.qe = QVector<QVector<double>>(B, QVector<double>(nL));
        double maxValue = 0.0;
        for (int b = 0; b < B; ++b)
            for (int i = 0; i < nL; ++i) {
                const double v = samplesInRows ? q.at(b, i) : q.at(i, b);
                result.response.qe[b][i] = v;
                if (std::isfinite(v))
                    maxValue = std::max(maxValue, v);
            }
        const bool percent = qe->name.contains(QLatin1String("percent"), Qt::CaseInsensitive) || maxValue > 1.5;
        cleanResponse(result.response, percent);
        result.notes << tr("%1 channels, %2 wavelengths (%3–%4 nm), from %5 and %6.")
                            .arg(B).arg(nL).arg(result.response.lambda.first()).arg(result.response.lambda.last())
                            .arg(lam->name, qe->name);
        return result;
    }
    const MatVariable* rgb = find({"reconRGB"});
    if (rgb && rgb->data.rows() == 3 && rgb->data.frameCount() == 1) {
        const Image2D m = rgb->data.frame(0);
        for (int r = 0; r < 3; ++r)
            for (int b = 0; b < m.width(); ++b)
                result.rgbMatrix << m.at(b, r);
        result.notes << tr("Colour matrix reconRGB (3 × %1), used as it is: its white balance was fixed when it was made.")
                            .arg(m.width());
        return result;
    }
    result.error = tr("%1 has neither a wavelength vector (lambda) with a QE matrix (QE_percent or QE) nor a 3 × N matrix reconRGB.")
                       .arg(QFileInfo(path).fileName());
    return result;
}

ResponseFile responseFromCsv(const QString& path)
{
    ResponseFile result;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.error = tr("Could not open %1: %2").arg(path, f.errorString());
        return result;
    }
    const QStringList lines = QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'));
    QChar sep = QLatin1Char(',');
    for (const QString& l : lines.mid(0, 20))
        if (l.count(QLatin1Char('\t')) > l.count(sep))
            sep = QLatin1Char('\t');
        else if (l.count(QLatin1Char(';')) > l.count(sep))
            sep = QLatin1Char(';');
    static const QRegularExpression bandRe(QStringLiteral("^\\s*\"?band\\s*(\\d+)\"?\\s*$"),
                                           QRegularExpression::CaseInsensitiveOption);
    int headerRow = -1;
    std::map<int, int> columnOfBand;  // band number → column
    for (int i = 0; i < lines.size() && headerRow < 0; ++i) {
        const QStringList cells = lines[i].split(sep);
        for (int c = 0; c < cells.size(); ++c) {
            const auto m = bandRe.match(cells[c]);
            if (m.hasMatch()) {
                columnOfBand[m.captured(1).toInt()] = c;
                headerRow = i;
            }
        }
    }
    if (headerRow < 0) {
        result.error = tr("%1 has no header row with Band1 … BandN columns.").arg(QFileInfo(path).fileName());
        return result;
    }
    const int B = int(columnOfBand.size());
    for (int b = 1; b <= B; ++b)
        if (!columnOfBand.count(b)) {
            result.error = tr("%1: the band columns must be Band1 … Band%2; Band%3 is missing.")
                               .arg(QFileInfo(path).fileName()).arg(B).arg(b);
            return result;
        }
    result.response.qe = QVector<QVector<double>>(B);
    double maxValue = 0.0;
    for (int i = headerRow + 1; i < lines.size(); ++i) {
        const QStringList cells = lines[i].split(sep);
        if (cells.isEmpty())
            continue;
        bool ok = false;
        const double lam = cells[0].trimmed().toDouble(&ok);
        if (!ok)
            continue;
        result.response.lambda << lam;
        for (int b = 1; b <= B; ++b) {
            const int col = columnOfBand[b];
            bool vok = false;
            const double v = col < cells.size() ? cells[col].trimmed().toDouble(&vok) : 0.0;
            result.response.qe[b - 1] << (vok ? v : std::numeric_limits<double>::quiet_NaN());
            if (vok)
                maxValue = std::max(maxValue, v);
        }
    }
    if (result.response.lambda.size() < 3) {
        result.error = tr("%1 has fewer than 3 rows of data under the header.").arg(QFileInfo(path).fileName());
        return result;
    }
    cleanResponse(result.response, maxValue > 1.5);
    result.notes << tr("%1 channels, %2 wavelengths (%3–%4 nm).")
                        .arg(B).arg(result.response.lambda.size()).arg(result.response.lambda.first())
                        .arg(result.response.lambda.last());
    return result;
}

} // namespace

ResponseFile loadResponse(const QString& path)
{
    ResponseFile r = QFileInfo(path).suffix().compare(QLatin1String("mat"), Qt::CaseInsensitive) == 0
                         ? responseFromMat(path)
                         : responseFromCsv(path);
    if (r.ok() && r.hasResponse()) {
        // Wavelengths must increase.
        for (int i = 1; i < r.response.samples(); ++i)
            if (!(r.response.lambda[i] > r.response.lambda[i - 1])) {
                r.error = tr("The wavelengths in %1 do not increase.").arg(QFileInfo(path).fileName());
                break;
            }
    }
    return r;
}

// ---------------------------------------------------------------------------
// Correction matrix
// ---------------------------------------------------------------------------

CorrectionFile loadCorrectionMatrix(const QString& path, int inBands)
{
    CorrectionFile out;
    const QString name = QFileInfo(path).fileName();
    QVector<QVector<double>> rows;
    QVector<double> centres;
    if (QFileInfo(path).suffix().compare(QLatin1String("mat"), Qt::CaseInsensitive) == 0) {
        const MatFileContents c = MatFileReader::read(path);
        if (!c.ok()) {
            out.error = c.error;
            return out;
        }
        const MatVariable* m = nullptr;
        for (const char* n : {"correction", "correctionMatrix", "C"})
            for (const MatVariable& v : c.variables)
                if (!m && v.supported && v.name == QLatin1String(n) && v.data.frameCount() == 1)
                    m = &v;
        if (!m)
            for (const MatVariable& v : c.variables)
                if (!m && v.supported && v.data.frameCount() == 1 && v.data.cols() == inBands && v.data.rows() > 1)
                    m = &v;
        if (!m) {
            out.error = tr("%1 has no correction matrix (a variable correction, correctionMatrix or C with %2 columns).")
                            .arg(name).arg(inBands);
            return out;
        }
        const Image2D f = m->data.frame(0);
        for (int r = 0; r < f.height(); ++r) {
            QVector<double> row;
            for (int col = 0; col < f.width(); ++col)
                row << f.at(col, r);
            rows << row;
        }
        for (const MatVariable& v : c.variables)
            if (v.supported && (v.name == QLatin1String("wavelengths") || v.name == QLatin1String("centres"))
                && qint64(v.data.rows()) * v.data.cols() == rows.size()) {
                const Image2D w = v.data.frame(0);
                centres = QVector<double>(w.constData(), w.constData() + w.pixelCount());
            }
    } else {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            out.error = tr("Could not open %1: %2").arg(path, f.errorString());
            return out;
        }
        static const QRegularExpression sep(QStringLiteral("[,;\\t ]+"));
        for (const QString& line : QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'))) {
            const QStringList cells = line.trimmed().split(sep, Qt::SkipEmptyParts);
            if (cells.isEmpty())
                continue;
            QVector<double> row;
            bool ok = true;
            for (const QString& cell : cells) {
                const double v = cell.toDouble(&ok);
                if (!ok)
                    break;
                row << v;
            }
            if (!ok)
                continue;  // a header or comment
            if (row.size() == inBands + 1) {
                centres << row.first();
                row.removeFirst();
            }
            rows << row;
        }
        if (centres.size() != rows.size())
            centres.clear();
    }
    if (rows.isEmpty()) {
        out.error = tr("%1 holds no matrix.").arg(name);
        return out;
    }
    for (const auto& row : rows)
        if (row.size() != inBands) {
            out.error = tr("%1: every row must have %2 values (one per channel), or %3 with the centre wavelength first.")
                            .arg(name).arg(inBands).arg(inBands + 1);
            return out;
        }
    SpectralCorrection& c = out.correction;
    c.outBands = int(rows.size());
    c.inBands = inBands;
    for (const auto& row : rows)
        c.matrix << row;
    c.centres = centres;
    c.source = QStringLiteral("loaded from %1").arg(name);
    return out;
}

// ---------------------------------------------------------------------------
// Sensor profile
// ---------------------------------------------------------------------------

bool writeSensorProfile(const SensorProfile& p, const QString& path, QString* error)
{
    QJsonObject o;
    o[QStringLiteral("format")] = QStringLiteral("ImageAnalyzerTool sensor profile");
    o[QStringLiteral("version")] = 1;
    o[QStringLiteral("name")] = p.name;
    QJsonArray layout;
    for (int ch : p.layout.channel)
        layout.append(ch);
    o[QStringLiteral("mosaicLayout")] = layout;
    o[QStringLiteral("bitDepth")] = p.bitDepth;
    o[QStringLiteral("blackLevel")] = p.blackLevel;
    o[QStringLiteral("clipNegative")] = p.clipNegative;
    o[QStringLiteral("extractionMethod")] = p.method;
    o[QStringLiteral("spectralResponse")] = p.responsePath;
    QJsonObject corr;
    corr[QStringLiteral("enabled")] = p.spectralCorrection;
    corr[QStringLiteral("targetFwhm")] = p.correctionFwhm;
    corr[QStringLiteral("regularisation")] = p.correctionRegularisation;
    corr[QStringLiteral("matrixFile")] = p.correctionMatrixPath;
    o[QStringLiteral("spectralCorrection")] = corr;
    QJsonObject frames;
    frames[QStringLiteral("dark")] = p.darkPath;
    frames[QStringLiteral("white")] = p.whitePath;
    frames[QStringLiteral("sceneExposureMs")] = p.sceneExposure;
    frames[QStringLiteral("whiteExposureMs")] = p.whiteExposure;
    frames[QStringLiteral("whiteReflectance")] = p.whiteReflectance;
    o[QStringLiteral("referenceFrames")] = frames;
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(QJsonDocument(o).toJson()) < 0 || !f.commit()) {
        if (error)
            *error = tr("Could not write %1: %2").arg(path, f.errorString());
        return false;
    }
    return true;
}

std::optional<SensorProfile> readSensorProfile(const QString& path, QString* error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = tr("Could not open %1: %2").arg(path, f.errorString());
        return std::nullopt;
    }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    const QJsonObject o = doc.object();
    if (pe.error != QJsonParseError::NoError || o.value(QStringLiteral("format")).toString() != QLatin1String("ImageAnalyzerTool sensor profile")) {
        if (error)
            *error = tr("%1 is not a sensor profile.").arg(QFileInfo(path).fileName());
        return std::nullopt;
    }
    SensorProfile p;
    p.name = o.value(QStringLiteral("name")).toString();
    const QJsonArray layout = o.value(QStringLiteral("mosaicLayout")).toArray();
    if (layout.size() == kBands) {
        MosaicLayout m;
        for (int i = 0; i < kBands; ++i)
            m.channel[size_t(i)] = layout[i].toInt();
        if (m.isValid())
            p.layout = m;
    }
    p.bitDepth = o.value(QStringLiteral("bitDepth")).toInt(14);
    p.blackLevel = o.value(QStringLiteral("blackLevel")).toDouble(0.0);
    p.clipNegative = o.value(QStringLiteral("clipNegative")).toBool(true);
    p.method = o.value(QStringLiteral("extractionMethod")).toInt(0);
    p.responsePath = o.value(QStringLiteral("spectralResponse")).toString();
    const QJsonObject corr = o.value(QStringLiteral("spectralCorrection")).toObject();
    p.spectralCorrection = corr.value(QStringLiteral("enabled")).toBool(false);
    p.correctionFwhm = corr.value(QStringLiteral("targetFwhm")).toDouble(0.0);
    p.correctionRegularisation = corr.value(QStringLiteral("regularisation")).toDouble(0.01);
    p.correctionMatrixPath = corr.value(QStringLiteral("matrixFile")).toString();
    const QJsonObject frames = o.value(QStringLiteral("referenceFrames")).toObject();
    p.darkPath = frames.value(QStringLiteral("dark")).toString();
    p.whitePath = frames.value(QStringLiteral("white")).toString();
    p.sceneExposure = frames.value(QStringLiteral("sceneExposureMs")).toDouble(0.0);
    p.whiteExposure = frames.value(QStringLiteral("whiteExposureMs")).toDouble(0.0);
    p.whiteReflectance = frames.value(QStringLiteral("whiteReflectance")).toDouble(1.0);
    return p;
}

// ---------------------------------------------------------------------------
// Spectra CSV
// ---------------------------------------------------------------------------

bool writeSpectraCsv(const QString& path, const QVector<double>& wavelengths, const QList<SpectrumSeries>& series,
                     QString* error)
{
    int B = 0;
    for (const SpectrumSeries& s : series)
        B = std::max(B, int(s.mean.size()));
    auto quote = [](QString s) {
        s.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QStringLiteral("\"%1\"").arg(s);
    };
    QString text;
    QStringList header = {QStringLiteral("band")};
    const bool wl = wavelengths.size() == B && B > 0;
    if (wl)
        header << QStringLiteral("wavelength_nm");
    for (const SpectrumSeries& s : series) {
        header << quote(s.name + QStringLiteral(" mean"));
        if (!s.std.isEmpty())
            header << quote(s.name + QStringLiteral(" std"));
    }
    text += header.join(QLatin1Char(',')) + QLatin1Char('\n');
    for (int b = 0; b < B; ++b) {
        QStringList row = {QString::number(b + 1)};
        if (wl)
            row << QString::number(wavelengths[b], 'g', 8);
        for (const SpectrumSeries& s : series) {
            row << (b < s.mean.size() ? QString::number(s.mean[b], 'g', 10) : QString());
            if (!s.std.isEmpty())
                row << (b < s.std.size() ? QString::number(s.std[b], 'g', 10) : QString());
        }
        text += row.join(QLatin1Char(',')) + QLatin1Char('\n');
    }
    QStringList counts = {QStringLiteral("pixels")};
    if (wl)
        counts << QString();
    for (const SpectrumSeries& s : series) {
        counts << QString::number(s.pixels);
        if (!s.std.isEmpty())
            counts << QString();
    }
    text += counts.join(QLatin1Char(',')) + QLatin1Char('\n');
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(text.toUtf8()) < 0 || !f.commit()) {
        if (error)
            *error = tr("Could not write %1: %2").arg(path, f.errorString());
        return false;
    }
    return true;
}

} // namespace iat::hsi
