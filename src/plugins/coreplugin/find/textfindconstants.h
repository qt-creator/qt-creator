// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QFlags>
#include <QTextDocument>

namespace Core {
namespace Constants {

const char C_FINDTOOLBAR[]     = "Find.ToolBar";

const char M_FIND[]            = "Find.FindMenu";
const char M_FIND_ADVANCED[]   = "Find.FindAdvancedMenu";
const char G_FIND_CURRENTDOCUMENT[] = "Find.FindMenu.CurrentDocument";
const char G_FIND_FILTERS[]    = "Find.FindMenu.Filters";
const char G_FIND_FLAGS[]      = "Find.FindMenu.Flags";
const char G_FIND_ACTIONS[]    = "Find.FindMenu.Actions";

const char ADVANCED_FIND[]     = "Find.Dialog";
const char FIND_IN_DOCUMENT[]  = "Find.FindInCurrentDocument";
const char FIND_NEXT_SELECTED[]= "Find.FindNextSelected";
const char FIND_PREV_SELECTED[]= "Find.FindPreviousSelected";
const char FIND_SELECT_ALL[]   = "Find.SelectAll";
const char FIND_NEXT[]         = "Find.FindNext";
const char FIND_PREVIOUS[]     = "Find.FindPrevious";
const char REPLACE[]           = "Find.Replace";
const char REPLACE_NEXT[]      = "Find.ReplaceNext";
const char REPLACE_PREVIOUS[]  = "Find.ReplacePrevious";
const char REPLACE_ALL[]       = "Find.ReplaceAll";
const char CASE_SENSITIVE[]    = "Find.CaseSensitive";
const char WHOLE_WORDS[]       = "Find.WholeWords";
const char REGULAR_EXPRESSIONS[] = "Find.RegularExpressions";
const char PRESERVE_CASE[]     = "Find.PreserveCase";
const char TASK_SEARCH[]       = "Find.Task.Search";

} // namespace Constants

} // namespace Core
