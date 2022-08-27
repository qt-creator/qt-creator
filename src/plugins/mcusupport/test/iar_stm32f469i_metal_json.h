// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto iar_stm32f469i_metal_json = R"({
  "qulVersion": "@CMAKE_PROJECT_VERSION@",
  "compatVersion": "@COMPATIBILITY_VERSION@",
  "platform": {
    "id": "STM32F469I-DISCOVERY-BAREMETAL",
    "vendor": "ST",
    "colorDepths": [
      24
    ],
    "pathEntries": [
      {
        "id": "STM32CubeProgrammer_PATH",
        "label": "STM32CubeProgrammer",
        "type": "path",
        "defaultValue": {
          "windows": "%{Env:PROGRAMSANDFILES}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/",
          "unix": "%{Env:HOME}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/"
        },
        "optional": false
      }
    ],
    "environmentEntries": [],
    "cmakeEntries": [
      {
        "id": "Qul_DIR",
        "label": "Qt for MCUs SDK",
        "type": "path",
        "cmakeVar": "Qul_ROOT",
        "optional": false
      }
    ]
  },
  "toolchain": {
    "id": "iar",
    "versions": [
      "8.50.9"
    ],
    "compiler": {
        "id": "IARToolchain",
        "setting": "IARToolchain",
        "envVar": "IAR_ARM_COMPILER_DIR",
        "label": "IAR ARM Compiler",
        "cmakeVar": "QUL_TARGET_TOOLCHAIN_DIR",
        "type": "path",
        "versionDetection" : {
            "filePattern": "bin/iccarm",
            "executableArgs": "--version",
            "regex": "\\bV(\\d+\\.\\d+\\.\\d+)\\.\\d+\\b"
        }
      },
      "file": {
        "id": "IAR_CMAKE_TOOLCHAIN_FILE",
        "label": "CMake Toolchain File",
        "cmakeVar": "CMAKE_TOOLCHAIN_FILE",
        "type": "file",
        "defaultValue": "/opt/qtformcu/2.2//lib/cmake/Qul/toolchain/iar.cmake",
        "visible": false,
        "optional": false
      }
  },
  "boardSdk": {
    "envVar": "STM32Cube_FW_F4_SDK_PATH",
    "setting": "STM32Cube_FW_F4_SDK_PATH",
    "versions": [ "1.25.0" ],
    "versionDetection" : {
        "filePattern": "package.xml",
        "xmlElement": "PackDescription",
        "xmlAttribute": "Release",
        "regex": "\b(\d+\.\d+\.\d+)\b"
    },
    "id": "ST_SDK_DIR",
    "label": "Board SDK for STM32F469I-Discovery",
    "cmakeVar": "QUL_BOARD_SDK_DIR",
    "type": "path",
    "optional": false
  }
})";
