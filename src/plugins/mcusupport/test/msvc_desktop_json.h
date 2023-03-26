// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto msvc_desktop_json = R"(
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
        "id": "msvc",
        "versions": [
            "19.20",
            "19.21",
            "19.22",
            "19.23",
            "19.24",
            "19.25",
            "19.26",
            "19.27",
            "19.28",
            "19.29"
        ],
        "compiler": {
            "label": "MSVC Toolchain",
            "setting": "MsvcToolchain",
            "defaultValue": "%{Env:MSVC_COMPILER_DIR}",
            "versionDetection": {
                "filePattern": "cl.exe",
                "executableArgs": "--version",
                "regex": "\\b(\\d+\\.\\d+)\\.\\d+\\b"
            },
            "detectionPath": "cl.exe"
        }
    }
}
)";
