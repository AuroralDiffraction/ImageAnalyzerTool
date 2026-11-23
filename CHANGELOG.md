# Changelog

## 0.5.0 (2026-11-23)

- **Hyperspectral workspace**, the third position of the layout switch (icon: a stack of layers). It turns a raw 4 × 4 Fabry-Perot mosaic into a 16-band cube and works with it:
  - **Correction** panel under Source: **Load spectral response…**, **Spectral correction** (a 16 × 16 matrix that turns the overlapping Fabry-Perot channels into narrow bands at the channel peaks, computed from the spectral response or loaded from a file; the residual and the noise gain are shown), optional **Dark frame** and **White frame** (from a file or any tab) for a per-pixel flat field in reflectance, with exposure-time normalisation. **Settings…** holds the details: the spectral response, a plot of the measured and corrected channel responses, the correction's width and regularisation, exposures, and **sensor profiles** (`.iatsensor`: layout, bit depth, method and the paths of the files).
  - **Source**: drop or open a raw mosaic (TIFF, PNG, JPEG, .mat), or **Import from workspace…** to take the single image, light or dark frames, or calibrated image of any open tab (values as they are, before image processing). A cube can be opened directly (ENVI .hdr / .img, .mat).
  - **Mosaic → cube**: 4 × 4 channel layout in a **Mosaic layout…** settings dialog with four presets — row by row, row by row (vertical), serpentine and serpentine (vertical) `[1 8 9 16; 2 7 10 15; 3 6 11 14; 4 5 12 13]`, the default, black level, bit depth (MSB-aligned data shifted down as in `hsi_reconstruct.m`), and four methods: sub-sample (identical to `extract_hsi_channels.m`), sub-sample + registration of every band to the super-pixel centre, full-resolution bilinear, and full-resolution PPID demosaicking.
  - **Export** the cube as ENVI (.hdr + .img), MATLAB .mat (`cube`, `wavelengths`, `mosaicLayout`) or multi-page TIFF, 32-bit float or 16-bit; the view as PNG; spectra as CSV.
  - **Colour reconstruction**: from the spectral response (.mat with `lambda` and `QE_percent`, or .csv with Band1 … Band16 columns) for sRGB colour by the method of `hsi_reconstruct.m` (Wiener estimate, CIE 1931, white balance: grey world, white patch, reference area or none; Bradford to D65). The 3 × 16 matrix matches MATLAB to 1e-12. The response file is only read, never copied or exported. The band peaks become the band wavelengths.
  - **Processing**: reflectance from a white reference area (drawn while reflectance is on; **Clear** removes it), MNF spectral denoising, guided (edge-preserving) denoising, anti-aliasing low-pass, absorbance, SNV / MSC / unit-length / unit-area / maximum normalisation, Savitzky–Golay smoothing and 1st / 2nd derivatives, 2–4× upscaling; A/B against the unprocessed cube. **Processing…** is shown over the image while the cube is being made.
  - **Analysis**: spectrum of the pixel under the mouse and of kept points and areas (mean ± std), all bands at once, false colour (natural or most-contrast band triplets), principal components, spectral indices from a formula (`b7`, `R531`, presets such as PRI; indices that need near-infrared are refused), similar areas by spectral angle, and k-means classes with their mean spectra.
  - **Two or four views** side by side, with zoom, pan and a mouse pointer mirrored in all of them. Click a view to make it active: **View:** and the options under the image act on it, and each view keeps its own mode, band and false-colour bands. With one view, it fills the whole area.
  - **Refresh** and **Reset** above the views (make the cube again with the current settings; processing off and colour and views back to their defaults), and a refresh and a reset button in every view's title for that view alone.
  - **Scores (PCA)** tab next to the spectra: a density plot of two principal components of every pixel, with the kept areas in colour, the loadings, and lasso selection — the selected pixels are tinted in the views and their mean spectrum is plotted (and can be kept).
  - The mouse starts in spectrum mode (middle button or Ctrl + drag pans); right-click a box's label to remove it.
  - **Match to drawn areas**: a class map in which every pixel takes the colour of the drawn area it resembles most (spectral angle mapper classification), shaded by similarity.
- Image views can draw labelled rectangles over the image, mirrored pointer markers, and have an optional region tool (click a pixel or drag a rectangle; right-click a label to remove it), used by the hyperspectral workspace.

## 0.4.0 (2026-11-16)

