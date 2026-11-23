# Architecture and plan

## What the app does (version 0.4)

| Requirement | Where it lives |
|---|---|
| Create new tabs | `MainWindow` (+ button, Ctrl+T, double-click on empty tab bar); tabs can be renamed (double-click / F2) and closed |
| Two layouts per tab: a single image (the default) or three panels (Light frames, Dark frames, Calibrated frames) | `SessionTab` holds three `ImagePanel`s in a splitter. `SessionTab::setLayoutMode` (`LayoutMode::Single` / `Triple`) hides the dark and calibrated panels and presents the light panel as "Image" (`ImagePanel::setSingleImage`) |
| Light / dark theme chosen in File → Settings; Windows 11 look | `theme::` (`src/ui/Theme.*`): style and colour scheme, applied at start-up and on change; `SettingsDialog` stores the choice |
| Chinese interface by default; language chosen in File → Settings | `language::` (`src/ui/Language.*`) installs Qt translators at start-up from the embedded `.qm` files; `SettingsDialog` stores the choice and offers a restart |
| Slide switch with icons to change the layout; every tab has its own | `ModeSwitch` in the tab bar's corner, next to +. `MainWindow` applies it to the current tab and moves it when the tab changes (`syncModeUi`) |
| Drag a .mat file (X × Y × Z), or one or more TIFF / PNG / JPEG images, onto Light or Dark | `ImagePanel` drop handling (`selectFiles` decides what is allowed) → `SessionTab::loadLight/loadDark` → `MatFileReader` or `ImageFileReader::readSequence` |
| Several image files form one stack (only images; only Light and Dark) | `ImageFileReader::readSequence`: frames in natural file-name order (`sortNaturally`), same size required |
| Light and Dark: Single frame or Mean, defaults Single (light) and Mean (dark) | Toggle in the panel header (`ImagePanel::frameMode`) → `SessionTab::refreshPanel` shows `ImageStack::frame(k)` or the mean (`processing::meanOverFrames`, MATLAB: `mean(stack, 3)`) |
| Choose a frame in Light and in Dark, first one by default | Frame slider + spin box in each panel, disabled in Mean mode |
| Calibrated = light image on display − dark image on display, automatic | `SessionTab::updateCalibrated`, runs whenever either image changes (frame, mode or file) |
| Calibrated image from a file; auto calibration off until removed | `SessionTab::loadCalibrated` / `restoreAutoCalibration`; red indicator `ImagePanel::setAutoCalibrationOff` |
| Mean, Std and Noise at the bottom of every panel | `ImagePanel` footer. Mean and Std come from `processing::calculateImageStats` (port of `matlab/calculateImageStats.m`), Noise from `processing::calculateImageNoise` (port of `matlab/calculateImageNoise.m`) |
| Histogram under every image, with Min and Max sliders for contrast | `HistogramWidget` + `ContrastControl` |
| ◀ / ▶ buttons for the frame | `ImagePanel::stepFrame` |
| The three images zoom and pan together | `ImageView::viewState / setViewState`, linked in `SessionTab::linkViews` |
| Export: raw data to .mat, displayed image to .png | Panel export row → `SessionTab::exportTo` → `MatFileWriter` (worker thread) / `QImage::save` |
| Image processing for every panel: a modular pipeline of stages (defect pixel / row / column correction, denoising, dynamic range compression, tone mapping, detail enhancement, sharpening, …), each with a choice of algorithms or bypassed | `isp::` in `src/core/Isp.*` (stages, algorithms, parameters as tables; `isp::run`). `PipelineDialog` draws the chain of nodes and the settings of the selected stage from those tables. `ImagePanel` owns the panel's `isp::Pipeline` and runs it on a worker thread |
| A/B button: a divider across the image for before and after | `ImagePanel::setAbCompare` → `ImageView::setCompareImage` (the unprocessed image drawn left of a draggable divider) |
| Right-click: copy image, copy image + histogram + Mean/Std/Noise | `ImagePanel::copyImageToClipboard / copyReportToClipboard / renderReport` |
| Hyperspectral workspace: third position of the switch | `LayoutMode::Hyperspectral`; `SessionTab` hides its splitter and shows its `HsiWorkspace` (made when first needed) |
| Image source: drop / open a file, or an image of any tab | `HsiWorkspace::loadFile`; **Import from workspace…** → `ImportImageDialog` over the candidates from every tab (`SessionTab::importCandidates`, collected by the provider `MainWindow` gives each tab) |
| Raw 4 × 4 mosaic → 16-band cube, four methods | `hsi::extract` (`src/core/Hsi.*`) |
| Cube export: ENVI, MAT, multi-page TIFF | `hsi::writeEnvi / writeMat / writeMultiPageTiff` (`src/io/HsiCubeIo.*`) |
| Correction panel: spectral correction, dark / white reference frames, sensor profiles | `hsi::computeCorrection / applyCorrection / correctedResponse / noiseGain / applyReferenceFrames` (`src/core/HsiCorrection.*`); `hsi::loadCorrectionMatrix`, `writeSensorProfile / readSensorProfile` (`src/io/HsiCubeIo.*`); details in `CorrectionDialog` |
| Chemometric preprocessing: absorbance, SNV / MSC / normalisations, Savitzky–Golay | `hsi::toAbsorbance / normaliseSpectra / savitzkyGolay`, called by `hsi::process` (`src/core/Hsi.*`) |
| PCA score plot with lasso selection | `ScorePlot` (density plot, groups, lasso); `hsi::selectInPolygon` (`src/core/HsiAnalysis.*`) |
| Colour reconstruction from a spectral response | `hsi::loadResponse` (io), `hsi::xyzMatrix / rgbMatrix / renderRgb` (`src/core/HsiColour.*`) |
| HSI processing: reflectance, MNF, guided filter, anti-aliasing, upscaling | `hsi::process` and the functions it calls (`src/core/Hsi.*`) |
| HSI analysis: spectra, similar areas, classes, PCA, indices, false colour | `src/core/HsiAnalysis.*`; spectrum plot `SpectrumPlot`; regions drawn with `ImageView::setRegionTool / setOverlays` |

