// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto armgcc_mimxrt1170_evk_freertos_json = R"(
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
                    "unix": "/usr/local/mcuxpressoide/"
                },
                "versionDetection": {
                    "filePattern": "ide/binaries/crt_emu_cm_redlink"
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
                "filePattern": "bin/arm-none-eabi-g++",
                "executableArgs": "--version",
                "regex": "\\bv(\\d+\\.\\d+\\.\\d+)\\b"
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
        "defaultValue": "%{QUL_BOARD_SDK_DIR}/rtos/freertos/freertos_kernel",
        "validation": "tasks.c",
        "type": "path",
        "optional": false
    }
}
)";
