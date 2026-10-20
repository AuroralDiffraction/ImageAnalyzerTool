#pragma once

#include "core/Histogram.h"

#include <QWidget>

namespace iat {

// Draws a histogram with two markers for the displayed value range.
// The markers can be dragged with the mouse; the region outside them is
// dimmed, and a strip underneath shows how values map to grey levels.
class HistogramWidget : public QWidget {
    Q_OBJECT
public:
    explicit HistogramWidget(QWidget* parent = nullptr);

    void setHistogram(const Histogram& histogram);
    void clear();
    void setDisplayRange(double low, double high);
    void setLogScale(bool on);
    bool logScale() const { return m_log; }

    // Paints a histogram into `area` with the given colours and label font.
    static void paint(QPainter& p, const QRectF& area, const Histogram& h, double low, double high,
                      bool logScale, const QPalette& palette, const QFont& labelFont);

    // Paints the widget exactly as it looks on screen (same colours, fonts,
    // proportions, markers and log setting), scaled to `width` pixels wide,
    // with its top-left corner at `topLeft`. Returns the height used.
    double paintAsShown(QPainter& p, const QPointF& topLeft, double width) const;

    QSize sizeHint() const override { return {320, 120}; }
    QSize minimumSizeHint() const override { return {120, 80}; }

signals:
    // Emitted while the user drags a marker.
    void displayRangeEdited(double low, double high);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    enum class Marker { None, Low, High };

    static QRectF plotRectFor(const QRectF& area, const QFont& labelFont);
    QFont labelFont() const;
    QRectF plotRect() const;
    double valueToX(double value) const;
    double xToValue(double x) const;
    Marker markerNear(double x) const;

    Histogram m_hist;
    double m_low = 0.0;
    double m_high = 1.0;
    bool m_log = false;
    Marker m_drag = Marker::None;
};

} // namespace iat
