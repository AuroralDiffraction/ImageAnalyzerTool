#include "ui/Icons.h"

#include <QApplication>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>

#include <algorithm>

namespace iat {
namespace icons {

namespace {

// Draws one glyph on a 16 × 16 grid, scaled to the size asked for.
class GlyphEngine : public QIconEngine {
public:
    GlyphEngine(Glyph glyph, const QColor& accent)
        : m_glyph(glyph)
        , m_accent(accent)
    {
    }

    void paint(QPainter* p, const QRect& rect, QIcon::Mode mode, QIcon::State) override
    {
        const QPalette pal = QApplication::palette();
        const bool disabled = mode == QIcon::Disabled;
        const QColor ink = pal.color(disabled ? QPalette::Disabled : QPalette::Active, QPalette::ButtonText);
        const QColor mark = m_accent.isValid() && !disabled ? m_accent : ink;

        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        const double s = std::min(rect.width(), rect.height()) / 16.0;
        p->translate(rect.x() + (rect.width() - 16.0 * s) / 2.0, rect.y() + (rect.height() - 16.0 * s) / 2.0);
        p->scale(s, s);
        const QPen line(ink, 1.3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p->setPen(line);
        p->setBrush(Qt::NoBrush);

        switch (m_glyph) {
        case Glyph::Open: {
            QPainterPath folder;
            folder.moveTo(1.7, 12.6);
            folder.lineTo(1.7, 3.6);
            folder.lineTo(6.0, 3.6);
            folder.lineTo(7.6, 5.4);
            folder.lineTo(13.2, 5.4);
            folder.lineTo(13.2, 7.2);
            p->drawPath(folder);
            QPainterPath flap;  // the open front
            flap.moveTo(1.7, 12.6);
            flap.lineTo(3.9, 7.2);
            flap.lineTo(14.6, 7.2);
            flap.lineTo(12.6, 12.6);
            flap.closeSubpath();
            p->drawPath(flap);
            break;
        }
        case Glyph::Process: {
            const double ys[3] = {3.8, 8.0, 12.2};
            const double knobs[3] = {10.4, 5.2, 11.4};
            for (int i = 0; i < 3; ++i)
                p->drawLine(QPointF(1.8, ys[i]), QPointF(14.2, ys[i]));
            p->setPen(QPen(mark, 1.3));
            p->setBrush(m_accent.isValid() && !disabled ? mark : pal.color(QPalette::Button));
            for (int i = 0; i < 3; ++i)
                p->drawEllipse(QPointF(knobs[i], ys[i]), 1.9, 1.9);
            break;
        }
        case Glyph::Data: {
            p->drawRoundedRect(QRectF(1.8, 2.8, 12.4, 10.4), 1.4, 1.4);
            p->drawLine(QPointF(1.8, 6.3), QPointF(14.2, 6.3));
            p->drawLine(QPointF(1.8, 9.8), QPointF(14.2, 9.8));
            p->drawLine(QPointF(6.0, 2.8), QPointF(6.0, 13.2));
            break;
        }
        case Glyph::Picture: {
            p->drawRoundedRect(QRectF(1.8, 2.8, 12.4, 10.4), 1.4, 1.4);
            QPainterPath hills;
            hills.moveTo(2.6, 12.0);
            hills.lineTo(6.4, 7.6);
            hills.lineTo(9.2, 10.6);
            hills.lineTo(10.8, 9.0);
            hills.lineTo(13.4, 12.0);
            p->drawPath(hills);
            p->setBrush(ink);
            p->drawEllipse(QPointF(10.6, 5.8), 0.9, 0.9);
            break;
        }
        }
        p->restore();
    }

    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override
    {
        QPixmap pm(size);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        paint(&p, QRect(QPoint(0, 0), size), mode, state);
        return pm;
    }

    QIconEngine* clone() const override { return new GlyphEngine(m_glyph, m_accent); }

private:
    Glyph m_glyph;
    QColor m_accent;
};

} // namespace

QIcon icon(Glyph glyph, const QColor& accent) { return QIcon(new GlyphEngine(glyph, accent)); }

} // namespace icons
} // namespace iat
