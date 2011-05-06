/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace Mercurial {
namespace Constants {

enum { debug = 0 };
const char * const MECURIALREPO = ".hg";
const char * const MERCURIALDEFAULT = "hg";

//changeset identifiers
const char * const CHANGESETID12 = " ([a-f0-9]{12,12}) "; //match 12 hex chars and capture
const char * const CHANGESETID40 = " ([a-f0-9]{40,40}) ";
const char * const CHANGEIDEXACT12 = "[a-f0-9]{12,12}"; //match 12 hex chars a
const char * const CHANGEIDEXACT40 = "[a-f0-9]{40,40}";
const char * const DIFFIDENTIFIER = "^[-+]{3,3} [ab]{1,1}.*"; // match e.g. +++ b/filename

//BaseEditorParameters
const char * const COMMANDLOG_ID = "Mercurial Command Log Editor";
const char * const COMMANDLOG_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Mercurial Command Log Editor");
const char * const COMMANDLOG = "Mercurial Command Log Editor";
const char * const COMMANDAPP = "application/vnd.nokia.text.scs_mercurial_commandlog";
const char * const COMMANDEXT = "vcsMercurialCommand";

const char * const FILELOG_ID = "Mercurial File Log Editor";
const char * const FILELOG_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Mercurial File Log Editor");
const char * const FILELOG = "Mercurial File Log Editor";
const char * const LOGAPP = "application/vnd.nokia.text.scs_mercurial_log";
const char * const LOGEXT = "vcsMercurialLog";

const char * const ANNOTATELOG_ID = "Mercurial Annotation Editor";
const char * const ANNOTATELOG_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Mercurial Annotation Editor");
const char * const ANNOTATELOG = "Mercurial Annotation Editor";
const char * const ANNOTATEAPP = "application/vnd.nokia.text.scs_mercurial_annotatelog";
const char * const ANNOTATEEXT = "vcsMercurialAnnotate";

const char * const DIFFLOG_ID = "Mercurial Diff Editor";
const char * const DIFFLOG_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Mercurial Diff Editor");
const char * const DIFFLOG = "Mercurial Diff Editor";
const char * const DIFFAPP = "text/x-patch";
const char * const DIFFEXT = "diff";

//SubmitEditorParameters
const char * const COMMIT_ID = "Mercurial Commit Log Editor";
const char * const COMMIT_DISPLAY_NAME = QT_TRANSLATE_NOOP("VCS", "Mercurial Commit Log Editor");
const char * const COMMITMIMETYPE = "application/vnd.nokia.text.scs_mercurial_commitlog";

//menu items
//File menu actions
const char * const ADD = "Mercurial.AddSingleFile";
const char * const DELETE = "Mercurial.DeleteSingleFile";
const char * const ANNOTATE = "Mercurial.Annotate";
const char * const DIFF = "Mercurial.DiffSingleFile";
const char * const LOG = "Mercurial.LogSingleFile";
const char * const REVERT = "Mercurial.RevertSingleFile";
const char * const STATUS = "Mercurial.Status";

//directory menu Actions
const char * const DIFFMULTI = "Mercurial.Action.DiffMulti";
const char * const REVERTMULTI = "Mercurial.Action.RevertMulti";
const char * const STATUSMULTI = "Mercurial.Action.StatusMulti";
const char * const LOGMULTI = "Mercurial.Action.Logmulti";

//repository menu actions
const char * const PULL = "Mercurial.Action.Pull";
const char * const PUSH = "Mercurial.Action.Push";
const char * const UPDATE = "Mercurial.Action.Update";
const char * const IMPORT = "Mercurial.Action.Import";
const char * const INCOMING = "Mercurial.Action.Incoming";
const char * const OUTGOING = "Mercurial.Action.Outgoing";
const char * const COMMIT = "Mercurial.Action.Commit";
const char * const CREATE_REPOSITORY = "Mercurial.Action.CreateRepository";

//submit editor actions
const char * const DIFFEDITOR = "Mercurial.Action.Editor.Diff";

} // namespace Constants
} // namespace mercurial

#endif // CONSTANTS_H
