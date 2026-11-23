#pragma once

#include "core/Hsi.h"

#include <QDialog>

#include <array>

class QLabel;
class QPushButton;
class QSpinBox;

namespace iat {

// Settings dialog for the 4 × 4 mosaic layout: which channel sits at each
// position of the super-pixel, with presets. OK is only possible when every
// channel 1–16 appears exactly once.
class MosaicLayoutDialog : public QDialog {
    Q_OBJECT
public:
    explicit MosaicLayoutDialog(const hsi::MosaicLayout& layout, QWidget* parent = nullptr);

    hsi::MosaicLayout layout() const;
    void setLayout(const hsi::MosaicLayout& layout);
    bool isValid() const { return layout().isValid(); }
    // Sets one cell (row, col 0-based), for tests.
    void setChannel(int row, int col, int channel);

    // The preset's name in lower case ("serpentine (vertical)") or the layout as text.
    static QString describe(const hsi::MosaicLayout& layout);
    static QString presetName(hsi::MosaicLayout::Preset preset);

private:
    void validate();

    std::array<QSpinBox*, hsi::kBands> m_spins{};
    QLabel* m_warning = nullptr;
    QPushButton* m_ok = nullptr;
};

} // namespace iat
