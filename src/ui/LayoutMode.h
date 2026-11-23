#pragma once

namespace iat {

// How a session tab is laid out. Every tab has its own mode.
enum class LayoutMode {
    Single,  // one panel showing one image (the default for a new tab)
    Triple,  // light frames, dark frames and calibrated frames side by side
    Hyperspectral,  // raw 4 × 4 mosaic → 16-band cube: extraction, colour, spectra, analysis
};

} // namespace iat
