/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#ifndef QT4PROJECTMANAGERCONSTANTS_H
#define QT4PROJECTMANAGERCONSTANTS_H

namespace Qt4ProjectManager {
namespace Constants {

//contexts
const char * const C_PROFILEEDITOR  = ".pro File Editor";
const char * const C_PROFILEEDITOR_PANEL  = ".pro File Editor (embedded)";

//settings pages
const char * const QT_CATEGORY            = "Qt4";
const char * const QTVERSION_PAGE         = "Qt Versions";
const char * const BUILD_ENVIRONMENT_PAGE = "Build Environments";

// kinds
const char * const PROJECT_KIND      = "Qt4";
const char * const PROFILE_EDITOR    = ".pro File Editor";
const char * const PROFILE_MIMETYPE  = "application/vnd.nokia.qt.qmakeprofile";
const char * const PROINCLUDEFILE_MIMETYPE  = "application/vnd.nokia.qt.qmakeproincludefile";
const char * const CPP_SOURCE_MIMETYPE = "text/x-c++src";
const char * const CPP_HEADER_MIMETYPE = "text/x-c++hdr";
const char * const FORM_MIMETYPE = "application/x-designer";

//actions
const char * const NEWMENU             = "Qt4.NewMenu";
const char * const PROJECT_NEWMENU_SEPARATOR      = "Qt4.NewMenuSeparator";
const char * const SUBPROJECT_NEWMENU_SEPARATOR   = "Qt4.SubProjectNewMenuSeparator";
const char * const ADDTOPROJECT        = "Qt4.AddToProject";
const char * const RUNQMAKE            = "Qt4Builder.RunQMake";
const char * const RUNQMAKECONTEXTMENU = "Qt4Builder.RunQMakeContextMenu";

//configurations
const char * const CONFIG_DEBUG     = "debug";
const char * const CONFIG_RELEASE   = "release";

//global configurations
const char * const GC_BUILDCONFIG   = "Qt4.BuildConfig";
const char * const GC_QTVERSION     = "Qt4.QtVersion";
const char * const GC_COMPILER      = "Qt4.Compiler";

// qmakestep
const char * const QMAKESTEP  = "trolltech.qt4projectmanager.qmake";
const char * const MAKESTEP   = "trolltech.qt4projectmanager.make";
const char * const GDBMACROSBUILDSTEP = "trolltech.qt4projectmanager.gdbmaros";
const char * const QT4RUNSTEP = "trolltech.qt4projectmanager.qt4runstep";

// build parsers
const char * const BUILD_PARSER_MSVC    = "BuildParser.MSVC";
const char * const BUILD_PARSER_GCC     = "BuildParser.Gcc";

// views
const char * const VIEW_DETAILED        = "Qt4.View.Detailed";
const char * const VIEW_PROFILESONLY    = "Qt4.View.ProjectHierarchy";
} // namespace Constants
} // namespace Qt4ProjectManager

#endif // QT4PROJECTMANAGERCONSTANTS_H
