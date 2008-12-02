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