Additions beyond the brief, kept small: a Load… button (as well as drag and drop), Auto / Full range contrast buttons, draggable histogram markers, log-scale histogram, zoom/pan, pixel read-out, and a check that light and dark sizes match.

## Layers

```
src/main.cpp           start-up, interface language, window icon, command-line options (--light, --dark)
translations/          interface translations (.ts), compiled into the executables
src/AppInfo.h.in       name / version / author / homepage, filled in by CMake
src/app/*.rc.in        Windows resources: exe icon and version info, filled in by CMake
resources/icons/       app icon (SVG sources, PNGs as Qt resources, .ico)
src/ui/      iaui      Qt Widgets
    MainWindow           tabs, menus, status bar
    AboutDialog          Help > About: icon, version, description, GitHub link
    Language             interface languages, the saved choice, installing translators
    Theme                widget style (Windows 11 style where Qt has it) and
                         light / dark colours; the saved choice, applying it
    Icons                the app's small icons (load, processing, data,
                         picture), drawn in code in the button-text colour
    SettingsDialog       File > Settings: interface language, theme
    ModeSwitch           slide switch with two icons: single image / three panels
    LayoutMode           the two layouts of a tab
    SessionTab           one tab: owns the data (one Source per panel: stack,
                         mean, noise), wires the three panels together, shows
                         one panel or all three
    ImagePanel           title, Single frame / Mean toggle, ImageView, frame
                         selector, histogram, sliders, statistics, export;
                         the panel's image processing pipeline
    PipelineDialog       the pipeline as a chain of nodes (on / bypass), with
                         the algorithm and parameters of the selected stage
    ImageView            QGraphicsView: zoom, pan, hover read-out; optional region
                         tool (click / drag a rectangle) and labelled overlays
    HsiWorkspace         the hyperspectral workspace of a tab: source, mosaic →
                         cube, processing, colour, views, analysis, export
    SpectrumPlot         spectra (mean ± std) against band or wavelength,
                         painted with the palette (no Qt Charts)
    ScorePlot            density scatter of two per-pixel scores (PCA), groups
                         in colour, lasso selection
    CorrectionDialog     the Correction panel's settings: spectral response,
                         spectral correction, exposures, sensor profile
    ImportImageDialog    choose a tab and one of its images
    HistogramWidget      histogram painting, draggable Min/Max markers
    ContrastControl      Min/Max sliders with spin boxes
src/core/    iacore    data model and processing (Qt Core only)
    ImageStack           [rows, cols, frames] in MATLAB order, original storage type
    Image2D              one frame as doubles, row-major
    ImageStats           min, max (NaN/Inf excluded), for histogram and contrast
    Histogram            binning (whole-number bins for integer data)
    processing::         meanOverFrames, subtract, percentile,
                         calculateImageStats, calculateImageNoise (MATLAB ports)
    isp::                image processing pipeline for monochrome sensor data:
                         tables of stages / algorithms / parameters, Pipeline
                         (the settings), run() (image in, image out), presets
    hsi::                hyperspectral code (Hsi, HsiColour, HsiAnalysis,
                         HsiCorrection, HsiLinalg): Cube, MosaicLayout, extract,
                         reference frames, spectral correction, process,
                         colour reconstruction, spectra, SAM, k-means, PCA,
                         band maths, rendering
matlab/                the original MATLAB functions the ports mirror
src/io/      iacore
    MatFileReader        MATLAB Level 5 MAT-files (-v6, -v7), including compressed
    MatFileWriter        writes -v7 MAT-files (compressed): numeric arrays, scalars, char
    TiffReader           baseline TIFF and BigTIFF: strips / tiles, none / LZW /
                         Deflate / PackBits, predictors, integer and float samples
    HsiCubeIo            hyperspectral cubes: ENVI read / write, MAT read / write,
                         multi-page TIFF write; spectral response files (.mat, .csv);
                         correction matrix files; sensor profiles (.iatsensor, JSON);
                         spectra as CSV
    ImageFileReader      one TIFF / PNG / JPEG file as an X × Y frame (PNG and JPEG
                         through QImage), several files as one stack, natural
                         file-name order, which files a panel accepts
```

