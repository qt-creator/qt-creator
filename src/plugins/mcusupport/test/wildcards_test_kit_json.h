// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// This file is intended for testing McuSupport logic by adding fake packages

#pragma once

constexpr auto wildcards_test_kit = R"(
{
    "qulVersion": "2.3.0",
    "compatVersion": "1",
    "platform": {
        "id": "fake-platform",
        "vendor": "fake-vendor",
        "colorDepths": [32],
        "cmakeEntries": [
            {
                "label": "FAKE_WILDCARD_TEST_1",
                "description": "Assert '*' is replaced by possible values",
                "defaultValue": "%{MCU_TESTING_FOLDER}/wildcards/folder-*",
                "envVar": "",
                "type": "path"
            },
            {
                "label": "FAKE_WILDCARD_TEST_2",
                "description": "Assert '*' is replaced by possible values",
                "defaultValue": "%{MCU_TESTING_FOLDER}/wildcards/file-*.exe",
                "envVar": "",
                "type": "path"
            },
            {
                "label": "FAKE_WILDCARD_TEST_3",
                "description": "Assert '*' is replaced by possible values",
                "defaultValue": "%{MCU_TESTING_FOLDER}/wildcards/*-file.exe",
                "envVar": "",
                "type": "path"
            },
            {
                "label": "FAKE_WILDCARD_TEST_MULTI",
                "description": "Assert '*' is replaced by possible values",
                "defaultValue": "%{MCU_TESTING_FOLDER}/wildcards/2019/*/VC/Tools/MSVC/*/bin/Hostx64/x64",
                "envVar": "",
                "type": "path"
            }
        ]
    },
    "toolchain": {
        "id": "fake-compiler",
        "versions": [
            "0.0.1"
        ],
        "compiler": {
            "cmakeVar": "QUL_TARGET_TOOLCHAIN_DIR",
            "setting": "",
            "label": "fake compiler",
            "type": "path",
            "optional": false
        },
        "file": {
            "cmakeVar": "CMAKE_TOOLCHAIN_FILE",
            "type": "file",
            "defaultValue": "",
            "visible": false,
            "optional": false
        }
    },
    "boardSdk": {
        "envVar": "",
        "setting": "",
        "versions": [
            "0.0.1"
        ],
        "label": "",
        "cmakeVar": "QUL_BOARD_SDK_DIR",
        "type": "path",
        "defaultValue": "",
        "versionDetection": {},
        "optional": false
    }
}
)";
