// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Mercurial::Constants {

enum { debug = 0 };
const char MERCURIAL_PLUGIN[] = "MercurialPlugin";
const char MERCURIALREPO[] = ".hg";
const char MERCURIALDEFAULT[] = "hg";
const char MERCURIAL_CONTEXT[] = "Mercurial Context";

// Changeset identifiers
const char CHANGESETID12[] = " ([a-f0-9]{12}) "; //match 12 hex chars and capture
const char CHANGESETID40[] = " ([a-f0-9]{40}) ";
const char CHANGEIDEXACT12[] = "[a-f0-9]{12}"; //match 12 hex chars
const char CHANGEIDEXACT40[] = "[a-f0-9]{40}";
// match diff header. e.g. +++ b/filename
const char DIFFIDENTIFIER[] = "^(?:diff --git a/|[+-]{3} (?:/dev/null|[ab]/(.+$)))";

// Base editor parameters
const char FILELOG_ID[] = "Mercurial File Log Editor";
const char FILELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Mercurial File Log Editor");
const char LOGAPP[] = "text/vnd.qtcreator.mercurial.log";

const char ANNOTATELOG_ID[] = "Mercurial Annotation Editor";
const char ANNOTATELOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Mercurial Annotation Editor");
const char ANNOTATEAPP[] = "text/vnd.qtcreator.mercurial.annotation";

const char DIFFLOG_ID[] = "Mercurial Diff Editor";
const char DIFFLOG_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Mercurial Diff Editor");
const char DIFFAPP[] = "text/x-patch";

// Submit editor parameters
const char COMMIT_ID[] = "Mercurial Commit Log Editor";
const char COMMIT_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Mercurial Commit Log Editor");
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

} // Mercurial::Constants
