// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Fossil::Constants {

inline constexpr char VCS_ID_FOSSIL[] = "I.Fossil";

inline constexpr char FOSSIL[] = "fossil";
#if defined(Q_OS_WIN) || defined(Q_OS_CYGWIN)
inline constexpr char FOSSILREPO[] = "_FOSSIL_";
#else
inline constexpr char FOSSILREPO[] = ".fslckout";
#endif
inline constexpr char FOSSILDEFAULT[] = "fossil";
inline constexpr char FOSSIL_CONTEXT[] = "Fossil Context";

inline constexpr char FOSSIL_FILE_SUFFIX[] = ".fossil";
inline constexpr char FOSSIL_FILE_FILTER[] = "Fossil Repositories (*.fossil *.fsl);;All Files (*)";

//changeset identifiers
inline constexpr char CHANGESET_ID[] = "([0-9a-f]{5,40})"; // match and capture
inline constexpr char CHANGESET_ID_EXACT[] = "[0-9a-f]{5,40}"; // match

//diff chunk identifiers
inline constexpr char DIFFFILE_ID_EXACT[] = "[+]{3} (.*)\\s*";  // match and capture

//BaseEditorParameters
inline constexpr char FILELOG_ID[] = "Fossil File Log Editor";
inline constexpr char LOGAPP[] = "text/vnd.qtcreator.fossil.log";

inline constexpr char ANNOTATELOG_ID[] = "Fossil Annotation Editor";
inline constexpr char ANNOTATEAPP[] = "text/vnd.qtcreator.fossil.annotation";

inline constexpr char DIFFLOG_ID[] = "Fossil Diff Editor";
inline constexpr char DIFFAPP[] = "text/x-patch";

//SubmitEditorParameters
inline constexpr char COMMIT_ID[] = "Fossil Commit Log Editor";
inline constexpr char COMMITMIMETYPE[] = "text/vnd.qtcreator.fossil.commit";

//menu items
//File menu actions
inline constexpr char ADD[] = "Fossil.AddSingleFile";
inline constexpr char DELETE[] = "Fossil.DeleteSingleFile";
inline constexpr char ANNOTATE[] = "Fossil.Annotate";
inline constexpr char DIFF[] = "Fossil.DiffSingleFile";
inline constexpr char LOG[] = "Fossil.LogSingleFile";
inline constexpr char REVERT[] = "Fossil.RevertSingleFile";
inline constexpr char STATUS[] = "Fossil.Status";

//directory menu Actions
inline constexpr char DIFFMULTI[] = "Fossil.Action.DiffMulti";
inline constexpr char REVERTMULTI[] = "Fossil.Action.RevertAll";
inline constexpr char STATUSMULTI[] = "Fossil.Action.StatusMulti";
inline constexpr char LOGMULTI[] = "Fossil.Action.LogMulti";

//repository menu actions
inline constexpr char PULL[] = "Fossil.Action.Pull";
inline constexpr char PUSH[] = "Fossil.Action.Push";
inline constexpr char UPDATE[] = "Fossil.Action.Update";
inline constexpr char COMMIT[] = "Fossil.Action.Commit";
inline constexpr char CONFIGURE_REPOSITORY[] = "Fossil.Action.Settings";
inline constexpr char CREATE_REPOSITORY[] = "Fossil.Action.CreateRepository";

// File status hint
inline constexpr char FSTATUS_NEW[] = "New";
inline constexpr char FSTATUS_ADDED[] = "Added";
inline constexpr char FSTATUS_ADDED_BY_MERGE[] = "Added by Merge";
inline constexpr char FSTATUS_ADDED_BY_INTEGRATE[] = "Added by Integrate";
inline constexpr char FSTATUS_DELETED[] = "Deleted";
inline constexpr char FSTATUS_EDITED[] = "Edited";
inline constexpr char FSTATUS_UPDATED_BY_MERGE[] = "Updated by Merge";
inline constexpr char FSTATUS_UPDATED_BY_INTEGRATE[] = "Updated by Integrate";
inline constexpr char FSTATUS_RENAMED[] = "Renamed";
inline constexpr char FSTATUS_UNKNOWN[] = "Unknown";

} // Fossil::Constants
