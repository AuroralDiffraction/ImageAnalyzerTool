#include "ui/ImportImageDialog.h"

#include "core/HsiAnalysis.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace iat {

ImportImageDialog::ImportImageDialog(const QList<ImportCandidate>& candidates, QWidget* parent)
    : QDialog(parent)
    , m_candidates(candidates)
{
    setWindowTitle(tr("Import from workspace"));
    auto* layout = new QVBoxLayout(this);
    auto* intro = new QLabel(tr("Choose a tab, then the image to use as the raw 4 × 4 mosaic. "
                                "The values are taken as they are in that tab, before any image processing."),
                             this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto* form = new QFormLayout;
    m_tab = new QComboBox(this);
    m_image = new QComboBox(this);
    form->addRow(tr("Tab:"), m_tab);
    form->addRow(tr("Image:"), m_image);
    layout->addLayout(form);

    m_preview = new QLabel(this);
    m_preview->setMinimumSize(320, 256);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setFrameShape(QFrame::StyledPanel);
    layout->addWidget(m_preview, 1);
    m_info = new QLabel(this);
    m_info->setForegroundRole(QPalette::PlaceholderText);
    layout->addWidget(m_info);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Import"));
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QStringList tabs;
    for (const ImportCandidate& c : m_candidates)
        if (!tabs.contains(c.tabName))
            tabs << c.tabName;
    m_tab->addItems(tabs);
    connect(m_tab, &QComboBox::currentIndexChanged, this, &ImportImageDialog::fillImages);
    connect(m_image, &QComboBox::currentIndexChanged, this, &ImportImageDialog::showPreview);
    fillImages();

    if (m_candidates.isEmpty()) {
        m_preview->setText(tr("No tab has an image yet.\n\nLoad an image in single image mode, or light and dark "
                              "frames in three-panel mode, then import it here."));
        buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

void ImportImageDialog::fillImages()
{
    const QSignalBlocker block(m_image);
    m_image->clear();
    for (int i = 0; i < m_candidates.size(); ++i)
        if (m_candidates[i].tabName == m_tab->currentText())
            m_image->addItem(m_candidates[i].imageName, i);
    showPreview();
}

void ImportImageDialog::showPreview()
{
    const int i = selectedIndex();
    if (i < 0) {
        m_info->clear();
        return;
    }
    const ImportCandidate& c = m_candidates[i];
    const auto range = hsi::autoRange(c.image);
    const QImage img = hsi::renderScalar(c.image, range.first, range.second);
    m_preview->setPixmap(QPixmap::fromImage(img).scaled(m_preview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_info->setText(c.info);
}

int ImportImageDialog::selectedIndex() const
{
    return m_image->currentIndex() >= 0 ? m_image->currentData().toInt() : -1;
}

bool ImportImageDialog::select(const QString& tabName, const QString& imageName)
{
    const int t = m_tab->findText(tabName);
    if (t < 0)
        return false;
    m_tab->setCurrentIndex(t);
    const int i = m_image->findText(imageName);
    if (i < 0)
        return false;
    m_image->setCurrentIndex(i);
    return true;
}

} // namespace iat
