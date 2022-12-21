// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace ClangTools {
namespace Constants {

const char PROJECT_PANEL_ID[] = "ClangTools";

const char RUN_CLANGTIDY_ON_PROJECT[] = "ClangTools.ClangTidy.RunOnProject";
const char RUN_CLAZY_ON_PROJECT[] = "ClangTools.Clazy.RunOnProject";
const char RUN_CLANGTIDY_ON_CURRENT_FILE[] = "ClangTools.ClangTidy.RunOnCurrentFile";
const char RUN_CLAZY_ON_CURRENT_FILE[] = "ClangTools.Clazy.RunOnCurrentFile";

const char SETTINGS_PAGE_ID[] = "Analyzer.ClangTools.Settings";
const char SETTINGS_ID[] = "ClangTools";
const char CLANGTIDYCLAZY_RUN_MODE[] = "ClangTidyClazy.RunMode";

const char CLANG_TIDY_EXECUTABLE_NAME[] = "clang-tidy";
const char CLAZY_STANDALONE_EXECUTABLE_NAME[] = "clazy-standalone";

const char CLANG_TOOL_FIXIT_AVAILABLE_MARKER_ID[] = "ClangToolFixItAvailableMarker";

const char DIAGNOSTIC_MARK_ID[] = "ClangTool.DiagnosticMark";

const char DIAG_CONFIG_TIDY_AND_CLAZY[] = "Builtin.DefaultTidyAndClazy";

} // Constants
} // ClangTools
