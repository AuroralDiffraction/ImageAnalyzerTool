"""Generates the example data in this folder: light and dark frames from a
simulated 14-bit image sensor, in every file format Image Analyzer Tool reads.

Run from the repository root:

    python examples/make_examples.py                       128 x 160 pixels, 8 frames
    python examples/make_examples.py --rows 1028 --cols 1408 --frames 20 --out my_examples

Needs numpy, tifffile and Pillow. The noise is random but reproducible (fixed
seed), so the files and reference.json only change if this script changes.

What is simulated (all values in DN, digital numbers):
  - a fixed pattern that is the same in every light and dark frame: a bias of
    about 8400 DN plus row, column and pixel offsets, and a few hot pixels,
    some of which saturate at 16383 (14-bit);
  - light frames: that pattern plus a vignetted test chart (bars, a disc, a
    grey ramp) of up to about 3000 DN, with 1 % pixel response
    non-uniformity and photon shot noise (gain 0.5 DN per electron);
  - read noise of 9.5 DN in every frame.

So light frame - mean dark frame removes the fixed pattern and leaves the chart.

Files written (Z = number of frames):
  light_frames.mat, dark_frames.mat   variable all_pixel, rows x cols x Z, class double
                                      stored as uint16, compressed: as MATLAB saves it
  light_tiff/, dark_tiff/             one 16-bit greyscale TIFF per frame (Deflate)
  light_png/, dark_png/               one 16-bit greyscale PNG per frame
  light_jpg/, dark_jpg/               one 8-bit greyscale JPEG per frame: values / 64,
                                      rounded (JPEG is 8-bit and lossy, so for trying
                                      things out only)
  calibrated_frame01.tif              light(:, :, 1) - mean(dark, 3) as a 32-bit float
                                      TIFF, to try loading an image into the
                                      Calibrated frames panel
  reference.json                      results computed with NumPy, checked by tst_core
"""

import argparse
import json
import struct
import zlib
from pathlib import Path

import numpy as np

SEED = 20261022
FULL_SCALE = 16383  # 14-bit sensor
READ_NOISE = 9.5    # DN
GAIN = 0.5          # DN per electron


# ---------------------------------------------------------------- writers
def mat_element(mi_type, payload, allow_small=True):
    n = len(payload)
    if allow_small and n <= 4:
        return struct.pack("<I", (n << 16) | mi_type) + payload + b"\0" * (4 - n)
    return struct.pack("<II", mi_type, n) + payload + b"\0" * ((-n) % 8)


def write_mat_double_as_uint16(path, name, arr):
    """A Level 5 MAT-file holding `arr` as class double, stored as uint16 and
    compressed, which is what MATLAB writes for whole-number doubles."""
    mx_double, mi_int32, mi_uint32, mi_int8, mi_uint16, mi_matrix, mi_compressed = 6, 5, 6, 1, 4, 14, 15
    flags = mat_element(mi_uint32, struct.pack("<II", mx_double, 0), allow_small=False)
    dims = mat_element(mi_int32, struct.pack("<%di" % arr.ndim, *arr.shape), allow_small=False)
    nm = mat_element(mi_int8, name.encode("ascii"))
    real = mat_element(mi_uint16, np.asarray(arr).astype("<u2").tobytes(order="F"), allow_small=False)
    body = flags + dims + nm + real
    matrix = struct.pack("<II", mi_matrix, len(body)) + body
    z = zlib.compress(matrix)
    header = (b"MATLAB 5.0 MAT-file, Platform: PCWIN64, Created by: make_examples.py".ljust(116, b" ")
              + b"\0" * 8 + struct.pack("<H", 0x0100) + b"IM")
    Path(path).write_bytes(header + struct.pack("<II", mi_compressed, len(z)) + z)


def write_png16(path, img):
    """16-bit greyscale PNG."""
    rows, cols = img.shape
    data = img.astype(">u2")
    raw = b"".join(b"\0" + data[r].tobytes() for r in range(rows))

    def chunk(kind, payload):
        return (struct.pack(">I", len(payload)) + kind + payload
                + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", cols, rows, 16, 0, 0, 0, 0)
    Path(path).write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(raw, 9))
                           + chunk(b"IEND", b""))


# ---------------------------------------------------------------- simulation
def simulate(rows, cols, frames):
    rng = np.random.default_rng(SEED)
    y, x = np.mgrid[0:rows, 0:cols].astype(np.float64)

    # Fixed pattern: bias, row / column / pixel offsets, hot pixels.
    offset = (8400.0 + rng.normal(0, 4, (rows, 1)) + rng.normal(0, 12, (1, cols))
              + rng.normal(0, 25, (rows, cols)))
    hot = rng.random((rows, cols)) < 0.002
    offset[hot] += rng.uniform(800, 9000, int(hot.sum()))

    # Scene: vignetted illumination times a test chart.
    r = np.hypot((y - (rows - 1) / 2) / rows, (x - (cols - 1) / 2) / cols)
    illumination = 3000.0 * (1.0 - 0.8 * r ** 2)
    chart = np.full((rows, cols), 0.75)
    left = x < cols / 3
    chart[left] = np.where((x[left] // max(2, cols // 40)) % 2 == 0, 0.95, 0.35)
    middle = (x >= cols / 3) & (x < 2 * cols / 3)
    disc = np.hypot(y - rows / 2, x - cols / 2) < min(rows, cols) / 5
    chart[middle & disc] = 0.15
    right = x >= 2 * cols / 3
    chart[right] = 0.2 + 0.8 * y[right] / max(1, rows - 1)
    prnu = 1.0 + rng.normal(0, 0.01, (rows, cols))
    signal = illumination * chart * prnu

    light = np.empty((rows, cols, frames), np.uint16)
    dark = np.empty((rows, cols, frames), np.uint16)
    for k in range(frames):
        shot = rng.normal(0, 1, (rows, cols)) * np.sqrt(GAIN * signal)
        l = offset + signal + shot + rng.normal(0, READ_NOISE, (rows, cols))
        d = offset + rng.normal(0, READ_NOISE, (rows, cols))
        light[:, :, k] = np.clip(np.rint(l), 0, FULL_SCALE)
        dark[:, :, k] = np.clip(np.rint(d), 0, FULL_SCALE)
    return light, dark


# ---------------------------------------------------------------- reference values
def stats(img):
    """calculateImageStats: mean(img, "all"), std(img, 1, "all")."""
    img = np.asarray(img, np.float64)
    return {"mean": float(img.mean()), "std": float(img.std(ddof=0))}


def noise(stack):
    """calculateImageNoise: sqrt(mean(var(stack, 1, 3), "all"))."""
    return float(np.sqrt(np.asarray(stack, np.float64).var(axis=2, ddof=0).mean()))


def main():
    parser = argparse.ArgumentParser(description="Generate the example light and dark frames.")
    parser.add_argument("--rows", type=int, default=128, help="X: rows per frame (default 128)")
    parser.add_argument("--cols", type=int, default=160, help="Y: columns per frame (default 160)")
    parser.add_argument("--frames", type=int, default=8, help="Z: frames per stack (default 8)")
    parser.add_argument("--out", type=Path, default=Path(__file__).resolve().parent,
                        help="output folder (default: this folder)")
    args = parser.parse_args()

    import tifffile
    from PIL import Image

    out = args.out
    out.mkdir(parents=True, exist_ok=True)
    light, dark = simulate(args.rows, args.cols, args.frames)
    z = args.frames
    width = max(2, len(str(z)))

    write_mat_double_as_uint16(out / "light_frames.mat", "all_pixel", light)
    write_mat_double_as_uint16(out / "dark_frames.mat", "all_pixel", dark)

    jpeg_means = {}
    for name, stack in (("light", light), ("dark", dark)):
        for fmt in ("tiff", "png", "jpg"):
            folder = out / f"{name}_{fmt}"
            folder.mkdir(exist_ok=True)
            for old in folder.glob(f"{name}_*"):
                old.unlink()
        for k in range(z):
            frame = stack[:, :, k]
            stem = f"{name}_{k + 1:0{width}d}"
            tifffile.imwrite(out / f"{name}_tiff" / f"{stem}.tif", frame, photometric="minisblack",
                             compression="zlib", predictor=True)
            write_png16(out / f"{name}_png" / f"{stem}.png", frame)
            jpg_path = out / f"{name}_jpg" / f"{stem}.jpg"
            Image.fromarray(np.rint(frame / 64.0).clip(0, 255).astype(np.uint8), "L").save(jpg_path, quality=95)
        # Mean of the first JPEG frame as decoded by Pillow. Other JPEG
        # decoders may differ by 1 in a few pixels, so tests allow a margin.
        first = out / f"{name}_jpg" / f"{name}_{1:0{width}d}.jpg"
        jpeg_means[name] = float(np.asarray(Image.open(first), np.float64).mean())

    light_d = light.astype(np.float64)
    dark_d = dark.astype(np.float64)
    dark_mean = dark_d.mean(axis=2)
    light_mean = light_d.mean(axis=2)
    calibrated = light_d[:, :, 0] - dark_mean
    tifffile.imwrite(out / "calibrated_frame01.tif", calibrated.astype(np.float32), photometric="minisblack",
                     compression="zlib", predictor=True)

    reference = {
        "description": "Values computed with NumPy by make_examples.py. Frame numbers are 1-based, "
                       "as in MATLAB. Mean/std follow calculateImageStats, noise calculateImageNoise.",
        "size": [args.rows, args.cols, z],
        "variable": "all_pixel",
        "pixels": {
            "light(1,1,1)": int(light[0, 0, 0]),
            "light(end,end,end)": int(light[-1, -1, -1]),
            "dark(3,5,2)": int(dark[2, 4, 1]),
            "saturated_dark_pixels_frame1": int((dark[:, :, 0] == FULL_SCALE).sum()),
        },
        "light": {
            "frame1": stats(light_d[:, :, 0]),
            "mean_image": stats(light_mean),
            "all_frames": stats(light_d),
            "noise": noise(light_d),
        },
        "dark": {
            "frame1": stats(dark_d[:, :, 0]),
            "mean_image": stats(dark_mean),
            "noise": noise(dark_d),
        },
        "calibrated": {
            "light_frame1_minus_dark_mean": stats(calibrated),
            "light_mean_minus_dark_mean": stats(light_mean - dark_mean),
            "light_frame1_minus_dark_frame1": stats(light_d[:, :, 0] - dark_d[:, :, 0]),
            "noise": noise(light_d - dark_mean[:, :, None]),
        },
        "jpeg_frame1_mean_pillow": jpeg_means,
    }
    (out / "reference.json").write_text(json.dumps(reference, indent=2) + "\n")
    print(f"Wrote {args.rows} x {args.cols} x {z} example frames to {out}")


if __name__ == "__main__":
    main()
