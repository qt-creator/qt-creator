/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TEXTFINDCONSTANTS_H
#define TEXTFINDCONSTANTS_H

#include "find_global.h"

#include <QMetaType>
#include <QFlags>
#include <QTextDocument>

namespace Find {
namespace Constants {

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
QTextDocument::FindFlags FIND_EXPORT textDocumentFlagsForFindFlags(Find::FindFlags flags);

} // namespace Find

Q_DECLARE_OPERATORS_FOR_FLAGS(Find::FindFlags)
Q_DECLARE_METATYPE(Find::FindFlags)

#endif // TEXTFINDCONSTANTS_H
