"""Regenerates the small .mat files in tests/data used by the unit tests.

Run from the repository root:  python tests/make_fixtures.py
Needs numpy and scipy. Some files are written with scipy (to test files from
a third-party writer); others with the tiny writer below, to reproduce
details of files saved by MATLAB itself (a double array stored as uint16,
big-endian files, a v7.3 header).
"""

import struct
import zlib
from pathlib import Path

import numpy as np
import scipy.io as sio

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


def main():
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

    print("Wrote fixtures to", OUT)


if __name__ == "__main__":
    main()
