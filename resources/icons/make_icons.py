"""Regenerates the PNG and .ico versions of the app icon from the SVGs.

Run from the repository root:  python resources/icons/make_icons.py
Needs CairoSVG (pip install cairosvg).

  app.svg      master artwork, used for 40 px and larger
  app_32.svg   pixel-aligned artwork on a 32 px grid, used for 32 px
  app_24.svg   simplified artwork on a 24 px grid, used for 20 and 24 px
  app_16.svg   simplified artwork on a 16 px grid, used for 16 px

Outputs app_<size>.png (used by Qt for the window icon) and app.ico (built
into ImageAnalyzerTool.exe, shown by Windows Explorer and the taskbar).
"""

import struct
from pathlib import Path

import cairosvg

HERE = Path(__file__).resolve().parent

# size -> source artwork
SOURCES = {16: "app_16.svg", 20: "app_24.svg", 24: "app_24.svg", 32: "app_32.svg", 40: "app.svg",
           48: "app.svg", 64: "app.svg", 128: "app.svg", 256: "app.svg"}
PNG_SIZES = [16, 24, 32, 48, 64, 128, 256]  # files Qt loads for the window icon


def render(svg_name, size):
    return cairosvg.svg2png(url=str(HERE / svg_name), output_width=size, output_height=size)


def write_ico(path, images):
    """images: list of (size, png_bytes). PNG-compressed entries (Windows Vista and later)."""
    header = struct.pack("<HHH", 0, 1, len(images))
    offset = 6 + 16 * len(images)
    directory = b""
    data = b""
    for size, png in images:
        dim = 0 if size >= 256 else size  # 0 means 256
        directory += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(png), offset + len(data))
        data += png
    path.write_bytes(header + directory + data)


def main():
    rendered = {size: render(src, size) for size, src in SOURCES.items()}
    for size in PNG_SIZES:
        (HERE / f"app_{size}.png").write_bytes(rendered[size])
    write_ico(HERE / "app.ico", sorted(rendered.items()))
    (HERE / "app_1024.png").write_bytes(render("app.svg", 1024))  # for documentation
    print("Wrote", ", ".join(f"app_{s}.png" for s in PNG_SIZES), "app_1024.png and app.ico")


if __name__ == "__main__":
    main()
