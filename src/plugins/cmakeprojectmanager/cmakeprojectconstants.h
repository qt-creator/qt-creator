/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtGlobal>

namespace CMakeProjectManager {
namespace Constants {

const char PROJECTCONTEXT[] = "CMakeProject.ProjectContext";
const char CMAKEMIMETYPE[]  = "text/x-cmake";
const char CMAKEPROJECTMIMETYPE[]  = "text/x-cmake-project";
const char CMAKE_EDITOR_ID[] = "CMakeProject.CMakeEditor";
const char CMAKE_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CMakeProjectManager::Internal::CMakeEditorFactory", "CMake Editor");
const char RUNCMAKE[] = "CMakeProject.RunCMake";
const char CLEARCMAKECACHE[] = "CMakeProject.ClearCache";
const char RESCANPROJECT[] = "CMakeProject.RescanProject";
const char RUNCMAKECONTEXTMENU[] = "CMakeProject.RunCMakeContextMenu";

// Project
const char CMAKEPROJECT_ID[] = "CMakeProjectManager.CMakeProject";

// Buildconfiguration
const char CMAKE_BC_ID[] = "CMakeProjectManager.CMakeBuildConfiguration";

// Menu
const char M_CONTEXT[] = "CMakeEditor.ContextMenu";

// Settings page
const char CMAKE_SETTINGSPAGE_ID[] = "Z.CMake";

// Snippets
const char CMAKE_SNIPPETS_GROUP_ID[] = "CMake";

// Icons
const char FILEOVERLAY_CMAKE[] = ":/cmakeproject/images/fileoverlay_cmake.png";

// Actions
const char BUILD_TARGET_CONTEXTMENU[] = "CMake.BuildTargetContextMenu";

// Build Step
const char CMAKE_BUILD_STEP_ID[] = "CMakeProjectManager.MakeStep";

} // namespace Constants
} // namespace CMakeProjectManager
