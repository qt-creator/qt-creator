/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
const char * const SUBMIT_CURRENT = "Nokia.Git.SubmitCurrentLog";
const char * const DIFF_SELECTED = "Nokia.Git.DiffSelectedFilesInLog";
const char * const SUBMIT_MIMETYPE = "application/vnd.nokia.text.git.submit";

// TODO: For the moment, trust p4 is loaded...
const char * const ICON_SUBMIT = ":/trolltech.perforce/images/submit.png";
const char * const ICON_DIFF = ":/trolltech.perforce/images/diff.png";

const char * const DIFF_FILE_INDICATOR = "--- ";
enum { debug = 0 };

} // namespace Constants
} // namespace Git

#endif // GIT_CONSTANTS_H
