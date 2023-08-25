// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Autotest {
namespace Constants {

const char ACTION_DISABLE_TMP[]              = "AutoTest.DisableTemp";
const char ACTION_SCAN_ID[]                  = "AutoTest.ScanAction";
const char ACTION_RUN_ALL_ID[]               = "AutoTest.RunAll";
const char ACTION_RUN_ALL_NODEPLOY_ID[]      = "AutoTest.RunAllNoDeploy";
const char ACTION_RUN_SELECTED_ID[]          = "AutoTest.RunSelected";
const char ACTION_RUN_SELECTED_NODEPLOY_ID[] = "AutoTest.RunSelectedNoDeploy";
const char ACTION_RUN_FAILED_ID[]            = "AutoTest.RunFailed";
const char ACTION_RUN_FILE_ID[]              = "AutoTest.RunFile";
const char ACTION_RUN_UCURSOR[]              = "AutoTest.RunUnderCursor";
const char ACTION_RUN_UCURSOR_NODEPLOY[]     = "AutoTest.RunUnderCursorNoDeploy";
const char ACTION_RUN_DBG_UCURSOR[]          = "AutoTest.RunDebugUnderCursor";
const char ACTION_RUN_DBG_UCURSOR_NODEPLOY[] = "AutoTest.RunDebugUnderCursorNoDeploy";
const char MENU_ID[]                         = "AutoTest.Menu";
const char AUTOTEST_ID[]                     = "AutoTest.ATP";
const char AUTOTEST_CONTEXT[]                = "Auto Tests";
const char TASK_INDEX[]                      = "AutoTest.Task.Index";
const char TASK_PARSE[]                      = "AutoTest.Task.Parse";
const char AUTOTEST_SETTINGS_CATEGORY[]      = "ZY.Tests";
const char AUTOTEST_SETTINGS_ID[]            = "A.AutoTest.0.General";
const char FRAMEWORK_PREFIX[]                = "AutoTest.Framework.";
const char SETTINGSPAGE_PREFIX[]             = "A.AutoTest.";
const char SETTINGSGROUP[]                   = "Autotest";
const char TASK_MARK_ID[]                    = "Autotest.TaskMark";
const char SK_USE_GLOBAL[]                   = "AutoTest.UseGlobal";
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