- **Image processing** for every panel: the new **Processing…** button opens the panel's processing pipeline, shown as a chain of nodes from Input to Output. Each node is one stage with an on/off switch (off = bypass, the wire is drawn around the node); click a node to choose its algorithm and settings. The panel's image updates as you change them, and **Show original** switches back to the unprocessed image.
- The stages, in the order of a camera ISP for a monochrome sensor: black level, defect pixels, defect rows / columns, digital gain, denoise, dynamic range compression, tone mapping, detail enhancement, sharpening, output quantisation.
- Algorithms: denoising with a bilateral filter, guided filter, non-local means, median or Gaussian filter; tone mapping from the full range to 8 bits with percentile stretch, gamma, log, Reinhard, plateau equalisation, CLAHE, base/detail split, exposure fusion or a plain bit shift; dither (random, ordered, Floyd–Steinberg) for the final rounding. See the README for the full list.
- **A/B** button on every panel (and in the pipeline window): it puts a divider on the processed image, with the unprocessed image to its left (A) and the processed one to its right (B). Drag the divider across the image to compare before and after; double-click it to centre it.
- **Themes**: **File → Settings…** has a theme choice (Follow the system, Light, Dark) that applies at once. On Windows the app now uses Qt's Windows 11 style where the Qt it is built with has it (Qt 6.8.1 or newer for both Windows 10 and 11); the release build moves from Qt 6.6 to Qt 6.8 for this.
- Design pass: the **Processing…** and **Load…** buttons have icons, and show only the icon when a panel is narrow (three panels side by side), so the header no longer crowds the title; the export buttons have icons too, and the menu arrow of **Data (.mat)** no longer overlaps its text. The icons are drawn in the text colour, so they suit both themes. The panel frame, colour strip and drop zone follow the theme.
- **Presets** for common situations (ordinary scene, bright and dark regions in one frame, low-contrast / thermal data, inspection, sensor clean-up only).
- While processing is on, the panel's histogram, contrast range, Mean and Std describe the processed image (the line under them says "processed"), the pixel read-out shows the processed and the unprocessed value, and "Image on display" exports hold the processed image with a `processing` text that lists the stages. Noise, "All frames" exports and the calibration (light − dark) keep using the unprocessed data.
- The pipeline runs on worker threads, so the window stays responsive.

## 0.3.2 (2026-11-16)

- Chinese interface: light frames are now called 场景帧 (scene frames) instead of 亮场帧; the light image is 场景图像.

## 0.3.1 (2026-11-16)

- The mouse cursor over an image is a crosshair instead of a hand, so it is clear which pixel it points at. It still turns into a closed hand while you drag to pan.

## 0.3.0 (2026-11-14)

- **Single image mode**: a tab can show one image in one panel, with the same histogram, contrast controls, frame selector and Mean / Std / Noise as before. The app now starts in this mode, and so does every new tab.
- A slide switch next to the tabs (top right, two icons: one panel / three panels) changes between a single image and the three panels (light, dark and calibrated frames). Every tab has its own mode, and the switch shows the mode of the tab on display.
- The single image is the tab's light stack: switching to three panels keeps it as the light frames, and switching back shows the light frames on their own. Dark frames and a loaded calibrated image are kept while hidden. Zoom and pan carry over.
- In single image mode, **File → Open image…** (Ctrl+L) replaces Open light frames…, and exported .mat files use neutral names (`frame`, `mean_frame`, `frames`; file names end in `_frame001`, `_mean`, `_all`). See the README.
- Command line: `--dark` and `--calibrated` switch the first tab to three panels; `--light` alone opens a single image.
- **Chinese interface** (简体中文), used by default. **File → Settings…** (文件 → 设置…) has the interface language, Chinese or English; a change takes effect when the app restarts, and the dialog offers to restart it. Error messages from the file readers and writers are translated too. Names in exported .mat files stay as they are.
- Building now needs Qt's Linguist tools (`lrelease`), which come with a standard Qt installation. The translation is `translations/ImageAnalyzerTool_zh_CN.ts`.

## 0.2.0 (2026-10-22)

- **Single frame / Mean** toggle in the Light frames and Dark frames panels. The light panel starts in Single frame mode and the dark panel in Mean mode, as before.
- The Dark frames panel has a frame selector too. The calibrated image is now the light image on display minus the dark image on display, so with both panels in Single frame mode it is `light(:, :, k) − dark(:, :, j)`.
- TIFF, PNG and JPEG files can be loaded. Each file is one X × Y frame; drop several image files onto the Light or Dark panel to stack them (natural file-name order). TIFF files are read by a built-in reader (8- to 64-bit integer and floating-point, LZW / Deflate / PackBits, tiles, BigTIFF).
- A calibrated image can be loaded into the Calibrated frames panel from a file. Automatic calibration is then off, shown by a red **● Auto calibration off**; click it to remove the image and calculate light − dark again.
- Exports: the Dark frames panel can save all frames, and exported .mat files say which images were used (`calibration`, `dark_frame_index`, `light_mean`, `dark_frame`, …). See the README.
- Removed the GitHub link from the status bar (it is still under Help and in the About box).
- Very small images can be zoomed in further (zooming in used to shrink an image that was already above 64× when fitted).
- New example data in `examples/` (simulated 14-bit sensor, every supported format) with NumPy reference values that `tst_core` checks. This replaces the optional test that needed the large example .mat file.
- GitHub Actions run the tests again on every push, and publish a release (with the Windows zip) when the version in `CMakeLists.txt` changes. The Windows package now includes the Visual C++ runtime.

## 0.1.0

- First version: light, dark and calibrated frames from MATLAB .mat files, Mean / Std / Noise, histograms and contrast, linked zoom and pan, clipboard copies and exports.
