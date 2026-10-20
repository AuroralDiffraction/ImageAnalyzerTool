# Architecture and plan: version 1

## What version 1 does

| Requirement | Where it lives |
|---|---|
| Create new tabs | `MainWindow` (+ button, Ctrl+T, double-click on empty tab bar); tabs can be renamed (double-click / F2) and closed |
| Three panels per tab: Light frames, Dark frames, Calibrated frames | `SessionTab` holds three `ImagePanel`s in a splitter |
| Drag a .mat file (X × Y × Z) onto Light or Dark | `ImagePanel` drop handling → `SessionTab::loadLight/loadDark` → `MatFileReader` |
| Light: choose one of the Z images, first one by default | Frame slider + spin box in the Light panel → `ImageStack::frame(k)` |
| Dark: pixel-by-pixel mean over Z | `processing::meanOverFrames` (MATLAB: `mean(dark, 3)`) |
| Calibrated = selected light frame − mean dark frame, automatic | `SessionTab::updateCalibrated`, runs whenever the light frame or the dark frame changes |
| Mean, Std and Noise at the bottom of every panel | `ImagePanel` footer. Mean and Std come from `processing::calculateImageStats` (port of `matlab/calculateImageStats.m`), Noise from `processing::calculateImageNoise` (port of `matlab/calculateImageNoise.m`) |
| Histogram under every image, with Min and Max sliders for contrast | `HistogramWidget` + `ContrastControl` |
| ◀ / ▶ buttons for the light frame | `ImagePanel::stepFrame` |
| The three images zoom and pan together | `ImageView::viewState / setViewState`, linked in `SessionTab::linkViews` |
| Export: raw data to .mat, displayed image to .png | Panel export row → `SessionTab::exportTo` → `MatFileWriter` (worker thread) / `QImage::save` |
| Right-click: copy image, copy image + histogram + Mean/Std/Noise | `ImagePanel::copyImageToClipboard / copyReportToClipboard / renderReport` |

Additions beyond the brief, kept small: a Load… button (as well as drag and drop), Auto / Full range contrast buttons, draggable histogram markers, log-scale histogram, zoom/pan, pixel read-out, and a check that light and dark sizes match.

## Layers

```
src/main.cpp           start-up, window icon, command-line options (--light, --dark)
src/AppInfo.h.in       name / version / author / homepage, filled in by CMake
src/app/*.rc.in        Windows resources: exe icon and version info, filled in by CMake
resources/icons/       app icon (SVG sources, PNGs as Qt resources, .ico)
src/ui/      iaui      Qt Widgets
    MainWindow           tabs, menus, status bar (with the author's GitHub link)
    AboutDialog          Help > About: icon, version, description, GitHub link
    SessionTab           one tab: owns the data, wires the three panels together
    ImagePanel           title, ImageView, frame selector, histogram, sliders, mean
    ImageView            QGraphicsView: zoom, pan, hover read-out
    HistogramWidget      histogram painting, draggable Min/Max markers
    ContrastControl      Min/Max sliders with spin boxes
src/core/    iacore    data model and processing (Qt Core only, no GUI)
    ImageStack           [rows, cols, frames] in MATLAB order, original storage type
    Image2D              one frame as doubles, row-major
    ImageStats           min, max (NaN/Inf excluded), for histogram and contrast
    Histogram            binning (whole-number bins for integer data)
    processing::         meanOverFrames, subtract, percentile,
                         calculateImageStats, calculateImageNoise (MATLAB ports)
matlab/                the original MATLAB functions the ports mirror
src/io/      iacore
    MatFileReader        MATLAB Level 5 MAT-files (-v6, -v7), including compressed
    MatFileWriter        writes -v7 MAT-files (compressed): numeric arrays, scalars, char
```

Dependencies only point downwards: `ui` → `core`/`io`, never the reverse. `iacore` has no GUI dependency, so the same code can drive a command-line batch tool later and is unit-tested on its own.

## Data flow

```
light.mat ─► MatFileReader ─► ImageStack ─► frame(k) ─► Image2D ─► Light panel
                (worker)                                    │
dark.mat  ─► MatFileReader ─► ImageStack ─► meanOverFrames ─► Image2D ─► Dark panel
                (worker)                      (worker)          │
                                                                ▼
                                     subtract(light frame k, dark mean) ─► Calibrated panel
```

Inside each panel: when the image changes, the statistics and histogram are computed once. Moving a slider only remaps values to grey levels (≤ Min → black, ≥ Max → white, linear in between) and redraws.

## Conventions

