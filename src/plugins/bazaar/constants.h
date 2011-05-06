/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef BAZAARCONSTANTS_H
#define BAZAARCONSTANTS_H

namespace Bazaar {
namespace Constants {

const char * const BAZAAR = "bazaar";
const char * const BAZAARREPO = ".bzr";
const char * const BAZAARDEFAULT = "bzr";

//changeset identifiers
const char * const CHANGESET_ID = "([0-9]+)"; // match and capture
const char * const CHANGESET_ID_EXACT = "[0-9]+"; // match

//BaseEditorParameters
const char * const COMMANDLOG_ID = "Bazaar Command Log Editor";
const char * const COMMANDLOG_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Bazaar Command Log Editor");
const char * const COMMANDLOG = "Bazaar Command Log Editor";
const char * const COMMANDAPP = "application/vnd.nokia.text.scs_bazaar_commandlog";
const char * const COMMANDEXT = "vcsBazaarCommand";

const char * const FILELOG_ID = "Bazaar File Log Editor";
const char * const FILELOG_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Bazaar File Log Editor");
const char * const FILELOG = "Bazaar File Log Editor";
const char * const LOGAPP = "application/vnd.nokia.text.scs_bazaar_log";
const char * const LOGEXT = "vcsBazaarLog";

const char * const ANNOTATELOG_ID = "Bazaar Annotation Editor";
const char * const ANNOTATELOG_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Bazaar Annotation Editor");
const char * const ANNOTATELOG = "Bazaar Annotation Editor";
const char * const ANNOTATEAPP = "application/vnd.nokia.text.scs_bazaar_annotatelog";
const char * const ANNOTATEEXT = "vcsBazaarAnnotate";

const char * const DIFFLOG_ID = "Bazaar Diff Editor";
const char * const DIFFLOG_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Bazaar Diff Editor");
const char * const DIFFLOG = "Bazaar Diff Editor";
const char * const DIFFAPP = "text/x-patch";
const char * const DIFFEXT = "diff";

//SubmitEditorParameters
const char * const COMMIT_ID = "Bazaar Commit Log Editor";
const char * const COMMIT_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Bazaar Commit Log Editor");
const char * const COMMITMIMETYPE = "application/vnd.nokia.text.scs_bazaar_commitlog";

//menu items
//File menu actions
const char * const ADD = "Bazaar.AddSingleFile";
const char * const DELETE = "Bazaar.DeleteSingleFile";
const char * const ANNOTATE = "Bazaar.Annotate";
const char * const DIFF = "Bazaar.DiffSingleFile";
const char * const LOG = "Bazaar.LogSingleFile";
const char * const REVERT = "Bazaar.RevertSingleFile";
const char * const STATUS = "Bazaar.Status";

//directory menu Actions
const char * const DIFFMULTI = "Bazaar.Action.DiffMulti";
const char * const REVERTMULTI = "Bazaar.Action.RevertALL";
const char * const STATUSMULTI = "Bazaar.Action.StatusMulti";
const char * const LOGMULTI = "Bazaar.Action.Logmulti";

//repository menu actions
const char * const PULL = "Bazaar.Action.Pull";
const char * const PUSH = "Bazaar.Action.Push";
const char * const UPDATE = "Bazaar.Action.Update";
const char * const COMMIT = "Bazaar.Action.Commit";
const char * const CREATE_REPOSITORY = "Bazaar.Action.CreateRepository";

//submit editor actions
const char * const DIFFEDITOR = "Bazaar.Action.Editor.Diff";

} // namespace Constants
} // namespace Bazaar

#endif // BAZAARCONSTANTS_H
