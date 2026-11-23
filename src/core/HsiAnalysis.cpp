#include "core/HsiAnalysis.h"

#include "core/HsiLinalg.h"
#include "core/Processing.h"

#include <QCoreApplication>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <random>
#include <vector>

namespace iat::hsi {

namespace {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

// Pixel indices to use for statistics: all, or an even subset of about `max`.
std::vector<qsizetype> samplePixels(qsizetype n, qsizetype max)
{
    std::vector<qsizetype> idx;
    const qsizetype step = std::max<qsizetype>(1, n / std::max<qsizetype>(1, max));
    idx.reserve(size_t(n / step + 1));
    for (qsizetype p = 0; p < n; p += step)
        idx.push_back(p);
    return idx;
}

bool finiteSpectrum(const Cube& c, qsizetype p)
{
    for (int b = 0; b < c.bands(); ++b)
        if (!std::isfinite(c.band(b)[p]))
            return false;
    return true;
}

double stretch(double v, double lo, double hi)
{
    if (!std::isfinite(v))
        return 0.0;
    return hi > lo ? std::clamp((v - lo) / (hi - lo), 0.0, 1.0) : 0.0;
}

} // namespace

// ---------------------------------------------------------------------------
// Region statistics
// ---------------------------------------------------------------------------

namespace {
template <typename Inside>
RegionStats statsOver(const Cube& cube, int x0, int y0, int x1, int y1, Inside inside)
{
    RegionStats s;
    const int B = cube.bands();
    QVector<double> sum(B, 0.0), sum2(B, 0.0);
    QVector<qint64> n(B, 0);
    qint64 pixels = 0;
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (!inside(x, y))
                continue;
            ++pixels;
            for (int b = 0; b < B; ++b) {
                const double v = cube.at(x, y, b);
                if (!std::isfinite(v))
                    continue;
                sum[b] += v;
                sum2[b] += v * v;
                ++n[b];
            }
        }
    if (pixels == 0)
        return s;
    s.count = pixels;
    s.mean.resize(B);
    s.std.resize(B);
    for (int b = 0; b < B; ++b) {
        if (n[b] == 0) {
            s.mean[b] = s.std[b] = kNaN;
            continue;
        }
        s.mean[b] = sum[b] / n[b];
        s.std[b] = std::sqrt(std::max(0.0, sum2[b] / n[b] - s.mean[b] * s.mean[b]));
    }
    return s;
}
} // namespace

RegionStats regionStats(const Cube& cube, const QRect& area)
{
    const QRect a = area.intersected(QRect(0, 0, cube.cols(), cube.rows()));
    if (cube.isNull() || a.isEmpty())
        return {};
    return statsOver(cube, a.left(), a.top(), a.right(), a.bottom(), [](int, int) { return true; });
}

RegionStats maskStats(const Cube& cube, const QVector<quint8>& mask)
{
    if (cube.isNull() || mask.size() != cube.planeSize())
        return {};
    return statsOver(cube, 0, 0, cube.cols() - 1, cube.rows() - 1,
                     [&](int x, int y) { return mask[qsizetype(y) * cube.cols() + x] != 0; });
}

// ---------------------------------------------------------------------------
// Spectral angle, k-means
// ---------------------------------------------------------------------------

Image2D spectralAngleMap(const Cube& cube, const QVector<double>& ref)
{
    if (cube.isNull() || ref.size() != cube.bands())
        return {};
    double refNorm = 0.0;
    for (double v : ref)
        refNorm += v * v;
    refNorm = std::sqrt(refNorm);
    Image2D out(cube.cols(), cube.rows(), kNaN);
    if (!(refNorm > 0.0))
        return out;
    double* dst = out.data();
    for (qsizetype p = 0; p < cube.planeSize(); ++p) {
        double dot = 0.0, norm = 0.0;
        for (int b = 0; b < cube.bands(); ++b) {
            const double v = cube.band(b)[p];
            dot += v * ref[b];
            norm += v * v;
        }
        if (norm > 0.0 && std::isfinite(norm))
            dst[p] = std::acos(std::clamp(dot / (std::sqrt(norm) * refNorm), -1.0, 1.0));
    }
    return out;
}

