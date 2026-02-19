#!/usr/bin/env python

# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import json
import os
import shutil
import subprocess
import sys
import urllib.request

from pathlib import Path


def recommendations_root():
    return (Path(__file__).parent / "../overview").resolve()


def recommendations_json():
    return recommendations_root() / "recommendations.json"


def download_file(url, file_name):
    if not os.path.isfile(file_name):
        print(f"Fetching {url}")
        urllib.request.urlretrieve(url, file_name)


def magick():
    magick_tool = shutil.which("magick")
    if magick_tool is None:
        magick_tool = shutil.which("convert")
        if magick_tool is None:
            sys.exit("ImageMagick was not found in Path.")
    return magick_tool


def convert_thumbnail(thumbnail_file_name, url, thumbnail_type):
    temp_file_name = Path(thumbnail_file_name).stem + Path(url).suffix
    download_file(url, temp_file_name)

    is_blog_pic = thumbnail_type == "blogpost"
    is_course_pic = thumbnail_type == "course"
    size = (
        # Learning::Internal::blogThumbSize
        {"w": 450 * 2, "h": 192 * 2} if is_blog_pic else
        # WelcomePageHelpers::WelcomeThumbnailSize
        {"w": 214 * 2, "h": 160 * 2}
    )
    size_str = f"{size['w']}x{size['h']}"

    command = [magick(), f"{temp_file_name}[0]"]
    command.extend([
        # https://imagemagick.org/script/command-line-options.php#filter
        "-filter", "Mitchell",
        # https://usage.imagemagick.org/resize/#fill
        "-resize", f"{size_str}^",
        "-gravity", "north",
        "-extent", size_str,
    ])
    if is_course_pic:
        command.extend([
            "+dither",
            "-colors", "15",
            # https://imagemagick.org/script/webp.php
            "-define", "webp:lossless=true",
        ])
    else:
        command.extend([
            # https://imagemagick.org/script/webp.php
            "-define", "webp:use-sharp-yuv=1",
            "-define", "webp:method=6",
        ])
    command.append(recommendations_root() / thumbnail_file_name)

    print(command)
    try:
        subprocess.check_call(command)
    except subprocess.CalledProcessError:
        print(f"Failed to convert to {thumbnail_file_name}.")


def main():
    with open(recommendations_json(), encoding="utf-8") as json_data:
        data = json.load(json_data)
        for item in data["contentitems"]:
            if "thumbnailurl" in item:
                convert_thumbnail(item["thumbnail"], item["thumbnailurl"],
                                  item["itemtype"])

    return 0


if __name__ == '__main__':
    sys.exit(main())
