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

namespace Subversion {
namespace Constants {

const char SUBVERSION_PLUGIN[] = "SubversionPlugin";

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
