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

constexpr auto armgcc_nxp_1064_json = R"({
    "compatVersion": "1",
    "qulVersion": "2.0.0",
    "boardSdk": {
        "cmakeCacheEntries": [
            {
                "cmakeOptionName": "QUL_BOARD_SDK_DIR",
                "description": "Board SDK for MIMXRT1064-EVK",
                "id": "NXP_SDK_DIR",
                "optional": false,
                "type": "path"
            }
         ],
         "envVar": "EVK_MIMXRT1064_SDK_PATH",
         "versions": ["2.10.0"]
    },
    "compatVersion": "1",
    "freeRTOS": {
        "cmakeCacheEntries": [
            {
                "cmakeOptionName": "FREERTOS_DIR",
                "defaultValue": "$QUL_BOARD_SDK_DIR/rtos/freertos/freertos_kernel",
                "description": "FreeRTOS SDK for MIMXRT1064-EVK",
                "id": "NXP_FREERTOS_DIR",
                "optional": false,
                "type": "path"
           }
        ],
        "envVar": "IMXRT1064_FREERTOS_DIR"
    },
    "platform": {
        "cmakeCacheEntries": [
            {
                "cmakeOptionName": "Qul_ROOT",
                "description": "Qt for MCUs SDK",
                "id": "Qul_DIR",
                "optional": false,
                "type": "path"
            },
            {
                "cmakeOptionName": "MCUXPRESSO_IDE_PATH",
                "defaultValue": {
                    "unix": "/usr/local/mcuxpressoide/",
                    "windows": "$ROOT/nxp/MCUXpressoIDE*"
                }
            }
        ],
        "colorDepths": [16],
        "environmentEntries": [],
        "id": "MIMXRT1064-EVK-FREERTOS",
        "pathEntries": [],
        "vendor": "NXP"
    },
    "qulVersion": "2.0.0",
    "toolchain": {
        "cmakeCacheEntries": [
            {
                "cmakeOptionName": "QUL_TARGET_TOOLCHAIN_DIR",
                "description": "IAR ARM Compiler",
                "id": "IAR_DIR",
                "optional": false,
                "type": "path"
            }
        ],
        "id": "iar",
        "versions": ["8.50.9"]
    }
})";
