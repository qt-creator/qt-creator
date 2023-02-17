// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace QmakeProjectManager {
namespace Constants {

// Menus
const char M_CONTEXT[] = "ProFileEditor.ContextMenu";

// Kinds
const char PROFILE_EDITOR_ID[] = "Qt4.proFileEditor";
const char PROFILE_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::Core", ".pro File Editor");
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

const char QMAKE_BC_ID[] = "Qt4ProjectManager.Qt4BuildConfiguration";
const char MAKESTEP_BS_ID[] = "Qt4ProjectManager.MakeStep";
const char QMAKE_BS_ID[] = "QtProjectManager.QMakeBuildStep";

// Kit
const char KIT_INFORMATION_ID[] = "QtPM4.mkSpecInformation";

} // namespace Constants
} // namespace QmakeProjectManager
