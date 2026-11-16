#include "ui/ModeSwitch.h"

#include <QHelpEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>
#include <QVariantAnimation>

#include <algorithm>
#include <cmath>

namespace iat {

namespace {

constexpr int kSegmentWidth = 30;
constexpr int kSegmentHeight = 20;
constexpr int kPadding = 2;  // between the track's edge and the thumb

int indexOf(LayoutMode mode) { return mode == LayoutMode::Single ? 0 : 1; }

QColor blend(const QColor& a, const QColor& b, double t)
{
    t = std::clamp(t, 0.0, 1.0);
    auto mix = [t](int from, int to) { return int(std::lround(from + (to - from) * t)); };
    return QColor(mix(a.red(), b.red()), mix(a.green(), b.green()), mix(a.blue(), b.blue()),
                  mix(a.alpha(), b.alpha()));
}

// One panel: a single rectangle.
void drawSingleIcon(QPainter& p, const QPointF& c)
{
    p.drawRoundedRect(QRectF(c.x() - 6.5, c.y() - 4.5, 13.0, 9.0), 1.5, 1.5);
}

// Three panels: three narrow rectangles side by side.
void drawTripleIcon(QPainter& p, const QPointF& c)
{
    QPen pen = p.pen();
    pen.setWidthF(1.2);
    p.setPen(pen);
    for (int i = -1; i <= 1; ++i)
        p.drawRoundedRect(QRectF(c.x() + i * 6.0 - 1.5, c.y() - 4.5, 4.0, 9.0), 0.8, 0.8);
}

} // namespace

ModeSwitch::ModeSwitch(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::TabFocus);
    setCursor(Qt::PointingHandCursor);
    setAccessibleName(tr("Panel layout"));
    setAccessibleDescription(tr("Single image, or three panels: light, dark and calibrated frames"));

    m_animation = new QVariantAnimation(this);
    m_animation->setDuration(140);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_position = v.toDouble();
        update();
    });
}

QSize ModeSwitch::sizeHint() const
{
    return QSize(2 * kSegmentWidth + 2 * kPadding, kSegmentHeight + 2 * kPadding);
}

QString ModeSwitch::tipFor(LayoutMode mode)
{
    return mode == LayoutMode::Single ? tr("Single image: one panel")
                                      : tr("Three panels: light frames, dark frames and calibrated frames");
}

QRectF ModeSwitch::segmentRect(int index) const
{
    return QRectF(kPadding + index * kSegmentWidth, kPadding, kSegmentWidth, kSegmentHeight);
}

void ModeSwitch::setMode(LayoutMode mode, bool animate)
{
    if (mode == m_mode && m_animation->state() != QAbstractAnimation::Running)
        return;
    m_mode = mode;
    const double target = indexOf(mode);
    m_animation->stop();
    if (animate && isVisible()) {
        m_animation->setStartValue(m_position);
        m_animation->setEndValue(target);
        m_animation->start();
    } else {
        m_position = target;
        update();
    }
}

void ModeSwitch::choose(LayoutMode mode)
{
    if (mode == m_mode)
        return;
    setMode(mode, true);
    emit modeChanged(mode);
}

bool ModeSwitch::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip) {
        // Each side has its own tooltip, since the icons carry no text.
        auto* help = static_cast<QHelpEvent*>(event);
        const LayoutMode side = help->pos().x() < width() / 2 ? LayoutMode::Single : LayoutMode::Triple;
        QToolTip::showText(help->globalPos(), tipFor(side), this, segmentRect(indexOf(side)).toRect());
        return true;
    }
    return QWidget::event(event);
}

void ModeSwitch::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    choose(event->position().x() < width() / 2.0 ? LayoutMode::Single : LayoutMode::Triple);
    event->accept();
}

void ModeSwitch::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        choose(LayoutMode::Single);
        return;
    case Qt::Key_Right:
        choose(LayoutMode::Triple);
        return;
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        choose(m_mode == LayoutMode::Single ? LayoutMode::Triple : LayoutMode::Single);
        return;
    default:
        QWidget::keyPressEvent(event);
    }
}

void ModeSwitch::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if (!isEnabled())
        p.setOpacity(0.5);

    const QPalette pal = palette();
    const QRectF track = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const double radius = 6.0;

    // Track.
    p.setPen(QPen(hasFocus() ? pal.color(QPalette::Highlight) : pal.color(QPalette::Mid), 1.0));
    p.setBrush(blend(pal.color(QPalette::Window), pal.color(QPalette::Mid), 0.35));
    p.drawRoundedRect(track, radius, radius);

    // Thumb, sliding between the two sides.
    QRectF thumb = segmentRect(0);
    thumb.moveLeft(kPadding + m_position * kSegmentWidth);
    p.setPen(QPen(pal.color(QPalette::Mid), 1.0));
    p.setBrush(pal.color(QPalette::Base));
    p.drawRoundedRect(thumb.adjusted(0.5, 0.5, -0.5, -0.5), radius - 2.0, radius - 2.0);

    // Icons: the one under the thumb takes the highlight colour.
    const QColor active = pal.color(QPalette::Highlight);
    const QColor idle = blend(pal.color(QPalette::Window), pal.color(QPalette::WindowText), 0.6);
    p.setBrush(Qt::NoBrush);
    for (int i = 0; i < 2; ++i) {
        const double under = 1.0 - std::abs(m_position - i);  // 1 when the thumb is on this side
        p.setPen(QPen(blend(idle, active, under), 1.5));
        const QPointF c = segmentRect(i).center();
        if (i == 0)
            drawSingleIcon(p, c);
        else
            drawTripleIcon(p, c);
    }
}

} // namespace iat
