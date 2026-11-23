#pragma once

#include <QColor>
#include <QIcon>

namespace iat {

// The app's own small icons, drawn in code.
//
// They are line drawings in the colour of the button text, taken from the
// palette each time they are painted, so they suit the light and the dark
// theme and follow a change of theme. Being vector drawings they are sharp
// at any screen scaling.
namespace icons {

enum class Glyph {
    Open,     // a folder: load files
    Process,  // three sliders: image processing
    Data,     // a table: save values (.mat)
    Picture,  // a picture: save the image (.png)
    Refresh,  // a circular arrow: do it again with the current settings
    Reset,    // an arrow turning back: back to the defaults
};

// `accent`, if valid, marks the icon as "on": part of the drawing takes
// that colour (the knobs of the Process sliders).
QIcon icon(Glyph glyph, const QColor& accent = QColor());

} // namespace icons
} // namespace iat
