#include "ui/SpectrumPlot.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>
#include <limits>

namespace iat {

namespace {

// "Nice" tick step for a range: 1, 2 or 5 × 10^n.
double niceStep(double range, int targetTicks)
{
    if (!(range > 0.0))
        return 1.0;
    const double raw = range / std::max(1, targetTicks);
    const double mag = std::pow(10.0, std::floor(std::log10(raw)));
    const double f = raw / mag;
    return (f < 1.5 ? 1.0 : f < 3.5 ? 2.0 : f < 7.5 ? 5.0 : 10.0) * mag;
}

QString tickLabel(double v, double step)
{
    const int decimals = step >= 1.0 ? 0 : std::min(6, int(std::ceil(-std::log10(step))));
    return QString::number(v, 'f', decimals);
}

} // namespace

SpectrumPlot::SpectrumPlot(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    m_yLabel = tr("Value");
}

void SpectrumPlot::setSeries(const QList<Series>& series)
{
    m_series = series;
    update();
}

void SpectrumPlot::setWavelengths(const QVector<double>& wavelengths)
{
    m_wavelengths = wavelengths;
    update();
}

void SpectrumPlot::setShowWavelength(bool on)
{
    m_showWavelength = on;
    update();
}

void SpectrumPlot::setNormalized(bool on)
{
    m_normalized = on;
    update();
}

void SpectrumPlot::setYLabel(const QString& label)
{
    m_yLabel = label;
    update();
}

void SpectrumPlot::setShowLegend(bool on)
{
    m_showLegend = on;
    update();
}

void SpectrumPlot::setShowMarkers(bool on)
{
    m_showMarkers = on;
    update();
}

void SpectrumPlot::setPlaceholder(const QString& text)
{
    m_placeholder = text;
    update();
}

int SpectrumPlot::bands() const
{
    int n = 0;
    for (const Series& s : m_series)
        n = std::max(n, int(s.values.size()));
    return n;
}

double SpectrumPlot::xOf(int band) const
{
    return showsWavelength() && band < m_wavelengths.size() ? m_wavelengths[band] : double(band + 1);
}

QRectF SpectrumPlot::plotRect() const
{
    const QFontMetrics fm(font());
    const double left = fm.horizontalAdvance(QStringLiteral("0.0000")) + fm.height() + 10;
    const double bottom = 2 * fm.height() + 10;
    return QRectF(left, fm.height() * 0.6, width() - left - 10, height() - bottom - fm.height() * 0.6);
}

void SpectrumPlot::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QPalette pal = palette();
    p.fillRect(rect(), pal.color(QPalette::Base));
    const QRectF pr = plotRect();
    const QFontMetrics fm(font());
    const QColor axis = pal.color(QPalette::Text);
    QColor grid = pal.color(QPalette::Mid);
    grid.setAlpha(90);

