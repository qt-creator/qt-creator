/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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
const char C_GIT_COMMAND_LOG_EDITOR[] = "Git Command Log Editor";
const char GIT_LOG_EDITOR_ID[] = "Git File Log Editor";
const char GIT_LOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Git File Log Editor");
const char C_GIT_LOG_EDITOR[] = "Git File Log Editor";
const char GIT_BLAME_EDITOR_ID[] = "Git Annotation Editor";
const char GIT_BLAME_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Git Annotation Editor");
const char C_GIT_BLAME_EDITOR[] = "Git Annotation Editor";
const char GIT_DIFF_EDITOR_ID[] = "Git Diff Editor";
const char GIT_DIFF_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Git Diff Editor");
const char C_GIT_DIFF_EDITOR[] = "Git Diff Editor";

const char C_GITSUBMITEDITOR[]  = "Git Submit Editor";
const char GITSUBMITEDITOR_ID[] = "Git Submit Editor";
const char GITSUBMITEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Git Submit Editor");
const char SUBMIT_CURRENT[] = "Git.SubmitCurrentLog";
const char DIFF_SELECTED[] = "Git.DiffSelectedFilesInLog";
const char SUBMIT_MIMETYPE[] = "application/vnd.nokia.text.git.submit";

} // namespace Constants
} // namespace Git

#endif // GIT_CONSTANTS_H
