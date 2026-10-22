#pragma once

#include <QGraphicsView>
#include <QImage>
#include <QPointF>

class QGraphicsPixmapItem;

namespace iat {

// Zoom and pan of a view, used to keep the three views of a tab in step.
struct ViewState {
    bool fit = true;          // fitted to the window (zoom follows the window size)
    double zoom = 1.0;        // screen pixels per image pixel, when not fitted
    QPointF center;           // image coordinates at the centre of the view
};

// Zoomable, pannable view of one image.
//  - mouse wheel: zoom around the cursor
//  - drag: pan
//  - double-click: fit the image to the view again
//  - right-click: emits contextMenuRequested
class ImageView : public QGraphicsView {
    Q_OBJECT
public:
    explicit ImageView(QWidget* parent = nullptr);

    // Shows `image`. Zoom and pan are kept if the size has not changed.
    void setImage(const QImage& image);
    void clearImage();
    bool hasImage() const { return !m_imageSize.isEmpty(); }

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

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void updateSmoothing();
    void onScrolled();
    double maxZoom() const;

    QGraphicsScene* m_scene = nullptr;
    QGraphicsPixmapItem* m_item = nullptr;
    QSize m_imageSize;
    bool m_fitMode = true;
    int m_internal = 0;  // > 0 while the view changes itself (no viewStateChanged)
};

} // namespace iat
