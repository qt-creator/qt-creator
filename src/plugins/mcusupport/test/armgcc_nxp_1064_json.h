// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto armgcc_nxp_1064_json = R"(
{
    "boardSdk": {
        "cmakeVar": "QUL_BOARD_SDK_DIR",
        "envVar": "EVK_MIMXRT1064_SDK_PATH",
        "label": "Board SDK for MIMXRT1064-EVK",
        "optional": false,
        "setting": "EVK_MIMXRT1064_SDK_PATH",
        "type": "path",
        "versions": ["2.11.1"]
    },
    "compatVersion": "1",
    "freeRTOS": {
        "cmakeVar": "FREERTOS_DIR",
        "defaultValue": "%{QUL_BOARD_SDK_DIR}/rtos/freertos/freertos_kernel",
        "envVar": "IMXRT1064_FREERTOS_DIR",
        "label": "FreeRTOS SDK for MIMXRT1064-EVK",
        "optional": false,
        "setting": "FreeRTOSSourcePackage_IMXRT1064",
        "type": "path"
    },
    "platform": {
        "cmakeCacheEntries": [
            {
                "camekVar": "Qul_ROOT",
                "envVar": "Qul_DIR",
                "label": "Qt for MCUs SDK",
                "optional": false,
                "type": "path"
            },
            {
                "camekVar": "MCUXPRESSO_IDE_PATH",
                "defaultValue": {
                    "unix": "/usr/local/mcuxpressoide/",
                    "windows": "%{Env:ROOT}/nxp/MCUXpressoIDE*"
                },
                "id": "MCU_XPRESSO_PATH",
                "label": "MCUXpresso IDE",
                "optional": false,
                "type": "path"
            }
        ],
        "colorDepths": [16],
        "environmentEntries": [],
        "id": "MIMXRT1064-EVK-FREERTOS",
        "pathEntries": [],
        "vendor": "NXP"
    },
    "qulVersion": "2.3.0",
    "toolchain": {
        "compiler": {
            "camekVar": "QUL_TARGET_TOOLCHAIN_DIR",
            "id": "ARMGCC_DIR",
            "label": "GNU Arm Embedded Toolchain",
            "optional": false,
            "type": "path"
        },
        "file": {
            "camekVar": "CMAKE_TOOLCHAIN_FILE",
            "defaultValue": "%{Qul_ROOT}/lib/cmake/Qul/toolchain/armgcc.cmake",
            "label": "CMake Toolchain File",
            "optional": false,
            "type": "file",
            "visible": false
        },
        "id": "armgcc",
        "versions": ["9.3.1"]
    }
})";
