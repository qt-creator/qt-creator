// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Git::Constants {

inline constexpr char GIT_PLUGIN[] = "GitPlugin";

inline constexpr char GIT_SVN_LOG_EDITOR_ID[] = "Git SVN Log Editor";
inline constexpr char GIT_LOG_EDITOR_ID[] = "Git Log Editor";
inline constexpr char GIT_REFLOG_EDITOR_ID[] = "Git Reflog Editor";
inline constexpr char GIT_BLAME_EDITOR_ID[] = "Git Annotation Editor";
inline constexpr char GIT_COMMIT_TEXT_EDITOR_ID[] = "Git Commit Editor";
inline constexpr char GIT_REBASE_EDITOR_ID[] = "Git Rebase Editor";
inline constexpr char GIT_BRANCH_VIEW_ID[] = "Git Branches";

inline constexpr char GIT_CONTEXT[] = "Git Context";
inline constexpr char GITSUBMITEDITOR_ID[] = "Git Submit Editor";
inline constexpr char SUBMIT_MIMETYPE[] = "text/vnd.qtcreator.git.submit";
inline constexpr char C_GITEDITORID[]  = "Git Editor";

inline constexpr int OBSOLETE_COMMIT_AGE_IN_DAYS = 90;
inline constexpr int MAX_OBSOLETE_COMMITS_TO_DISPLAY = 5;

inline constexpr char EXPAND_BRANCHES[] = "Branches: <Expand>";
const char DEFAULT_COMMENT_CHAR = '#';

inline constexpr char TEXT_MARK_CATEGORY_BLAME[] = "Git.Mark.Blame";

} // Git::Constants
