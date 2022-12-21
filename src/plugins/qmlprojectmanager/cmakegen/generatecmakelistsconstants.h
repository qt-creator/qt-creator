// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef GENERATECMAKELISTSCONSTANTS_H
#define GENERATECMAKELISTSCONSTANTS_H

#pragma once

namespace QmlProjectManager {
namespace GenerateCmake {
namespace Constants {

const char DIRNAME_CONTENT[] = "content";
const char DIRNAME_IMPORT[] = "imports";
const char DIRNAME_ASSET[] = "assets";
const char DIRNAME_ASSETIMPORT[] = "asset_imports";
const char DIRNAME_CPP[] = "src";
const char DIRNAME_DESIGNER[] = "designer";

const char FILENAME_CMAKELISTS[] = "CMakeLists.txt";
const char FILENAME_APPMAINQML[] = "App.qml";
const char FILENAME_MAINQML[] = "main.qml";
const char FILENAME_MAINCPP[] = "main.cpp";
const char FILENAME_MAINCPP_HEADER[] = "import_qml_plugins.h";
const char FILENAME_MODULES[] = "qmlmodules";
const char FILENAME_QMLDIR[] = "qmldir";
const char FILENAME_ENV_HEADER[] = "app_environment.h";

const char FILENAME_SUFFIX_QMLPROJECT[] = "qmlproject";
const char FILENAME_SUFFIX_QML[] = "qml";
const char FILENAME_SUFFIX_USER[] = "user";

const char FILENAME_FILTER_QMLPROJECT[] = "*.qmlproject";
const char FILENAME_FILTER_QML[] = "*.qml";

const char ENV_VARIABLE_CONTROLCONF[] = "QT_QUICK_CONTROLS_CONF";

} //Constants
} //GenerateCmake
} //QmlProjectManager

#endif // GENERATECMAKELISTSCONSTANTS_H
