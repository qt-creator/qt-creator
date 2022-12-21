// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace CMakeProjectManager {
namespace Constants {

const char CMAKE_MIMETYPE[] = "text/x-cmake";
const char CMAKE_PROJECT_MIMETYPE[] = "text/x-cmake-project";
const char CMAKE_EDITOR_ID[] = "CMakeProject.CMakeEditor";
const char RUN_CMAKE[] = "CMakeProject.RunCMake";
const char CLEAR_CMAKE_CACHE[] = "CMakeProject.ClearCache";
const char RESCAN_PROJECT[] = "CMakeProject.RescanProject";
const char RUN_CMAKE_CONTEXT_MENU[] = "CMakeProject.RunCMakeContextMenu";
const char BUILD_FILE_CONTEXT_MENU[] = "CMakeProject.BuildFileContextMenu";
const char BUILD_FILE[] = "CMakeProject.BuildFile";
const char CMAKE_HOME_DIR[] = "CMakeProject.HomeDirectory";
const char QML_DEBUG_SETTING[] = "CMakeProject.EnableQmlDebugging";

const char CMAKEFORMATTER_SETTINGS_GROUP[] = "CMakeFormatter";
const char CMAKEFORMATTER_GENERAL_GROUP[] = "General";
const char CMAKEFORMATTER_ACTION_ID[] = "CMakeFormatter.Action";
const char CMAKEFORMATTER_MENU_ID[] = "CMakeFormatter.Menu";

// Project
const char CMAKE_PROJECT_ID[] = "CMakeProjectManager.CMakeProject";

const char CMAKE_BUILDCONFIGURATION_ID[] = "CMakeProjectManager.CMakeBuildConfiguration";

// Menu
const char M_CONTEXT[] = "CMakeEditor.ContextMenu";

namespace Settings {
const char GENERAL_ID[] = "CMakeSpecifcSettings";
const char TOOLS_ID[] = "K.CMake.Tools";
const char FORMATTER_ID[] = "K.CMake.Formatter";
const char CATEGORY[] = "K.CMake";
} // namespace Settings

// Snippets
const char CMAKE_SNIPPETS_GROUP_ID[] = "CMake";

namespace Icons {
const char FILE_OVERLAY[] = ":/cmakeproject/images/fileoverlay_cmake.png";
const char SETTINGS_CATEGORY[] = ":/cmakeproject/images/settingscategory_cmakeprojectmanager.png";
} // namespace Icons

// Actions
const char BUILD_TARGET_CONTEXT_MENU[] = "CMake.BuildTargetContextMenu";

// Build Step
const char CMAKE_BUILD_STEP_ID[] = "CMakeProjectManager.MakeStep";

// Install Step
const char CMAKE_INSTALL_STEP_ID[] = "CMakeProjectManager.InstallStep";


// Features
const char CMAKE_FEATURE_ID[] = "CMakeProjectManager.Wizard.FeatureCMake";

// Tool
const char TOOL_ID[] = "CMakeProjectManager.CMakeKitInformation";


} // namespace Constants
} // namespace CMakeProjectManager
