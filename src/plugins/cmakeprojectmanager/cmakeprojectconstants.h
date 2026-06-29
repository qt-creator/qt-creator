// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace CMakeProjectManager::Constants {

inline constexpr char CMAKE_EDITOR_ID[] = "CMakeProject.CMakeEditor";
inline constexpr char RUN_CMAKE[] = "CMakeProject.RunCMake";
inline constexpr char RUN_CMAKE_PROFILER[] = "CMakeProject.RunCMakeProfiler";
inline constexpr char RUN_CMAKE_DEBUGGER[] = "CMakeProject.RunCMakeDebugger";
inline constexpr char CLEAR_CMAKE_CACHE[] = "CMakeProject.ClearCache";
inline constexpr char CLEAR_CMAKE_CACHE_CONTEXT_MENU[] = "CMakeProject.ClearCacheContextMenu";
inline constexpr char RESCAN_PROJECT[] = "CMakeProject.RescanProject";
inline constexpr char RUN_CMAKE_CONTEXT_MENU[] = "CMakeProject.RunCMakeContextMenu";
inline constexpr char CMAKE_HOME_DIR[] = "CMakeProject.HomeDirectory";
inline constexpr char QML_DEBUG_SETTING[] = "CMakeProject.EnableQmlDebugging";
inline constexpr char RELOAD_CMAKE_PRESETS[] = "CMakeProject.ReloadCMakePresets";
inline constexpr char BUILD_SUBPROJECT[] = "CMakeProject.BuildSubProject";
inline constexpr char CLEAN_SUBPROJECT[] = "CMakeProject.CleanSubProject";
inline constexpr char REBUILD_SUBPROJECT[] = "CMakeProject.RebuildSubProject";

inline constexpr char CMAKE_CURRENT_SOURCE_DIR[] = "${CMAKE_CURRENT_SOURCE_DIR}";

inline constexpr char CMAKEFORMATTER_SETTINGS_GROUP[] = "CMakeFormatter";
inline constexpr char CMAKEFORMATTER_GENERAL_GROUP[] = "General";
inline constexpr char CMAKEFORMATTER_ACTION_ID[] = "CMakeFormatter.Action";
inline constexpr char CMAKEFORMATTER_MENU_ID[] = "CMakeFormatter.Menu";
inline constexpr char CMAKE_DEBUGGING_GROUP[] = "Debugger.Group.CMake";

inline constexpr char VCPKG_ROOT[] = "VCPKG_ROOT";

inline constexpr char CMAKE_LISTS_TXT[] = "CMakeLists.txt";
inline constexpr char CMAKE_CACHE_TXT[] = "CMakeCache.txt";
inline constexpr char CMAKE_CACHE_TXT_PREV[] = "CMakeCache.txt.prev";

// Project
inline constexpr char CMAKE_PROJECT_ID[] = "CMakeProjectManager.CMakeProject";

inline constexpr char CMAKE_BUILDCONFIGURATION_ID[] = "CMakeProjectManager.CMakeBuildConfiguration";
inline constexpr char CMAKE_IMPORTED_BUILD[] = "CMake.Imported";

// Menu
inline constexpr char M_CONTEXT[] = "CMakeEditor.ContextMenu";

namespace Settings {
inline constexpr char GENERAL_ID[] = "CMakeSpecificSettings";
inline constexpr char TOOLS_ID[] = "K.CMake.Tools";
inline constexpr char FORMATTER_ID[] = "K.CMake.Formatter";
inline constexpr char CATEGORY[] = "K.CMake";
inline constexpr char USE_GLOBAL_SETTINGS[] = "UseGlobalSettings";
} // namespace Settings

// Snippets
inline constexpr char CMAKE_SNIPPETS_GROUP_ID[] = "CMake";

namespace Icons {
inline constexpr char FILE_OVERLAY[] = ":/cmakeproject/images/fileoverlay_cmake.png";
inline constexpr char SETTINGS_CATEGORY[] = ":/cmakeproject/images/settingscategory_cmakeprojectmanager.png";
} // namespace Icons

// Build Step
inline constexpr char CMAKE_BUILD_STEP_ID[] = "CMakeProjectManager.MakeStep";

// Install Step
inline constexpr char CMAKE_INSTALL_STEP_ID[] = "CMakeProjectManager.InstallStep";


// Features
inline constexpr char CMAKE_FEATURE_ID[] = "CMakeProjectManager.Wizard.FeatureCMake";

// Tool
inline constexpr char TOOL_ID[] = "CMakeProjectManager.CMakeKitInformation";

// Data
inline constexpr char BUILD_FOLDER_ROLE[] = "CMakeProjectManager.data.buildFolder";

// Output
inline constexpr char OUTPUT_PREFIX[] = "[cmake] ";

inline constexpr char VXWORKS_DEVICE_TYPE[] = "VxWorks.Device.Type";

inline constexpr char KIT_BUILDINFO_LIST[] = "CMakeProjectManager.BuildInfoList";
inline constexpr char PRESETS_KITS_PROGRESS[] = "CMakeProjectManager.Presets.Kits.Progress";

} // namespace CMakeProjectManager::Constants
