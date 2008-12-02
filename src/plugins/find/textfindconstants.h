/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#ifndef TEXTFINDCONSTANTS_H
#define TEXTFINDCONSTANTS_H

    namespace Find {
        namespace Constants {
            const char * const M_FIND = "Find.FindMenu";
            const char * const G_FIND_FILTERS = "Find.FindMenu.Filters";
            const char * const G_FIND_FLAGS = "Find.FindMenu.Flags";
            const char * const G_FIND_ACTIONS = "Find.FindMenu.Actions";

            const char * const FIND                 = "Find.FindReplace";
            const char * const FIND_IN_DOCUMENT     = "Find.FindInCurrentDocument";
            const char * const FIND_NEXT             = "Find.FindNext";
            const char * const FIND_PREVIOUS         = "Find.FindPrevious";
            const char * const FIND_ALL             = "Find.FindAll";
            const char * const REPLACE_NEXT          = "Find.ReplaceNext";
            const char * const REPLACE_PREVIOUS     = "Find.ReplacePrevious";
            const char * const REPLACE_ALL          = "Find.ReplaceAll";
            const char * const CASE_SENSITIVE = "Find.CaseSensitive";
            const char * const WHOLE_WORDS = "Find.WholeWords";
            const char * const TASK_SEARCH = "Find.Task.Search";
        }
    } //Find

#endif //TEXTFINDCONSTANTS_H
