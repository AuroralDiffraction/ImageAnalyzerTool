#include "ui/MosaicLayoutDialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <algorithm>

namespace iat {

using namespace hsi;

MosaicLayoutDialog::MosaicLayoutDialog(const MosaicLayout& layout, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Mosaic layout"));
    auto* l = new QVBoxLayout(this);
    auto* title = new QLabel(tr("Channel at each position of the 4 × 4 super-pixel (as seen on the sensor):"), this);
    title->setWordWrap(true);
    l->addWidget(title);

    auto* grid = new QGridLayout;
    grid->setSpacing(4);
    for (int i = 0; i < kBands; ++i) {
        auto* s = new QSpinBox(this);
        s->setRange(1, kBands);
        s->setAlignment(Qt::AlignCenter);
        s->setMinimumWidth(56);
        m_spins[size_t(i)] = s;
        grid->addWidget(s, i / kPattern, i % kPattern);
        connect(s, &QSpinBox::valueChanged, this, &MosaicLayoutDialog::validate);
    }
    auto* row = new QHBoxLayout;
    row->addLayout(grid);
    auto* presets = new QVBoxLayout;
    using Preset = MosaicLayout::Preset;
    for (Preset p : {Preset::RowByRow, Preset::RowByRowVertical, Preset::Serpentine, Preset::SerpentineVertical}) {
        auto* b = new QPushButton(presetName(p), this);
        b->setObjectName(QStringLiteral("mosaicPreset%1").arg(int(p)));
        b->setToolTip(QStringLiteral("[%1]").arg(MosaicLayout::preset(p).toString()));
        presets->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, p] { setLayout(MosaicLayout::preset(p)); });
    }
    presets->addStretch(1);
    row->addSpacing(12);
    row->addLayout(presets);
    l->addLayout(row);

    m_warning = new QLabel(this);
    m_warning->setWordWrap(true);
    QPalette warn = m_warning->palette();
    warn.setColor(QPalette::WindowText, QColor(0xd0, 0x30, 0x30));
    m_warning->setPalette(warn);
    m_warning->hide();
    l->addWidget(m_warning);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_ok = buttons->button(QDialogButtonBox::Ok);
    l->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setLayout(layout);
}

MosaicLayout MosaicLayoutDialog::layout() const
{
    MosaicLayout m;
    for (int i = 0; i < kBands; ++i)
        m.channel[size_t(i)] = m_spins[size_t(i)]->value();
    return m;
}

void MosaicLayoutDialog::setLayout(const MosaicLayout& layout)
{
    for (int i = 0; i < kBands; ++i) {
        const QSignalBlocker block(m_spins[size_t(i)]);
        m_spins[size_t(i)]->setValue(layout.channel[size_t(i)]);
    }
    validate();
}

void MosaicLayoutDialog::setChannel(int row, int col, int channel)
{
    m_spins[size_t(row * kPattern + col)]->setValue(channel);
}

void MosaicLayoutDialog::validate()
{
    const MosaicLayout m = layout();
    const bool ok = m.isValid();
    if (!ok) {
        QStringList missing;
        for (int ch = 1; ch <= kBands; ++ch)
            if (std::find(m.channel.begin(), m.channel.end(), ch) == m.channel.end())
                missing << QString::number(ch);
        m_warning->setText(tr("Every channel 1–16 must appear exactly once. Missing: %1.").arg(missing.join(QStringLiteral(", "))));
    }
    m_warning->setVisible(!ok);
    m_ok->setEnabled(ok);
}

QString MosaicLayoutDialog::presetName(MosaicLayout::Preset p)
{
    switch (p) {
    case MosaicLayout::Preset::RowByRow:
        return tr("Row by row");
    case MosaicLayout::Preset::RowByRowVertical:
        return tr("Row by row (vertical)");
    case MosaicLayout::Preset::Serpentine:
        return tr("Serpentine");
    case MosaicLayout::Preset::SerpentineVertical:
        return tr("Serpentine (vertical)");
    }
    return QString();
}

QString MosaicLayoutDialog::describe(const MosaicLayout& layout)
{
    if (const auto p = MosaicLayout::presetOf(layout))
        return presetName(*p).toLower();
    return QStringLiteral("[%1]").arg(layout.toString());
}

} // namespace iat
