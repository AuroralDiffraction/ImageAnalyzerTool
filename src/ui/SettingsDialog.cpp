#include "ui/SettingsDialog.h"

#include "ui/Language.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace iat {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));

    m_language = new QComboBox(this);
    m_language->setObjectName(QStringLiteral("language"));
    for (const LanguageInfo& l : language::languages())
        m_language->addItem(l.name, l.code);  // each language under its own name
    setLanguageCode(language::savedCode());

    // The label is in both languages, so it can be found whatever the current one is.
    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("语言 / Language"), m_language);

    m_theme = new QComboBox(this);
    m_theme->setObjectName(QStringLiteral("theme"));
    m_theme->addItem(tr("Follow the system"), int(theme::Mode::System));
    m_theme->addItem(tr("Light"), int(theme::Mode::Light));
    m_theme->addItem(tr("Dark"), int(theme::Mode::Dark));
    m_theme->setToolTip(tr("Light or dark colours for the whole app. It changes as soon as you click OK."));
    setThemeMode(theme::savedMode());
    form->addRow(QStringLiteral("主题 / Theme"), m_theme);

    auto* hint = new QLabel(tr("A new language is used after the app restarts."), this);
    hint->setWordWrap(true);
    hint->setForegroundRole(QPalette::PlaceholderText);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 14);
    layout->addLayout(form);
    layout->addWidget(hint);
    layout->addSpacing(10);
    layout->addWidget(buttons);
    setMinimumWidth(360);
}

QString SettingsDialog::languageCode() const
{
    return m_language->currentData().toString();
}

void SettingsDialog::setLanguageCode(const QString& code)
{
    const int index = m_language->findData(code);
    if (index >= 0)
        m_language->setCurrentIndex(index);
}

theme::Mode SettingsDialog::themeMode() const
{
    return theme::Mode(m_theme->currentData().toInt());
}

void SettingsDialog::setThemeMode(theme::Mode mode)
{
    const int index = m_theme->findData(int(mode));
    if (index >= 0)
        m_theme->setCurrentIndex(index);
}

void SettingsDialog::accept()
{
    // The theme needs no restart.
    const theme::Mode mode = themeMode();
    theme::saveMode(mode);
    if (mode != theme::activeMode())
        theme::apply(mode);

    const QString code = languageCode();
    language::saveCode(code);
    if (code != language::activeCode()) {
        QMessageBox box(QMessageBox::Question, tr("Restart to change the language"),
                        tr("The interface changes to %1 when the app restarts.\n\n"
                           "Restarting closes all tabs; images that are loaded are not kept.")
                            .arg(m_language->currentText()),
                        QMessageBox::NoButton, this);
        QPushButton* now = box.addButton(tr("Restart now"), QMessageBox::AcceptRole);
        box.addButton(tr("Later"), QMessageBox::RejectRole);
        box.exec();
        m_restart = box.clickedButton() == now;
    }
    QDialog::accept();
}

} // namespace iat
