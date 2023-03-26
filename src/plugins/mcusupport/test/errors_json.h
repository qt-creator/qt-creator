// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto errors_json = R"(
{
    "compatVersion": "1",
    "qulVersion": "2.0.0",
    "boardSdk": {
        "cmakeEntries": [
            {
                "id": "NXP_SDK_DIR",
                "cmakeVar": "QUL_BOARD_SDK_DIR",
                "label": "This package has no label",
                "optional": false,
                "type": "path",
                "versions": ["2.10.0"]
            }
            {
                "id": "NXP_SDK_DIR",
                "cmakeVar": "QUL_BOARD_SDK_DIR",
                "label": "",
                "label": "This package has empty label",
                "optional": false,
                "type": "path",
                "versions": ["2.10.0"]
            }
         ],
         "envVar": "EVK_MIMXRT1064_SDK_PATH",
         "versions": ["2.10.0"]
    }
}
)";

constexpr auto unpaired_square_bracket_json = R"(
{
    "compatVersion": "1",
    "qulVersion": "2.0.0",
    "toolchain": {
      "id": "greenhills",
      "versions": [
        "2018.1.5"
      ],
      "compiler": {
          "label": "Green Hills Compiler",
          "cmakeVar": "QUL_TARGET_TOOLCHAIN_DIR",
          "setting": "GHSToolchain",
          "label": "IAR ARM Compiler",
          "type": "path",
          "optional": false
      },
      "file": {
          "cmakeVar": "CMAKE_TOOLCHAIN_FILE",
          "type": "file",
          "defaultValue": "%{Qul_ROOT}/lib/cmake/Qul/toolchain/ghs.cmake",
          "visible": false,
          "optional": false
        }
      ]
    },
}
)";

constexpr auto mulitple_toolchain_versions = R"(
{
  "compatVersion": "1",
  "qulVersion": "2.0.0",
  "platform": {
    "id": "id",
    "vendor": "ST",
    "colorDepths": [
      32
    ],
    "cmakeEntries": [
      {
        "label": "Qt for MCUs SDK",
        "type": "path",
        "setting": "QtForMCUsSdk",
        "cmakeVar": "Qul_ROOT",
        "envVar": "Qul_DIR",
        "optional": false,
        "versionDetection": {
          "filePattern": "bin/qmltocpp"
        }
      },
      {
        "id": "STM32CubeProgrammer_PATH",
        "label": "STM32CubeProgrammer",
        "type": "path",
        "setting": "Stm32CubeProgrammer",
        "defaultValue": {
          "windows": "%{Env:PROGRAMFILES}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/",
          "linux": "%{Env:HOME}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/"
        },
        "optional": false
      }
    ]
  },
  "toolchain": {
    "id": "greenhills",
    "versions": [
      "2018.1.5",
      "123"
    ],
    "compiler": {
      "label": "IAR ARM Compiler",
      "cmakeVar": "QUL_TARGET_TOOLCHAIN_DIR",
      "setting": "GHSToolchain",
      "type": "path",
      "optional": false
    },
    "file": {
      "cmakeVar": "CMAKE_TOOLCHAIN_FILE",
      "type": "file",
      "defaultValue": "%{Qul_ROOT}/lib/cmake/Qul/toolchain/ghs.cmake",
      "visible": false,
      "optional": false
    }
  }
}
)";
