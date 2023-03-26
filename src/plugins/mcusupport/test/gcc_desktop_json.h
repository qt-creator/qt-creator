// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto gcc_desktop_json = R"(
{
    "qulVersion": "2.3.0",
    "compatVersion": "1",
    "platform": {
        "id": "Qt",
        "platformName": "Desktop",
        "vendor": "Qt",
        "colorDepths": [
            32
        ]
    },
    "toolchain": {
        "id": "gcc",
        "versions": [
            "9.4.0",
            "10.3.1"
        ],
        "compiler": {
            "label": "GNU Toolchain",
            "setting": "GnuToolchain",
            "defaultValue": "/usr",
            "versionDetection": {
                "executableArgs": "--version",
                "filePattern": "bin/g++",
                "regex": "\\b(\\d+\\.\\d+\\.\\d+)\\b"
            },
            "detectionPath": "bin/g++"
        }
    }
}
)";
