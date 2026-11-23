#include "ui/ImageView.h"

#include <QContextMenuEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QScrollBar>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace iat {

namespace {
constexpr double kMinZoom = 0.01;
constexpr double kMaxZoom = 64.0;
constexpr int kDividerGrab = 9;  // pixels either side of the A/B divider that grab it

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
    // ScrollHandDrag shows an open hand, which hides the pixel under it. A
    // crosshair points at one pixel; the closed hand still shows while dragging.
    viewport()->setCursor(Qt::CrossCursor);
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
    setCompareImage(QImage());
    m_item->setPixmap(QPixmap());
    m_imageSize = QSize();
    m_fitMode = true;
    resetTransform();
}

// ---------------------------------------------------------------------------
// A/B comparison
// ---------------------------------------------------------------------------

void ImageView::setCompareImage(const QImage& before)
{
    const bool was = hasCompareImage();
    m_before = before.isNull() ? QPixmap() : QPixmap::fromImage(before);
    if (was != hasCompareImage()) {
        // The divider and its labels are drawn at fixed places in the
        // window, so the view must not scroll its pixels while it is on.
        setViewportUpdateMode(hasCompareImage() ? QGraphicsView::FullViewportUpdate
                                                : QGraphicsView::MinimalViewportUpdate);
        if (!hasCompareImage()) {
            m_draggingDivider = false;
            viewport()->setCursor(Qt::CrossCursor);
        }
    }
    viewport()->update();
}

void ImageView::setComparePosition(double position)
{
    position = std::clamp(position, 0.0, 1.0);
    if (position == m_comparePos)
        return;
    m_comparePos = position;
    if (hasCompareImage())
        viewport()->update();
}

QRect ImageView::visibleImageRect() const
{
    if (!hasImage())
        return {};
    return mapFromScene(sceneRect()).boundingRect().intersected(viewport()->rect());
}

int ImageView::compareDividerX() const
{
    const QRect vis = visibleImageRect();
    if (!hasCompareImage() || vis.isEmpty())
        return -1;
    return vis.left() + int(std::lround(m_comparePos * vis.width()));
}

bool ImageView::nearDivider(const QPoint& viewportPos) const
{
    const int x = compareDividerX();
    return x >= 0 && std::abs(viewportPos.x() - x) <= kDividerGrab;
}

void ImageView::setRegionTool(bool on)
{
    if (on == m_regionTool)
        return;
    m_regionTool = on;
    m_selecting = false;
    setDragMode(on ? QGraphicsView::NoDrag : QGraphicsView::ScrollHandDrag);
    viewport()->setCursor(Qt::CrossCursor);
    viewport()->update();
}

void ImageView::setOverlays(const QList<ViewOverlay>& overlays)
{
    m_overlays = overlays;
    viewport()->update();
}

void ImageView::setPointers(const QList<QPointF>& imagePoints)
{
    if (imagePoints == m_pointers)
        return;
    m_pointers = imagePoints;
    viewport()->update();
}

QRect ImageView::overlayLabelRect(int index) const
{
    if (index < 0 || index >= m_overlays.size() || m_overlays[index].label.isEmpty())
        return {};
    QFont f = font();
    f.setBold(true);
    const QFontMetrics fm(f);
    const QRectF v = viewportTransform().mapRect(m_overlays[index].rect);
    const QString& label = m_overlays[index].label;
    return QRectF(v.left(), v.top() - fm.height() - 4, fm.horizontalAdvance(label) + 8, fm.height() + 4).toAlignedRect();
}

int ImageView::labelAt(const QPoint& pos) const
{
    for (int i = int(m_overlays.size()) - 1; i >= 0; --i)
        if (m_overlays[i].removable && overlayLabelRect(i).contains(pos))
            return i;
    return -1;
}

