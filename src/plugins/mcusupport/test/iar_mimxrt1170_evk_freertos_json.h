// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto iar_mimxrt1170_evk_freertos_json = R"(
{
    "qulVersion": "2.3.0",
    "compatVersion": "1",
    "platform": {
        "id": "MIMXRT1170-EVK-FREERTOS",
        "vendor": "NXP",
        "colorDepths": [
            32
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
        "envVar": "EVK_MIMXRT1170_SDK_PATH",
        "versions": [
            "2.12.0"
        ],
        "label": "Board SDK for MIMXRT1170-EVK",
        "type": "path",
        "setting": "EVK_MIMXRT1170_SDK_PATH",
        "versionDetection": {
            "filePattern": "*_manifest_*.xml",
            "regex": ".*",
            "xmlAttribute": "version",
            "xmlElement": "ksdk"
        }
    },
    "freeRTOS": {
        "cmakeVar": "FREERTOS_DIR",
        "envVar": "EVK_MIMXRT1170_FREERTOS_PATH",
        "label": "FreeRTOS SDK for MIMXRT1170-EVK",
        "setting": "FreeRTOSSourcePackage_IMXRT1170",
        "defaultValue": "%{QUL_BOARD_SDK_DIR}/rtos/freertos/freertos_kernel",
        "detectionPath": "tasks.c",
        "type": "path",
        "optional": false
    }
}
)";
