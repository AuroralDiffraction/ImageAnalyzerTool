#pragma once

#include <QGraphicsView>
#include <QImage>
#include <QPixmap>
#include <QPointF>
#include <QColor>
#include <QList>
#include <QString>

class QGraphicsPixmapItem;

namespace iat {

// A rectangle drawn over the image (image coordinates), e.g. a region of
// interest with its label.
struct ViewOverlay {
    QRectF rect;
    QColor color;
    QString label;
    bool dashed = false;
    bool removable = false;  // right-click on the label emits overlayRemoveRequested
};

// Zoom and pan of a view, used to keep the three views of a tab in step.
struct ViewState {
    bool fit = true;          // fitted to the window (zoom follows the window size)
    double zoom = 1.0;        // screen pixels per image pixel, when not fitted
    QPointF center;           // image coordinates at the centre of the view
};

// Zoomable, pannable view of one image.
//  - mouse wheel: zoom around the cursor
//  - drag: pan
//  - the cursor is a crosshair, so it is clear which pixel it points at
//    (a closed hand while dragging)
//  - double-click: fit the image to the view again
//  - right-click: emits contextMenuRequested
//
// A/B comparison (setCompareImage): a second image of the same size, the
// "before" image (A), is drawn over the left part of the picture, up to a
// vertical divider; the image itself (B) shows to the right of it. The
// divider is dragged with the mouse. Its position is a fraction of the
// visible part of the image, so it stays on the picture however the view is
// zoomed or panned.
class ImageView : public QGraphicsView {
    Q_OBJECT
public:
    explicit ImageView(QWidget* parent = nullptr);

    // Shows `image`. Zoom and pan are kept if the size has not changed.
    void setImage(const QImage& image);
    void clearImage();
    bool hasImage() const { return !m_imageSize.isEmpty(); }

    // Turns the A/B comparison on (a non-null image the size of the image on
    // display) or off (a null image).
    void setCompareImage(const QImage& before);
    bool hasCompareImage() const { return !m_before.isNull(); }
    // Divider position: 0 (left edge of the visible image) to 1 (right edge).
    double comparePosition() const { return m_comparePos; }
    void setComparePosition(double position);
    // The divider's x coordinate in the viewport, or -1 without a comparison.
    int compareDividerX() const;

    // Region tool (off by default): a left-button drag draws a rectangle
    // (regionSelected) and a click picks a pixel (pixelClicked), instead of
    // panning. The wheel still zooms; drag with the middle button, or with
    // Ctrl held, to pan.
    void setRegionTool(bool on);
    bool regionTool() const { return m_regionTool; }
    // Rectangles drawn over the image (cleared with an empty list).
    void setOverlays(const QList<ViewOverlay>& overlays);
    const QList<ViewOverlay>& overlays() const { return m_overlays; }
    // Where overlay `index`'s label is drawn, in viewport coordinates
    // (empty if it has no label).
    QRect overlayLabelRect(int index) const;
    // Pointer markers (image coordinates): show where the mouse is in
    // another view of the same scene. Empty list: none.
    void setPointers(const QList<QPointF>& imagePoints);
    const QList<QPointF>& pointers() const { return m_pointers; }

    ViewState viewState() const;
    // Applies a zoom/pan state without emitting viewStateChanged.
    void setViewState(const ViewState& state);

public slots:
    void fitToWindow();        // programmatic; no signal
    void fitToWindowByUser();  // as double-click: fits and emits viewStateChanged

signals:
    // Pixel under the mouse (0-based column x, row y); (-1, -1) when outside.
    void hoveredPixel(int x, int y);
    // The user zoomed or panned this view.
    void viewStateChanged();
    // A new image of a different size was shown (and fitted).
    void imageSizeChanged();
    void contextMenuRequested(const QPoint& globalPos);
    // Region tool: a click on pixel (x, y), 0-based.
    void pixelClicked(int x, int y);
    // Region tool: a rectangle drawn with the mouse, in whole image pixels
    // (clipped to the image).
    void regionSelected(const QRect& imageRect);
    // Right-click on the label of a removable overlay.
    void overlayRemoveRequested(int index);

protected:
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void drawOverlays(QPainter* painter);
    void updateSmoothing();
    void onScrolled();
    double maxZoom() const;
    QRect visibleImageRect() const;  // the part of the image on screen, in viewport coordinates
    bool nearDivider(const QPoint& viewportPos) const;

    QGraphicsScene* m_scene = nullptr;
    QGraphicsPixmapItem* m_item = nullptr;
    QSize m_imageSize;
    bool m_fitMode = true;
    int m_internal = 0;  // > 0 while the view changes itself (no viewStateChanged)

    bool m_regionTool = false;
    bool m_selecting = false;
    QPointF m_selectStart;       // image coordinates
    QPoint m_selectStartView;    // viewport coordinates
    QRectF m_selection;          // image coordinates, while dragging
    QList<ViewOverlay> m_overlays;
    QList<QPointF> m_pointers;
    bool m_panning = false;      // region tool: middle-button / Ctrl drag
    QPoint m_panLast;
    int labelAt(const QPoint& viewportPos) const;

    QPixmap m_before;            // A/B comparison: the "before" image (null: off)
    double m_comparePos = 0.5;
    bool m_draggingDivider = false;
};

} // namespace iat
