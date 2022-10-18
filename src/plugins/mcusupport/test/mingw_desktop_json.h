// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

constexpr auto mingw_desktop_json = R"(
{
    "qulVersion": "2.3.0",
    "compatVersion": "1",
    "platform": {
        "id": "Qt",
        "platformName": "Desktop",
        "vendor": "Qt",
        "colorDepths": [
            32
        ]
    },
    "toolchain": {
        "id": "mingw",
        "versions": [
            "11.2.0"
        ],
        "compiler": {
            "label": "MinGW Toolchain",
            "defaultValue": "%{QT_TOOLS_DIR}/mingw1120_64"
        }
    }
}
)";
