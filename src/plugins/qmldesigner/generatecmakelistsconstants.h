/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#ifndef GENERATECMAKELISTSCONSTANTS_H
#define GENERATECMAKELISTSCONSTANTS_H

#pragma once

namespace QmlDesigner {
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
} //QmlDesigner

#endif // GENERATECMAKELISTSCONSTANTS_H
