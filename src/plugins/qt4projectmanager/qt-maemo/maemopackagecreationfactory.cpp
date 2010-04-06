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

#include "maemopackagecreationfactory.h"

#include "maemopackagecreationstep.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <qt4projectmanagerconstants.h>

using ProjectExplorer::BuildConfiguration;
using ProjectExplorer::StepType;
using ProjectExplorer::BuildStep;

namespace Qt4ProjectManager {
namespace Internal {

MaemoPackageCreationFactory::MaemoPackageCreationFactory(QObject *parent)
    : ProjectExplorer::IBuildStepFactory(parent)
{

}

QStringList MaemoPackageCreationFactory::availableCreationIds(BuildConfiguration *parent,
                StepType type) const
{
    QStringList ids;
    if (type == ProjectExplorer::Build
        && parent->target()->id() == Constants::MAEMO_DEVICE_TARGET_ID)
        ids << MaemoPackageCreationStep::CreatePackageId;
    return ids;
}

QString MaemoPackageCreationFactory::displayNameForId(const QString &id) const
{
    return id == MaemoPackageCreationStep::CreatePackageId
        ? tr("Maemo Package Creation")
        : QString();
}

bool MaemoPackageCreationFactory::canCreate(BuildConfiguration *parent,
         StepType type, const QString &id) const
{
    return type == ProjectExplorer::Build
        && id == MaemoPackageCreationStep::CreatePackageId
        && parent->target()->id() == Constants::MAEMO_DEVICE_TARGET_ID;
}

BuildStep *MaemoPackageCreationFactory::create(BuildConfiguration *parent,
               ProjectExplorer::StepType type, const QString &id)
{
    Q_ASSERT(canCreate(parent, type, id));
    return new MaemoPackageCreationStep(parent);
}

bool MaemoPackageCreationFactory::canRestore(BuildConfiguration *parent,
         StepType type, const QVariantMap &map) const
{
    qDebug("%s: %d", Q_FUNC_INFO, canCreate(parent, type, ProjectExplorer::idFromMap(map)));
    return canCreate(parent, type, ProjectExplorer::idFromMap(map));
}

BuildStep *MaemoPackageCreationFactory::restore(BuildConfiguration *parent,
               StepType type, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, type, map));
    return new MaemoPackageCreationStep(parent);
}

bool MaemoPackageCreationFactory::canClone(BuildConfiguration *parent,
         StepType type, BuildStep *product) const
{
    return canCreate(parent, type, product->id());
}

BuildStep *MaemoPackageCreationFactory::clone(BuildConfiguration *parent,
               StepType type, BuildStep *product)
{
    Q_ASSERT(canClone(parent, type, product));
    return new MaemoPackageCreationStep(parent, static_cast<MaemoPackageCreationStep *>(product));
}

} // namespace Internal
} // namespace Qt4ProjectManager
