#pragma once

#include <QWidget>

class QDoubleSpinBox;
class QSlider;

namespace iat {

// Two sliders (with matching spin boxes) that set the lowest and highest
// displayed value. Values below "Min" show as black, above "Max" as white.
class ContrastControl : public QWidget {
    Q_OBJECT
public:
    explicit ContrastControl(QWidget* parent = nullptr);

    // Range the sliders cover (normally the image's min and max).
    void setDataRange(double min, double max, bool integerValued);
    // Moves the sliders without emitting displayRangeChanged.
    void setDisplayRange(double low, double high);

    double low() const { return m_low; }
    double high() const { return m_high; }

    // Applies a range as if the user had set it (clamps, keeps low < high,
    // updates the widgets) and emits displayRangeChanged.
    void applyRange(double low, double high, bool lowMoved);

signals:
    void displayRangeChanged(double low, double high);

private:
    int toSlider(double value) const;
    double fromSlider(int position) const;
    double minimumGap() const;
    void syncWidgets();

    QSlider* m_lowSlider = nullptr;
    QSlider* m_highSlider = nullptr;
    QDoubleSpinBox* m_lowSpin = nullptr;
    QDoubleSpinBox* m_highSpin = nullptr;

    double m_min = 0.0;
    double m_max = 1.0;
    double m_low = 0.0;
    double m_high = 1.0;
    bool m_integer = false;
    int m_steps = 10000;
};

} // namespace iat
