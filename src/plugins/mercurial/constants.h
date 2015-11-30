/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MERCURIAL_CONSTANTS_H
#define MERCURIAL_CONSTANTS_H

namespace Mercurial {
namespace Constants {

enum { debug = 0 };
const char MERCURIALREPO[] = ".hg";
const char MERCURIALDEFAULT[] = "hg";
const char MERCURIAL_CONTEXT[] = "Mercurial Context";

// Changeset identifiers
const char CHANGESETID12[] = " ([a-f0-9]{12,12}) "; //match 12 hex chars and capture
const char CHANGESETID40[] = " ([a-f0-9]{40,40}) ";
const char CHANGEIDEXACT12[] = "[a-f0-9]{12,12}"; //match 12 hex chars a
const char CHANGEIDEXACT40[] = "[a-f0-9]{40,40}";
// match diff header. e.g. +++ b/filename
const char DIFFIDENTIFIER[] = "^(?:diff --git a/|[+-]{3} (?:/dev/null|[ab]/(.+$)))";

// Base editor parameters
const char FILELOG_ID[] = "Mercurial File Log Editor";
const char FILELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Mercurial File Log Editor");
const char LOGAPP[] = "text/vnd.qtcreator.mercurial.log";

const char ANNOTATELOG_ID[] = "Mercurial Annotation Editor";
const char ANNOTATELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Mercurial Annotation Editor");
const char ANNOTATEAPP[] = "text/vnd.qtcreator.mercurial.annotation";

const char DIFFLOG_ID[] = "Mercurial Diff Editor";
const char DIFFLOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Mercurial Diff Editor");
const char DIFFAPP[] = "text/x-patch";

// Submit editor parameters
const char COMMIT_ID[] = "Mercurial Commit Log Editor";
const char COMMIT_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Mercurial Commit Log Editor");
const char COMMITMIMETYPE[] = "text/vnd.qtcreator.mercurial.commit";

// File menu actions
const char ADD[] = "Mercurial.AddSingleFile";
const char DELETE[] = "Mercurial.DeleteSingleFile";
const char ANNOTATE[] = "Mercurial.Annotate";
const char DIFF[] = "Mercurial.DiffSingleFile";
const char LOG[] = "Mercurial.LogSingleFile";
const char REVERT[] = "Mercurial.RevertSingleFile";
const char STATUS[] = "Mercurial.Status";

// Directory menu actions
const char DIFFMULTI[] = "Mercurial.Action.DiffMulti";
const char REVERTMULTI[] = "Mercurial.Action.RevertMulti";
const char STATUSMULTI[] = "Mercurial.Action.StatusMulti";
const char LOGMULTI[] = "Mercurial.Action.Logmulti";

// Repository menu actions
const char PULL[] = "Mercurial.Action.Pull";
const char PUSH[] = "Mercurial.Action.Push";
const char UPDATE[] = "Mercurial.Action.Update";
const char IMPORT[] = "Mercurial.Action.Import";
const char INCOMING[] = "Mercurial.Action.Incoming";
const char OUTGOING[] = "Mercurial.Action.Outgoing";
const char COMMIT[] = "Mercurial.Action.Commit";
const char CREATE_REPOSITORY[] = "Mercurial.Action.CreateRepository";

// Submit editor actions
const char DIFFEDITOR[] = "Mercurial.Action.Editor.Diff";

} // namespace Constants
} // namespace mercurial

#endif // MERCURIAL_CONSTANTS_H
