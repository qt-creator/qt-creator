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
    "versions": [
      "2.0.0a"
    ],
    "cmakeEntries": [
      {
        "id": "RGL_DIR",
        "description": "Renesas Graphics Library",
        "cmakeVar": "QUL_BOARD_SDK_DIR",
        "type": "path",
        "defaultValue": "/Renesas_Electronics/D1x_RGL/rgl_ghs_D1Mx_obj_V.2.0.0a",
        "optional": false
      }
    ]
  }
})";