Clustering kMeans(const Cube& cube, int k, int iterations, bool normalize, quint32 seed)
{
    Clustering result;
    const int B = cube.bands();
    const qsizetype N = cube.planeSize();
    if (cube.isNull() || k < 1 || N < k)
        return result;
    auto vectorAt = [&](qsizetype p, double* v) {
        double norm = 0.0;
        for (int b = 0; b < B; ++b) {
            v[b] = cube.band(b)[p];
            norm += v[b] * v[b];
        }
        if (normalize) {
            norm = std::sqrt(norm);
            for (int b = 0; b < B; ++b)
                v[b] = norm > 0.0 ? v[b] / norm : 0.0;
        }
    };
    // Train on up to 60 000 pixels.
    std::vector<qsizetype> train;
    for (qsizetype p : samplePixels(N, 60000))
        if (finiteSpectrum(cube, p))
            train.push_back(p);
    if (qsizetype(train.size()) < k)
        return result;
    const size_t T = train.size();
    std::vector<double> X(T * size_t(B));
    for (size_t i = 0; i < T; ++i)
        vectorAt(train[i], &X[i * B]);
    auto dist2 = [&](const double* a, const double* c) {
        double s = 0.0;
        for (int b = 0; b < B; ++b)
            s += (a[b] - c[b]) * (a[b] - c[b]);
        return s;
    };

    // k-means++ start.
    std::mt19937 rng(seed);
    std::vector<double> C(size_t(k) * B);
    std::uniform_int_distribution<size_t> pick(0, T - 1);
    std::copy_n(&X[pick(rng) * B], B, &C[0]);
    std::vector<double> d(T, std::numeric_limits<double>::max());
    for (int c = 1; c < k; ++c) {
        double total = 0.0;
        for (size_t i = 0; i < T; ++i) {
            d[i] = std::min(d[i], dist2(&X[i * B], &C[size_t(c - 1) * B]));
            total += d[i];
        }
        std::uniform_real_distribution<double> u(0.0, total);
        double r = u(rng);
        size_t chosen = T - 1;
        for (size_t i = 0; i < T; ++i) {
            r -= d[i];
            if (r <= 0.0) {
                chosen = i;
                break;
            }
        }
        std::copy_n(&X[chosen * B], B, &C[size_t(c) * B]);
    }

    std::vector<int> label(T, 0);
    for (int it = 0; it < iterations; ++it) {
        bool changed = false;
        for (size_t i = 0; i < T; ++i) {
            int best = 0;
            double bestD = dist2(&X[i * B], &C[0]);
            for (int c = 1; c < k; ++c) {
                const double dd = dist2(&X[i * B], &C[size_t(c) * B]);
                if (dd < bestD) {
                    bestD = dd;
                    best = c;
                }
            }
            if (label[i] != best || it == 0) {
                changed = changed || label[i] != best;
                label[i] = best;
            }
        }
        std::vector<double> sum(size_t(k) * B, 0.0);
        std::vector<qint64> cnt(size_t(k), 0);
        for (size_t i = 0; i < T; ++i) {
            for (int b = 0; b < B; ++b)
                sum[size_t(label[i]) * B + b] += X[i * B + b];
            ++cnt[size_t(label[i])];
        }
        for (int c = 0; c < k; ++c)
            if (cnt[size_t(c)] > 0)
                for (int b = 0; b < B; ++b)
                    C[size_t(c) * B + b] = sum[size_t(c) * B + b] / double(cnt[size_t(c)]);
        if (!changed && it > 0)
            break;
    }

    // Assign every pixel; order classes by brightness of their mean spectrum.
    result.k = k;
    result.labels.resize(N);
    std::vector<double> v(static_cast<size_t>(B));
    for (qsizetype p = 0; p < N; ++p) {
        if (!finiteSpectrum(cube, p)) {
            result.labels[p] = -1;
            continue;
        }
        vectorAt(p, v.data());
        int best = 0;
        double bestD = dist2(v.data(), &C[0]);
        for (int c = 1; c < k; ++c) {
            const double dd = dist2(v.data(), &C[size_t(c) * B]);
            if (dd < bestD) {
                bestD = dd;
                best = c;
            }
        }
        result.labels[p] = best;
    }
    result.centres = QVector<QVector<double>>(k, QVector<double>(B, 0.0));
    result.sizes = QVector<qint64>(k, 0);
    for (qsizetype p = 0; p < N; ++p) {
        const int c = result.labels[p];
        if (c < 0)
            continue;
        for (int b = 0; b < B; ++b)
            result.centres[c][b] += cube.band(b)[p];
        ++result.sizes[c];
    }
    for (int c = 0; c < k; ++c)
        for (int b = 0; b < B; ++b)
            result.centres[c][b] = result.sizes[c] > 0 ? result.centres[c][b] / double(result.sizes[c]) : kNaN;
    std::vector<int> order(static_cast<size_t>(k));
    for (int c = 0; c < k; ++c)
        order[size_t(c)] = c;
    auto brightness = [&](int c) {
        double s = 0.0;
        for (double x : result.centres[c])
            s += std::isfinite(x) ? x : 0.0;
        return s;
    };
    std::sort(order.begin(), order.end(), [&](int a, int b) { return brightness(a) > brightness(b); });
    std::vector<int> rank(static_cast<size_t>(k));
    for (int i = 0; i < k; ++i)
        rank[size_t(order[size_t(i)])] = i;
    for (int& l : result.labels)
        if (l >= 0)
            l = rank[size_t(l)];
    QVector<QVector<double>> centres(k);
    QVector<qint64> sizes(k);
    for (int c = 0; c < k; ++c) {
        centres[rank[size_t(c)]] = result.centres[c];
        sizes[rank[size_t(c)]] = result.sizes[c];
    }
    result.centres = centres;
    result.sizes = sizes;
    return result;
}

// ---------------------------------------------------------------------------
// Matching to reference areas (SAM classification)
// ---------------------------------------------------------------------------

