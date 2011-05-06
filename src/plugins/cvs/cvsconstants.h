/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CVS_CONSTANTS_H
#define CVS_CONSTANTS_H

namespace CVS {
namespace Constants {

const char * const CVS_SUBMIT_MIMETYPE = "application/vnd.nokia.text.cvs.submit";
const char * const CVSCOMMITEDITOR  = "CVS Commit Editor";
const char * const CVSCOMMITEDITOR_ID  = "CVS Commit Editor";
const char * const CVSCOMMITEDITOR_DISPLAY_NAME  = QT_TRANSLATE_NOOP("VCS", "CVS Commit Editor");
const char * const SUBMIT_CURRENT = "CVS.SubmitCurrentLog";
const char * const DIFF_SELECTED = "CVS.DiffSelectedFilesInLog";
enum { debug = 0 };

} // namespace Constants
} // namespace SubVersion

#endif // CVS_CONSTANTS_H
