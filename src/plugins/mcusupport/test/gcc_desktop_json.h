// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto gcc_desktop_json = R"({
  "qulVersion": "2.3.0",
  "compatVersion": "1",
  "platform": {
      "id": "Qt",
      "platformName": "Desktop",
      "vendor": "Qt",
      "colorDepths": [
          32
      ],
      "pathEntries": [
      ],
      "environmentEntries": [
      ],
      "cmakeEntries": [
         {
              "envVar": "Qul_DIR",
              "label": "Qt for MCUs SDK",
              "type": "path",
              "cmakeVar": "Qul_ROOT",
              "optional": false
         }
      ]
  },
  "toolchain": {
    "id": "gcc",
    "versions": [
      "9.4.0",
      "10.3.1"
    ],
    "compiler": {
      "defaultValue": "/usr",
      "versionDetection": {
          "filePattern": "bin/g++",
          "executableArgs": "--version",
          "regex": "\\b(\\d+\\.\\d+\\.\\d+)\\b"
      }
    }
  }
})";
