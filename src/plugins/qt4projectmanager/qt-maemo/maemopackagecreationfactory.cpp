/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "maemopackagecreationfactory.h"

#include "maemoglobal.h"
#include "maemopackagecreationstep.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4projectmanagerconstants.h>

#include <QtCore/QCoreApplication>

using ProjectExplorer::BuildStepList;
using ProjectExplorer::BuildStep;

namespace Qt4ProjectManager {
namespace Internal {

MaemoPackageCreationFactory::MaemoPackageCreationFactory(QObject *parent)
    : ProjectExplorer::IBuildStepFactory(parent)
{
}

QStringList MaemoPackageCreationFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        && MaemoGlobal::isMaemoTargetId(parent->target()->id())
        && !parent->contains(MaemoPackageCreationStep::CreatePackageId))
        return QStringList() << MaemoPackageCreationStep::CreatePackageId;
    return QStringList();
}

QString MaemoPackageCreationFactory::displayNameForId(const QString &id) const
{
    if (id == MaemoPackageCreationStep::CreatePackageId)
        return QCoreApplication::translate("Qt4ProjectManager::Internal::MaemoPackageCreationFactory",
                                           "Create Debian Package");
    return QString();
}

bool MaemoPackageCreationFactory::canCreate(ProjectExplorer::BuildStepList *parent, const QString &id) const
{
    return parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        && id == QLatin1String(MaemoPackageCreationStep::CreatePackageId)
        && MaemoGlobal::isMaemoTargetId(parent->target()->id())
        && !parent->contains(MaemoPackageCreationStep::CreatePackageId);
}

BuildStep *MaemoPackageCreationFactory::create(ProjectExplorer::BuildStepList *parent, const QString &id)
{
    Q_ASSERT(canCreate(parent, id));
    return new MaemoPackageCreationStep(parent);
}

bool MaemoPackageCreationFactory::canRestore(ProjectExplorer::BuildStepList *parent,
                                             const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

BuildStep *MaemoPackageCreationFactory::restore(ProjectExplorer::BuildStepList *parent,
                                                const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    MaemoPackageCreationStep * const step
        = new MaemoPackageCreationStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool MaemoPackageCreationFactory::canClone(ProjectExplorer::BuildStepList *parent,
                                           ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *MaemoPackageCreationFactory::clone(ProjectExplorer::BuildStepList *parent,
                                              ProjectExplorer::BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new MaemoPackageCreationStep(parent, static_cast<MaemoPackageCreationStep *>(product));
}

} // namespace Internal
} // namespace Qt4ProjectManager
