# Changelog

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
