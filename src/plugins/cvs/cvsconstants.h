/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CVS_CONSTANTS_H
#define CVS_CONSTANTS_H

namespace CVS {
namespace Constants {

const char * const CVS_SUBMIT_MIMETYPE = "application/vnd.nokia.text.cvs.submit";
const char * const CVSEDITOR  = "CVS Editor";
const char * const CVSEDITOR_KIND  = "CVS Editor";
const char * const CVSCOMMITEDITOR  = "CVS Commit Editor";
const char * const CVSCOMMITEDITOR_ID  = "CVS Commit Editor";
const char * const CVSCOMMITEDITOR_DISPLAY_NAME  = QT_TRANSLATE_NOOP("VCS", "CVS Commit Editor");
const char * const SUBMIT_CURRENT = "CVS.SubmitCurrentLog";
const char * const DIFF_SELECTED = "CVS.DiffSelectedFilesInLog";
enum { debug = 0 };

} // namespace Constants
} // namespace SubVersion

#endif // CVS_CONSTANTS_H
