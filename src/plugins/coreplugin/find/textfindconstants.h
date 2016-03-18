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

#include <coreplugin/core_global.h>

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

enum FindFlag {
    FindBackward = 0x01,
    FindCaseSensitively = 0x02,
    FindWholeWords = 0x04,
    FindRegularExpression = 0x08,
    FindPreserveCase = 0x10
};
Q_DECLARE_FLAGS(FindFlags, FindFlag)

// defined in findplugin.cpp
QTextDocument::FindFlags CORE_EXPORT textDocumentFlagsForFindFlags(FindFlags flags);

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::FindFlags)
Q_DECLARE_METATYPE(Core::FindFlags)
