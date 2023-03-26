// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto armgcc_example_baremetal_json = R"(
{
    "qulVersion": "2.3.0",
    "compatVersion": "1",
    "platform": {
        "colorDepths": [
            16
        ],
        "id": "EXAMPLE-BAREMETAL",
        "vendor": "Vendor Name"
    },
    "toolchain": {
        "id": "armgcc",
        "versions": [
            "10.3.1"
        ],
        "compiler": {
            "id": "ARMGCC_DIR",
            "label": "GNU Arm Embedded Toolchain",
            "cmakeVar": "QUL_TARGET_TOOLCHAIN_DIR",
            "envVar": "ARMGCC_DIR",
            "setting": "GNUArmEmbeddedToolchain",
            "type": "path",
            "optional": false,
            "versionDetection": {
                "filePattern": {
                    "windows": "bin/arm-none-eabi-g++.exe",
                    "linux": "bin/arm-none-eabi-g++"
                },
                "executableArgs": "--version",
                "regex": "\\b(\\d+\\.\\d+\\.\\d+)\\b"
            },
            "detectionPath": {
                "windows": "bin/arm-none-eabi-g++.exe",
                "linux": "bin/arm-none-eabi-g++"
            }
        },
        "file": {
            "id": "ARMGCC_CMAKE_TOOLCHAIN_FILE",
            "cmakeVar": "CMAKE_TOOLCHAIN_FILE",
            "type": "file",
            "defaultValue": "%{Qul_ROOT}//lib/cmake/Qul/toolchain/armgcc.cmake",
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
