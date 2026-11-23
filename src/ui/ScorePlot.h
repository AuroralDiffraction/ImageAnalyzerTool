#pragma once

#include "core/Image2D.h"

#include <QColor>
#include <QImage>
#include <QPolygonF>
#include <QWidget>

namespace iat {

// Scatter of two per-pixel scores (e.g. principal components 1 and 2) as a
// density plot: every pixel of the image is a point, counted into a grid and
// shown on a log scale. A loop drawn with the left button selects the pixels
// inside it (selectionDrawn, in score units); right-click or Esc clears it.
// Groups of pixels (the kept areas) can be drawn on top in their colour.
class ScorePlot : public QWidget {
    Q_OBJECT
public:
    struct Group {
        QColor color;
        QVector<QPointF> points;  // score pairs
    };

    explicit ScorePlot(QWidget* parent = nullptr);

    void setScores(const Image2D& x, const Image2D& y, const QString& xLabel, const QString& yLabel);
    void clearScores();
    bool hasScores() const { return !m_density.isNull(); }
    void setGroups(const QList<Group>& groups);
    void setSelection(const QPolygonF& polygon);  // in score units; empty: none
    QPolygonF selection() const { return m_selection; }
    void setMarker(double x, double y);           // NaN: none
    void setPlaceholder(const QString& text);
    // Score units ↔ widget coordinates (for tests).
    QPointF toWidget(const QPointF& score) const;
    QPointF toScore(const QPointF& widget) const;
    QRectF plotRect() const;

signals:
    void selectionDrawn(const QPolygonF& polygon);
    void selectionCleared();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    QSize sizeHint() const override { return {320, 260}; }
    QSize minimumSizeHint() const override { return {160, 140}; }

private:
    QImage m_density;  // kBins × kBins, y up
    QRectF m_range;    // score range shown: x from left to right, y from top() (low) to bottom() (high)
    QString m_xLabel, m_yLabel, m_placeholder;
    QList<Group> m_groups;
    QPolygonF m_selection;
    QPolygonF m_drawing;  // widget coordinates while a loop is drawn
    bool m_drawingActive = false;
    double m_markerX, m_markerY;
};

} // namespace iat
