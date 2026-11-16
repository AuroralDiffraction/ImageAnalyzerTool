#include "ui/Language.h"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QSettings>
#include <QTranslator>

namespace iat {
namespace language {

namespace {

const char kSettingsKey[] = "language";
const char kSourceLanguage[] = "en";  // the language of the texts in the code

QString g_active = QString::fromLatin1(kSourceLanguage);
QTranslator* g_appTranslator = nullptr;  // this app's texts
QTranslator* g_qtTranslator = nullptr;   // Qt's own texts: standard buttons, About Qt, ...

bool isKnown(const QString& code)
{
    for (const LanguageInfo& l : languages())
        if (l.code == code)
            return true;
    return false;
}

void remove(QTranslator*& translator)
{
    if (!translator)
        return;
    QCoreApplication::removeTranslator(translator);
    delete translator;
    translator = nullptr;
}

} // namespace

QList<LanguageInfo> languages()
{
    return {
        {QStringLiteral("zh_CN"), QStringLiteral("简体中文")},
        {QString::fromLatin1(kSourceLanguage), QStringLiteral("English")},
    };
}

QString defaultCode() { return QStringLiteral("zh_CN"); }

QString savedCode()
{
    const QString code = QSettings().value(QLatin1String(kSettingsKey)).toString();
    return isKnown(code) ? code : defaultCode();
}

void saveCode(const QString& code)
{
    if (isKnown(code))
        QSettings().setValue(QLatin1String(kSettingsKey), code);
}

QString activeCode() { return g_active; }

bool install(const QString& code)
{
    remove(g_appTranslator);
    remove(g_qtTranslator);
    g_active = isKnown(code) ? code : QString::fromLatin1(kSourceLanguage);
    if (g_active == QLatin1String(kSourceLanguage))
        return true;  // the texts in the code are used as they are

    g_appTranslator = new QTranslator(QCoreApplication::instance());
    const bool ok = g_appTranslator->load(QStringLiteral(":/i18n/ImageAnalyzerTool_%1.qm").arg(g_active));
    if (ok)
        QCoreApplication::installTranslator(g_appTranslator);
    else
        remove(g_appTranslator);

    // Qt's texts: embedded at build time when the Qt installation has them,
    // otherwise from the Qt translations folder, if there is one.
    g_qtTranslator = new QTranslator(QCoreApplication::instance());
    const QString qtFile = QStringLiteral("qtbase_%1").arg(g_active);
    if (g_qtTranslator->load(QStringLiteral(":/i18n/%1.qm").arg(qtFile))
        || g_qtTranslator->load(qtFile, QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QCoreApplication::installTranslator(g_qtTranslator);
    else
        remove(g_qtTranslator);
    return ok;
}

} // namespace language
} // namespace iat