AreaMatch matchToReferences(const Cube& cube, const QVector<QVector<double>>& refs, double maxAngle)
{
    AreaMatch m;
    const int B = cube.bands();
    if (cube.isNull() || refs.isEmpty())
        return m;
    QVector<double> norms;
    for (const auto& r : refs) {
        if (r.size() != B)
            return m;
        double n = 0.0;
        for (double v : r)
            n += v * v;
        norms << std::sqrt(n);
    }
    m.k = int(refs.size());
    m.maxAngle = maxAngle;
    const qsizetype N = cube.planeSize();
    m.best = QVector<int>(N, -1);
    m.angle = QVector<float>(N, std::numeric_limits<float>::quiet_NaN());
    m.counts = QVector<qint64>(m.k, 0);
    std::vector<double> v(static_cast<size_t>(B));
    for (qsizetype p = 0; p < N; ++p) {
        double norm = 0.0;
        for (int b = 0; b < B; ++b) {
            v[size_t(b)] = cube.band(b)[p];
            norm += v[size_t(b)] * v[size_t(b)];
        }
        if (!(norm > 0.0) || !std::isfinite(norm))
            continue;
        norm = std::sqrt(norm);
        int best = -1;
        double bestAngle = std::numeric_limits<double>::max();
        for (int k = 0; k < m.k; ++k) {
            if (!(norms[k] > 0.0))
                continue;
            double dot = 0.0;
            for (int b = 0; b < B; ++b)
                dot += v[size_t(b)] * refs[k][b];
            const double a = std::acos(std::clamp(dot / (norm * norms[k]), -1.0, 1.0));
            if (a < bestAngle) {
                bestAngle = a;
                best = k;
            }
        }
        m.angle[p] = float(bestAngle);
        if (best >= 0 && bestAngle <= maxAngle) {
            m.best[p] = best;
            ++m.counts[best];
        }
    }
    return m;
}

QImage renderAreaMatch(const AreaMatch& m, int cols, int rows, const QVector<QColor>& colours, bool shade,
                       const QImage& background)
{
    if (!m.isValid() || m.best.size() != qsizetype(cols) * rows || colours.size() < m.k)
        return {};
    QImage img = background.size() == QSize(cols, rows) ? background.convertToFormat(QImage::Format_RGB888)
                                                        : QImage(cols, rows, QImage::Format_RGB888);
    if (background.size() != QSize(cols, rows))
        img.fill(Qt::black);
    for (int y = 0; y < rows; ++y) {
        uchar* line = img.scanLine(y);
        for (int x = 0; x < cols; ++x) {
            const qsizetype p = qsizetype(y) * cols + x;
            const int k = m.best[p];
            if (k < 0)
                continue;
            double f = 1.0;
            if (shade && m.maxAngle > 0.0)
                f = 1.0 - 0.65 * std::clamp(double(m.angle[p]) / m.maxAngle, 0.0, 1.0);
            const QColor& c = colours[k];
            line[3 * x] = uchar(std::lround(c.red() * f));
            line[3 * x + 1] = uchar(std::lround(c.green() * f));
            line[3 * x + 2] = uchar(std::lround(c.blue() * f));
        }
    }
    return img;
}

// ---------------------------------------------------------------------------
// PCA
// ---------------------------------------------------------------------------

QVector<double> Pca::explained() const
{
    double total = 0.0;
    for (double v : variances)
        total += std::max(0.0, v);
    QVector<double> e;
    for (double v : variances)
        e << (total > 0.0 ? std::max(0.0, v) / total : 0.0);
    return e;
}

Pca principalComponents(const Cube& cube)
{
    Pca pca;
    const int B = cube.bands();
    if (cube.isNull())
        return pca;
    std::vector<qsizetype> idx;
    for (qsizetype p : samplePixels(cube.planeSize(), 250000))
        if (finiteSpectrum(cube, p))
            idx.push_back(p);
    if (idx.size() < 2)
        return pca;
    QVector<double> mean(B, 0.0), cov(B * B, 0.0);
    for (qsizetype p : idx)
        for (int b = 0; b < B; ++b)
            mean[b] += cube.band(b)[p];
    for (double& m : mean)
        m /= double(idx.size());
    std::vector<double> v(static_cast<size_t>(B));
    for (qsizetype p : idx) {
        for (int b = 0; b < B; ++b)
            v[size_t(b)] = cube.band(b)[p] - mean[b];
        for (int i = 0; i < B; ++i)
            for (int j = 0; j <= i; ++j)
                cov[i * B + j] += v[size_t(i)] * v[size_t(j)];
    }
    for (int i = 0; i < B; ++i)
        for (int j = 0; j <= i; ++j) {
            cov[i * B + j] /= double(idx.size());
            cov[j * B + i] = cov[i * B + j];
        }
    linalg::symmetricEigen(cov, B, pca.variances, pca.vectors);
    // Sign convention: the largest coefficient of each component is positive.
    for (int c = 0; c < B; ++c) {
        int big = 0;
        for (int r = 1; r < B; ++r)
            if (std::abs(pca.vectors[r * B + c]) > std::abs(pca.vectors[big * B + c]))
                big = r;
        if (pca.vectors[big * B + c] < 0)
            for (int r = 0; r < B; ++r)
                pca.vectors[r * B + c] = -pca.vectors[r * B + c];
    }
    pca.mean = mean;
    pca.bands = B;
    return pca;
}

QVector<quint8> selectInPolygon(const Image2D& x, const Image2D& y, const QPolygonF& polygon)
{
    QVector<quint8> mask;
    if (x.isNull() || !x.sameSize(y) || polygon.size() < 3)
        return mask;
    mask = QVector<quint8>(x.pixelCount(), 0);
    const QRectF bounds = polygon.boundingRect();
    for (qsizetype i = 0; i < x.pixelCount(); ++i) {
        const QPointF p(x.constData()[i], y.constData()[i]);
        if (std::isfinite(p.x()) && std::isfinite(p.y()) && bounds.contains(p) && polygon.containsPoint(p, Qt::OddEvenFill))
            mask[i] = 1;
    }
    return mask;
}

