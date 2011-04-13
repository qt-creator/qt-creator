/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TEXTFINDCONSTANTS_H
#define TEXTFINDCONSTANTS_H

#include "find_global.h"

#include <QtCore/QFlags>
#include <QtGui/QTextDocument>

namespace Find {
namespace Constants {

const char * const M_FIND            = "Find.FindMenu";
const char * const M_FIND_ADVANCED   = "Find.FindAdvancedMenu";
const char * const G_FIND_CURRENTDOCUMENT = "Find.FindMenu.CurrentDocument";
const char * const G_FIND_FILTERS    = "Find.FindMenu.Filters";
const char * const G_FIND_FLAGS      = "Find.FindMenu.Flags";
const char * const G_FIND_ACTIONS    = "Find.FindMenu.Actions";

const char * const ADVANCED_FIND     = "Find.Dialog";
const char * const FIND              = "Find.FindReplace";
const char * const FIND_IN_DOCUMENT  = "Find.FindInCurrentDocument";
const char * const FIND_NEXT         = "Find.FindNext";
const char * const FIND_PREVIOUS     = "Find.FindPrevious";
const char * const FIND_ALL          = "Find.FindAll";
const char * const REPLACE           = "Find.Replace";
const char * const REPLACE_NEXT      = "Find.ReplaceNext";
const char * const REPLACE_PREVIOUS  = "Find.ReplacePrevious";
const char * const REPLACE_ALL       = "Find.ReplaceAll";
const char * const CASE_SENSITIVE    = "Find.CaseSensitive";
const char * const WHOLE_WORDS       = "Find.WholeWords";
const char * const REGULAR_EXPRESSIONS="Find.RegularExpressions";
const char * const TASK_SEARCH       = "Find.Task.Search";

} // namespace Constants

enum FindFlag {
    FindBackward = 0x01,
    FindCaseSensitively = 0x02,
    FindWholeWords = 0x04,
    FindRegularExpression = 0x08
};
Q_DECLARE_FLAGS(FindFlags, FindFlag)

// defined in findplugin.cpp
QTextDocument::FindFlags FIND_EXPORT textDocumentFlagsForFindFlags(Find::FindFlags flags);

} // namespace Find

Q_DECLARE_OPERATORS_FOR_FLAGS(Find::FindFlags)

#endif // TEXTFINDCONSTANTS_H
