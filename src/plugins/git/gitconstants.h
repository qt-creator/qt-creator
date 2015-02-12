/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef GIT_CONSTANTS_H
#define GIT_CONSTANTS_H

#include <QtGlobal>

namespace Git {
namespace Constants {

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

} // namespace Constants
} // namespace Git

#endif // GIT_CONSTANTS_H
