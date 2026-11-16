#pragma once

#include <QDialog>

class QComboBox;

namespace iat {

// File > Settings: the interface language.
//
// The language is applied when the app starts, so after a change the dialog
// offers to restart the app (MainWindow asks via restartRequested()).
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    // The language selected in the dialog.
    QString languageCode() const;
    void setLanguageCode(const QString& code);
    // True once the dialog was accepted with a language other than the one in use.
    bool restartRequested() const { return m_restart; }

    void accept() override;

private:
    QComboBox* m_language = nullptr;
    bool m_restart = false;
};

} // namespace iat
