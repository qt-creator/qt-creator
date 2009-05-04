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

#ifndef QMLPROJECTCONSTANTS_H
#define QMLPROJECTCONSTANTS_H

namespace QmlProjectManager {
namespace Constants {

const char *const PROJECTCONTEXT     = "QmlProject.ProjectContext";
const char *const QMLMIMETYPE    = "text/x-qml-project"; // ### FIXME
const char *const MAKESTEP           = "QmlProjectManager.MakeStep";

// contexts
const char *const C_FILESEDITOR      = ".files Editor";

// kinds
const char *const PROJECT_KIND       = "Qml";

const char *const FILES_EDITOR       = ".files Editor";
const char *const FILES_MIMETYPE     = "application/vnd.nokia.qt.qml.files";

const char *const INCLUDES_EDITOR    = ".includes Editor";
const char *const INCLUDES_MIMETYPE  = "application/vnd.nokia.qt.qml.includes";

const char *const CONFIG_EDITOR      = ".includes Editor";
const char *const CONFIG_MIMETYPE    = "application/vnd.nokia.qt.qml.config";

} // namespace Constants
} // namespace QmlProjectManager

#endif // QMLPROJECTCONSTANTS_H
