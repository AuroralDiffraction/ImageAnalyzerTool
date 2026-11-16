#pragma once

#include <QList>
#include <QString>

namespace iat {

// Interface language.
//
// The texts in the source code are English. A translation is a Qt .qm file
// built from translations/ImageAnalyzerTool_<code>.ts and embedded in the
// .exe as :/i18n/ImageAnalyzerTool_<code>.qm. Chinese is the default; the
// user chooses the language in File > Settings, and the choice is stored in
// the settings and applied when the app starts (see main.cpp).
//
// To add a language: add its .ts file to IAT_TS_FILES in CMakeLists.txt and
// its code and name to languages() in Language.cpp.
struct LanguageInfo {
    QString code;  // "zh_CN", "en"
    QString name;  // in the language itself: "简体中文", "English"
};

namespace language {

QList<LanguageInfo> languages();
QString defaultCode();  // "zh_CN"

// The language chosen in the settings (the default if none, or an unknown one, is stored).
QString savedCode();
void saveCode(const QString& code);

// Installs the translators for `code` in the application, replacing those of
// another language. Call before any window is created: widgets take their
// texts when they are built. Returns false if a translation file is missing
// (the interface is then English).
bool install(const QString& code);
// The language install() was last called with ("en" before any call).
QString activeCode();

} // namespace language
} // namespace iat
