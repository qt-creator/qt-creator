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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef GENERICPROJECTCONSTANTS_H
#define GENERICPROJECTCONSTANTS_H

namespace GenericProjectManager {
namespace Constants {

const char *const PROJECTCONTEXT     = "GenericProject.ProjectContext";
const char *const GENERICMIMETYPE    = "text/x-generic-project"; // ### FIXME
const char *const MAKESTEP           = "GenericProjectManager.MakeStep";

// contexts
const char *const C_FILESEDITOR      = ".files Editor";

// kinds
const char *const PROJECT_KIND       = "Generic";

const char *const FILES_EDITOR_ID    = "QT4.FilesEditor";
const char *const FILES_EDITOR_DISPLAY_NAME = QT_TRANSLATE_NOOP("OpenWith::Editors", ".files Editor");

const char *const FILES_MIMETYPE     = "application/vnd.nokia.qt.generic.files";

const char *const INCLUDES_EDITOR    = ".includes Editor";
const char *const INCLUDES_MIMETYPE  = "application/vnd.nokia.qt.generic.includes";

const char *const CONFIG_EDITOR      = ".config Editor";
const char *const CONFIG_MIMETYPE    = "application/vnd.nokia.qt.generic.config";

// Project
const char *const GENERICPROJECT_ID  = "GenericProjectManager.GenericProject";

} // namespace Constants
} // namespace GenericProjectManager

#endif // GENERICPROJECTCONSTANTS_H
