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

namespace QmakeProjectManager {
namespace Constants {

// Menus
const char M_CONTEXT[] = "ProFileEditor.ContextMenu";

// Kinds
const char PROJECT_ID[] = "Qt4.Qt4Project";
const char PROFILE_EDITOR_ID[] = "Qt4.proFileEditor";
const char PROFILE_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", ".pro File Editor");
const char PROFILE_MIMETYPE[] = "application/vnd.qt.qmakeprofile";
const char PROINCLUDEFILE_MIMETYPE [] = "application/vnd.qt.qmakeproincludefile";
const char PROFEATUREFILE_MIMETYPE [] = "application/vnd.qt.qmakeprofeaturefile";
const char PROCONFIGURATIONFILE_MIMETYPE [] = "application/vnd.qt.qmakeproconfigurationfile";
const char PROCACHEFILE_MIMETYPE [] = "application/vnd.qt.qmakeprocachefile";
const char PROSTASHFILE_MIMETYPE [] = "application/vnd.qt.qmakeprostashfile";

// Actions
const char RUNQMAKE[] = "Qt4Builder.RunQMake";
const char RUNQMAKECONTEXTMENU[] = "Qt4Builder.RunQMakeContextMenu";
const char BUILDSUBDIR[] = "Qt4Builder.BuildSubDir";
const char REBUILDSUBDIR[] = "Qt4Builder.RebuildSubDir";
const char CLEANSUBDIR[] = "Qt4Builder.CleanSubDir";
const char BUILDFILE[] = "Qt4Builder.BuildFile";
const char BUILDSUBDIRCONTEXTMENU[] = "Qt4Builder.BuildSubDirContextMenu";
const char REBUILDSUBDIRCONTEXTMENU[] = "Qt4Builder.RebuildSubDirContextMenu";
const char CLEANSUBDIRCONTEXTMENU[] = "Qt4Builder.CleanSubDirContextMenu";
const char BUILDFILECONTEXTMENU[] = "Qt4Builder.BuildFileContextMenu";
const char ADDLIBRARY[] = "Qt4.AddLibrary";

// Tasks
const char PROFILE_EVALUATE[] = "Qt4ProjectManager.ProFileEvaluate";

// Project
const char QMAKEPROJECT_ID[] = "Qt4ProjectManager.Qt4Project";

// ICONS
const char ICON_QTQUICK_APP[] = ":/wizards/images/qtquickapp.png";

} // namespace Constants
} // namespace QmakeProjectManager
