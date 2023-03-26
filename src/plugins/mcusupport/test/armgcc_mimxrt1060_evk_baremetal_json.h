// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto armgcc_mimxrt1060_evk_baremetal_json = R"(
{
    "qulVersion": "2.3.0",
    "compatVersion": "1",
    "platform": {
        "id": "MIMXRT1060-EVK-BAREMETAL",
        "vendor": "NXP",
        "colorDepths": [
            16
        ],
        "cmakeEntries": [
            {
                "id": "MCU_XPRESSO_PATH",
                "label": "MCUXpresso IDE",
                "type": "path",
                "cmakeVar": "MCUXPRESSO_IDE_PATH",
                "setting": "MCUXpressoIDE",
                "defaultValue": {
                    "windows": "%{Env:ROOT}/nxp/MCUXpressoIDE*",
                    "linux": "/usr/local/mcuxpressoide/"
                },
                "detectionPath": {
                    "windows": "ide/binaries/crt_emu_cm_redlink.exe",
                    "linux": "ide/binaries/crt_emu_cm_redlink"
                },
                "optional": false,
                "addToSystemPath": true
            }
        ]
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
        "envVar": "EVK_MIMXRT1060_SDK_PATH",
        "versions": [
            "2.12.0"
        ],
        "id": "NXP_SDK_DIR",
        "label": "Board SDK for MIMXRT1060-EVK",
        "setting": "EVK_MIMXRT1060_SDK_PATH",
        "cmakeVar": "QUL_BOARD_SDK_DIR",
        "type": "path",
        "optional": false
    }
}
)";
