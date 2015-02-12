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

#ifndef SUBVERSION_CONSTANTS_H
#define SUBVERSION_CONSTANTS_H

#include <QtGlobal>

namespace Subversion {
namespace Constants {

const char NON_INTERACTIVE_OPTION[] = "--non-interactive";
enum { debug = 0 };

const char SUBVERSION_CONTEXT[]        = "Subversion Context";

const char SUBVERSION_COMMIT_EDITOR_ID[]  = "Subversion Commit Editor";
const char SUBVERSION_COMMIT_EDITOR_DISPLAY_NAME[]  = QT_TRANSLATE_NOOP("VCS", "Subversion Commit Editor");
const char SUBVERSION_SUBMIT_MIMETYPE[] = "text/vnd.qtcreator.svn.submit";

const char SUBVERSION_LOG_EDITOR_ID[] = "Subversion File Log Editor";
const char SUBVERSION_LOG_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Subversion File Log Editor");
const char SUBVERSION_LOG_MIMETYPE[] = "text/vnd.qtcreator.svn.log";

const char SUBVERSION_BLAME_EDITOR_ID[] = "Subversion Annotation Editor";
const char SUBVERSION_BLAME_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "Subversion Annotation Editor");
const char SUBVERSION_BLAME_MIMETYPE[] = "text/vnd.qtcreator.svn.annotation";

const char SUBMIT_CURRENT[] = "Subversion.SubmitCurrentLog";
const char DIFF_SELECTED[] = "Subversion.DiffSelectedFilesInLog";

} // namespace Constants
} // namespace Subversion

#endif // SUBVERSION_CONSTANTS_H
