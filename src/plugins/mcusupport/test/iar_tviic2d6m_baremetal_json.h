// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto iar_tviic2d6m_baremetal_json = R"(
{
    "qulVersion": "2.3.0",
    "compatVersion": "1",
    "platform": {
        "id": "TVIIC2D6M-BAREMETAL",
        "vendor": "CYPRESS",
        "colorDepths": [
            32
        ],
        "cmakeEntries": [
            {
                "id": "INFINEON_AUTO_FLASH_UTILITY_DIR",
                "setting": "CypressAutoFlashUtil",
                "label": "Cypress Auto Flash Utility",
                "type": "path",
                "cmakeVar": "INFINEON_AUTO_FLASH_UTILITY_DIR",
                "detectionPath": "bin/openocd.exe",
                "optional": false,
                "addToSystemPath": true
            }
        ]
    },
    "toolchain": {
        "id": "iar",
        "versions": [
            "8.22.3"
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
            "detectionPath": "bin/iccarm.exe",
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
        "envVar": "TVII_GRAPHICS_DRIVER_DIR",
        "setting": "TVII_GRAPHICS_DRIVER_DIR",
        "versions": [
            "V1e.1.0"
        ],
        "id": "TVII_GRAPHICS_DRIVER_DIR",
        "label": "Graphics Driver for Traveo II Cluster Series",
        "cmakeVar": "QUL_BOARD_SDK_DIR",
        "type": "path",
        "optional": false,
        "versionDetection": {
            "regex": "V\\w+\\.\\d+\\.\\d+"
    }
}
)";
