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

#ifndef PROJECTEXPLORERMETATYPEDECLARATIONS_H
#define PROJECTEXPLORERMETATYPEDECLARATIONS_H

#include <QtCore/QMetaType>
#include <QtCore/QList>

namespace QWorkbench {
class FileInterface;
}
namespace ProjectExplorer {
class ProjectInterface;
class IProjectManager;
class SessionManager;
class IApplicationOutput;
class BuildParserInterface;
class GlobalConfigManagerInterface;

namespace Internal {
class CommandQObject;
}
}

Q_DECLARE_METATYPE(QWorkbench::FileInterface*)
Q_DECLARE_METATYPE(QList<QWorkbench::FileInterface*>)

Q_DECLARE_METATYPE(ProjectExplorer::Project*)
Q_DECLARE_METATYPE(QList<ProjectExplorer::Project*>)
Q_DECLARE_METATYPE(ProjectExplorer::SessionManager*)
Q_DECLARE_METATYPE(ProjectExplorer::IProjectManager*)
Q_DECLARE_METATYPE(ProjectExplorer::IApplicationOutput*)
Q_DECLARE_METATYPE(ProjectExplorer::Internal::CommandQObject*)
Q_DECLARE_METATYPE(QList<ProjectExplorer::Internal::CommandQObject*>)
Q_DECLARE_METATYPE(ProjectExplorer::BuildParserInterface*)

Q_DECLARE_METATYPE(ProjectExplorer::GlobalConfigManagerInterface*)
#endif // PROJECTEXPLORERMETATYPEDECLARATIONS_H