Image2D pcaComponent(const Cube& cube, const Pca& pca, int index)
{
    const int B = cube.bands();
    if (!pca.isValid() || pca.bands != B || index < 0 || index >= B)
        return {};
    Image2D out(cube.cols(), cube.rows());
    double* dst = out.data();
    for (qsizetype p = 0; p < cube.planeSize(); ++p) {
        double s = 0.0;
        for (int b = 0; b < B; ++b)
            s += (cube.band(b)[p] - pca.mean[b]) * pca.vectors[b * B + index];
        dst[p] = s;
    }
    return out;
}

// ---------------------------------------------------------------------------
// Band maths
// ---------------------------------------------------------------------------

struct BandMath::Node {
    enum Kind { Number, Band, Add, Sub, Mul, Div, Pow, Neg, Func } kind = Number;
    double value = 0.0;
    int band = 0;
    QString func;
    std::shared_ptr<Node> a, b;
};

namespace {

using NodePtr = std::shared_ptr<BandMath::Node>;

class Parser {
public:
    Parser(const QString& text, int bands, const QVector<double>& wl)
        : m_s(text), m_bands(bands), m_wl(wl) {}

    NodePtr parse(QString* error, QString* resolved)
    {
        NodePtr n = expr();
        skip();
        if (!m_error.isEmpty() || !n) {
            *error = m_error.isEmpty() ? QCoreApplication::translate("iat::hsi", "Empty formula.") : m_error;
            return nullptr;
        }
        if (m_pos < m_s.size()) {
            *error = QCoreApplication::translate("iat::hsi", "Unexpected “%1” at position %2.").arg(m_s.mid(m_pos, 1)).arg(m_pos + 1);
            return nullptr;
        }
        *resolved = m_resolved.simplified();
        return n;
    }

private:
    void skip()
    {
        while (m_pos < m_s.size() && m_s[m_pos].isSpace())
            m_resolved += m_s[m_pos++];
    }
    bool take(QChar c)
    {
        skip();
        if (m_pos < m_s.size() && m_s[m_pos] == c) {
            m_resolved += c;
            ++m_pos;
            return true;
        }
        return false;
    }
    bool takeAny(std::initializer_list<QChar> cs, QChar& which)
    {
        skip();
        if (m_pos >= m_s.size())
            return false;
        for (QChar c : cs)
            if (m_s[m_pos] == c) {
                which = c;
                m_resolved += c;
                ++m_pos;
                return true;
            }
        return false;
    }
    NodePtr make(BandMath::Node::Kind k, NodePtr a = nullptr, NodePtr b = nullptr)
    {
        auto n = std::make_shared<BandMath::Node>();
        n->kind = k;
        n->a = std::move(a);
        n->b = std::move(b);
        return n;
    }
    NodePtr fail(const QString& msg)
    {
        if (m_error.isEmpty())
            m_error = msg;
        return nullptr;
    }
    NodePtr expr()
    {
        NodePtr n = term();
        QChar op;
        while (n && takeAny({QLatin1Char('+'), QLatin1Char('-'), QChar(0x2212)}, op)) {
            NodePtr r = term();
            if (!r)
                return nullptr;
            n = make(op == QLatin1Char('+') ? BandMath::Node::Add : BandMath::Node::Sub, n, r);
        }
        return n;
    }
    NodePtr term()
    {
        NodePtr n = power();
        QChar op;
        while (n && takeAny({QLatin1Char('*'), QLatin1Char('/'), QChar(0x00D7), QChar(0x00F7)}, op)) {
            NodePtr r = power();
            if (!r)
                return nullptr;
            n = make(op == QLatin1Char('/') || op == QChar(0x00F7) ? BandMath::Node::Div : BandMath::Node::Mul, n, r);
        }
        return n;
    }
    NodePtr power()
    {
        NodePtr n = unary();
        if (n && take(QLatin1Char('^'))) {
            NodePtr r = power();
            if (!r)
                return nullptr;
            return make(BandMath::Node::Pow, n, r);
        }
        return n;
    }
    NodePtr unary()
    {
        QChar op;
        if (takeAny({QLatin1Char('-'), QChar(0x2212)}, op)) {
            NodePtr n = unary();
            return n ? make(BandMath::Node::Neg, n) : nullptr;
        }
        if (take(QLatin1Char('+')))
            return unary();
        return primary();
    }
    NodePtr primary()
    {
        skip();
        if (m_pos >= m_s.size())
            return fail(QCoreApplication::translate("iat::hsi", "The formula ends too early."));
        if (take(QLatin1Char('('))) {
            NodePtr n = expr();
            if (!n)
                return nullptr;
            if (!take(QLatin1Char(')')))
                return fail(QCoreApplication::translate("iat::hsi", "A closing parenthesis is missing."));
            return n;
        }
        const QChar c = m_s[m_pos];
        if (c.isDigit() || c == QLatin1Char('.')) {
            const int start = m_pos;
            while (m_pos < m_s.size() && (m_s[m_pos].isDigit() || m_s[m_pos] == QLatin1Char('.')))
                ++m_pos;
            if (m_pos < m_s.size() && (m_s[m_pos] == QLatin1Char('e') || m_s[m_pos] == QLatin1Char('E'))) {
                int p = m_pos + 1;
                if (p < m_s.size() && (m_s[p] == QLatin1Char('+') || m_s[p] == QLatin1Char('-')))
                    ++p;
                if (p < m_s.size() && m_s[p].isDigit()) {
                    m_pos = p;
                    while (m_pos < m_s.size() && m_s[m_pos].isDigit())
                        ++m_pos;
                }
            }
            bool ok = false;
            const QString num = m_s.mid(start, m_pos - start);
            const double v = num.toDouble(&ok);
            if (!ok)
                return fail(QCoreApplication::translate("iat::hsi", "“%1” is not a number.").arg(num));
            m_resolved += num;
            auto n = make(BandMath::Node::Number);
            n->value = v;
            return n;
        }
        if (c.isLetter()) {
            const int start = m_pos;
            while (m_pos < m_s.size() && m_s[m_pos].isLetter())
                ++m_pos;
            const QString word = m_s.mid(start, m_pos - start);
            const QString lower = word.toLower();
            if ((lower == QLatin1String("b") || lower == QLatin1String("r")) && m_pos < m_s.size() && m_s[m_pos].isDigit()) {
                const int numStart = m_pos;
                while (m_pos < m_s.size() && (m_s[m_pos].isDigit() || m_s[m_pos] == QLatin1Char('.')))
                    ++m_pos;
                const double num = m_s.mid(numStart, m_pos - numStart).toDouble();
                auto n = make(BandMath::Node::Band);
                if (lower == QLatin1String("b")) {
                    const int band = int(num);
                    if (band < 1 || band > m_bands || band != num)
                        return fail(QCoreApplication::translate("iat::hsi", "There is no band %1 (the cube has bands 1–%2).")
                                        .arg(m_s.mid(numStart, m_pos - numStart)).arg(m_bands));
                    n->band = band - 1;
                } else {
                    if (m_wl.size() != m_bands || m_bands == 0)
                        return fail(QCoreApplication::translate("iat::hsi", "R%1 needs the band wavelengths: load a spectral response (QE) file first, or use b1 … b%2.")
                                        .arg(num).arg(m_bands));
                    const auto [lo, hi] = std::minmax_element(m_wl.begin(), m_wl.end());
                    const double spacing = m_bands > 1 ? (*hi - *lo) / (m_bands - 1) : 10.0;
                    if (num < *lo - spacing || num > *hi + spacing)
                        return fail(QCoreApplication::translate("iat::hsi", "%1 nm is outside the sensor's range (%2–%3 nm).")
                                        .arg(num).arg(*lo, 0, 'f', 0).arg(*hi, 0, 'f', 0));
                    int best = 0;
                    for (int b = 1; b < m_bands; ++b)
                        if (std::abs(m_wl[b] - num) < std::abs(m_wl[best] - num))
                            best = b;
                    n->band = best;
                }
                m_resolved += QStringLiteral("b%1").arg(n->band + 1);
                return n;
            }
            static const QStringList funcs = {QStringLiteral("sqrt"), QStringLiteral("abs"), QStringLiteral("log"),
                                              QStringLiteral("exp")};
            if (funcs.contains(lower)) {
                m_resolved += lower;
                if (!take(QLatin1Char('(')))
                    return fail(QCoreApplication::translate("iat::hsi", "%1 must be followed by “(”.").arg(word));
                NodePtr arg = expr();
                if (!arg)
                    return nullptr;
                if (!take(QLatin1Char(')')))
                    return fail(QCoreApplication::translate("iat::hsi", "A closing parenthesis is missing."));
                auto n = make(BandMath::Node::Func, arg);
                n->func = lower;
                return n;
            }
            return fail(QCoreApplication::translate("iat::hsi", "Unknown name “%1”. Use b1 … b%2 for bands, R<nm> for wavelengths.")
                            .arg(word).arg(m_bands));
        }
        return fail(QCoreApplication::translate("iat::hsi", "Unexpected “%1” at position %2.").arg(c).arg(m_pos + 1));
    }

