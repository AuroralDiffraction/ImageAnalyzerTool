#include "ui/ScorePlot.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>
#include <limits>

namespace iat {

namespace {

constexpr int kBins = 256;

// Value at fraction q of the sorted finite values.
double quantile(QVector<float>& v, double q)
{
    if (v.isEmpty())
        return 0.0;
    const qsizetype k = std::clamp<qsizetype>(qsizetype(q * double(v.size() - 1)), 0, v.size() - 1);
    std::nth_element(v.begin(), v.begin() + k, v.end());
    return v[k];
}

// Dark blue → teal → yellow (like viridis), t in [0, 1].
QRgb densityColour(double t)
{
    static const double stops[5][3] = {{68, 1, 84}, {59, 82, 139}, {33, 145, 140}, {94, 201, 98}, {253, 231, 37}};
    t = std::clamp(t, 0.0, 1.0) * 4.0;
    const int i = std::min(3, int(t));
    const double f = t - i;
    auto mix = [&](int c) { return int(std::lround(stops[i][c] + f * (stops[i + 1][c] - stops[i][c]))); };
    return qRgb(mix(0), mix(1), mix(2));
}

QString tickText(double v)
{
    return QString::number(v, 'g', 3);
}

} // namespace

ScorePlot::ScorePlot(QWidget* parent)
    : QWidget(parent)
    , m_markerX(std::numeric_limits<double>::quiet_NaN())
    , m_markerY(std::numeric_limits<double>::quiet_NaN())
{
    setFocusPolicy(Qt::ClickFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

void ScorePlot::setScores(const Image2D& x, const Image2D& y, const QString& xLabel, const QString& yLabel)
{
    m_xLabel = xLabel;
    m_yLabel = yLabel;
    m_density = QImage();
    if (x.isNull() || !x.sameSize(y)) {
        update();
        return;
    }
    QVector<float> xs, ys;
    xs.reserve(x.pixelCount());
    ys.reserve(x.pixelCount());
    for (qsizetype i = 0; i < x.pixelCount(); ++i) {
        const double a = x.constData()[i], b = y.constData()[i];
        if (std::isfinite(a) && std::isfinite(b)) {
            xs << float(a);
            ys << float(b);
        }
    }
    if (xs.isEmpty()) {
        update();
        return;
    }
    // A robust range (0.1–99.9 %) with a margin, so a few outliers don't
    // squeeze the rest into a corner.
    QVector<float> tx = xs, ty = ys;
    double x0 = quantile(tx, 0.001), x1 = quantile(tx, 0.999);
    double y0 = quantile(ty, 0.001), y1 = quantile(ty, 0.999);
    if (!(x1 > x0)) { x0 -= 1.0; x1 += 1.0; }
    if (!(y1 > y0)) { y0 -= 1.0; y1 += 1.0; }
    const double mx = 0.05 * (x1 - x0), my = 0.05 * (y1 - y0);
    m_range = QRectF(QPointF(x0 - mx, y0 - my), QPointF(x1 + mx, y1 + my));

    QVector<int> counts(kBins * kBins, 0);
    for (qsizetype i = 0; i < xs.size(); ++i) {
        const int bx = int((xs[i] - m_range.left()) / m_range.width() * kBins);
        const int by = int((ys[i] - m_range.top()) / m_range.height() * kBins);
        if (bx >= 0 && bx < kBins && by >= 0 && by < kBins)
            ++counts[by * kBins + bx];
    }
    const int maxCount = *std::max_element(counts.begin(), counts.end());
    m_density = QImage(kBins, kBins, QImage::Format_ARGB32);
    m_density.fill(Qt::transparent);
    const double norm = std::log1p(double(std::max(1, maxCount)));
    for (int by = 0; by < kBins; ++by) {
        QRgb* line = reinterpret_cast<QRgb*>(m_density.scanLine(kBins - 1 - by));  // y up
        for (int bx = 0; bx < kBins; ++bx)
            if (const int c = counts[by * kBins + bx])
                line[bx] = densityColour(0.15 + 0.85 * std::log1p(double(c)) / norm);
    }
    update();
}

void ScorePlot::clearScores()
{
    m_density = QImage();
    m_groups.clear();
    m_selection.clear();
    update();
}

void ScorePlot::setGroups(const QList<Group>& groups)
{
    m_groups = groups;
    update();
}

void ScorePlot::setSelection(const QPolygonF& polygon)
{
    m_selection = polygon;
    update();
}

void ScorePlot::setMarker(double x, double y)
{
    m_markerX = x;
    m_markerY = y;
    update();
}

void ScorePlot::setPlaceholder(const QString& text)
{
    m_placeholder = text;
    update();
}

QRectF ScorePlot::plotRect() const
{
    const QFontMetrics fm(font());
    const double left = fm.horizontalAdvance(QStringLiteral("−0.000")) + fm.height() + 10;
    const double bottom = 2 * fm.height() + 8;
    return QRectF(left, 8, std::max(10.0, width() - left - 10), std::max(10.0, height() - bottom - 8));
}

QPointF ScorePlot::toWidget(const QPointF& s) const
{
    const QRectF p = plotRect();
    return {p.left() + (s.x() - m_range.left()) / m_range.width() * p.width(),
            p.bottom() - (s.y() - m_range.top()) / m_range.height() * p.height()};
}

QPointF ScorePlot::toScore(const QPointF& w) const
{
    const QRectF p = plotRect();
    return {m_range.left() + (w.x() - p.left()) / p.width() * m_range.width(),
            m_range.top() + (p.bottom() - w.y()) / p.height() * m_range.height()};
}

void ScorePlot::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().color(QPalette::Base));
    const QRectF p = plotRect();
    if (m_density.isNull()) {
        painter.setPen(palette().color(QPalette::PlaceholderText));
        painter.drawText(rect().adjusted(8, 8, -8, -8), Qt::AlignCenter | Qt::TextWordWrap, m_placeholder);
        return;
    }
    painter.setPen(palette().color(QPalette::Mid));
    painter.drawRect(p);
    painter.save();
    painter.setClipRect(p);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawImage(p, m_density);
    painter.setRenderHint(QPainter::Antialiasing);
    // The kept areas, in their colours.
    for (const Group& g : std::as_const(m_groups)) {
        QColor c = g.color;
        c.setAlpha(200);
        painter.setPen(Qt::NoPen);
        painter.setBrush(c);
        const qsizetype step = std::max<qsizetype>(1, g.points.size() / 4000);  // at most ~4000 dots
        for (qsizetype i = 0; i < g.points.size(); i += step)
            painter.drawEllipse(toWidget(g.points[i]), 1.8, 1.8);
    }
    // The selection loop.
    auto drawLoop = [&](const QPolygonF& poly) {
        QPen pen(QColor(255, 0, 200), 1.5);
        painter.setPen(pen);
        painter.setBrush(QColor(255, 0, 200, 40));
        painter.drawPolygon(poly);
    };
    if (m_drawingActive && m_drawing.size() > 1)
        drawLoop(m_drawing);
    else if (m_selection.size() > 2) {
        QPolygonF w;
        for (const QPointF& s : m_selection)
            w << toWidget(s);
        drawLoop(w);
    }
    // The pixel under the mouse in the image.
    if (std::isfinite(m_markerX) && std::isfinite(m_markerY)) {
        const QPointF m = toWidget(QPointF(m_markerX, m_markerY));
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::white, 3));
        painter.drawEllipse(m, 5, 5);
        painter.setPen(QPen(Qt::black, 1.5));
        painter.drawEllipse(m, 5, 5);
    }
    painter.restore();

    // Axes: range ends and zero, then titles.
    const QFontMetrics fm(font());
    painter.setPen(palette().color(QPalette::Text));
    auto xTick = [&](double v) {
        const double wx = toWidget(QPointF(v, m_range.top())).x();
        painter.drawLine(QPointF(wx, p.bottom()), QPointF(wx, p.bottom() + 3));
        const QString t = tickText(v);
        const double tw = fm.horizontalAdvance(t);
        painter.drawText(QPointF(std::clamp(wx - tw / 2, 0.0, width() - tw), p.bottom() + 3 + fm.ascent()), t);
    };
    auto yTick = [&](double v) {
        const double wy = toWidget(QPointF(m_range.left(), v)).y();
        painter.drawLine(QPointF(p.left() - 3, wy), QPointF(p.left(), wy));
        const QString t = tickText(v);
        painter.drawText(QPointF(p.left() - 5 - fm.horizontalAdvance(t), wy + fm.ascent() / 2.0 - 1), t);
    };
    xTick(m_range.left());
    xTick(m_range.right());
    yTick(m_range.top());
    yTick(m_range.bottom());
    if (m_range.left() < 0 && m_range.right() > 0) {
        xTick(0.0);
        painter.setPen(QPen(palette().color(QPalette::Mid), 1, Qt::DotLine));
        const double wx = toWidget(QPointF(0, 0)).x();
        painter.drawLine(QPointF(wx, p.top()), QPointF(wx, p.bottom()));
        painter.setPen(palette().color(QPalette::Text));
    }
    if (m_range.top() < 0 && m_range.bottom() > 0) {
        yTick(0.0);
        painter.setPen(QPen(palette().color(QPalette::Mid), 1, Qt::DotLine));
        const double wy = toWidget(QPointF(0, 0)).y();
        painter.drawLine(QPointF(p.left(), wy), QPointF(p.right(), wy));
        painter.setPen(palette().color(QPalette::Text));
    }
    painter.drawText(QRectF(p.left(), height() - fm.height() - 2, p.width(), fm.height()), Qt::AlignCenter, m_xLabel);
    painter.save();
    painter.translate(fm.height() / 2.0 + 1, p.center().y());
    painter.rotate(-90);
    painter.drawText(QRectF(-p.height() / 2, -fm.height() / 2.0, p.height(), fm.height()), Qt::AlignCenter, m_yLabel);
    painter.restore();
}

