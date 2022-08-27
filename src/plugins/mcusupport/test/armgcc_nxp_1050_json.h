// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto armgcc_nxp_1050_json = R"({
  "qulVersion": "2.0.0",
  "compatVersion": "1",
  "platform": {
    "id": "MIMXRT1050-EVK-FREERTOS",
    "vendor": "NXP",
    "colorDepths": [
      16
    ],
    "cmakeEntries": [
      {
        "label": "Qt for MCUs SDK",
        "type": "path",
        "cmakeVar": "Qul_ROOT",
        "envVar": "Qul_DIR",
        "setting": "QtForMCUsSdk",
        "optional": false
      },
      {
        "id": "MCU_XPRESSO_PATH",
        "label": "MCUXpresso IDE",
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
      "9.3.1",
      "10.3.1"
    ],
    "compiler": {
        "label": "GNU Arm Embedded Toolchain",
        "cmakeVar": "QUL_TARGET_TOOLCHAIN_DIR",
        "envVar": "ARMGCC_DIR",
        "setting": "GNUArmEmbeddedToolchain",
        "type": "path",
        "optional": false,
        "versionDetection" : {
            "filePattern": "bin/arm-none-eabi-g++",
            "executableArgs": "--version",
            "regex": "\\bv(\\d+\\.\\d+\\.\\d+)\\b"
        }
      },
      "file": {
        "label": "CMake Toolchain File",
        "cmakeVar": "CMAKE_TOOLCHAIN_FILE",
        "type": "file",
        "defaultValue": "%{Qul_ROOT}/lib/cmake/Qul/toolchain/armgcc.cmake",
        "visible": false,
        "optional": false
      }
  },
    "boardSdk": {
        "cmakeVar": "QUL_BOARD_SDK_DIR",
        "envVar": "EVKB_IMXRT1050_SDK_PATH",
        "label": "Board SDK for MIMXRT1050-EVK",
        "optional": false,
        "setting": "EVKB_IMXRT1050_SDK_PATH",
        "type": "path",
        "versionDetection": {
            "filePattern": "*_manifest_*.xml",
            "regex": ".*",
            "xmlAttribute": "version",
            "xmlElement": "ksdk"
        },
        "versions": ["2.11.0"]
    },
    "freeRTOS": {
        "cmakeVar": "FREERTOS_DIR",
        "defaultValue": "%{QUL_BOARD_SDK_DIR}/rtos/freertos/freertos_kernel",
        "envVar": "IMXRT1050_FREERTOS_DIR",
        "label": "FreeRTOS SDK for MIMXRT1050-EVK",
        "optional": false,
        "setting": "FreeRTOSSourcePackage_IMXRT1050",
        "type": "path"
    }
})";
