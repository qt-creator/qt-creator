// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Autotest {
namespace Constants {

inline constexpr char ACTION_DISABLE_TMP[]              = "AutoTest.DisableTemp";
inline constexpr char ACTION_SCAN_ID[]                  = "AutoTest.ScanAction";
inline constexpr char ACTION_RUN_ALL_ID[]               = "AutoTest.RunAll";
inline constexpr char ACTION_RUN_ALL_NODEPLOY_ID[]      = "AutoTest.RunAllNoDeploy";
inline constexpr char ACTION_RUN_SELECTED_ID[]          = "AutoTest.RunSelected";
inline constexpr char ACTION_RUN_SELECTED_NODEPLOY_ID[] = "AutoTest.RunSelectedNoDeploy";
inline constexpr char ACTION_RUN_ALL_DBG_ID[]           = "AutoTest.RunAllDebug";
inline constexpr char ACTION_RUN_ALL_DBG_NDEP_ID[]      = "AutoTest.RunAllDebugNoDeploy";
inline constexpr char ACTION_RUN_SELECTED_DBG_ID[]      = "AutoTest.RunSelectedDebug";
inline constexpr char ACTION_RUN_SELECTED_DBG_NDEP_ID[] = "AutoTest.RunSelectedDebugNoDeploy";
inline constexpr char ACTION_RUN_FAILED_ID[]            = "AutoTest.RunFailed";
inline constexpr char ACTION_RUN_FILE_ID[]              = "AutoTest.RunFile";
inline constexpr char ACTION_RUN_UCURSOR[]              = "AutoTest.RunUnderCursor";
inline constexpr char ACTION_RUN_UCURSOR_NODEPLOY[]     = "AutoTest.RunUnderCursorNoDeploy";
inline constexpr char ACTION_RUN_DBG_UCURSOR[]          = "AutoTest.RunDebugUnderCursor";
inline constexpr char ACTION_RUN_DBG_UCURSOR_NODEPLOY[] = "AutoTest.RunDebugUnderCursorNoDeploy";
inline constexpr char MENU_ID[]                         = "AutoTest.Menu";
inline constexpr char AUTOTEST_ID[]                     = "AutoTest.ATP";
inline constexpr char AUTOTEST_CONTEXT[]                = "Auto Tests";
inline constexpr char TASK_INDEX[]                      = "AutoTest.Task.Index";
inline constexpr char TASK_PARSE[]                      = "AutoTest.Task.Parse";
inline constexpr char AUTOTEST_SETTINGS_CATEGORY[]      = "ZY.Tests";
inline constexpr char AUTOTEST_SETTINGS_ID[]            = "A.AutoTest.0.General";
inline constexpr char FRAMEWORK_PREFIX[]                = "AutoTest.Framework.";
inline constexpr char SETTINGSPAGE_PREFIX[]             = "A.AutoTest.";
inline constexpr char SETTINGSGROUP[]                   = "Autotest";
inline constexpr char TASK_MARK_ID[]                    = "Autotest.TaskMark";
inline constexpr char SK_USE_GLOBAL[]                   = "AutoTest.UseGlobal";
} // namespace Constants

enum class TestRunMode
{
    None,
    Run,
    RunWithoutDeploy,
    Debug,
    DebugWithoutDeploy,
    RunAfterBuild
};

enum class OutputChannel { StdOut, StdErr };

} // namespace Autotest
