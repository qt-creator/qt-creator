// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto ghs_rh850_d1m1a_baremetal_json = R"(
{
    "qulVersion": "2.4.0",
    "compatVersion": "1",
    "platform": {
        "id": "RH850-D1M1A-BAREMETAL",
        "vendor": "RENESAS",
        "colorDepths": [
            32
        ],
        "cmakeEntries": [
            {
                "id": "FlashProgrammer_path",
                "setting": "FlashProgrammerPath",
                "label": "Renesas Flash Programmer",
                "type": "path",
                "cmakeVar": "RENESAS_FLASH_PROGRAMMER_PATH",
                "defaultValue": {
                    "linux": "",
                    "windows": "%{Env:PROGRAMFILES(x86)}/Renesas Electronics/Programming Tools/Renesas Flash Programmer V3.09"
                },
                "detectionPath": {
                    "windows": "rfp-cli.exe",
                    "linux": "rfp-cli"
                },
                "envVar": "RenesasFlashProgrammer_PATH",
                "optional": true,
                "addToSystemPath": true
            }
        ]
    },
    "toolchain": {
        "id": "greenhills",
        "versions": [
            "2018.1.5"
        ],
        "compiler": {
            "cmakeVar": "QUL_TARGET_TOOLCHAIN_DIR",
            "setting": "GHSToolchain",
            "label": "Green Hills Compiler",
            "type": "path",
            "defaultValue": {
                "linux": "",
                "windows": "C:/ghs/comp_201815"
            },
            "optional": false,
            "versionDetection": {
                "filePattern": {
                    "windows": "gversion.exe",
                    "linux": "gversion"
                },
                "executableArgs": "-help",
                "regex": "\\bv(\\d+\\.\\d+\\.\\d+)\\b"
            },
            "detectionPath": {
                "windows": "cxv850.exe",
                "linux": "cxv850"
            }
        },
        "file": {
            "cmakeVar": "CMAKE_TOOLCHAIN_FILE",
            "type": "file",
            "defaultValue": "%{Qul_ROOT}/lib/cmake/Qul/toolchain/ghs.cmake",
            "visible": false,
            "optional": false
        }
    },
    "boardSdk": {
        "envVar": "RGL_DIR",
        "setting": "RGL_DIR",
        "versions": [
            "2.0.0",
            "2.0.0a"
        ],
        "label": "Renesas Graphics Library",
        "cmakeVar": "QUL_BOARD_SDK_DIR",
        "type": "path",
        "defaultValue": {
            "linux": "",
            "windows": "%{Env:PROGRAMFILES(x86)}/Renesas_Electronics/D1x_RGL/rgl_ghs_D1Mx_obj_V.2.0.0a"
        },
        "versionDetection": {
            "regex": "\\d+\\.\\d+\\.\\w+"
        },
        "optional": false
    }
}
)";