void ImageView::drawOverlays(QPainter* painter)
{
    // Overlays and the rectangle being drawn, in window coordinates (thin
    // lines at any zoom).
    if (!m_overlays.isEmpty() || m_selecting || !m_pointers.isEmpty()) {
        painter->save();
        const QTransform toView = viewportTransform();
        painter->resetTransform();
        painter->setRenderHint(QPainter::Antialiasing);
        QFont f = font();
        f.setBold(true);
        painter->setFont(f);
        const QFontMetrics fm(f);
        auto drawBox = [&](const QRectF& r, const QColor& colour, const QString& label, bool dashed) {
            const QRectF v = toView.mapRect(r);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(QColor(0, 0, 0, 160), 3.0));
            painter->drawRect(v);
            QPen pen(colour, 1.6);
            if (dashed)
                pen.setStyle(Qt::DashLine);
            painter->setPen(pen);
            painter->drawRect(v);
            if (!label.isEmpty()) {
                const QRectF t(v.left(), v.top() - fm.height() - 4, fm.horizontalAdvance(label) + 8, fm.height() + 4);
                painter->setPen(Qt::NoPen);
                painter->setBrush(QColor(0, 0, 0, 150));
                painter->drawRoundedRect(t, 3, 3);
                painter->setPen(colour.lighter(130));
                painter->drawText(t, Qt::AlignCenter, label);
            }
        };
        for (const ViewOverlay& o : std::as_const(m_overlays))
            drawBox(o.rect, o.color, o.label, o.dashed);
        if (m_selecting)
            drawBox(m_selection, QColor(255, 255, 255), QString(), true);
        // Pointer markers: a small cross with a ring, white on a dark outline.
        for (const QPointF& ip : std::as_const(m_pointers)) {
            const QPointF c = toView.map(ip);
            for (int pass = 0; pass < 2; ++pass) {
                painter->setPen(pass == 0 ? QPen(QColor(0, 0, 0, 200), 3.5) : QPen(QColor(255, 255, 255), 1.5));
                painter->setBrush(Qt::NoBrush);
                painter->drawEllipse(c, 5.0, 5.0);
                painter->drawLine(c + QPointF(-11, 0), c + QPointF(-6, 0));
                painter->drawLine(c + QPointF(6, 0), c + QPointF(11, 0));
                painter->drawLine(c + QPointF(0, -11), c + QPointF(0, -6));
                painter->drawLine(c + QPointF(0, 6), c + QPointF(0, 11));
            }
        }
        painter->restore();
    }
}

void ImageView::drawForeground(QPainter* painter, const QRectF& rect)
{
    QGraphicsView::drawForeground(painter, rect);
    const QRect vis = visibleImageRect();
    if (!hasCompareImage() || vis.isEmpty()) {
        drawOverlays(painter);
        return;
    }
    const int x = compareDividerX();

    // A: the "before" image over everything left of the divider.
    if (x > vis.left()) {
        painter->save();
        const QRectF left = mapToScene(QRect(vis.left(), vis.top(), x - vis.left(), vis.height())).boundingRect();
        painter->setClipRect(left.intersected(sceneRect()));
        painter->setRenderHint(QPainter::SmoothPixmapTransform, transform().m11() < 1.0);
        painter->drawPixmap(sceneRect(), m_before, QRectF(m_before.rect()));
        painter->restore();
    }
    drawOverlays(painter);  // over both sides

    // The divider, its handle and the A / B labels, in window coordinates.
    painter->save();
    painter->resetTransform();
    painter->setRenderHint(QPainter::Antialiasing);
    const QColor light(255, 255, 255, 235), dark(0, 0, 0, 150);
    painter->setPen(QPen(dark, 4));
    painter->drawLine(QPointF(x, vis.top()), QPointF(x, vis.bottom() + 1));
    painter->setPen(QPen(light, 1.6));
    painter->drawLine(QPointF(x, vis.top()), QPointF(x, vis.bottom() + 1));

    const QPointF c(x, vis.center().y());
    painter->setPen(QPen(dark, 1.5));
    painter->setBrush(light);
    painter->drawEllipse(c, 11, 11);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(40, 40, 40));
    QPolygonF leftArrow, rightArrow;
    leftArrow << c + QPointF(-7, 0) << c + QPointF(-2.5, -4) << c + QPointF(-2.5, 4);
    rightArrow << c + QPointF(7, 0) << c + QPointF(2.5, -4) << c + QPointF(2.5, 4);
    painter->drawPolygon(leftArrow);
    painter->drawPolygon(rightArrow);

    QFont f = font();
    f.setBold(true);
    painter->setFont(f);
    const QFontMetrics fm(f);
    const int side = fm.height() + 6;
    const auto badge = [&](const QString& text, const QRectF& r) {
        if (r.left() < vis.left() || r.right() > vis.right() + 1)
            return;  // no room on that side of the divider
        painter->setPen(Qt::NoPen);
        painter->setBrush(dark);
        painter->drawRoundedRect(r, 4, 4);
        painter->setPen(light);
        painter->drawText(r, Qt::AlignCenter, text);
    };
    badge(QStringLiteral("A"), QRectF(x - 8 - side, vis.top() + 8, side, side));
    badge(QStringLiteral("B"), QRectF(x + 8, vis.top() + 8, side, side));
    painter->restore();
}

