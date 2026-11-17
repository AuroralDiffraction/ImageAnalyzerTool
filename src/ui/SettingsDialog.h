#pragma once

#include "ui/Theme.h"

#include <QDialog>

class QComboBox;

namespace iat {

// File > Settings: the interface language and the theme (light / dark).
//
// The language is applied when the app starts, so after a change the dialog
// offers to restart the app (MainWindow asks via restartRequested()). The
// theme is applied at once when the dialog is accepted.
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    // The language selected in the dialog.
    QString languageCode() const;
    void setLanguageCode(const QString& code);
    // The theme selected in the dialog.
    theme::Mode themeMode() const;
    void setThemeMode(theme::Mode mode);
    // True once the dialog was accepted with a language other than the one in use.
    bool restartRequested() const { return m_restart; }

    void accept() override;

private:
    QComboBox* m_language = nullptr;
    QComboBox* m_theme = nullptr;
    bool m_restart = false;
};

} // namespace iat
