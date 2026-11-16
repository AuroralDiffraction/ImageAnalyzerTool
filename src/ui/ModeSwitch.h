#pragma once

#include "ui/LayoutMode.h"

#include <QWidget>

class QVariantAnimation;

namespace iat {

// Slide switch with two positions, each marked by an icon:
//
//   [ ▭ | ▯▯▯ ]     one panel (single image)  |  three panels
//
// A thumb slides to the chosen side. Click an icon to choose it; with the
// keyboard, Left / Right choose a side and Space switches to the other one.
//
// The switch shows the mode of the current tab: MainWindow moves it with
// setMode() when the tab changes, and applies modeChanged() to the tab.
class ModeSwitch : public QWidget {
    Q_OBJECT
public:
    explicit ModeSwitch(QWidget* parent = nullptr);

    LayoutMode mode() const { return m_mode; }
    // Moves the switch without emitting modeChanged.
    void setMode(LayoutMode mode, bool animate = false);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    // The user moved the switch.
    void modeChanged(iat::LayoutMode mode);

protected:
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void choose(LayoutMode mode);  // by the user: animates and emits modeChanged
    QRectF segmentRect(int index) const;
    static QString tipFor(LayoutMode mode);

    LayoutMode m_mode = LayoutMode::Single;
    double m_position = 0.0;  // thumb position: 0 = Single … 1 = Triple
    QVariantAnimation* m_animation = nullptr;
};

} // namespace iat
