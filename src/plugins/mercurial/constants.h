// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Mercurial::Constants {

enum { debug = 0 };
inline constexpr char MERCURIAL_PLUGIN[] = "MercurialPlugin";
inline constexpr char MERCURIALREPO[] = ".hg";
inline constexpr char MERCURIALDEFAULT[] = "hg";
inline constexpr char MERCURIAL_CONTEXT[] = "Mercurial Context";

// Changeset identifiers
inline constexpr char CHANGESETID12[] = " ([a-f0-9]{12}) "; //match 12 hex chars and capture
inline constexpr char CHANGESETID40[] = " ([a-f0-9]{40}) ";
inline constexpr char CHANGEIDEXACT12[] = "[a-f0-9]{12}"; //match 12 hex chars
inline constexpr char CHANGEIDEXACT40[] = "[a-f0-9]{40}";
// match diff header. e.g. +++ b/filename
inline constexpr char DIFFIDENTIFIER[] = "^(?:diff --git a/|[+-]{3} (?:/dev/null|[ab]/(.+$)))";

// Base editor parameters
inline constexpr char FILELOG_ID[] = "Mercurial File Log Editor";
inline constexpr char LOGAPP[] = "text/vnd.qtcreator.mercurial.log";

inline constexpr char ANNOTATELOG_ID[] = "Mercurial Annotation Editor";
inline constexpr char ANNOTATEAPP[] = "text/vnd.qtcreator.mercurial.annotation";

inline constexpr char DIFFLOG_ID[] = "Mercurial Diff Editor";
inline constexpr char DIFFAPP[] = "text/x-patch";

// Submit editor parameters
inline constexpr char COMMIT_ID[] = "Mercurial Commit Log Editor";
inline constexpr char COMMITMIMETYPE[] = "text/vnd.qtcreator.mercurial.commit";

// File menu actions
inline constexpr char ADD[] = "Mercurial.AddSingleFile";
inline constexpr char DELETE[] = "Mercurial.DeleteSingleFile";
inline constexpr char ANNOTATE[] = "Mercurial.Annotate";
inline constexpr char DIFF[] = "Mercurial.DiffSingleFile";
inline constexpr char LOG[] = "Mercurial.LogSingleFile";
inline constexpr char REVERT[] = "Mercurial.RevertSingleFile";
inline constexpr char STATUS[] = "Mercurial.Status";

// Directory menu actions
inline constexpr char DIFFMULTI[] = "Mercurial.Action.DiffMulti";
inline constexpr char REVERTMULTI[] = "Mercurial.Action.RevertMulti";
inline constexpr char STATUSMULTI[] = "Mercurial.Action.StatusMulti";
inline constexpr char LOGMULTI[] = "Mercurial.Action.Logmulti";

// Repository menu actions
inline constexpr char PULL[] = "Mercurial.Action.Pull";
inline constexpr char PUSH[] = "Mercurial.Action.Push";
inline constexpr char UPDATE[] = "Mercurial.Action.Update";
inline constexpr char IMPORT[] = "Mercurial.Action.Import";
inline constexpr char INCOMING[] = "Mercurial.Action.Incoming";
inline constexpr char OUTGOING[] = "Mercurial.Action.Outgoing";
inline constexpr char COMMIT[] = "Mercurial.Action.Commit";
inline constexpr char CREATE_REPOSITORY[] = "Mercurial.Action.CreateRepository";

} // Mercurial::Constants
