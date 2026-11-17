#pragma once

#include <QPalette>
#include <QString>

namespace iat {

// Look of the application: widget style and light / dark colours.
//
// Style. On Windows the app uses Qt's "windows11" style (the look of
// Windows 11: rounded controls, light and dark) wherever the Qt it was built
// with has it: Qt 6.7 or newer on Windows 11, Qt 6.8.1 or newer on
// Windows 10. Otherwise Qt's default style for the platform is kept. The
// older default on Windows ("windowsvista") cannot draw dark controls, so
// the platform-independent "fusion" style takes over when dark is wanted.
//
// Colours. The user chooses Follow the system, Light or Dark in
// File > Settings; the choice is stored in the settings and applied when the
// app starts (see main.cpp) and whenever it is changed. With Qt 6.8 or newer
// Qt itself switches the colours (QStyleHints::setColorScheme), including
// the window frame. With older versions an explicit Light or Dark uses the
// fusion style with a palette defined here.
//
// Widgets must take their colours from the palette when they paint (not
// keep copies), so that a change of theme reaches them at once.
namespace theme {

enum class Mode { System, Light, Dark };

// The mode chosen in the settings (System if none is stored).
Mode savedMode();
void saveMode(Mode mode);

// Applies style and colours to the running application. Can be called at
// any time; open windows are updated.
void apply(Mode mode);
// The mode apply() was last called with (System before any call).
Mode activeMode();

// True if the application's colours are dark now.
bool isDark();
// Name of the widget style in use, e.g. "windows11", "fusion".
QString styleName();

// The palettes used for an explicit Light or Dark with Qt older than 6.8.
// The light one is also for figures drawn on white, whatever the theme.
QPalette lightPalette();
QPalette darkPalette();

} // namespace theme
} // namespace iat
