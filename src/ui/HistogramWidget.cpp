#include "ui/HistogramWidget.h"

#include <QFontMetricsF>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace iat {

namespace {
constexpr double kMarkerGrabPx = 6.0;

QString formatValue(double v) { return QString::number(v, 'g', 6); }
} // namespace

HistogramWidget::HistogramWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(120);
}

void HistogramWidget::setHistogram(const Histogram& histogram)
{
    m_hist = histogram;
    update();
}

void HistogramWidget::clear()
{
    m_hist = Histogram();
    update();
}

void HistogramWidget::setDisplayRange(double low, double high)
{
    m_low = low;
    m_high = high;
    update();
}

void HistogramWidget::setLogScale(bool on)
{
    m_log = on;
    update();
}

QFont HistogramWidget::labelFont() const
{
    QFont f = font();
    f.setPointSizeF(std::max(7.0, f.pointSizeF() - 1.0));
    return f;
}

QRectF HistogramWidget::plotRectFor(const QRectF& area, const QFont& labelFont)
{
    const double labelH = QFontMetricsF(labelFont).height();
    const double stripH = std::max(6.0, labelH * 0.45);
    const double bottom = stripH + labelH + 6;
    return QRectF(area.left() + 6, area.top() + 4, std::max(10.0, area.width() - 12),
                  std::max(10.0, area.height() - 4 - bottom));
}

QRectF HistogramWidget::plotRect() const { return plotRectFor(rect(), labelFont()); }

double HistogramWidget::valueToX(double value) const
{
    const QRectF r = plotRect();
    if (m_hist.isEmpty() || !(m_hist.hi > m_hist.lo))
        return r.left();
    return r.left() + (value - m_hist.lo) / (m_hist.hi - m_hist.lo) * r.width();
}

double HistogramWidget::xToValue(double x) const
{
    const QRectF r = plotRect();
    return m_hist.lo + (x - r.left()) / r.width() * (m_hist.hi - m_hist.lo);
}

HistogramWidget::Marker HistogramWidget::markerNear(double x) const
{
    if (m_hist.isEmpty())
        return Marker::None;
    const double dLow = std::abs(x - valueToX(m_low));
    const double dHigh = std::abs(x - valueToX(m_high));
    if (dLow > kMarkerGrabPx && dHigh > kMarkerGrabPx)
        return Marker::None;
    return dLow <= dHigh ? Marker::Low : Marker::High;
}

void HistogramWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    paint(p, rect(), m_hist, m_low, m_high, m_log, palette(), labelFont());
}

double HistogramWidget::paintAsShown(QPainter& p, const QPointF& topLeft, double width) const
{
    // The size it has in the panel (or its preferred size if never laid out).
    const QSizeF shown = size().isEmpty() ? QSizeF(sizeHint()) : QSizeF(size());
    const double scale = width / shown.width();
    p.save();
    p.translate(topLeft);
    p.scale(scale, scale);  // vector painting, so lines and text stay sharp
    paint(p, QRectF(QPointF(0, 0), shown), m_hist, m_low, m_high, m_log, palette(), labelFont());
    p.restore();
    return shown.height() * scale;
}

