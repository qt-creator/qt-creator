// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto iar_example_baremetal_json = R"(
{
    "qulVersion": "2.3.0",
    "compatVersion": "1",
    "platform": {
        "id": "EXAMPLE-BAREMETAL",
        "vendor": "Vendor Name",
        "colorDepths": [
            16
        ]
    },
    "toolchain": {
        "id": "iar",
        "versions": [
            "9.20.4"
        ],
        "compiler": {
            "id": "IARToolchain",
            "setting": "IARToolchain",
            "envVar": "IAR_ARM_COMPILER_DIR",
            "label": "IAR ARM Compiler",
            "cmakeVar": "QUL_TARGET_TOOLCHAIN_DIR",
            "type": "path",
            "versionDetection": {
                "filePattern": {
                    "windows": "bin/iccarm.exe",
                    "linux": "bin/iccarm"
                },
                "executableArgs": "--version",
                "regex": "\\bV(\\d+\\.\\d+\\.\\d+)\\.\\d+\\b"
            },
            "detectionPath": {
                "windows": "bin/iccarm.exe",
                "linux": "bin/iccarm"
            },
            "optional": false
        },
        "file": {
            "id": "IAR_CMAKE_TOOLCHAIN_FILE",
            "cmakeVar": "CMAKE_TOOLCHAIN_FILE",
            "type": "file",
            "defaultValue": "%{Qul_ROOT}/lib/cmake/Qul/toolchain/iar.cmake",
            "visible": false,
            "optional": false
        }
    },
    "boardSdk": {
        "versions": [
            "1.16.0"
        ],
        "id": "EXAMPLE_SDK_DIR",
        "label": "Board SDK",
        "cmakeVar": "QUL_BOARD_SDK_DIR",
        "type": "path",
        "optional": false
    }
}
)";