- **Dimensions.** A MATLAB array of size `[X Y Z]` is read as X rows × Y columns × Z frames and drawn like `imagesc`: X runs top to bottom, Y left to right. Your example file `all_pixel` is 1028 × 1408 × 20.
- **Indices** shown in the UI (frame number, pixel row/column) are 1-based, as in MATLAB.
- **Precision.** All processing is done in double. `ImageStack` keeps values in the type they were stored in, to save memory: MATLAB saved `all_pixel` (class double) as uint16, so the 20-frame stack takes 58 MB instead of 232 MB.
- **Mean, Std and Noise** follow the MATLAB functions exactly, including normalising by N and letting NaN propagate. Sums use compensated summation, so results agree with an exact calculation to about 12 significant digits. Which data each value describes:

  | Panel | Mean / Std: `calculateImageStats` of the image on display | Noise: `calculateImageNoise` of the stack behind it |
  |---|---|---|
  | Light frames | `light(:, :, k)` | `light` (all Z frames) |
  | Dark frames | `mean(dark, 3)` | `dark` (all Z frames) |
  | Calibrated frames | `light(:, :, k) − mean(dark, 3)` | `light − mean(dark, 3)` (all Z frames); this always equals the light noise, because subtracting the same frame from every frame leaves the temporal variance unchanged |

  Mean and Std change when you pick another frame; Noise doesn't. The noise values are computed on a worker thread. Hover over a value to see its definition.
- **Histogram and contrast** use the finite pixels only, so a few NaN/Inf pixels don't break the display.
- **Linked views.** Zoom and pan are shared by the three views of a tab, as one `ViewState` (fit flag, zoom, centre in image coordinates). A view reports only user actions (wheel, drag, scroll bars, double-click); changes it makes to itself (fitting on resize, showing a new image) are not reported, so the views never feed back into each other. A view that starts showing a new image adopts the shared state.
- **Exports** hold raw values, not the 8-bit display: frames are saved as double, and "All frames" of the light stack keeps the original class and storage type, so it is identical to the input. The calibrated stack is written frame by frame straight into the output buffer, so it isn't held in memory twice. On the example file (in the test environment): one frame saves in about 0.2 s, the light stack in about 1 s, and the calibrated stack in about 4 s, to an 87 MB file. Saving runs on a worker thread, so the window stays usable. The PNG is the displayed image (current contrast, 8-bit grey, full resolution). "Copy image with histogram and statistics" puts a single image on the clipboard, with no text, so every application pastes it as a figure. The figure shows the displayed image at full resolution, then the histogram painted by the panel's own widget (`HistogramWidget::paintAsShown`: same colours, markers, log setting and proportions) scaled up to the image width, then Mean / Std / Noise. Text and spacing are scaled by the same factor, so the figure looks like an enlarged panel.
- **Contrast.** It resets to Auto (the 0.5–99.5 % percentiles) when a new file is loaded, so a few hot or saturated pixels (16383 in the example dark frames) don't wash the image out. **Full range** shows min to max instead. The range is kept when you only change frame, so you can step through frames with a fixed display range.

## Threading

Reading a file, averaging the dark stack and computing the noise values all run on Qt's global thread pool (`SessionTab::runInBackground`, built on `QtConcurrent::run` plus `QFutureWatcher`), so the window stays responsive. Each load gets a generation number, so if you drop a second file before the first has finished, the older result is discarded. On the example file: reading takes about 0.45 s, the dark mean about 0.06 s, and each noise value about 0.1 s.

## MAT-file support

- Supported: Level 5 files (MATLAB `-v6` and the default `-v7`), compressed or not, either byte order. Real numeric and logical arrays with 2 or 3 dimensions, in any storage type.
- Recognised but not loaded: struct, cell, char, sparse, complex arrays and objects. These are listed in the error message so you can see what the file contains.
- Not supported yet: `-v7.3` files, which are HDF5 files. The app says so and suggests `save('file.mat', 'var', '-v7')`. Adding them needs the HDF5 library, most simply through matio (BSD licence), behind the same `MatFileReader` interface.

## How this grows into the longer-term plan

- **Steps and batch processing.** The `processing::` functions are pure: images in, image out, no GUI. When batch processing arrives, each one gets wrapped as a step with saveable parameters, and the chain a tab performs (mean dark → subtract) becomes the first saved recipe. The GUI and a command-line runner then execute the same recipe.
- **MATLAB.** The MATLAB Engine goes in a separate plugin DLL that implements the same step interface and is loaded only if MATLAB is installed, so the main `.exe` keeps running on PCs without MATLAB.
- **Hyperspectral.** "Raw frame → cube" becomes another step (mosaic layout from a sensor profile), and the Light panel gains a band selector next to the frame selector.

## Open decisions

- **Licence.** Not set yet. If it goes out as GPL and later loads MATLAB's proprietary Engine libraries, add an explicit linking exception for MATLAB, and avoid GPL-only Qt modules such as Qt Charts.

## Candidates for version 2

1. Pixel readout in all three panels at once (light, dark and calibrated value at the same pixel).
2. Flat-field correction, as a fourth panel.
3. TIFF export (16-bit / 32-bit float) alongside .mat.
4. v7.3 file support.
