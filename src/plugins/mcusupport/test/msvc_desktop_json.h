// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto msvc_desktop_json = R"({
  "qulVersion": "@CMAKE_PROJECT_VERSION@",
  "compatVersion": "@COMPATIBILITY_VERSION@",
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
              "id": "Qul_DIR",
              "label": "Qt for MCUs SDK",
              "type": "path",
              "cmakeVar": "Qul_ROOT",
              "optional": false
         }
      ]
  },
  "toolchain": {
    "id": "msvc",
    "versions": [
        "19.20","19.21","19.22","19.23","19.24",
        "19.25","19.26","19.27","19.28","19.29"
    ],
    "compiler": {
      "defaultValue": "$MSVC_COMPILER_DIR",
      "versionDetection" : {
            "filePattern": "cl.exe",
            "executableArgs": "--version",
            "regex": "\\b(\\d+\\.\\d+)\\.\\d+\\b"
        }
    }
  }
})";
