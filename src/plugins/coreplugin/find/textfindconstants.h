// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QFlags>
#include <QTextDocument>

namespace Core::Constants {

inline constexpr char C_FINDTOOLBAR[]     = "Find.ToolBar";

inline constexpr char M_FIND[]            = "Find.FindMenu";
inline constexpr char M_FIND_ADVANCED[]   = "Find.FindAdvancedMenu";
inline constexpr char G_FIND_CURRENTDOCUMENT[] = "Find.FindMenu.CurrentDocument";
inline constexpr char G_FIND_FILTERS[]    = "Find.FindMenu.Filters";
inline constexpr char G_FIND_FLAGS[]      = "Find.FindMenu.Flags";
inline constexpr char G_FIND_ACTIONS[]    = "Find.FindMenu.Actions";

inline constexpr char ADVANCED_FIND[]     = "Find.Dialog";
inline constexpr char FIND_IN_DOCUMENT[]  = "Find.FindInCurrentDocument";
inline constexpr char FIND_NEXT_SELECTED[]= "Find.FindNextSelected";
inline constexpr char FIND_PREV_SELECTED[]= "Find.FindPreviousSelected";
inline constexpr char FIND_SELECT_ALL[]   = "Find.SelectAll";
inline constexpr char FIND_NEXT[]         = "Find.FindNext";
inline constexpr char FIND_PREVIOUS[]     = "Find.FindPrevious";
inline constexpr char REPLACE[]           = "Find.Replace";
inline constexpr char REPLACE_NEXT[]      = "Find.ReplaceNext";
inline constexpr char REPLACE_PREVIOUS[]  = "Find.ReplacePrevious";
inline constexpr char REPLACE_ALL[]       = "Find.ReplaceAll";
inline constexpr char CASE_SENSITIVE[]    = "Find.CaseSensitive";
inline constexpr char WHOLE_WORDS[]       = "Find.WholeWords";
inline constexpr char REGULAR_EXPRESSIONS[] = "Find.RegularExpressions";
inline constexpr char PRESERVE_CASE[]     = "Find.PreserveCase";
inline constexpr char TASK_SEARCH[]       = "Find.Task.Search";

} // namespace Core::Constants
