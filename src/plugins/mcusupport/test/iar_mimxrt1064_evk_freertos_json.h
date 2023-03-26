// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto iar_mimxrt1064_evk_freertos_json = R"(
{
    "qulVersion": "2.3.0",
    "compatVersion": "1",
    "platform": {
        "id": "MIMXRT1064-EVK-FREERTOS",
        "vendor": "NXP",
        "colorDepths": [
            16
        ],
        "cmakeEntries": [
            {
                "label": "MCUXpresso IDE",
                "type": "path",
                "cmakeVar": "MCUXPRESSO_IDE_PATH",
                "envVar": "MCUXpressoIDE_PATH",
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
        "envVar": "EVK_MIMXRT1064_SDK_PATH",
        "label": "Board SDK for MIMXRT1064-EVK",
        "optional": false,
        "setting": "EVK_MIMXRT1064_SDK_PATH",
        "type": "path",
        "versions": [
            "2.12.0"
        ],
        "versionDetection": {
            "filePattern": "*_manifest_*.xml",
            "regex": ".*",
            "xmlAttribute": "version",
            "xmlElement": "ksdk"
        }
    },
    "freeRTOS": {
        "cmakeVar": "FREERTOS_DIR",
        "defaultValue": "%{QUL_BOARD_SDK_DIR}/rtos/freertos/freertos_kernel",
        "detectionPath": "tasks.c",
        "envVar": "IMXRT1064_FREERTOS_DIR",
        "label": "FreeRTOS SDK for MIMXRT1064-EVK",
        "optional": false,
        "setting": "FreeRTOSSourcePackage_IMXRT1064",
        "type": "path"
    }
}
)";
