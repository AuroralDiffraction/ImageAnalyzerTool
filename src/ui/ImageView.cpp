#include "ui/ImageView.h"

#include <QContextMenuEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPixmap>
#include <QScrollBar>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace iat {

namespace {
constexpr double kMinZoom = 0.01;
constexpr double kMaxZoom = 64.0;

// Marks a block in which the view changes its own zoom/pan, so the scroll
// bar changes it causes are not reported as user actions.
class InternalChange {
public:
    explicit InternalChange(int& counter) : m_counter(counter) { ++m_counter; }
    ~InternalChange() { --m_counter; }
    InternalChange(const InternalChange&) = delete;
    InternalChange& operator=(const InternalChange&) = delete;

private:
    int& m_counter;
};
} // namespace

ImageView::ImageView(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);
    m_item = m_scene->addPixmap(QPixmap());
    m_item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);

    setFrameShape(QFrame::NoFrame);
    setBackgroundBrush(QColor(38, 38, 40));
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);

    // Let file drops reach the panel that owns this view.
    setAcceptDrops(false);
    viewport()->setAcceptDrops(false);

    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &ImageView::onScrolled);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &ImageView::onScrolled);
}

void ImageView::setImage(const QImage& image)
{
    const bool sizeChanged = image.size() != m_imageSize;
    {
        InternalChange guard(m_internal);
        m_item->setPixmap(QPixmap::fromImage(image));
        m_imageSize = image.size();
        if (sizeChanged) {
            m_scene->setSceneRect(QRectF(QPointF(0, 0), QSizeF(m_imageSize)));
            m_fitMode = true;
        }
        if (m_fitMode)
            fitToWindow();
        updateSmoothing();
    }
    if (sizeChanged)
        emit imageSizeChanged();
}

void ImageView::clearImage()
{
    InternalChange guard(m_internal);
    m_item->setPixmap(QPixmap());
    m_imageSize = QSize();
    m_fitMode = true;
    resetTransform();
}

ViewState ImageView::viewState() const
{
    ViewState s;
    s.fit = m_fitMode;
    s.zoom = transform().m11();
    s.center = mapToScene(viewport()->rect()).boundingRect().center();
    return s;
}

void ImageView::setViewState(const ViewState& state)
{
    if (!hasImage())
        return;
    InternalChange guard(m_internal);
    if (state.fit) {
        fitToWindow();
        return;
    }
    const double zoom = std::clamp(state.zoom, kMinZoom, maxZoom());
    setTransform(QTransform::fromScale(zoom, zoom));
    centerOn(state.center);
    m_fitMode = false;
    updateSmoothing();
}

double ImageView::maxZoom() const
{
    // Normally 64 screen pixels per image pixel. A very small image can
    // already be larger than that when fitted, so allow 4 × the fit zoom.
    if (!hasImage())
        return kMaxZoom;
    const double fit = std::min(viewport()->width() / double(m_imageSize.width()),
                                viewport()->height() / double(m_imageSize.height()));
    return std::max(kMaxZoom, 4.0 * fit);
}

void ImageView::fitToWindow()
{
    InternalChange guard(m_internal);
    m_fitMode = true;
    if (!hasImage())
        return;
    fitInView(m_item, Qt::KeepAspectRatio);
    updateSmoothing();
}

void ImageView::fitToWindowByUser()
{
    fitToWindow();
    emit viewStateChanged();
}

void ImageView::onScrolled()
{
    if (m_internal == 0 && hasImage())
        emit viewStateChanged();  // panned by dragging, scroll bars or keys
}

void ImageView::updateSmoothing()
{
    // Smooth when shrinking (avoids aliasing), nearest-neighbour when
    // magnifying so individual pixels stay sharp.
    const double zoom = transform().m11();
    m_item->setTransformationMode(zoom < 1.0 ? Qt::SmoothTransformation : Qt::FastTransformation);
}

void ImageView::wheelEvent(QWheelEvent* event)
{
    if (!hasImage()) {
        event->ignore();
        return;
    }
    {
        InternalChange guard(m_internal);
        const double steps = event->angleDelta().y() / 120.0;
        double factor = std::pow(1.25, steps);
        const double zoom = transform().m11();
        const double maxZ = std::max(maxZoom(), zoom);  // zooming in never shrinks the image
        if (zoom * factor < kMinZoom) factor = kMinZoom / zoom;
        if (zoom * factor > maxZ) factor = maxZ / zoom;
        scale(factor, factor);
        m_fitMode = false;
        updateSmoothing();
    }
    event->accept();
    emit viewStateChanged();
}

void ImageView::mouseMoveEvent(QMouseEvent* event)
{
    QGraphicsView::mouseMoveEvent(event);
    if (!hasImage())
        return;
    const QPointF p = mapToScene(event->position().toPoint());
    const int x = int(std::floor(p.x()));
    const int y = int(std::floor(p.y()));
    if (x >= 0 && y >= 0 && x < m_imageSize.width() && y < m_imageSize.height())
        emit hoveredPixel(x, y);
    else
        emit hoveredPixel(-1, -1);
}

void ImageView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        fitToWindowByUser();
        event->accept();
        return;
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void ImageView::leaveEvent(QEvent* event)
{
    QGraphicsView::leaveEvent(event);
    emit hoveredPixel(-1, -1);
}

void ImageView::resizeEvent(QResizeEvent* event)
{
    InternalChange guard(m_internal);
    QGraphicsView::resizeEvent(event);
    if (m_fitMode)
        fitToWindow();
}

void ImageView::contextMenuEvent(QContextMenuEvent* event)
{
    emit contextMenuRequested(event->globalPos());
    event->accept();
}

} // namespace iat