void ImageView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && nearDivider(event->position().toPoint())) {
        m_draggingDivider = true;  // drag the divider, not the image
        viewport()->setCursor(Qt::SplitHCursor);
        event->accept();
        return;
    }
    if (m_regionTool && hasImage()
        && (event->button() == Qt::MiddleButton
            || (event->button() == Qt::LeftButton && (event->modifiers() & Qt::ControlModifier)))) {
        m_panning = true;
        m_panLast = event->position().toPoint();
        viewport()->setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    if (m_regionTool && event->button() == Qt::LeftButton && hasImage()) {
        m_selecting = true;
        m_selectStartView = event->position().toPoint();
        m_selectStart = mapToScene(m_selectStartView);
        m_selection = QRectF(m_selectStart, m_selectStart);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

// ---------------------------------------------------------------------------

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
    const QPoint pos = event->position().toPoint();
    if (m_draggingDivider) {
        const QRect vis = visibleImageRect();
        if (!vis.isEmpty())
            setComparePosition(double(pos.x() - vis.left()) / double(vis.width()));
        event->accept();
    } else if (m_panning) {
        const QPoint d = pos - m_panLast;
        m_panLast = pos;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - d.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - d.y());
        event->accept();
    } else if (m_selecting) {
        m_selection = QRectF(m_selectStart, mapToScene(pos)).normalized();
        viewport()->update();
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
        if (event->buttons() == Qt::NoButton) {
            const bool onLabel = labelAt(pos) >= 0;
            if (hasCompareImage() && nearDivider(pos))
                viewport()->setCursor(Qt::SplitHCursor);
            else
                viewport()->setCursor(onLabel ? Qt::PointingHandCursor : Qt::CrossCursor);
            viewport()->setToolTip(onLabel ? tr("Right-click to remove") : QString());
        }
    }
    if (!hasImage())
        return;
    const QPointF p = mapToScene(pos);
    const int x = int(std::floor(p.x()));
    const int y = int(std::floor(p.y()));
    if (x >= 0 && y >= 0 && x < m_imageSize.width() && y < m_imageSize.height())
        emit hoveredPixel(x, y);
    else
        emit hoveredPixel(-1, -1);
}

void ImageView::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_draggingDivider && event->button() == Qt::LeftButton) {
        m_draggingDivider = false;
        viewport()->setCursor(nearDivider(event->position().toPoint()) ? Qt::SplitHCursor : Qt::CrossCursor);
        event->accept();
        return;
    }
    if (m_panning && (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton)) {
        m_panning = false;
        viewport()->setCursor(Qt::CrossCursor);
        event->accept();
        return;
    }
    if (m_selecting && event->button() == Qt::LeftButton) {
        m_selecting = false;
        viewport()->update();
        const QPoint pos = event->position().toPoint();
        const QRect bounds(QPoint(0, 0), m_imageSize);
        if ((pos - m_selectStartView).manhattanLength() < 4) {
            const QPointF p = mapToScene(pos);
            const QPoint px(int(std::floor(p.x())), int(std::floor(p.y())));
            if (bounds.contains(px))
                emit pixelClicked(px.x(), px.y());
        } else {
            const QRectF r = QRectF(m_selectStart, mapToScene(pos)).normalized();
            const QRect pixels = QRect(QPoint(int(std::floor(r.left())), int(std::floor(r.top()))),
                                       QPoint(int(std::ceil(r.right())) - 1, int(std::ceil(r.bottom())) - 1))
                                     .intersected(bounds);
            if (!pixels.isEmpty())
                emit regionSelected(pixels);
        }
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
    viewport()->setCursor(Qt::CrossCursor);  // the base class puts the open hand back after a drag
}

void ImageView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && nearDivider(event->position().toPoint())) {
        setComparePosition(0.5);  // double-click on the divider: back to the middle
        event->accept();
        return;
    }
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
    const int label = labelAt(event->pos());
    if (label >= 0) {
        emit overlayRemoveRequested(label);
        event->accept();
        return;
    }
    emit contextMenuRequested(event->globalPos());
    event->accept();
}

} // namespace iat
