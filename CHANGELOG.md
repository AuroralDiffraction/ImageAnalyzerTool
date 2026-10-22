# Changelog

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
