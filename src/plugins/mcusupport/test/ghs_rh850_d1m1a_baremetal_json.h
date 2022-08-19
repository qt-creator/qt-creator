// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto ghs_rh850_d1m1a_baremetal_json = R"({
  "qulVersion": "2.1.5",
  "compatVersion": "1",
  "platform": {
    "id": "RH850-D1M1A-BAREMETAL",
    "vendor": "RENESAS",
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
      }
    ]
  },
  "toolchain": {
    "id": "greenhills",
    "versions": [
      "2018.1.5"
    ],
    "compiler": {
        "description": "Green Hills Compiler",
        "cmakeVar": "QUL_TARGET_TOOLCHAIN_DIR",
        "setting": "GHSToolchain",
        "label": "Green Hills Compiler",
        "type": "path",
        "optional": false
    },
    "file": {
        "description": "CMake Toolchain File",
        "cmakeVar": "CMAKE_TOOLCHAIN_FILE",
        "type": "file",
        "defaultValue": "/opt/qtformcu/2.2/lib/cmake/Qul/toolchain/ghs.cmake",
        "visible": false,
        "optional": false
      }
  },
  "boardSdk": {
    "envVar": "RGL_DIR",
    "setting": "RGL_DIR",
    "versions": [ "2.0.0a" ],
    "id": "RGL_DIR",
    "description": "Renesas Graphics Library",
    "cmakeVar": "QUL_BOARD_SDK_DIR",
    "type": "path",
    "defaultValue": "/Renesas_Electronics/D1x_RGL/rgl_ghs_D1Mx_obj_V.2.0.0a",
    "versionDetection": {
       "filePattern" : "rgl_*_obj_*",
       "regex" : "\\d+\\.\\d+\\.\\w+",
       "isFile" : false
    },
    "optional": false
  }
})";
