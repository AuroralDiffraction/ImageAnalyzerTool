#include "ui/ContrastControl.h"

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>

#include <algorithm>
#include <cmath>

namespace iat {

namespace {
constexpr int kMaxSteps = 10000;

int decimalsFor(double span, bool integer)
{
    if (integer)
        return 0;
    if (!(span > 0))
        return 3;
    // Enough decimals to resolve roughly 1/1000 of the range.
    const int d = int(std::ceil(-std::log10(span / 1000.0)));
    return std::clamp(d, 0, 8);
}
} // namespace

ContrastControl::ContrastControl(QWidget* parent)
    : QWidget(parent)
{
    auto* grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(4);

    auto makeRow = [&](int row, const QString& label, QSlider*& slider, QDoubleSpinBox*& spin) {
        auto* l = new QLabel(label, this);
        slider = new QSlider(Qt::Horizontal, this);
        slider->setRange(0, m_steps);
        spin = new QDoubleSpinBox(this);
        spin->setKeyboardTracking(false);  // apply typed values on Enter / focus out
        spin->setMinimumWidth(90);
        spin->setAlignment(Qt::AlignRight);
        grid->addWidget(l, row, 0);
        grid->addWidget(slider, row, 1);
        grid->addWidget(spin, row, 2);
    };
    makeRow(0, tr("Min"), m_lowSlider, m_lowSpin);
    makeRow(1, tr("Max"), m_highSlider, m_highSpin);
    grid->setColumnStretch(1, 1);

    m_lowSlider->setToolTip(tr("Lowest displayed value: anything at or below it is black"));
    m_highSlider->setToolTip(tr("Highest displayed value: anything at or above it is white"));

    connect(m_lowSlider, &QSlider::valueChanged, this,
            [this](int pos) { applyRange(fromSlider(pos), m_high, true); });
    connect(m_highSlider, &QSlider::valueChanged, this,
            [this](int pos) { applyRange(m_low, fromSlider(pos), false); });
    connect(m_lowSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { applyRange(v, m_high, true); });
    connect(m_highSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this](double v) { applyRange(m_low, v, false); });

    setDataRange(0.0, 1.0, false);
}

void ContrastControl::setDataRange(double min, double max, bool integerValued)
{
    if (!(max > min)) {
        min -= 0.5;
        max += 0.5;
    }
    m_min = min;
    m_max = max;
    m_integer = integerValued;

    const double span = m_max - m_min;
    m_steps = (m_integer && span >= 1 && span <= kMaxSteps) ? int(span) : kMaxSteps;

    const int decimals = decimalsFor(span, m_integer);
    const double step = m_integer ? 1.0 : std::max(span / 100.0, std::pow(10.0, -decimals));
    for (QSlider* s : {m_lowSlider, m_highSlider}) {
        const QSignalBlocker block(s);
        s->setRange(0, m_steps);
        s->setPageStep(std::max(1, m_steps / 20));
    }
    for (QDoubleSpinBox* s : {m_lowSpin, m_highSpin}) {
        const QSignalBlocker block(s);
        s->setDecimals(decimals);
        s->setRange(m_min, m_max);
        s->setSingleStep(step);
    }
    m_low = std::clamp(m_low, m_min, m_max);
    m_high = std::clamp(m_high, m_min, m_max);
    syncWidgets();
}

void ContrastControl::setDisplayRange(double low, double high)
{
    m_low = std::clamp(low, m_min, m_max);
    m_high = std::clamp(high, m_min, m_max);
    syncWidgets();
}

double ContrastControl::minimumGap() const
{
    return m_integer ? 1.0 : (m_max - m_min) / kMaxSteps;
}

void ContrastControl::applyRange(double low, double high, bool lowMoved)
{
    low = std::clamp(low, m_min, m_max);
    high = std::clamp(high, m_min, m_max);
    const double gap = std::min(minimumGap(), m_max - m_min);
    // Moving one end past the other pushes the other one along.
    if (high - low < gap) {
        if (lowMoved) {
            high = std::min(m_max, low + gap);
            low = high - gap;
        } else {
            low = std::max(m_min, high - gap);
            high = low + gap;
        }
    }
    if (low == m_low && high == m_high) {
        syncWidgets();
        return;
    }
    m_low = low;
    m_high = high;
    syncWidgets();
    emit displayRangeChanged(m_low, m_high);
}

int ContrastControl::toSlider(double value) const
{
    const double f = (value - m_min) / (m_max - m_min);
    return std::clamp(int(std::lround(f * m_steps)), 0, m_steps);
}

double ContrastControl::fromSlider(int position) const
{
    double v = m_min + (m_max - m_min) * double(position) / double(m_steps);
    if (m_integer)
        v = std::round(v);
    return std::clamp(v, m_min, m_max);
}

void ContrastControl::syncWidgets()
{
    const QSignalBlocker b1(m_lowSlider);
    const QSignalBlocker b2(m_highSlider);
    const QSignalBlocker b3(m_lowSpin);
    const QSignalBlocker b4(m_highSpin);
    m_lowSlider->setValue(toSlider(m_low));
    m_highSlider->setValue(toSlider(m_high));
    m_lowSpin->setValue(m_low);
    m_highSpin->setValue(m_high);
}

} // namespace iat