void ScorePlot::mousePressEvent(QMouseEvent* event)
{
    if (m_density.isNull())
        return;
    if (event->button() == Qt::RightButton) {
        if (!m_selection.isEmpty()) {
            m_selection.clear();
            update();
            emit selectionCleared();
        }
        return;
    }
    if (event->button() == Qt::LeftButton && plotRect().contains(event->position())) {
        m_drawingActive = true;
        m_drawing = QPolygonF() << event->position();
        update();
    }
}

void ScorePlot::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_drawingActive)
        return;
    const QRectF p = plotRect();
    const QPointF at(std::clamp(event->position().x(), p.left(), p.right()), std::clamp(event->position().y(), p.top(), p.bottom()));
    if (QLineF(m_drawing.last(), at).length() >= 2.0) {
        m_drawing << at;
        update();
    }
}

void ScorePlot::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_drawingActive || event->button() != Qt::LeftButton)
        return;
    m_drawingActive = false;
    QPolygonF loop;
    for (const QPointF& w : std::as_const(m_drawing))
        loop << toScore(w);
    m_drawing.clear();
    if (loop.size() < 3) {  // a click, not a loop
        update();
        return;
    }
    m_selection = loop;
    update();
    emit selectionDrawn(loop);
}

void ScorePlot::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape && !m_selection.isEmpty()) {
        m_selection.clear();
        update();
        emit selectionCleared();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace iat
