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

#ifndef CMAKEPROJECTCONSTANTS_H
#define CMAKEPROJECTCONSTANTS_H

namespace CMakeProjectManager {
namespace Constants {

const char * const PROJECTCONTEXT = "CMakeProject.ProjectContext";
const char * const CMAKEMIMETYPE  = "text/x-cmake"; // TOOD check that this is correct
const char * const CMAKE_EDITOR_ID = "CMakeProject.CMakeEditor";
const char * const CMAKE_EDITOR_DISPLAY_NAME = "CMake Editor";
const char * const C_CMAKEEDITOR = "CMakeProject.Context.CMakeEditor";
const char * const RUNCMAKE            = "CMakeProject.RunCMake";
const char * const RUNCMAKECONTEXTMENU = "CMakeProject.RunCMakeContextMenu";

// Project
const char * const CMAKEPROJECT_ID = "CMakeProjectManager.CMakeProject";

} // namespace Constants
} // namespace CMakeProjectManager

#endif // CMAKEPROJECTCONSTANTS_H
