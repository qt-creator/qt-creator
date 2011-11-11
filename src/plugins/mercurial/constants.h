/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace Mercurial {
namespace Constants {

enum { debug = 0 };
const char MECURIALREPO[] = ".hg";
const char MERCURIALDEFAULT[] = "hg";

//changeset identifiers
const char CHANGESETID12[] = " ([a-f0-9]{12,12}) "; //match 12 hex chars and capture
const char CHANGESETID40[] = " ([a-f0-9]{40,40}) ";
const char CHANGEIDEXACT12[] = "[a-f0-9]{12,12}"; //match 12 hex chars a
const char CHANGEIDEXACT40[] = "[a-f0-9]{40,40}";
const char DIFFIDENTIFIER[] = "^[-+]{3,3} [ab]{1,1}.*"; // match e.g. +++ b/filename

//BaseEditorParameters
const char COMMANDLOG_ID[] = "Mercurial Command Log Editor";
const char COMMANDLOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Mercurial Command Log Editor");
const char COMMANDLOG[] = "Mercurial Command Log Editor";
const char COMMANDAPP[] = "application/vnd.nokia.text.scs_mercurial_commandlog";
const char COMMANDEXT[] = "vcsMercurialCommand";

const char FILELOG_ID[] = "Mercurial File Log Editor";
const char FILELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Mercurial File Log Editor");
const char FILELOG[] = "Mercurial File Log Editor";
const char LOGAPP[] = "application/vnd.nokia.text.scs_mercurial_log";
const char LOGEXT[] = "vcsMercurialLog";

const char ANNOTATELOG_ID[] = "Mercurial Annotation Editor";
const char ANNOTATELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Mercurial Annotation Editor");
const char ANNOTATELOG[] = "Mercurial Annotation Editor";
const char ANNOTATEAPP[] = "application/vnd.nokia.text.scs_mercurial_annotatelog";
const char ANNOTATEEXT[] = "vcsMercurialAnnotate";

const char DIFFLOG_ID[] = "Mercurial Diff Editor";
const char DIFFLOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Mercurial Diff Editor");
const char DIFFLOG[] = "Mercurial Diff Editor";
const char DIFFAPP[] = "text/x-patch";
const char DIFFEXT[] = "diff";

//SubmitEditorParameters
const char COMMIT_ID[] = "Mercurial Commit Log Editor";
const char COMMIT_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Mercurial Commit Log Editor");
const char COMMITMIMETYPE[] = "application/vnd.nokia.text.scs_mercurial_commitlog";

//menu items
//File menu actions
const char ADD[] = "Mercurial.AddSingleFile";
const char DELETE[] = "Mercurial.DeleteSingleFile";
const char ANNOTATE[] = "Mercurial.Annotate";
const char DIFF[] = "Mercurial.DiffSingleFile";
const char LOG[] = "Mercurial.LogSingleFile";
const char REVERT[] = "Mercurial.RevertSingleFile";
const char STATUS[] = "Mercurial.Status";

//directory menu Actions
const char DIFFMULTI[] = "Mercurial.Action.DiffMulti";
const char REVERTMULTI[] = "Mercurial.Action.RevertMulti";
const char STATUSMULTI[] = "Mercurial.Action.StatusMulti";
const char LOGMULTI[] = "Mercurial.Action.Logmulti";

//repository menu actions
const char PULL[] = "Mercurial.Action.Pull";
const char PUSH[] = "Mercurial.Action.Push";
const char UPDATE[] = "Mercurial.Action.Update";
const char IMPORT[] = "Mercurial.Action.Import";
const char INCOMING[] = "Mercurial.Action.Incoming";
const char OUTGOING[] = "Mercurial.Action.Outgoing";
const char COMMIT[] = "Mercurial.Action.Commit";
const char CREATE_REPOSITORY[] = "Mercurial.Action.CreateRepository";

//submit editor actions
const char DIFFEDITOR[] = "Mercurial.Action.Editor.Diff";

} // namespace Constants
} // namespace mercurial

#endif // CONSTANTS_H