Dependencies only point downwards: `ui` → `core`/`io`, never the reverse. `iacore` has no widget dependency (it uses Qt Gui only for QImage's PNG and JPEG decoders, which work headless), so the same code can drive a command-line batch tool later and is unit-tested on its own.

## Data flow

```
light.mat or images ─► reader ─► ImageStack ─┬─ frame(k) ───────────┐ Single frame
                       (worker)              └─ meanOverFrames ─────┤ Mean            ─► Light panel
                                                (worker, + noise)   ▼
                                                              light image L
dark.mat or images  ─► reader ─► ImageStack ─┬─ frame(j) ───────────┐ Single frame
                       (worker)              └─ meanOverFrames ─────┤ Mean            ─► Dark panel
                                                (worker, + noise)   ▼
                                                              dark image D
                                                                    │
                                         subtract(L, D) ◄───────────┘ ─► Calibrated panel
                                         (or an image loaded from a file: auto calibration off)
```

When a stack is loaded, its mean over frames and its noise are computed together on a worker thread, so switching between Single frame and Mean is instant afterwards. A panel in Mean mode shows "Calculating the mean …" until the mean is ready.

Inside each panel: when the image changes, the statistics and histogram are computed once. Moving a slider only remaps values to grey levels (≤ Min → black, ≥ Max → white, linear in between) and redraws.

## Image processing pipeline

```
image from the tab ─► [black level] ─► [defect pixels] ─► [defect rows / columns] ─► [digital gain]
(ImagePanel::rawImage)
        ─► [denoise] ─► [dynamic range compression] ─► [tone mapping] ─► [detail enhancement]
        values in DN ─────────────────────────────────────┘ display values 0..255 (double)
        ─► [sharpening] ─► [output quantisation] ─► image on display (ImagePanel::image)
```

- **One table, three users.** `isp::stages()` lists the stages in processing order; every stage lists its algorithms, every algorithm its parameters (id, label, range, default, linear / log / whole number / choice, unit). `isp::Pipeline` holds the settings (per stage: on or bypassed, which algorithm, the values of every algorithm so that switching back keeps them), `isp::run` executes them, and `PipelineDialog` builds its nodes and parameter rows from the same table. Adding an algorithm means adding one table entry and one branch in the stage's function in `Isp.cpp`; the dialog needs no change. Names and descriptions are marked with `QT_TRANSLATE_NOOP("iat::isp", …)` and translated with `isp::text()`.
- **The order is fixed**, as in a camera ISP: sensor corrections first (they must see the raw statistics), then gain and denoising (before anything amplifies the noise), then the mapping to display values, then detail enhancement and sharpening on the display values, and the single rounding to whole grey levels at the very end. The colour stages of a camera ISP (demosaic, white balance, colour matrix) are left out, since the data is monochrome.
- **Units.** The stages before tone mapping keep DN. Tone mapping produces display values 0–255 as doubles; nothing is rounded until output quantisation, so stretching never works on already-rounded data. Output quantisation is skipped (with a note) if tone mapping is off.
- **Noise-relative settings.** Thresholds of the defect, denoise, detail and sharpening stages are given in units of the pixel noise σ, which `isp::estimateNoise` measures from the image itself (the median absolute response of a 3 × 3 difference kernel that is blind to flat and linearly sloping content). The defaults therefore suit a 14-bit sensor frame and an 8-bit display image alike.
- **Defect rows / columns** are found pixel by pixel: a row counts as defective only if it differs from the rows above *and* below in the same direction, which an edge or a brightness slope in the picture does not. Detection runs in rounds (worst lines first), because a defective line disturbs what is predicted for its neighbours. Columns are handled as the rows of the transposed image.
- **Tone mapping** is a C++ port of the nine operators of the "14-bit to 8-bit tone mapping" comparison page (bit shift, percentile stretch, gamma, log, Reinhard global, plateau equalisation, CLAHE on log-encoded data, base / detail split with a guided filter, Mertens exposure fusion), working on doubles instead of a 14-bit lookup table, so calibrated images with negative or fractional values work too.
- **NaN / Inf pixels** take the median of the finite pixels while the filters run and are put back afterwards, so they neither spread nor disappear.
- **In the panel.** `ImagePanel::setImage` keeps the image from the tab as `rawImage()`. If a stage is on, `isp::run` is started on Qt's thread pool (`QtConcurrent::run` + `QFutureWatcher`); the previous picture stays until the result arrives. There is one run at a time per panel: a request made meanwhile (another frame, a moved slider) is remembered and started when the current run ends, and a result for older settings is still shown if it belongs to the image the panel has now, so the picture follows a slider while it is dragged. The panel reports `processingChanged`, which the tab counts as pending work like a file load (busy cursor, tests can wait for it).
- **Display range.** A tone-mapped result is shown from 0 to 255 (tone mapping has already chosen the grey levels). Otherwise the Min/Max range is kept as long as the settings that define the value scale (black level, gain, dynamic range compression, tone mapping) are unchanged (`Pipeline::sameScaleAs`), so tuning the denoiser does not reset the contrast.
- **A/B comparison.** With the panel's A/B button on, `ImagePanel::updateCompareImage` renders the unprocessed image (`m_shownRaw`, the one the image on display was made from) to 8-bit and hands it to `ImageView::setCompareImage`. The view paints it in `drawForeground` over the part of the picture left of a divider, then the divider, its handle and the A / B labels in window coordinates. The divider's position is a fraction of the *visible* part of the image, so it stays on the picture at any zoom or pan; pressing within a few pixels of it drags the divider instead of panning. While the comparison is on, the view repaints its whole viewport on scrolling (the divider is at a fixed place in the window). The A side uses the panel's Min/Max range when `Pipeline::sameScaleAs` says the values are comparable, otherwise the automatic 0.5–99.5 % stretch of the unprocessed image. Nothing else changes: `image()`, the statistics, copies and exports are those of the processed image.
- **What it does not touch.** The tab calculates the calibrated image from its own sources (`Source::shown`), never from a panel's processed image, so each panel's pipeline is independent. Noise values describe the raw stacks. "All frames" exports are raw; "Image on display" exports the processed image and records `Pipeline::summary(false)` (English, like all names in exported files) as `processing`.
- **Speed** (1028 × 1408, 2 cores, in the test environment): the global tone curves and CLAHE take about 20–30 ms, exposure fusion 0.2 s, guided and bilateral denoising 0.1 s, non-local means 0.8 s, defect lines 0.2 s. The row loops of the filters run on several threads (`std::thread`).

## Hyperspectral workspace

```
raw mosaic (file, or an image of any tab) ─► hsi::applyReferenceFrames (optional) ─► hsi::extract ─► cube (extracted)
    ─► hsi::applyCorrection (optional) ─► cube (unprocessed) ─► hsi::process ─► cube
                                              layout, black level,      │              reflectance → MNF →
                                              method                    │              guided → anti-alias → upscale
                                                                        └─ A/B "before" ───────────┐
                                                                                                    ▼
          view: mosaic / band / all bands / colour / false colour / PCA / index / similar / classes
          spectra: pixel under the mouse, kept points and areas (mean ± std), class means
```

- **Data.** `hsi::Cube` holds the bands one after the other (BSQ) as `float`, in an implicitly shared `QVector`, with the band wavelengths, units ("DN" or "reflectance") and a history of the steps (English; written into exports). Band b is channel b+1, whatever the mosaic layout.
- **Extraction** matches `extract_hsi_channels.m` bit for bit in *Sub-sample* mode (checked against MATLAB's output on sensor data by `tst_hsi` when `IAT_LOCAL_HSI_DATA` is set). *Registration* resamples every band from its phase (r, c) in the super-pixel to the centre (1.5, 1.5); *Bilinear* and *PPID* interpolate every band's samples at their true positions to full resolution, PPID around a pseudo-panchromatic image (the mosaic filtered with [1 2 2 2 1]ᵀ[1 2 2 2 1]/64, in which every channel has the same weight; the border is extended by whole mosaic periods so it stays balanced).
- **Colour.** `xyzMatrix` is `cmfᵀ (dλ ∘ R)` with the Wiener estimator `R = K Aᵀ (A K Aᵀ + α I)⁻¹` of `hsi_reconstruct.m`, CIE 1931 interpolated with MATLAB's `pchip`, so the 3 × 16 matrices agree with MATLAB to about 1e-13. `rgbMatrix` adds the white balance (scene white from grey world, white patch, a reference area or a flat spectrum; Bradford to D65) and XYZ → linear sRGB; `renderRgb` applies it with the exposure percentile (MATLAB's nearest-rank `pct`) and the sRGB curve. The spectral response is held in memory only.
- **Processing order** is fixed, like the ISP: corrections that need the raw statistics first (reflectance), denoising before anything that spreads noise, resampling last. MNF estimates the noise covariance from differences of horizontally adjacent pixels, whitens with its Cholesky factor, keeps the first *k* components of the whitened data's eigenvectors and transforms back, all as one B × B matrix per pixel. The guided filter uses the mean of all bands as the guide for every band.
- **Correction.** Reference frames are applied to the raw mosaic, before extraction, so the flat field is per sensor pixel (every channel of every super-pixel has its own gain); the image and the frames are first brought to the same units (MSB-aligned data shifted down by `msbScale`) and then extracted with bit depth 0, and a dark frame replaces the black level. The spectral correction is applied to the extracted cube (`m_cubeExtracted` → `m_cubeRaw`), so changing it does not extract again, and A/B treats the corrected cube as "unprocessed". `effectiveResponse()` (the response `C·A` when correction is on) feeds the colour matrix and the wavelengths. The noise gain of a correction is the mean over its rows of ‖row‖₂ / |Σ row|: when most of every channel's response is background shared with the others, narrowing the bands amplifies noise many times over, the more so the smaller the regularisation.
- **Views.** Up to four panes (`HsiWorkspace::Pane`: an `ImageView`, its view mode, band and false-colour bands); a click on a pane (event filter on its viewport and title) makes it the active pane, and the toolbar's *View* and the options panel under the views edit the active pane's settings. `renderMode` reads the band and bands of the pane being drawn (`m_renderPane`). The grid has equal row and column stretches and the options panel a fixed height (its tallest page), so activating a pane never moves the views; only the rows and columns in use have a stretch (`updateGridStretch`), so one pane fills the grid. `refreshAll` sets `m_needExtract` / `m_needCorrect`, drops the cached colour and analysis results and starts the job at once; `refreshPane` drops only what that pane's mode caches and draws again; `resetPane` restores a pane's default mode, band and bands and fits it; `resetAll` also resets the processing and colour settings (one job, since `setProcessingSettings` schedules it). A pane that is zoomed or panned stores the shared state in processed-cube pixels (`m_sharedView`) and the others apply it at their own scale (the raw mosaic is 4× larger); the band gallery is not synchronised. The pixel under the mouse in any pane becomes pointer markers (`ImageView::setPointers`) in the other panes — in every tile of the gallery. Overlays carry what they stand for (`OverlayRef`), so right-clicking a label (`ImageView::overlayRemoveRequested`) removes that area or reference.
- **A/B.** Each pane renders its "after" picture first and keeps its display stretch (`Stretch`: band ranges or the colour exposure scale); the "before" picture of the unprocessed cube reuses it when both cubes have the same units, so differences are not hidden by a separate contrast stretch.
- **Coordinates.** Kept areas, the white reference and the colour reference are stored in pixels of the unprocessed cube and scaled to the processed cube (after upscaling) and to the picture on display (the raw mosaic is 4× larger in sub-sample mode).
- **Threads.** Extraction and processing run as one job on Qt's thread pool (`QtConcurrent::run` + `QFutureWatcher`), started 120 ms after the last change of a setting; a change during a run starts another run when it ends. k-means runs on its own. The workspace reports `busyChanged`, which the tab counts as pending work like a file load, and shows **Processing…** over the image when work lasts longer than 200 ms. The filters write each band from its own thread into a cube copied from the input: they call `Cube::detach()` first, because Qt's copy-on-write is not thread-safe (several threads detaching one shared copy at once freed memory still in use; `tst_hsi` runs every combination of steps on a shared cube). The views are rendered on the GUI thread (a 256 × 320 × 16 cube renders in milliseconds; PCA and the angle map take a few tens of milliseconds at full resolution).
- **Private data.** The spectral response and real sensor captures are for local development only: `.gitignore` excludes them (including `_cube.mat` files, which contain the QE curves), and the tests in the repository use synthetic data.

## Conventions

- **Dimensions.** A MATLAB array of size `[X Y Z]` is read as X rows × Y columns × Z frames and drawn like `imagesc`: X runs top to bottom, Y left to right. An image file is one X × Y frame, rows × columns as `imread` returns it. The sensor data this was first written for (`all_pixel`) is 1028 × 1408 × 20.
- **Indices** shown in the UI (frame number, pixel row/column) are 1-based, as in MATLAB.
- **Precision.** All processing is done in double. `ImageStack` keeps values in the type they were stored in, to save memory: MATLAB saves a whole-number double array such as `all_pixel` as uint16, so a 1028 × 1408 × 20 stack takes 58 MB instead of 232 MB. 16-bit TIFF and PNG files are kept as uint16 in the same way.
- **Mean, Std and Noise** follow the MATLAB functions exactly, including normalising by N and letting NaN propagate. Sums use compensated summation, so results agree with an exact calculation to about 12 significant digits. Which data each value describes:

  | Panel | Mean / Std: `calculateImageStats` of the image on display | Noise: `calculateImageNoise` of the stack behind it |
  |---|---|---|
  | Light frames | `light(:, :, k)` (Single frame) or `mean(light, 3)` (Mean) | `light` (all Z frames) |
  | Dark frames | `mean(dark, 3)` (Mean) or `dark(:, :, j)` (Single frame) | `dark` (all Z frames) |
  | Calibrated frames | light image − dark image, e.g. `light(:, :, k) − mean(dark, 3)` | `light − D` (all Z light frames minus the dark image D on display); this always equals the light noise, because subtracting the same image from every frame leaves the temporal variance unchanged |
  | Calibrated frames, loaded from a file | the loaded image (or frame) | the loaded stack |

  Mean and Std change when you pick another frame or mode; Noise doesn't. The noise values are computed on a worker thread; the calibrated noise is recalculated when the dark image changes, 150 ms after the last change so stepping through dark frames stays smooth. Hover over a value to see its definition.
- **Histogram and contrast** use the finite pixels only, so a few NaN/Inf pixels don't break the display.
- **Single image mode.** The single image is the light stack shown in the light panel on its own, so nothing is loaded twice and both layouts use the same code for the histogram, contrast and Mean / Std / Noise. Switching to three panels keeps the image as the light frames; dark frames and a loaded calibrated image are kept while their panels are hidden. While hidden, the calibrated panel is not recalculated (stepping through frames stays as fast as with one image); `setLayoutMode` brings it up to date when the three panels return, and the views that were hidden take over the zoom and pan. In this mode the panel's texts, the default export names (`_frame001`, `_mean`, `_all`) and the exported variables (`frame`, `mean_frame`, `frames`) don't mention light frames. File → Open dark frames… and Open calibrated image… are disabled.
- **Theme.** `theme::apply` (`src/ui/Theme.*`) is called at start-up with the saved choice (Follow the system, Light, Dark) and again when the choice changes in File → Settings; it needs no restart. It picks the widget style and the colours. Style: on Windows, Qt's `windows11` style when the Qt in use has it for this Windows version (Qt 6.8.1+ on Windows 10 and 11), otherwise Qt's default; the classic `windowsvista` style draws from light bitmaps only, so `fusion` replaces it when dark is wanted. Colours: with Qt 6.8+ `QStyleHints::setColorScheme` (Qt then also colours the window frame); with older Qt an explicit Light or Dark sets `fusion` and a palette defined in `Theme.cpp`. In Follow-the-system mode the choice is made again when the system switches (`QStyleHints::colorSchemeChanged`). For this to reach open windows, **widgets take colours from the palette when they paint and keep no copies**: secondary text uses `setForegroundRole(QPalette::PlaceholderText)` instead of a palette copy, the custom-painted widgets (histogram, mode switch, pipeline nodes) read `palette()` in `paintEvent`, and `ImagePanel` draws its rounded frame, colour strip and drop zone itself rather than with a style sheet (a widget with a style sheet, and everything inside it, keeps the palette it had when the sheet was applied). The figure for the clipboard is always on white and is drawn with `theme::lightPalette()`.
- **Icons and header buttons.** `icons::icon` (`src/ui/Icons.*`) returns icons backed by a `QIconEngine` that draws the glyph on a 16 × 16 grid each time it is asked, in the palette's button-text colour (disabled colour when disabled), so one icon serves both themes, follows a change of theme and is sharp at any screen scaling. The Processing icon's slider knobs take the panel's colour while processing is on. In a panel narrower than 640 px (three panels in a normal window) the Processing… and Load… buttons show the icon only; wider panels show icon and text (`ImagePanel::updateHeaderButtons`, from `resizeEvent`). The buttons keep their text either way, for tooltips, accessibility and tests.
- **Languages.** Texts in the code are English and wrapped in `tr()`; `iacore`, which has no QObject classes, uses `QCoreApplication::translate("iat::<Reader>", …)`. The readers' parse errors travel as exceptions: fixed messages are marked with `QT_TRANSLATE_NOOP` where they are thrown and translated where they are caught, and messages with values in them are translated when thrown. `translations/ImageAnalyzerTool_zh_CN.ts` is compiled by `lrelease` during the build (`qt_add_lrelease`) and embedded as `:/i18n/ImageAnalyzerTool_zh_CN.qm`, together with Qt's own `qtbase_zh_CN.qm` (standard buttons, About Qt) when the Qt installation has it. `main.cpp` installs the saved language (Chinese if none) before any widget exists. Widgets take their texts when they are built, so a change of language applies after a restart rather than live; this keeps every widget free of retranslation code. The tests run without a translator, so they see the English texts. Names in exported .mat files and MATLAB expressions in tooltips are not translated.
- **Linked views.** Zoom and pan are shared by the three views of a tab, as one `ViewState` (fit flag, zoom, centre in image coordinates). A view reports only user actions (wheel, drag, scroll bars, double-click); changes it makes to itself (fitting on resize, showing a new image) are not reported, so the views never feed back into each other. A view that starts showing a new image adopts the shared state.
- **Exports** hold raw values, not the 8-bit display: frames are saved as double, and "All frames" of the light and dark stacks (and of a stack loaded into the Calibrated panel) keeps the original class and storage type, so it is identical to the input. The calibrated stack is written frame by frame straight into the output buffer, so it isn't held in memory twice. On a 1028 × 1408 × 20 stack (in the test environment): one frame saves in about 0.2 s, the light stack in about 1 s, and the calibrated stack in about 4 s, to an 87 MB file. Saving runs on a worker thread, so the window stays usable. The PNG is the displayed image (current contrast, 8-bit grey, full resolution). "Copy image with histogram and statistics" puts a single image on the clipboard, with no text, so every application pastes it as a figure. The figure shows the displayed image at full resolution, then the histogram painted by the panel's own widget (`HistogramWidget::paintAsShown`: same colours, markers, log setting and proportions) scaled up to the image width, then Mean / Std / Noise. Text and spacing are scaled by the same factor, so the figure looks like an enlarged panel.
- **Contrast.** It resets to Auto (the 0.5–99.5 % percentiles) when a new file is loaded, so a few hot or saturated pixels (16383 in 14-bit dark frames) don't wash the image out. **Full range** shows min to max instead. The range is kept when you only change frame or switch between Single frame and Mean, so you can compare with a fixed display range.
- **Image files.** Values are kept as stored, like `imread`. Grey images saved as RGB keep their values; colour images are converted with `rgb2gray`'s weights to double. A stack of files of different types is converted to double. Several files can only be combined for images, so a drop of two `.mat` files, or a `.mat` file with images, is refused (the drag shows a "no" cursor and the status bar says why).
- **Automatic calibration.** The Calibrated panel is calculated from the other two unless an image was loaded into it from a file. That is shown by the red **● Auto calibration off** next to its title, and changes in the light and dark panels then leave it alone. Clicking the indicator removes the loaded image and calculates light − dark again.

## Threading

Reading files, computing the mean over frames and computing the noise values all run on Qt's global thread pool (`SessionTab::runInBackground`, built on `QtConcurrent::run` plus `QFutureWatcher`), so the window stays responsive. Each load gets a generation number, and each loaded stack an id, so if you drop a second file before the first has finished, the older result is discarded. On a 1028 × 1408 × 20 uint16 stack: reading the .mat file takes about 0.45 s, the mean about 0.06 s, and each noise value about 0.1 s; reading the same 20 frames from TIFF or PNG files takes about 0.4 s.

## MAT-file support

- Supported: Level 5 files (MATLAB `-v6` and the default `-v7`), compressed or not, either byte order. Real numeric and logical arrays with 2 or 3 dimensions, in any storage type.
- Recognised but not loaded: struct, cell, char, sparse, complex arrays and objects. These are listed in the error message so you can see what the file contains.
- Not supported yet: `-v7.3` files, which are HDF5 files. The app says so and suggests `save('file.mat', 'var', '-v7')`. Adding them needs the HDF5 library, most simply through matio (BSD licence), behind the same `MatFileReader` interface.

## How this grows into the longer-term plan

- **Steps and batch processing.** The `processing::` functions are pure: images in, image out, no GUI. The image processing pipeline (`isp::`) is the first set of steps with a table of parameters and a settings object (`isp::Pipeline`); saving that object to a file (JSON: stage key, algorithm id, parameter ids and values, all of which are stable names) gives saved recipes, and the chain a tab performs (mean dark → subtract) becomes steps in front of it. The GUI and a command-line runner then execute the same recipe.
- **MATLAB.** The MATLAB Engine goes in a separate plugin DLL that implements the same step interface and is loaded only if MATLAB is installed, so the main `.exe` keeps running on PCs without MATLAB.
- **Hyperspectral.** The workspace exists (see above). Next steps: saving the mosaic layout and spectral response together as a sensor profile, spectral correction (a channel cross-talk matrix from the QE curves), endmember extraction and unmixing, supervised classification from labelled areas, and HDF5 export for large cubes.

## Open decisions

- **Licence.** Not set yet. If it goes out as GPL and later loads MATLAB's proprietary Engine libraries, add an explicit linking exception for MATLAB, and avoid GPL-only Qt modules such as Qt Charts.

## Candidates for later versions


1. Pixel readout in all three panels at once (light, dark and calibrated value at the same pixel).
2. Flat-field correction, as a fourth panel.
3. TIFF export (16-bit / 32-bit float) alongside .mat.
4. v7.3 file support.
5. Multi-page TIFF files as stacks (now only the first page is read).
6. Image processing: saving and loading pipeline settings; flat-field (shading) correction from a reference frame; temporal denoising over the frames of a stack; a defect map (a fixed list of bad pixels and lines) instead of detection per image; the colour stages (demosaic, white balance, colour matrix) once colour sensors are supported.
