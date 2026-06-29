// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace ClangTools::Constants {

inline constexpr char PROJECT_PANEL_ID[] = "ClangTools";

inline constexpr char RUN_CLANGTIDY_ON_PROJECT[] = "ClangTools.ClangTidy.RunOnProject";
inline constexpr char RUN_CLAZY_ON_PROJECT[] = "ClangTools.Clazy.RunOnProject";
inline constexpr char RUN_CLANGTIDY_ON_CURRENT_FILE[] = "ClangTools.ClangTidy.RunOnCurrentFile";
inline constexpr char RUN_CLAZY_ON_CURRENT_FILE[] = "ClangTools.Clazy.RunOnCurrentFile";

inline constexpr char SETTINGS_PAGE_ID[] = "Analyzer.ClangTools.Settings";
inline constexpr char SETTINGS_ID[] = "ClangTools";
inline constexpr char CLANGTIDYCLAZY_RUN_MODE[] = "ClangTidyClazy.RunMode";

inline constexpr char CLANG_TIDY_EXECUTABLE_NAME[] = "clang-tidy";
inline constexpr char CLAZY_STANDALONE_EXECUTABLE_NAME[] = "clazy-standalone";

inline constexpr char CLANG_TOOL_FIXIT_AVAILABLE_MARKER_ID[] = "ClangToolFixItAvailableMarker";

inline constexpr char DIAGNOSTIC_MARK_ID[] = "ClangTool.DiagnosticMark";

inline constexpr char DIAG_CONFIG_TIDY_AND_CLAZY[] = "Builtin.DefaultTidyAndClazy";

} // ClangTools::Constants
