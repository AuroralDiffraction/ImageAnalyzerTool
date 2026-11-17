#include "ui/Theme.h"

#include <QApplication>
#include <QGuiApplication>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QWidget>

namespace iat {
namespace theme {

namespace {

const char kSettingsKey[] = "theme";

Mode g_mode = Mode::System;
QString g_platformStyle;   // the style Qt chose for this platform, before any change
bool g_applying = false;
bool g_watching = false;
bool g_ownPalette = false;  // an application palette of ours is set

QString keyOf(Mode mode)
{
    switch (mode) {
    case Mode::Light: return QStringLiteral("light");
    case Mode::Dark:  return QStringLiteral("dark");
    case Mode::System: break;
    }
    return QStringLiteral("system");
}

bool hasStyle(const QString& name)
{
    return QStyleFactory::keys().contains(name, Qt::CaseInsensitive);
}

// The system's own preference (false where Qt cannot tell).
bool systemIsDark()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    return false;
#endif
}

// The style to use; `ownPalette`: the app sets a palette of its own.
QString styleFor(bool wantDark, bool ownPalette)
{
    QString style = g_platformStyle;
#if defined(Q_OS_WIN) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 1)
    // The Windows 11 look, also on Windows 10 (where it is not Qt's default).
    if (hasStyle(QStringLiteral("windows11")))
        style = QStringLiteral("windows11");
#endif
    // The classic Windows style draws its controls from light bitmaps only.
    if (wantDark && style == QLatin1String("windowsvista") && hasStyle(QStringLiteral("fusion")))
        style = QStringLiteral("fusion");
    // A palette of our own: the fusion style is the one that follows a
    // palette completely (the native styles draw some parts their own way).
    if (ownPalette && hasStyle(QStringLiteral("fusion")))
        style = QStringLiteral("fusion");
    return style;
}

// Widgets with a style sheet keep the colours the sheet was resolved with
// ("palette(mid)", …) until they are polished again.
void refreshStyleSheets()
{
    const auto widgets = QApplication::allWidgets();
    for (QWidget* w : widgets) {
        if (!w->styleSheet().isEmpty()) {
            w->style()->unpolish(w);
            w->style()->polish(w);
        }
        w->update();
    }
}

void setColor(QPalette& p, QPalette::ColorRole role, const QColor& active, const QColor& disabled)
{
    p.setColor(QPalette::Active, role, active);
    p.setColor(QPalette::Inactive, role, active);
    p.setColor(QPalette::Disabled, role, disabled);
}

} // namespace

QPalette darkPalette()
{
    const QColor window(43, 43, 46), base(30, 30, 32), text(232, 232, 234), off(128, 128, 132);
    const QColor button(56, 56, 60), accent(76, 156, 236);
    QPalette p;
    setColor(p, QPalette::Window, window, window);
    setColor(p, QPalette::WindowText, text, off);
    setColor(p, QPalette::Base, base, window);
    setColor(p, QPalette::AlternateBase, QColor(38, 38, 41), window);
    setColor(p, QPalette::Text, text, off);
    setColor(p, QPalette::Button, button, QColor(48, 48, 51));
    setColor(p, QPalette::ButtonText, text, off);
    setColor(p, QPalette::BrightText, Qt::white, Qt::white);
    setColor(p, QPalette::Highlight, accent, QColor(80, 80, 84));
    setColor(p, QPalette::HighlightedText, Qt::white, QColor(170, 170, 174));
    setColor(p, QPalette::ToolTipBase, QColor(60, 60, 64), QColor(60, 60, 64));
    setColor(p, QPalette::ToolTipText, text, text);
    setColor(p, QPalette::PlaceholderText, QColor(158, 158, 164), QColor(110, 110, 114));
    setColor(p, QPalette::Link, QColor(110, 178, 255), off);
    setColor(p, QPalette::LinkVisited, QColor(190, 150, 255), off);
    setColor(p, QPalette::Light, QColor(82, 82, 86), QColor(70, 70, 74));
    setColor(p, QPalette::Midlight, QColor(68, 68, 72), QColor(60, 60, 64));
    setColor(p, QPalette::Mid, QColor(92, 92, 97), QColor(72, 72, 76));
    setColor(p, QPalette::Dark, QColor(26, 26, 28), QColor(26, 26, 28));
    setColor(p, QPalette::Shadow, QColor(8, 8, 8), QColor(8, 8, 8));
    return p;
}

QPalette lightPalette()
{
    const QColor window(243, 243, 243), base(255, 255, 255), text(27, 27, 28), off(150, 150, 152);
    const QColor button(251, 251, 251), accent(0, 103, 192);
    QPalette p;
    setColor(p, QPalette::Window, window, window);
    setColor(p, QPalette::WindowText, text, off);
    setColor(p, QPalette::Base, base, window);
    setColor(p, QPalette::AlternateBase, QColor(247, 247, 247), window);
    setColor(p, QPalette::Text, text, off);
    setColor(p, QPalette::Button, button, QColor(245, 245, 245));
    setColor(p, QPalette::ButtonText, text, off);
    setColor(p, QPalette::BrightText, Qt::white, Qt::white);
    setColor(p, QPalette::Highlight, accent, QColor(200, 200, 202));
    setColor(p, QPalette::HighlightedText, Qt::white, QColor(110, 110, 112));
    setColor(p, QPalette::ToolTipBase, QColor(255, 255, 255), QColor(255, 255, 255));
    setColor(p, QPalette::ToolTipText, text, text);
    setColor(p, QPalette::PlaceholderText, QColor(110, 110, 114), QColor(165, 165, 168));
    setColor(p, QPalette::Link, QColor(0, 95, 184), off);
    setColor(p, QPalette::LinkVisited, QColor(110, 60, 170), off);
    setColor(p, QPalette::Light, QColor(255, 255, 255), QColor(255, 255, 255));
    setColor(p, QPalette::Midlight, QColor(234, 234, 234), QColor(238, 238, 238));
    setColor(p, QPalette::Mid, QColor(184, 184, 188), QColor(205, 205, 208));
    setColor(p, QPalette::Dark, QColor(150, 150, 154), QColor(180, 180, 184));
    setColor(p, QPalette::Shadow, QColor(100, 100, 104), QColor(140, 140, 144));
    return p;
}

Mode savedMode()
{
    const QString key = QSettings().value(QLatin1String(kSettingsKey)).toString();
    for (Mode m : {Mode::Light, Mode::Dark})
        if (key == keyOf(m))
            return m;
    return Mode::System;
}

void saveMode(Mode mode) { QSettings().setValue(QLatin1String(kSettingsKey), keyOf(mode)); }

Mode activeMode() { return g_mode; }

bool isDark()
{
    const QPalette p = QApplication::palette();
    return p.color(QPalette::Window).lightness() < p.color(QPalette::WindowText).lightness();
}

QString styleName()
{
    return QApplication::style() ? QApplication::style()->name().toLower() : QString();
}

void apply(Mode mode)
{
    if (g_applying || !qobject_cast<QApplication*>(QCoreApplication::instance()))
        return;
    g_applying = true;
    if (g_platformStyle.isEmpty())
        g_platformStyle = styleName();
    g_mode = mode;

    // An explicit Light or Dark: ask Qt for that colour scheme (Qt 6.8+);
    // Qt then switches the colours of every style that can show them, and of
    // the window frames. Where Qt cannot (older Qt, or a platform without
    // colour schemes, such as the off-screen platform of the tests and some
    // Linux desktops), the app sets a palette of its own instead.
    bool ownPalette = mode != Mode::System;
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    QStyleHints* hints = QGuiApplication::styleHints();
    hints->unsetColorScheme();  // so that the system's own preference can be read
    if (mode != Mode::System) {
        const Qt::ColorScheme wanted = mode == Mode::Dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
        hints->setColorScheme(wanted);
        ownPalette = hints->colorScheme() != wanted;  // the platform ignored the request
        if (ownPalette)
            hints->unsetColorScheme();
    }
#endif
    const bool wantDark = mode == Mode::Dark || (mode == Mode::System && systemIsDark());
    const QString style = styleFor(wantDark, ownPalette);
    if (style != styleName())
        QApplication::setStyle(style);
    if (ownPalette) {
        QApplication::setPalette(mode == Mode::Dark ? darkPalette() : lightPalette());
        g_ownPalette = true;
    } else if (g_ownPalette) {
        QApplication::setPalette(QPalette());  // back to the system's colours
        g_ownPalette = false;
    }
    refreshStyleSheets();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // "Follow the system": when the system changes between light and dark
    // while the app runs, choose the style again.
    if (!g_watching) {
        g_watching = true;
        QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, qApp, [] {
            if (g_mode == Mode::System)
                apply(Mode::System);
        });
    }
#endif
    g_applying = false;
}

} // namespace theme
} // namespace iat
