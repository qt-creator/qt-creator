/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLPROJECTCONSTANTS_H
#define QMLPROJECTCONSTANTS_H

namespace QmlProjectManager {
namespace Constants {

const char *const PROJECTCONTEXT     = "QmlProject.ProjectContext";
const char *const LANG_QML           = "QML";
const char *const QMLMIMETYPE        = "application/x-qmlproject";

// contexts
const char *const C_FILESEDITOR      = ".files Editor";

// kinds
const char *const PROJECT_KIND       = "QML";

const char *const FILES_EDITOR_ID    = "Qt4.QmlProjectEditor";
const char *const FILES_EDITOR_DISPLAY_NAME = QT_TRANSLATE_NOOP("OpenWith::Editors", ".qmlproject Editor");
const char *const FILES_MIMETYPE     = QMLMIMETYPE;

} // namespace Constants
} // namespace QmlProjectManager

#endif // QMLPROJECTCONSTANTS_H
