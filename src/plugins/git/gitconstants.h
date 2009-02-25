/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef GIT_CONSTANTS_H
#define GIT_CONSTANTS_H

namespace Git {
namespace Constants {

const char * const GIT_COMMAND_LOG_EDITOR_KIND = "Git Command Log Editor";
const char * const GIT_LOG_EDITOR_KIND = "Git File Log Editor";
const char * const GIT_BLAME_EDITOR_KIND = "Git Annotation Editor";
const char * const GIT_DIFF_EDITOR_KIND = "Git Diff Editor";

const char * const C_GITSUBMITEDITOR  = "Git Submit Editor";
const char * const GITSUBMITEDITOR_KIND = "Git Submit Editor";
const char * const SUBMIT_CURRENT = "Git.SubmitCurrentLog";
const char * const DIFF_SELECTED = "Git.DiffSelectedFilesInLog";
const char * const SUBMIT_MIMETYPE = "application/vnd.nokia.text.git.submit";
const char * const GIT_BINARY = "git";

const char * const DIFF_FILE_INDICATOR = "--- ";
enum { debug = 0 };

} // namespace Constants
} // namespace Git

#endif // GIT_CONSTANTS_H