    const int n = bands();
    if (n == 0 || pr.width() < 20 || pr.height() < 20) {
        p.setPen(pal.color(QPalette::PlaceholderText));
        p.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap,
                   !m_placeholder.isEmpty() ? m_placeholder
                                            : tr("Hover over the image for the spectrum of a pixel;\nclick or drag with the spectrum tool to keep one."));
        return;
    }

    // Values to draw (normalised if asked) and their range.
    QList<QVector<double>> values, lows, highs;
    double yMin = std::numeric_limits<double>::max(), yMax = -std::numeric_limits<double>::max();
    for (const Series& s : m_series) {
        double scale = 1.0;
        if (m_normalized) {
            double m = 0.0;
            for (double v : s.values)
                if (std::isfinite(v))
                    m = std::max(m, std::abs(v));
            scale = m > 0.0 ? 1.0 / m : 1.0;
        }
        QVector<double> v, lo, hi;
        for (int b = 0; b < s.values.size(); ++b) {
            const double x = s.values[b] * scale;
            const double sd = b < s.std.size() ? s.std[b] * scale : 0.0;
            v << x;
            lo << x - sd;
            hi << x + sd;
            if (std::isfinite(x)) {
                yMin = std::min(yMin, x - sd);
                yMax = std::max(yMax, x + sd);
            }
        }
        values << v;
        lows << lo;
        highs << hi;
    }
    if (yMin > yMax) {
        yMin = 0.0;
        yMax = 1.0;
    }
    if (yMin > 0.0 && yMin < 0.35 * yMax)
        yMin = 0.0;  // start at zero when the data is not far from it
    if (yMax - yMin < 1e-12)
        yMax = yMin + 1.0;
    const double pad = 0.05 * (yMax - yMin);
    yMax += pad;
    if (yMin != 0.0)
        yMin -= pad;

    const double xMin = xOf(0), xMax = std::max(xOf(n - 1), xMin + 1e-9);
    const double xPad = n > 1 ? 0.03 * (xMax - xMin) : 0.5;
    auto X = [&](double x) { return pr.left() + (x - (xMin - xPad)) / (xMax - xMin + 2 * xPad) * pr.width(); };
    auto Y = [&](double y) { return pr.bottom() - (y - yMin) / (yMax - yMin) * pr.height(); };

    // Grid and axes.
    p.setPen(grid);
    const double ys = niceStep(yMax - yMin, 5);
    for (double y = std::ceil(yMin / ys) * ys; y <= yMax + 1e-12; y += ys) {
        p.drawLine(QPointF(pr.left(), Y(y)), QPointF(pr.right(), Y(y)));
    }
    const double xs = showsWavelength() ? niceStep(xMax - xMin, 6) : std::max(1.0, niceStep(n, 8));
    for (double x = std::ceil((xMin - xPad) / xs) * xs; x <= xMax + xPad + 1e-9; x += xs)
        p.drawLine(QPointF(X(x), pr.top()), QPointF(X(x), pr.bottom()));
    p.setPen(axis);
    p.drawRect(pr);
    for (double y = std::ceil(yMin / ys) * ys; y <= yMax + 1e-12; y += ys) {
        const QString t = tickLabel(std::abs(y) < ys * 1e-6 ? 0.0 : y, ys);
        p.drawText(QRectF(0, Y(y) - fm.height() / 2.0, pr.left() - 4, fm.height()), Qt::AlignRight | Qt::AlignVCenter, t);
    }
    for (double x = std::ceil((xMin - xPad) / xs) * xs; x <= xMax + xPad + 1e-9; x += xs) {
        const QString t = tickLabel(x, xs);
        p.drawText(QRectF(X(x) - 30, pr.bottom() + 2, 60, fm.height()), Qt::AlignHCenter | Qt::AlignTop, t);
    }
    p.drawText(QRectF(pr.left(), pr.bottom() + fm.height() + 4, pr.width(), fm.height()), Qt::AlignHCenter,
               showsWavelength() ? tr("Wavelength (nm)") : tr("Band"));
    p.save();
    p.translate(fm.height() * 0.2, pr.center().y());
    p.rotate(-90);
    p.drawText(QRectF(-pr.height() / 2, 0, pr.height(), fm.height()), Qt::AlignHCenter,
               m_normalized ? tr("Normalised") : m_yLabel);
    p.restore();

    // Curves.
    p.setClipRect(pr.adjusted(-2, -2, 2, 2));
    for (int i = 0; i < m_series.size(); ++i) {
        const Series& s = m_series[i];
        if (!s.std.isEmpty()) {
            QPainterPath band;
            bool started = false;
            for (int b = 0; b < values[i].size(); ++b) {
                if (!std::isfinite(highs[i][b]))
                    continue;
                const QPointF pt(X(xOf(b)), Y(highs[i][b]));
                started ? band.lineTo(pt) : band.moveTo(pt);
                started = true;
            }
            for (int b = int(values[i].size()) - 1; b >= 0; --b)
                if (std::isfinite(lows[i][b]))
                    band.lineTo(QPointF(X(xOf(b)), Y(lows[i][b])));
            QColor fill = s.color;
            fill.setAlpha(45);
            p.fillPath(band, fill);
        }
        QPainterPath line;
        bool started = false;
        for (int b = 0; b < values[i].size(); ++b) {
            if (!std::isfinite(values[i][b])) {
                started = false;
                continue;
            }
            const QPointF pt(X(xOf(b)), Y(values[i][b]));
            started ? line.lineTo(pt) : line.moveTo(pt);
            started = true;
        }
        QPen pen(s.color, 1.8);
        if (s.dashed)
            pen.setStyle(Qt::DashLine);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPath(line);
        if (m_showMarkers) {
            p.setBrush(s.color);
            p.setPen(Qt::NoPen);
            for (int b = 0; b < values[i].size(); ++b)
                if (std::isfinite(values[i][b]))
                    p.drawEllipse(QPointF(X(xOf(b)), Y(values[i][b])), 2.2, 2.2);
        }
    }
    p.setClipping(false);

    // Legend (top right).
    double ly = pr.top() + 4;
    for (const Series& s : m_showLegend ? m_series : QList<Series>()) {
        const QString name = fm.elidedText(s.name, Qt::ElideRight, int(pr.width() * 0.5));
        const double w = fm.horizontalAdvance(name) + 26;
        const QRectF box(pr.right() - w - 4, ly, w, fm.height() + 2);
        QColor bg = pal.color(QPalette::Base);
        bg.setAlpha(200);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRect(box);
        QPen pen(s.color, 2.0);
        if (s.dashed)
            pen.setStyle(Qt::DashLine);
        p.setPen(pen);
        p.drawLine(QPointF(box.left() + 3, box.center().y()), QPointF(box.left() + 19, box.center().y()));
        p.setPen(axis);
        p.drawText(box.adjusted(23, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, name);
        ly += fm.height() + 3;
        if (ly > pr.bottom() - fm.height())
            break;
    }

    // Read-out at the band under the mouse.
    if (m_hoverBand >= 0 && m_hoverBand < n) {
        const double x = X(xOf(m_hoverBand));
        QColor c = axis;
        c.setAlpha(110);
        p.setPen(QPen(c, 1.0, Qt::DotLine));
        p.drawLine(QPointF(x, pr.top()), QPointF(x, pr.bottom()));
        QStringList lines;
        lines << (showsWavelength() ? tr("Band %1 · %2 nm").arg(m_hoverBand + 1).arg(xOf(m_hoverBand), 0, 'f', 0)
                                    : tr("Band %1").arg(m_hoverBand + 1));
        for (int i = 0; i < (m_showLegend ? m_series.size() : 0); ++i)
            if (m_hoverBand < m_series[i].values.size())
                lines << QStringLiteral("%1: %2").arg(m_series[i].name).arg(m_series[i].values[m_hoverBand], 0, 'g', 5);
        const QString text = lines.join(QLatin1Char('\n'));
        const QRectF tb = fm.boundingRect(QRect(0, 0, 400, 400), 0, text).adjusted(-5, -3, 5, 3);
        QRectF box(x + 8, pr.top() + 4, tb.width(), tb.height());
        if (box.right() > pr.right())
            box.moveRight(x - 8);
        QColor bg = pal.color(QPalette::ToolTipBase);
        bg.setAlpha(230);
        p.setPen(pal.color(QPalette::Mid));
        p.setBrush(bg);
        p.drawRoundedRect(box, 3, 3);
        p.setPen(pal.color(QPalette::ToolTipText));
        p.drawText(box.adjusted(5, 3, -5, -3), Qt::AlignLeft | Qt::AlignTop, text);
    }
}

void SpectrumPlot::mouseMoveEvent(QMouseEvent* event)
{
    const QRectF pr = plotRect();
    const int n = bands();
    int best = -1;
    if (n > 0 && pr.contains(event->position())) {
        const double xMin = xOf(0), xMax = std::max(xOf(n - 1), xMin + 1e-9);
        const double xPad = n > 1 ? 0.03 * (xMax - xMin) : 0.5;
        const double x = (xMin - xPad) + (event->position().x() - pr.left()) / pr.width() * (xMax - xMin + 2 * xPad);
        double d = std::numeric_limits<double>::max();
        for (int b = 0; b < n; ++b)
            if (std::abs(xOf(b) - x) < d) {
                d = std::abs(xOf(b) - x);
                best = b;
            }
    }
    if (best != m_hoverBand) {
        m_hoverBand = best;
        update();
    }
}

void SpectrumPlot::leaveEvent(QEvent*)
{
    m_hoverBand = -1;
    update();
}

} // namespace iat
