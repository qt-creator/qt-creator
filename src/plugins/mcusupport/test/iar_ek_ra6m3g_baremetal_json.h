// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto iar_ek_ra6m3g_baremetal_json = R"(
{
    "qulVersion": "2.3.0",
    "compatVersion": "1",
    "platform": {
        "id": "EK-RA6M3G-BAREMETAL",
        "vendor": "RENESAS",
        "colorDepths": [
            16
        ],
        "cmakeEntries": [
            {
                "cmakeVar": "JLINK_PATH",
                "setting": "JLinkPath",
                "envVar": "JLINK_PATH",
                "label": "Path to SEGGER J-Link",
                "type": "path",
                "defaultValue": {
                    "windows": "%{Env:PROGRAMFILES}/SEGGER/JLink",
                    "linux": "/opt/SEGGER/JLink"
                },
                "detectionPath": {
                    "windows": "JLink.exe",
                    "linux": "JLinkExe"
                },
                "optional": true,
                "addToSystemPath": true
            }
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
        "cmakeVar": "QUL_BOARD_SDK_DIR",
        "envVar": "EK_RA6M3G_FSP_PATH",
        "setting": "EK_RA6M3G_FSP_PATH",
        "id": "EK_RA6M3G_FSP_PATH",
        "label": "Flexible Software Package for Renesas RA MCU Family",
        "optional": false,
        "type": "path",
        "versions": [
            "3.8.0"
        ]
    }
}
)";