void HistogramWidget::paint(QPainter& p, const QRectF& area, const Histogram& h, double low, double high,
                            bool logScale, const QPalette& pal, const QFont& labelFont)
{
    p.save();
    const QRectF plot = plotRectFor(area, labelFont);
    const double labelH = QFontMetricsF(labelFont).height();
    const double stripH = std::max(6.0, labelH * 0.45);
    const double unit = std::max(1.0, labelH / 14.0);  // scales markers with the font

    p.fillRect(area, pal.color(QPalette::Base));
    p.setPen(pal.color(QPalette::Mid));
    p.setBrush(Qt::NoBrush);
    p.drawRect(plot.adjusted(-0.5, -0.5, 0.5, 0.5));

    if (h.isEmpty()) {
        p.setPen(pal.color(QPalette::PlaceholderText));
        p.setFont(labelFont);
        p.drawText(plot, Qt::AlignCenter, tr("No image"));
        p.restore();
        return;
    }

    auto valueToX = [&](double v) { return plot.left() + (v - h.lo) / (h.hi - h.lo) * plot.width(); };

    // Bars as one filled step outline.
    const double maxCount = double(std::max<qint64>(1, h.maxCount));
    const double logMax = std::log10(1.0 + maxCount);
    auto barHeight = [&](qint64 count) {
        const double f = logScale ? std::log10(1.0 + double(count)) / logMax : double(count) / maxCount;
        return f * plot.height();
    };

    QPainterPath path;
    path.moveTo(plot.left(), plot.bottom());
    const double binW = h.binWidth();
    for (int i = 0; i < h.binCount(); ++i) {
        const double x0 = valueToX(h.lo + i * binW);
        const double x1 = valueToX(h.lo + (i + 1) * binW);
        const double y = plot.bottom() - barHeight(h.counts[i]);
        path.lineTo(x0, y);
        path.lineTo(x1, y);
    }
    path.lineTo(plot.right(), plot.bottom());
    path.closeSubpath();

    QColor barColor = pal.color(QPalette::Text);
    barColor.setAlphaF(0.65f);
    p.save();
    p.setClipRect(plot);
    p.fillPath(path, barColor);

    // Dim the parts outside the displayed range.
    const double xLow = std::clamp(valueToX(low), plot.left(), plot.right());
    const double xHigh = std::clamp(valueToX(high), plot.left(), plot.right());
    QColor dim = pal.color(QPalette::Base);
    dim.setAlphaF(0.6f);
    p.fillRect(QRectF(plot.left(), plot.top(), xLow - plot.left(), plot.height()), dim);
    p.fillRect(QRectF(xHigh, plot.top(), plot.right() - xHigh, plot.height()), dim);
    p.restore();

    // Markers.
    const QColor accent = pal.color(QPalette::Highlight);
    p.setPen(QPen(accent, 1.5 * unit));
    p.setBrush(accent);
    for (double x : {xLow, xHigh}) {
        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        QPolygonF handle;
        handle << QPointF(x - 4 * unit, plot.top()) << QPointF(x + 4 * unit, plot.top())
               << QPointF(x, plot.top() + 6 * unit);
        p.drawPolygon(handle);
    }

    // Grey-level strip: black up to the low marker, white from the high marker.
    const QRectF strip(plot.left(), plot.bottom() + 3, plot.width(), stripH);
    QLinearGradient g(QPointF(xLow, 0), QPointF(std::max(xHigh, xLow + 0.5), 0));
    g.setColorAt(0.0, Qt::black);
    g.setColorAt(1.0, Qt::white);
    g.setSpread(QGradient::PadSpread);
    p.setPen(Qt::NoPen);
    p.fillRect(strip, g);
    p.setPen(pal.color(QPalette::Mid));
    p.setBrush(Qt::NoBrush);
    p.drawRect(strip);

    // Data range labels.
    p.setPen(pal.color(QPalette::WindowText));
    p.setFont(labelFont);
    const QRectF labels(plot.left(), strip.bottom() + 2, plot.width(), labelH);
    p.drawText(labels, Qt::AlignLeft | Qt::AlignVCenter, formatValue(h.dataMin));
    p.drawText(labels, Qt::AlignRight | Qt::AlignVCenter, formatValue(h.dataMax));
    p.drawText(labels, Qt::AlignHCenter | Qt::AlignVCenter,
               logScale ? tr("pixel count (log)") : tr("pixel count"));
    p.restore();
}

void HistogramWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_drag = markerNear(event->position().x());
        if (m_drag != Marker::None) {
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void HistogramWidget::mouseMoveEvent(QMouseEvent* event)
{
    const double x = event->position().x();
    if (m_drag == Marker::None) {
        setCursor(markerNear(x) != Marker::None ? Qt::SizeHorCursor : Qt::ArrowCursor);
        return;
    }

    const double v = std::clamp(xToValue(x), m_hist.dataMin, m_hist.dataMax);
    // Keep the markers in order; the control that receives the signal
    // enforces the minimum gap.
    double low = m_low;
    double high = m_high;
    if (m_drag == Marker::Low)
        low = std::min(v, high);
    else
        high = std::max(v, low);
    emit displayRangeEdited(low, high);
}

void HistogramWidget::mouseReleaseEvent(QMouseEvent* event)
{
    m_drag = Marker::None;
    QWidget::mouseReleaseEvent(event);
}

} // namespace iat
