# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

"""Renders qtprofiler.svg to qtprofiler.ico and qtprofiler-<size>.png.

Requires ImageMagick 7 (magick) with an SVG renderer, and optipng in Path.
"""

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

PNG_SIZES = (16, 32, 128)
ICO_SIZES = (16, 24, 32, 48)
SVG_SIZE = 1024  # width and height of qtprofiler.svg
SVG_DPI = 96  # ImageMagick's default rasterization density


def magick(*args):
    subprocess.run(["magick", *map(str, args)], check=True)


def main():
    folder = Path(__file__).resolve().parent
    svg = folder / "qtprofiler.svg"
    if not svg.exists():
        sys.exit(f"Cannot find {svg}")
    optipng = shutil.which("optipng")
    if optipng is None:
        sys.exit("Optipng was not found in Path.")

    with tempfile.TemporaryDirectory() as temp:
        def rasterize(size, output):
            magick("-background", "none", "-density", SVG_DPI * size / SVG_SIZE, svg,
                   f"PNG32:{output}")

        for size in PNG_SIZES:
            png = folder / f"qtprofiler-{size}.png"
            rasterize(size, png)
            print(f"Optimizing: {png.name}")
            subprocess.run([optipng, "-quiet", "-o7", "-strip", "all", png], check=True)

        pngs = []
        for size in ICO_SIZES:
            png = Path(temp) / f"{size}.png"
            rasterize(size, png)
            pngs.append(png)
        ico = folder / "qtprofiler.ico"
        magick(*pngs, ico)
        print(f"{ico.name} contains sizes: {', '.join(map(str, ICO_SIZES))}")


if __name__ == "__main__":
    main()