    QString m_s;
    int m_pos = 0;
    int m_bands;
    QVector<double> m_wl;
    QString m_error;
    QString m_resolved;
};

std::vector<double> evalNode(const BandMath::Node& n, const Cube& cube)
{
    const qsizetype N = cube.planeSize();
    switch (n.kind) {
    case BandMath::Node::Number:
        return std::vector<double>(size_t(N), n.value);
    case BandMath::Node::Band: {
        const float* s = cube.band(n.band);
        return std::vector<double>(s, s + N);
    }
    case BandMath::Node::Neg: {
        std::vector<double> a = evalNode(*n.a, cube);
        for (double& v : a)
            v = -v;
        return a;
    }
    case BandMath::Node::Func: {
        std::vector<double> a = evalNode(*n.a, cube);
        for (double& v : a) {
            if (n.func == QLatin1String("sqrt"))
                v = std::sqrt(v);
            else if (n.func == QLatin1String("abs"))
                v = std::abs(v);
            else if (n.func == QLatin1String("log"))
                v = std::log(v);
            else
                v = std::exp(v);
        }
        return a;
    }
    default:
        break;
    }
    std::vector<double> a = evalNode(*n.a, cube);
    const std::vector<double> b = evalNode(*n.b, cube);
    for (size_t i = 0; i < a.size(); ++i) {
        switch (n.kind) {
        case BandMath::Node::Add: a[i] += b[i]; break;
        case BandMath::Node::Sub: a[i] -= b[i]; break;
        case BandMath::Node::Mul: a[i] *= b[i]; break;
        case BandMath::Node::Div: a[i] = b[i] != 0.0 ? a[i] / b[i] : kNaN; break;
        case BandMath::Node::Pow: a[i] = std::pow(a[i], b[i]); break;
        default: break;
        }
    }
    return a;
}

void collectBands(const BandMath::Node* n, QVector<int>& out)
{
    if (!n)
        return;
    if (n->kind == BandMath::Node::Band && !out.contains(n->band))
        out << n->band;
    collectBands(n->a.get(), out);
    collectBands(n->b.get(), out);
}

} // namespace

