#pragma once

#include <QDialog>

namespace iat {

// Help > About: app icon, name, version, what it does, the author's GitHub
// and the Qt version.
class AboutDialog : public QDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget* parent = nullptr);

    static QString homepage();   // https://github.com/AuroralDiffraction
    static QString author();     // AuroralDiffraction (Tanatonium)
    static void openHomepage();  // opens the GitHub page in the browser
};

} // namespace iat
