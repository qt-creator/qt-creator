/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

constexpr auto armgcc_nxp_1064_json = R"(
{
    "boardSdk": {
        "cmakeVar": "QUL_BOARD_SDK_DIR",
        "envVar": "EVK_MIMXRT1064_SDK_PATH",
        "id": "NXP_SDK_DIR",
        "label": "Board SDK for MIMXRT1064-EVK",
        "optional": false,
        "setting": "EVK_MIMXRT1064_SDK_PATH",
        "type": "path",
        "versions": ["2.11.1"]
    },
    "compatVersion": "1",
    "freeRTOS": {
        "cmakeVar": "FREERTOS_DIR",
        "defaultValue": "$QUL_BOARD_SDK_DIR/rtos/freertos/freertos_kernel",
        "envVar": "IMXRT1064_FREERTOS_DIR",
        "id": "NXP_FREERTOS_DIR",
        "label": "FreeRTOS SDK for MIMXRT1064-EVK",
        "optional": false,
        "setting": "FreeRTOSSourcePackage_IMXRT1064",
        "type": "path"
    },
    "platform": {
        "cmakeCacheEntries": [
            {
                "camekVar": "Qul_ROOT",
                "id": "Qul_DIR",
                "label": "Qt for MCUs SDK",
                "optional": false,
                "type": "path"
            },
            {
                "camekVar": "MCUXPRESSO_IDE_PATH",
                "defaultValue": {
                    "unix": "/usr/local/mcuxpressoide/",
                    "windows": "$ROOT/nxp/MCUXpressoIDE*"
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
            "defaultValue": "$Qul_ROOT/lib/cmake/Qul/toolchain/armgcc.cmake",
            "id": "ARMGCC_CMAKE_TOOLCHAIN_FILE",
            "label": "CMake Toolchain File",
            "optional": false,
            "type": "file",
            "visible": false
        },
        "id": "armgcc",
        "versions": ["9.3.1"]
    }
})";
