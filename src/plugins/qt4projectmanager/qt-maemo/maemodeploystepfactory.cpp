/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "maemodeploystepfactory.h"

#include "maemodeploystep.h"
#include "maemoglobal.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>

#include <QtCore/QCoreApplication>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

MaemoDeployStepFactory::MaemoDeployStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QStringList MaemoDeployStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        && MaemoGlobal::isMaemoTargetId(parent->target()->id())
        && !parent->contains(MaemoDeployStep::Id))
        return QStringList() << MaemoDeployStep::Id;
    return QStringList();
}

QString MaemoDeployStepFactory::displayNameForId(const QString &id) const
{
    if (id == MaemoDeployStep::Id)
        return QCoreApplication::translate("Qt4ProjectManager::Internal::MaemoDeployStepFactory",
                                           "Deploy to device");
    return QString();
}

bool MaemoDeployStepFactory::canCreate(BuildStepList *parent, const QString &id) const
{
    return parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
            && id == QLatin1String(MaemoDeployStep::Id)
            && MaemoGlobal::isMaemoTargetId(parent->target()->id())
            && !parent->contains(MaemoDeployStep::Id);
}

BuildStep *MaemoDeployStepFactory::create(BuildStepList *parent, const QString &id)
{
    Q_ASSERT(canCreate(parent, id));
    return new MaemoDeployStep(parent);
}

bool MaemoDeployStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *MaemoDeployStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    MaemoDeployStep * const step = new MaemoDeployStep(parent);
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool MaemoDeployStepFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *MaemoDeployStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    Q_ASSERT(canClone(parent, product));
    return new MaemoDeployStep(parent, static_cast<MaemoDeployStep*>(product));
}

} // namespace Internal
} // namespace Qt4ProjectManager
