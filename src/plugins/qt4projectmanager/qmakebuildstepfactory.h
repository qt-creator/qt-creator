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

#ifndef QMAKEBUILDSTEPFACTORY_H
#define QMAKEBUILDSTEPFACTORY_H

#include <projectexplorer/buildstep.h>

#include <QString>

namespace ProjectExplorer {
class BuildStep;
class IBuildStepFactory;
class Project;
}

namespace Qt4ProjectManager {
namespace Internal {

class QMakeBuildStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT
public:
    QMakeBuildStepFactory();
    virtual ~QMakeBuildStepFactory();
    bool canCreate(const QString & name) const;
    ProjectExplorer::BuildStep * create(ProjectExplorer::Project * pro, const QString & name) const;
    QStringList canCreateForProject(ProjectExplorer::Project *pro) const;
    QString displayNameForName(const QString &name) const;
};

class MakeBuildStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT
public:
    MakeBuildStepFactory();
    virtual ~MakeBuildStepFactory();
    bool canCreate(const QString & name) const;
    ProjectExplorer::BuildStep * create(ProjectExplorer::Project * pro, const QString & name) const;
    QStringList canCreateForProject(ProjectExplorer::Project *pro) const;
    QString displayNameForName(const QString &name) const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QMAKEBUILDSTEPFACTORY_H
