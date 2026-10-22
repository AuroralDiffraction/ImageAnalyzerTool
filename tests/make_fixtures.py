"""Regenerates the small test files in tests/data used by the unit tests.

Run from the repository root:

    python tests/make_fixtures.py            all files
    python tests/make_fixtures.py --mat      only the .mat files
    python tests/make_fixtures.py --images   only the TIFF / PNG / JPEG files

The .mat files need numpy and scipy. Some are written with scipy (to test
files from a third-party writer); others with the tiny writer below, to
reproduce details of files saved by MATLAB itself (a double array stored as
uint16, big-endian files, a v7.3 header).

The image files need numpy, tifffile, imagecodecs (LZW and JPEG inside TIFF)
and Pillow (JPEG). PNG files are written by the small writer below, so the
exact bit depth and colour type are under control.
"""

import argparse
import struct
import sys
import zlib
from pathlib import Path

import numpy as np

OUT = Path(__file__).resolve().parent / "data"


def grid(rows, cols, frames):
    r, c, z = np.meshgrid(np.arange(rows), np.arange(cols), np.arange(frames), indexing="ij")
    return r, c, z


# ---------------------------------------------------------------- tiny writer
def element(mi_type, payload, endian="<", allow_small=True):
    n = len(payload)
    if allow_small and n <= 4:
        return struct.pack(endian + "I", (n << 16) | mi_type) + payload + b"\0" * (4 - n)
    return struct.pack(endian + "II", mi_type, n) + payload + b"\0" * ((-n) % 8)


def matrix(name, arr, mx_class, store_dtype, mi_store, endian="<"):
    flags = element(6, struct.pack(endian + "II", mx_class, 0), endian, allow_small=False)
    dims = element(5, struct.pack(endian + "%di" % arr.ndim, *arr.shape), endian, allow_small=False)
    nm = element(1, name.encode("ascii"), endian)
    data = np.asarray(arr).astype(np.dtype(store_dtype).newbyteorder(endian)).tobytes(order="F")
    real = element(mi_store, data, endian, allow_small=False)
    body = flags + dims + nm + real
    return struct.pack(endian + "II", 14, len(body)) + body


def header(text, endian="<", version=0x0100):
    return (text.encode("ascii").ljust(116, b" ") + b"\0" * 8
            + struct.pack(endian + "H", version) + (b"IM" if endian == "<" else b"MI"))


def compressed(matrix_bytes, endian="<"):
    z = zlib.compress(matrix_bytes)
    return struct.pack(endian + "II", 15, len(z)) + z


MX_DOUBLE = 6
MI_UINT16 = 4
MI_DOUBLE = 9


def write_mat_fixtures():
    import scipy.io as sio

    OUT.mkdir(parents=True, exist_ok=True)

    # Light frames: uint16, 6 x 8 x 4, compressed (scipy).
    r, c, z = grid(6, 8, 4)
    light = (1000 + 100 * r + 10 * c + z).astype(np.uint16)
    sio.savemat(OUT / "light_small.mat", {"frames": light}, do_compression=True)

    # Dark frames: double, 6 x 8 x 3, uncompressed (scipy).
    r, c, z = grid(6, 8, 3)
    dark = (r + 0.5 * c + 10.0 * z).astype(np.float64)
    sio.savemat(OUT / "dark_small.mat", {"dark": dark}, do_compression=False)

    # A single 2-D single-precision image.
    r, c, _ = grid(5, 7, 1)
    img = (r * 7 + c).astype(np.float32)[:, :, 0] / 4.0
    sio.savemat(OUT / "single2d.mat", {"img": img}, do_compression=True)

    # Several variables: two images plus things that are not images.
    r, c, z = grid(4, 5, 2)
    sio.savemat(OUT / "multi.mat", {
        "a": (r - c + z).astype(np.int16),
        "b": np.arange(48, dtype=np.int32).reshape(6, 8),
        "scalar": 3.0,
        "txt": "hello",
        "s": {"x": 1.0},
    }, do_compression=True)

    # NaN and Inf values.
    nan_img = np.arange(12, dtype=np.float64).reshape(3, 4)
    nan_img[0, 0] = np.nan
    nan_img[2, 3] = np.inf
    sio.savemat(OUT / "nan_values.mat", {"v": nan_img})

    # Like MATLAB: class double, but stored as uint16, compressed; the short
    # name "img" uses the small-data-element format.
    r, c, z = grid(6, 8, 4)
    values = 5000 + 8 * r + c + 100 * z
    data = header("MATLAB 5.0 MAT-file, Platform: PCWIN64, Created on: test")
    data += compressed(matrix("img", values, MX_DOUBLE, np.uint16, MI_UINT16))
    (OUT / "double_as_uint16.mat").write_bytes(data)

    # Big-endian file, uncompressed doubles.
    r, c, z = grid(3, 4, 2)
    values = r * 1.5 - c + z * 0.25
    data = header("MATLAB 5.0 MAT-file, Platform: SOL2, Created on: test", endian=">")
    data += matrix("be", values, MX_DOUBLE, np.float64, MI_DOUBLE, endian=">")
    (OUT / "bigendian.mat").write_bytes(data)

    # Header of a v7.3 (HDF5) file.
    data = header("MATLAB 7.3 MAT-file, Platform: PCWIN64, Created on: test HDF5 schema 1.00 .", version=0x0200)
    data = data.ljust(512, b"\0") + b"\x89HDF\r\n\x1a\n" + b"\0" * 64
    (OUT / "v73_header.mat").write_bytes(data)

    (OUT / "not_a_mat.mat").write_bytes(b"This is just a text file pretending to be a MAT-file.\n" * 4)

    print("Wrote .mat fixtures to", OUT)


