/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace Mercurial {
namespace Constants {

enum { debug = 0 };
const char * const MERCURIAL = "mercurial";
const char * const MECURIALREPO = ".hg";
const char * const MERCURIALDEFAULT = "hg";

//options page items
const char * const MERCURIALPATH = "Mercurial_Path";
const char * const MERCURIALUSERNAME = "Mercurial_Username";
const char * const MERCURIALEMAIL = "Mercurial_Email";
const char * const MERCURIALLOGCOUNT = "Mercurial_LogCount";
const char * const MERCURIALTIMEOUT = "Mercurial_Timeout";
const char * const MERCURIALPROMPTSUBMIT = "Mercurial_PromptOnSubmit";

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

//Repository Management
const char * const MERGE = "Mercurial.Action.Merge";
const char * const BRANCH = "Mercurial.Action.Branch";
const char * const HEADS = "Mercurial.Action.Heads";
const char * const PARENTS = "Mercurial.Action.Parents";
const char * const TAGS = "Mercurial.Action.Tags";
const char * const TIP = "Mercurial.Action.TIP";
const char * const PATHS = "Mercurial.Action.Paths";

//Less commonly used menu actions
const char * const CLONE = "Mercurial.Action.Clone";
const char * const INIT = "Mercurial.Action.Init";
const char * const SERVE = "Mercurial.Action.Serve";

//submit editor actions
const char * const COMMITEDITOR = "Mercurial.Action.Editor.Commit";
const char * const DIFFEDITOR = "Mercurial.Action.Editor.Diff";

} // namespace Constants
} // namespace mercurial

#endif // CONSTANTS_H
