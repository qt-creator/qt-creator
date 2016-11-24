/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

namespace Git {
namespace Constants {

const char GIT_PLUGIN[] = "GitPlugin";

const char GIT_COMMAND_LOG_EDITOR_ID[] = "Git Command Log Editor";
const char GIT_COMMAND_LOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Git Command Log Editor");
const char GIT_LOG_EDITOR_ID[] = "Git File Log Editor";
const char GIT_LOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Git File Log Editor");
const char GIT_BLAME_EDITOR_ID[] = "Git Annotation Editor";
const char GIT_BLAME_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Git Annotation Editor");
const char GIT_COMMIT_TEXT_EDITOR_ID[] = "Git Commit Editor";
const char GIT_COMMIT_TEXT_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Git Commit Editor");
const char GIT_REBASE_EDITOR_ID[] = "Git Rebase Editor";
const char GIT_REBASE_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Git Rebase Editor");

const char GIT_CONTEXT[] = "Git Context";
const char GITSUBMITEDITOR_ID[] = "Git Submit Editor";
const char GITSUBMITEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Git Submit Editor");
const char SUBMIT_CURRENT[] = "Git.SubmitCurrentLog";
const char DIFF_SELECTED[] = "Git.DiffSelectedFilesInLog";
const char SUBMIT_MIMETYPE[] = "text/vnd.qtcreator.git.submit";
const char C_GITEDITORID[]  = "Git Editor";

const int OBSOLETE_COMMIT_AGE_IN_DAYS = 90;

} // namespace Constants
} // namespace Git
