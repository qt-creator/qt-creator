// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Git {
namespace Constants {

const char GIT_PLUGIN[] = "GitPlugin";

const char GIT_SVN_LOG_EDITOR_ID[] = "Git SVN Log Editor";
const char GIT_SVN_LOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Git SVN Log Editor");
const char GIT_LOG_EDITOR_ID[] = "Git Log Editor";
const char GIT_LOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Git Log Editor");
const char GIT_REFLOG_EDITOR_ID[] = "Git Reflog Editor";
const char GIT_REFLOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Git Reflog Editor");
const char GIT_BLAME_EDITOR_ID[] = "Git Annotation Editor";
const char GIT_BLAME_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Git Annotation Editor");
const char GIT_COMMIT_TEXT_EDITOR_ID[] = "Git Commit Editor";
const char GIT_COMMIT_TEXT_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Git Commit Editor");
const char GIT_REBASE_EDITOR_ID[] = "Git Rebase Editor";
const char GIT_REBASE_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Git Rebase Editor");
const char GIT_BRANCH_VIEW_ID[] = "Git Branches";

const char GIT_CONTEXT[] = "Git Context";
const char GITSUBMITEDITOR_ID[] = "Git Submit Editor";
const char GITSUBMITEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::VcsBase", "Git Submit Editor");
const char SUBMIT_MIMETYPE[] = "text/vnd.qtcreator.git.submit";
const char C_GITEDITORID[]  = "Git Editor";

const int OBSOLETE_COMMIT_AGE_IN_DAYS = 90;
const int MAX_OBSOLETE_COMMITS_TO_DISPLAY = 5;

const char EXPAND_BRANCHES[] = "Branches: <Expand>";
const char DEFAULT_COMMENT_CHAR = '#';

const char TEXT_MARK_CATEGORY_BLAME[] = "Git.Mark.Blame";

} // namespace Constants
} // namespace Git
