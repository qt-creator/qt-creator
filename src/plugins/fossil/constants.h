/****************************************************************************
**
** Copyright (c) 2018 Artur Shepilko
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

#include <QtGlobal>

namespace Fossil {
namespace Constants {

const char VCS_ID_FOSSIL[] = "I.Fossil";

const char FOSSIL[] = "fossil";
#if defined(Q_OS_WIN) || defined(Q_OS_CYGWIN)
const char FOSSILREPO[] = "_FOSSIL_";
#else
const char FOSSILREPO[] = ".fslckout";
#endif
const char FOSSILDEFAULT[] = "fossil";
const char FOSSIL_CONTEXT[] = "Fossil Context";

const char FOSSIL_FILE_SUFFIX[] = ".fossil";
const char FOSSIL_FILE_FILTER[] = "Fossil Repositories (*.fossil *.fsl);;All Files (*)";

//changeset identifiers
const char CHANGESET_ID[] = "([0-9a-f]{5,40})"; // match and capture
const char CHANGESET_ID_EXACT[] = "[0-9a-f]{5,40}"; // match

//diff chunk identifiers
const char DIFFFILE_ID_EXACT[] = "[+]{3} (.*)\\s*";  // match and capture

//BaseEditorParameters
const char FILELOG_ID[] = "Fossil File Log Editor";
const char FILELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Fossil File Log Editor");
const char LOGAPP[] = "text/vnd.qtcreator.fossil.log";

const char ANNOTATELOG_ID[] = "Fossil Annotation Editor";
const char ANNOTATELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Fossil Annotation Editor");
const char ANNOTATEAPP[] = "text/vnd.qtcreator.fossil.annotation";

const char DIFFLOG_ID[] = "Fossil Diff Editor";
const char DIFFLOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Fossil Diff Editor");
const char DIFFAPP[] = "text/x-patch";

//SubmitEditorParameters
const char COMMIT_ID[] = "Fossil Commit Log Editor";
const char COMMIT_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Fossil Commit Log Editor");
const char COMMITMIMETYPE[] = "text/vnd.qtcreator.fossil.commit";

//menu items
//File menu actions
const char ADD[] = "Fossil.AddSingleFile";
const char DELETE[] = "Fossil.DeleteSingleFile";
const char ANNOTATE[] = "Fossil.Annotate";
const char DIFF[] = "Fossil.DiffSingleFile";
const char LOG[] = "Fossil.LogSingleFile";
const char REVERT[] = "Fossil.RevertSingleFile";
const char STATUS[] = "Fossil.Status";

//directory menu Actions
const char DIFFMULTI[] = "Fossil.Action.DiffMulti";
const char REVERTMULTI[] = "Fossil.Action.RevertAll";
const char STATUSMULTI[] = "Fossil.Action.StatusMulti";
const char LOGMULTI[] = "Fossil.Action.LogMulti";

//repository menu actions
const char PULL[] = "Fossil.Action.Pull";
const char PUSH[] = "Fossil.Action.Push";
const char UPDATE[] = "Fossil.Action.Update";
const char COMMIT[] = "Fossil.Action.Commit";
const char CONFIGURE_REPOSITORY[] = "Fossil.Action.Settings";
const char CREATE_REPOSITORY[] = "Fossil.Action.CreateRepository";

// File status hint
const char FSTATUS_ADDED[] = "Added";
const char FSTATUS_ADDED_BY_MERGE[] = "Added by Merge";
const char FSTATUS_ADDED_BY_INTEGRATE[] = "Added by Integrate";
const char FSTATUS_DELETED[] = "Deleted";
const char FSTATUS_EDITED[] = "Edited";
const char FSTATUS_UPDATED_BY_MERGE[] = "Updated by Merge";
const char FSTATUS_UPDATED_BY_INTEGRATE[] = "Updated by Integrate";
const char FSTATUS_RENAMED[] = "Renamed";
const char FSTATUS_UNKNOWN[] = "Unknown";

// Fossil Json Wizards
const char WIZARD_PATH[] = ":/fossil/wizard";

} // namespace Constants
} // namespace Fossil