std::shared_ptr<BandMath> BandMath::parse(const QString& formula, int bands, const QVector<double>& wavelengths,
                                          QString* error)
{
    QString err, resolved;
    Parser p(formula, bands, wavelengths);
    NodePtr root = p.parse(&err, &resolved);
    if (!root) {
        if (error)
            *error = err;
        return nullptr;
    }
    auto bm = std::shared_ptr<BandMath>(new BandMath);
    bm->m_root = root;
    bm->m_resolved = resolved;
    return bm;
}

Image2D BandMath::evaluate(const Cube& cube) const
{
    if (!m_root || cube.isNull())
        return {};
    for (int b : bandsUsed())
        if (b >= cube.bands())
            return {};
    const std::vector<double> v = evalNode(*m_root, cube);
    Image2D out(cube.cols(), cube.rows());
    std::copy(v.begin(), v.end(), out.data());
    return out;
}

QVector<int> BandMath::bandsUsed() const
{
    QVector<int> out;
    collectBands(m_root.get(), out);
    std::sort(out.begin(), out.end());
    return out;
}

QVector<IndexPreset> indexPresets()
{
    return {
        {QStringLiteral("PRI"), QStringLiteral("(R531 - R570) / (R531 + R570)"),
         QStringLiteral("Photochemical reflectance index: light-use efficiency and stress of plants")},
        {QStringLiteral("Green-red difference (550/600 nm)"), QStringLiteral("(R550 - R600) / (R550 + R600)"),
         QStringLiteral("Separates green vegetation from soil and dry material (a narrow-band variant of NGRDI)")},
        {QStringLiteral("Blue-green difference (490/550 nm)"), QStringLiteral("(R490 - R550) / (R490 + R550)"),
         QStringLiteral("Blue versus green: water, shadows and blue materials stand out")},
        {QStringLiteral("Green peak ratio (550/510 nm)"), QStringLiteral("R550 / R510"),
         QStringLiteral("Height of the green reflectance peak of leaves")},
        {QStringLiteral("NDVI"), QStringLiteral("(R800 - R670) / (R800 + R670)"),
         QStringLiteral("Normalised difference vegetation index; needs near-infrared (800 nm)")},
        {QStringLiteral("NDRE"), QStringLiteral("(R790 - R720) / (R790 + R720)"),
         QStringLiteral("Red-edge index; needs 720 and 790 nm")},
    };
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

QColor colormapColor(Colormap map, double t)
{
    t = std::clamp(std::isfinite(t) ? t : 0.0, 0.0, 1.0);
    auto rgb = [](double r, double g, double b) {
        return QColor::fromRgbF(float(std::clamp(r, 0.0, 1.0)), float(std::clamp(g, 0.0, 1.0)),
                                float(std::clamp(b, 0.0, 1.0)));
    };
    switch (map) {
    case Colormap::Grey:
        return rgb(t, t, t);
    case Colormap::Viridis: {
        // Polynomial fit of matplotlib's viridis (public domain, by mattz).
        const double r = 0.2777273272234177 + t * (0.1050930431085774 + t * (-0.3308618287255563 + t * (-4.634230498983486 + t * (6.228269936347081 + t * (4.776384997670288 + t * -5.435455855934631)))));
        const double g = 0.005407344544966578 + t * (1.404613529898575 + t * (0.214847559468213 + t * (-5.799100973351585 + t * (14.17993336680509 + t * (-13.74514537774601 + t * 4.645852612178535)))));
        const double b = 0.3340998053353061 + t * (1.384590162594685 + t * (0.09509516302823659 + t * (-19.33244095627987 + t * (56.69055260068105 + t * (-65.35303263337234 + t * 26.3124352495832)))));
        return rgb(r, g, b);
    }
    case Colormap::Turbo: {
        // Polynomial approximation of Google's Turbo (Apache-2.0).
        const double r = 0.13572138 + t * (4.61539260 + t * (-42.66032258 + t * (132.13108234 + t * (-152.94239396 + t * 59.28637943))));
        const double g = 0.09140261 + t * (2.19418839 + t * (4.84296658 + t * (-14.18503333 + t * (4.27729857 + t * 2.82956604))));
        const double b = 0.10667330 + t * (12.64194608 + t * (-60.58204836 + t * (110.36276771 + t * (-89.90310912 + t * 27.34824973))));
        return rgb(r, g, b);
    }
    case Colormap::Diverging: {
        // Blue - white - red.
        if (t < 0.5) {
            const double s = t / 0.5;
            return rgb(0.23 + 0.77 * s, 0.30 + 0.70 * s, 0.75 + 0.25 * s);
        }
        const double s = (t - 0.5) / 0.5;
        return rgb(1.0 - 0.29 * s, 1.0 - 0.98 * s, 1.0 - 0.85 * s);
    }
    }
    return rgb(t, t, t);
}

std::pair<double, double> autoRange(const Image2D& image, double lowFraction, double highFraction)
{
    if (image.isNull())
        return {0.0, 1.0};
    auto r = processing::percentileRange(image, lowFraction, highFraction);
    if (!std::isfinite(r.first) || !std::isfinite(r.second))
        return {0.0, 1.0};
    if (r.second <= r.first)
        r.second = r.first + 1.0;
    return r;
}

QImage renderScalar(const Image2D& image, double low, double high, Colormap map)
{
    if (image.isNull())
        return {};
    QImage img(image.width(), image.height(), QImage::Format_RGB888);
    QRgb lut[256];
    for (int i = 0; i < 256; ++i)
        lut[i] = colormapColor(map, i / 255.0).rgb();
    for (int y = 0; y < image.height(); ++y) {
        uchar* line = img.scanLine(y);
        for (int x = 0; x < image.width(); ++x) {
            const double v = image.at(x, y);
            QRgb c = qRgb(0, 0, 0);
            if (std::isfinite(v))
                c = lut[int(std::lround(stretch(v, low, high) * 255.0))];
            line[3 * x] = uchar(qRed(c));
            line[3 * x + 1] = uchar(qGreen(c));
            line[3 * x + 2] = uchar(qBlue(c));
        }
    }
    return img;
}

QImage renderComposite(const Image2D& r, const Image2D& g, const Image2D& b, std::array<Range, 3>& ranges, bool useGiven)
{
    if (r.isNull() || !r.sameSize(g) || !r.sameSize(b))
        return {};
    if (!useGiven)
        ranges = {autoRange(r), autoRange(g), autoRange(b)};
    const auto& rr = ranges[0];
    const auto& gr = ranges[1];
    const auto& br = ranges[2];
    QImage img(r.width(), r.height(), QImage::Format_RGB888);
    for (int y = 0; y < r.height(); ++y) {
        uchar* line = img.scanLine(y);
        for (int x = 0; x < r.width(); ++x) {
            line[3 * x] = uchar(std::lround(255.0 * stretch(r.at(x, y), rr.first, rr.second)));
            line[3 * x + 1] = uchar(std::lround(255.0 * stretch(g.at(x, y), gr.first, gr.second)));
            line[3 * x + 2] = uchar(std::lround(255.0 * stretch(b.at(x, y), br.first, br.second)));
        }
    }
    return img;
}

namespace {
QImage composeRgb(const Image2D& r, const Image2D& g, const Image2D& b)
{
    std::array<Range, 3> ranges;
    return renderComposite(r, g, b, ranges, false);
}
} // namespace

QImage renderFalseColour(const Cube& cube, int r, int g, int b)
{
    if (cube.isNull() || r < 0 || g < 0 || b < 0 || r >= cube.bands() || g >= cube.bands() || b >= cube.bands())
        return {};
    return composeRgb(cube.bandImage(r), cube.bandImage(g), cube.bandImage(b));
}

QImage renderPcaComposite(const Cube& cube, const Pca& pca)
{
    if (!pca.isValid() || cube.bands() < 3)
        return {};
    return composeRgb(pcaComponent(cube, pca, 0), pcaComponent(cube, pca, 1), pcaComponent(cube, pca, 2));
}

void pseudoTrueColourBands(const Cube& cube, int& r, int& g, int& b)
{
    const int B = cube.bands();
    if (cube.hasWavelengths()) {
        r = cube.nearestBand(610.0);
        g = cube.nearestBand(550.0);
        b = cube.nearestBand(490.0);
        return;
    }
    r = std::min(14, B - 1);
    g = std::min(8, B - 1);
    b = std::min(1, B - 1);
}

void maxContrastBands(const Cube& cube, int& r, int& g, int& b)
{
    const int B = cube.bands();
    r = g = b = 0;
    if (B < 3) {
        pseudoTrueColourBands(cube, r, g, b);
        return;
    }
    std::vector<qsizetype> idx;
    for (qsizetype p : samplePixels(cube.planeSize(), 50000))
        if (finiteSpectrum(cube, p))
            idx.push_back(p);
    QVector<double> mean(B, 0.0), sd(B, 0.0), corr(B * B, 0.0);
    for (qsizetype p : idx)
        for (int k = 0; k < B; ++k)
            mean[k] += cube.band(k)[p];
    for (double& m : mean)
        m /= std::max<size_t>(1, idx.size());
    for (qsizetype p : idx)
        for (int i = 0; i < B; ++i) {
            const double di = cube.band(i)[p] - mean[i];
            for (int j = 0; j <= i; ++j)
                corr[i * B + j] += di * (cube.band(j)[p] - mean[j]);
        }
    for (int i = 0; i < B; ++i)
        sd[i] = std::sqrt(std::max(corr[i * B + i], 1e-30));
    double best = std::numeric_limits<double>::max();
    for (int i = 0; i < B; ++i)
        for (int j = i + 1; j < B; ++j)
            for (int k = j + 1; k < B; ++k) {
                const double c = std::abs(corr[j * B + i] / (sd[i] * sd[j])) + std::abs(corr[k * B + i] / (sd[i] * sd[k]))
                                 + std::abs(corr[k * B + j] / (sd[j] * sd[k]));
                if (c < best) {
                    best = c;
                    // Longest wavelength (highest channel) on red.
                    r = k;
                    g = j;
                    b = i;
                }
            }
}

QVector<QPointF> galleryPositions(const Cube& cube, double x, double y, int gap)
{
    QVector<QPointF> out;
    if (cube.isNull())
        return out;
    const int gridCols = int(std::ceil(std::sqrt(double(cube.bands()))));
    for (int b = 0; b < cube.bands(); ++b)
        out << QPointF((b % gridCols) * (cube.cols() + gap) + x, (b / gridCols) * (cube.rows() + gap) + y);
    return out;
}

QImage renderBandGallery(const Cube& cube, int gap, Range* range, bool useGiven)
{
    const int B = cube.bands();
    if (cube.isNull())
        return {};
    const int gridCols = int(std::ceil(std::sqrt(double(B))));
    const int gridRows = (B + gridCols - 1) / gridCols;
    // One shared stretch, from a sample of all bands.
    std::vector<double> sample;
    for (int b = 0; b < B; ++b)
        for (qsizetype p : samplePixels(cube.planeSize(), 20000)) {
            const double v = cube.band(b)[p];
            if (std::isfinite(v))
                sample.push_back(v);
        }
    double lo = 0.0, hi = 1.0;
    if (range && useGiven) {
        lo = range->first;
        hi = range->second;
    } else if (!sample.empty()) {
        std::sort(sample.begin(), sample.end());
        lo = sample[size_t(0.005 * double(sample.size() - 1))];
        hi = sample[size_t(0.995 * double(sample.size() - 1))];
        if (hi <= lo)
            hi = lo + 1.0;
    }
    if (range && !useGiven)
        *range = {lo, hi};
    QImage img(gridCols * cube.cols() + (gridCols - 1) * gap, gridRows * cube.rows() + (gridRows - 1) * gap,
               QImage::Format_RGB888);
    img.fill(QColor(40, 40, 40));
    for (int b = 0; b < B; ++b) {
        const int ox = (b % gridCols) * (cube.cols() + gap), oy = (b / gridCols) * (cube.rows() + gap);
        const float* src = cube.band(b);
        for (int y = 0; y < cube.rows(); ++y) {
            uchar* line = img.scanLine(oy + y);
            for (int x = 0; x < cube.cols(); ++x) {
                const uchar v = uchar(std::lround(255.0 * stretch(src[qsizetype(y) * cube.cols() + x], lo, hi)));
                line[3 * (ox + x)] = line[3 * (ox + x) + 1] = line[3 * (ox + x) + 2] = v;
            }
        }
    }
    return img;
}

QColor classColor(int index)
{
    static const QColor colours[] = {
        QColor(0x1f, 0x77, 0xb4), QColor(0xff, 0x7f, 0x0e), QColor(0x2c, 0xa0, 0x2c), QColor(0xd6, 0x27, 0x28),
        QColor(0x94, 0x67, 0xbd), QColor(0x8c, 0x56, 0x4b), QColor(0xe3, 0x77, 0xc2), QColor(0x7f, 0x7f, 0x7f),
        QColor(0xbc, 0xbd, 0x22), QColor(0x17, 0xbe, 0xcf), QColor(0xae, 0xc7, 0xe8), QColor(0xff, 0xbb, 0x78),
    };
    constexpr int n = int(sizeof(colours) / sizeof(colours[0]));
    return colours[((index % n) + n) % n];
}

QImage renderClasses(const Clustering& c, int cols, int rows)
{
    if (!c.isValid() || c.labels.size() != qsizetype(cols) * rows)
        return {};
    QImage img(cols, rows, QImage::Format_RGB888);
    for (int y = 0; y < rows; ++y) {
        uchar* line = img.scanLine(y);
        for (int x = 0; x < cols; ++x) {
            const int l = c.labels[qsizetype(y) * cols + x];
            const QColor col = l >= 0 ? classColor(l) : QColor(Qt::black);
            line[3 * x] = uchar(col.red());
            line[3 * x + 1] = uchar(col.green());
            line[3 * x + 2] = uchar(col.blue());
        }
    }
    return img;
}

QImage renderMaskOverlay(const QImage& baseIn, const QVector<quint8>& mask, const QColor& color, double opacity)
{
    if (baseIn.isNull() || mask.size() != qsizetype(baseIn.width()) * baseIn.height())
        return baseIn;
    QImage img = baseIn.convertToFormat(QImage::Format_RGB888);
    for (int y = 0; y < img.height(); ++y) {
        uchar* line = img.scanLine(y);
        for (int x = 0; x < img.width(); ++x) {
            if (!mask[qsizetype(y) * img.width() + x])
                continue;
            line[3 * x] = uchar(std::lround(line[3 * x] * (1 - opacity) + color.red() * opacity));
            line[3 * x + 1] = uchar(std::lround(line[3 * x + 1] * (1 - opacity) + color.green() * opacity));
            line[3 * x + 2] = uchar(std::lround(line[3 * x + 2] * (1 - opacity) + color.blue() * opacity));
        }
    }
    return img;
}

} // namespace iat::hsi