# ---------------------------------------------------------------- images
def write_png(path, arr, bit_depth, color_type):
    """Minimal PNG writer: arr is (rows, cols) or (rows, cols, channels)."""
    arr = np.asarray(arr)
    rows, cols = arr.shape[:2]
    dtype = ">u2" if bit_depth == 16 else "u1"
    flat = arr.reshape(rows, -1).astype(dtype)
    raw = b"".join(b"\0" + flat[r].tobytes() for r in range(rows))

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", cols, rows, bit_depth, color_type, 0, 0, 0)
    Path(path).write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(raw, 9))
                           + chunk(b"IEND", b""))




def write_image_fixtures():
    import tifffile
    from PIL import Image

    # Light frames as four 16-bit greyscale PNGs: the same values as the
    # frames of light_small.mat (1000 + 100 r + 10 c + z).
    folder = OUT / "light_png"
    folder.mkdir(parents=True, exist_ok=True)
    r, c, z = grid(6, 8, 4)
    light = (1000 + 100 * r + 10 * c + z).astype(np.uint16)
    for k in range(4):
        write_png(folder / f"light_{k + 1}.png", light[:, :, k], 16, 0)

    # Dark frames as three float32 TIFFs: the frames of dark_small.mat
    # (r + 0.5 c + 10 z). Named 1, 2, 10 so that only a natural sort puts
    # them in the right order.
    folder = OUT / "dark_tiff"
    folder.mkdir(parents=True, exist_ok=True)
    r, c, z = grid(6, 8, 3)
    dark = (r + 0.5 * c + 10.0 * z).astype(np.float32)
    for k, name in enumerate(["dark_1.tif", "dark_2.tif", "dark_10.tif"]):
        tifffile.imwrite(folder / name, dark[:, :, k], photometric="minisblack")

    folder = OUT / "formats"
    folder.mkdir(parents=True, exist_ok=True)
    r, c, _ = grid(6, 8, 1)
    r, c = r[:, :, 0], c[:, :, 0]

    # --- TIFF variants
    base16 = (1000 + 100 * r + 10 * c).astype(np.uint16)
    tifffile.imwrite(folder / "u16_le_strips.tif", base16, photometric="minisblack", rowsperstrip=2)
    # Random values, so LZW cannot compress them and its code table fills up
    # and restarts several times. The tests compare every compressed variant
    # with the uncompressed copy.
    busy = np.random.default_rng(20261022).integers(0, 65536, size=(64, 80), dtype=np.uint16)
    tifffile.imwrite(folder / "u16_random_uncompressed.tif", busy, photometric="minisblack")
    tifffile.imwrite(folder / "u16_be_lzw_predictor.tif", busy, photometric="minisblack", byteorder=">",
                     compression="lzw", predictor=True)
    tifffile.imwrite(folder / "u16_bigtiff_deflate.tif", busy, photometric="minisblack", bigtiff=True,
                     compression="zlib", predictor=True)
    tifffile.imwrite(folder / "u16_adobe_deflate_tiles.tif", busy, photometric="minisblack",
                     compression="deflate", tile=(16, 32))
    rf, cf, _ = grid(20, 36, 1)
    rf, cf = rf[:, :, 0], cf[:, :, 0]
    f32 = (rf * 0.25 - cf * 1.5 + 0.125).astype(np.float32)
    tifffile.imwrite(folder / "f32_tiles_deflate_fp_predictor.tif", f32, photometric="minisblack",
                     compression="zlib", predictor=True, tile=(16, 16))
    tifffile.imwrite(folder / "f64_be.tif", f32.astype(np.float64) * 3, photometric="minisblack", byteorder=">")
    i16 = ((r - 3) * 100 + c).astype(np.int16)
    tifffile.imwrite(folder / "i16_packbits.tif", i16, photometric="minisblack", compression="packbits")
    rgb = np.stack([r * 30, c * 20, (r + c) * 10], axis=-1).astype(np.uint8)
    tifffile.imwrite(folder / "rgb8.tif", rgb, photometric="rgb")
    grey_rgb = np.repeat((r * 30 + c).astype(np.uint8)[:, :, None], 3, axis=-1)
    tifffile.imwrite(folder / "rgb8_equal_channels.tif", grey_rgb, photometric="rgb")
    rgb16 = np.stack([base16, base16 // 2, base16 // 4], axis=-1).astype(np.uint16)
    tifffile.imwrite(folder / "rgb16_planar.tif", np.moveaxis(rgb16, -1, 0), photometric="rgb", planarconfig="separate")
    pages = np.stack([base16, base16 + 1, base16 + 2]).astype(np.uint16)
    tifffile.imwrite(folder / "u16_three_pages.tif", pages, photometric="minisblack")
    tifffile.imwrite(folder / "bilevel.tif", (r + c) % 3 == 0, photometric="minisblack")
    tifffile.imwrite(folder / "jpeg_compressed.tif", (r * 30 + c).astype(np.uint8), photometric="minisblack",
                     compression="jpeg")
    colormap = np.zeros((3, 256), dtype=np.uint16)
    tifffile.imwrite(folder / "palette.tif", (r * 8 + c).astype(np.uint8), photometric="palette", colormap=colormap)

    # --- PNG variants
    write_png(folder / "grey8.png", (r * 30 + c).astype(np.uint8), 8, 0)
    write_png(folder / "rgb8.png", rgb, 8, 2)
    rgba16 = np.concatenate([np.repeat(base16[:, :, None], 3, axis=-1), np.full((6, 8, 1), 65535, np.uint16)], axis=-1)
    write_png(folder / "rgba16_equal_channels.png", rgba16, 16, 6)
    write_png(folder / "grey16_5x7.png", np.arange(35, dtype=np.uint16).reshape(5, 7), 16, 0)

    # --- JPEG variants (lossy: the tests allow for small differences)
    flat = np.full((16, 24), 100, np.uint8)
    flat[8:, :] = 200
    Image.fromarray(flat, "L").save(folder / "grey.jpg", quality=100)
    colour = np.zeros((16, 24, 3), np.uint8)
    colour[..., 0] = 200
    colour[..., 1] = 50
    colour[..., 2] = 100
    Image.fromarray(colour, "RGB").save(folder / "colour.jpg", quality=100)

    (folder / "not_an_image.png").write_bytes(b"This is just a text file pretending to be a PNG image.\n")

    print("Wrote image fixtures to", OUT)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--mat", action="store_true", help="only write the .mat files")
    parser.add_argument("--images", action="store_true", help="only write the TIFF / PNG / JPEG files")
    args = parser.parse_args()
    both = not args.mat and not args.images
    if both or args.mat:
        write_mat_fixtures()
    if both or args.images:
        write_image_fixtures()


if __name__ == "__main__":
    sys.exit(main())
