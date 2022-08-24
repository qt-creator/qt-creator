// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto armgcc_nxp_mimxrt1170_evk_freertos_json = R"({
  "qulVersion": "2.1.5",
  "compatVersion": "1",
  "platform": {
    "id": "MIMXRT1170-EVK-FREERTOS",
    "vendor": "NXP",
    "colorDepths": [
      32
    ],
    "pathEntries": [],
    "environmentEntries": [],
    "cmakeEntries": [
      {
        "id": "Qul_DIR",
        "description": "Qt for MCUs SDK",
        "type": "path",
        "cmakeVar": "Qul_ROOT",
        "optional": false
      },
      {
        "id": "MCU_XPRESSO_PATH",
        "description": "MCUXpresso IDE",
        "type": "path",
        "cmakeVar": "MCUXPRESSO_IDE_PATH",
        "defaultValue": {
          "windows": "%{Env:ROOT}/nxp/MCUXpressoIDE*",
          "unix": "/usr/local/mcuxpressoide/"
        },
        "optional": false
      }
    ]
  },
  "toolchain": {
    "id": "armgcc",
    "versions": [
      "9.3.1"
    ],
    "compiler": {
        "id": "ARMGCC_DIR",
        "label": "GNU Arm Embedded Toolchain",
        "description": "GNU Arm Embedded Toolchain",
        "cmakeVar": "QUL_TARGET_TOOLCHAIN_DIR",
        "envVar": "ARMGCC_DIR",
        "setting": "GNUArmEmbeddedToolchain",
        "type": "path",
        "optional": false
      },
      "file": {
        "id": "ARMGCC_CMAKE_TOOLCHAIN_FILE",
        "label": "CMake Toolchain File",
        "description": "CMake Toolchain File",
        "cmakeVar": "CMAKE_TOOLCHAIN_FILE",
        "type": "file",
        "defaultValue": "%{Qul_ROOT}//lib/cmake/Qul/toolchain/armgcc.cmake",
        "visible": false,
        "optional": false
      }
  },
  "boardSdk": {
    "envVar": "EVK_MIMXRT1170_SDK_PATH",
    "versions": [
      "2.10.1"
    ],
    "cmakeEntries": [
      {
        "id": "NXP_SDK_DIR",
        "description": "Board SDK for MIMXRT1170-EVK",
        "cmakeVar": "QUL_BOARD_SDK_DIR",
        "type": "path",
        "optional": false
      }
    ]
  },
  "freeRTOS": {
    "envVar": "EVK_MIMXRT1170_FREERTOS_PATH",
    "cmakeEntries": [
      {
        "id": "NXP_FREERTOS_DIR",
        "description": "FreeRTOS SDK for MIMXRT1170-EVK",
        "cmakeVar": "FREERTOS_DIR",
        "defaultValue": "%{QUL_BOARD_SDK_DIR}/rtos/freertos/freertos_kernel",
        "type": "path",
        "optional": false
      }
    ]
  }
})";
