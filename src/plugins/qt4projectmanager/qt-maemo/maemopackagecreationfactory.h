/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAEMOPACKAGECREATIONFACTORY_H
#define MAEMOPACKAGECREATIONFACTORY_H

#include <projectexplorer/buildstep.h>

namespace Qt4ProjectManager {
namespace Internal {

class MaemoPackageCreationFactory : public ProjectExplorer::IBuildStepFactory
{
public:
    MaemoPackageCreationFactory(QObject *parent);

    virtual QStringList availableCreationIds(ProjectExplorer::BuildConfiguration *parent,
                                             ProjectExplorer::StepType type) const;
    virtual QString displayNameForId(const QString &id) const;

    virtual bool canCreate(ProjectExplorer::BuildConfiguration *parent,
                           ProjectExplorer::StepType type,
                           const QString &id) const;
    virtual ProjectExplorer::BuildStep *
            create(ProjectExplorer::BuildConfiguration *parent,
                   ProjectExplorer::StepType type, const QString &id);

    virtual bool canRestore(ProjectExplorer::BuildConfiguration *parent,
                            ProjectExplorer::StepType type,
                            const QVariantMap &map) const;
    virtual ProjectExplorer::BuildStep *
            restore(ProjectExplorer::BuildConfiguration *parent,
                    ProjectExplorer::StepType type, const QVariantMap &map);

    virtual bool canClone(ProjectExplorer::BuildConfiguration *parent,
                          ProjectExplorer::StepType type,
                          ProjectExplorer::BuildStep *product) const;
    virtual ProjectExplorer::BuildStep *
            clone(ProjectExplorer::BuildConfiguration *parent,
                  ProjectExplorer::StepType type,
                  ProjectExplorer::BuildStep *product);

};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOPACKAGECREATIONFACTORY_H
