/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "genericremotelinuxdeploystepfactory.h"

#include "genericdirectuploadstep.h"
#include "remotelinuxcheckforfreediskspacestep.h"
#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxcustomcommanddeploymentstep.h"
#include "tarpackagecreationstep.h"
#include "uploadandinstalltarpackagestep.h"

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

GenericRemoteLinuxDeployStepFactory::GenericRemoteLinuxDeployStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<Core::Id> GenericRemoteLinuxDeployStepFactory::availableCreationIds(BuildStepList *parent) const
{
    QList<Core::Id> ids;
    if (!qobject_cast<RemoteLinuxDeployConfiguration *>(parent->parent()))
        return ids;
    ids << TarPackageCreationStep::stepId() << UploadAndInstallTarPackageStep::stepId()
        << GenericDirectUploadStep::stepId()
        << GenericRemoteLinuxCustomCommandDeploymentStep::stepId()
        << RemoteLinuxCheckForFreeDiskSpaceStep::stepId();
    return ids;
}

QString GenericRemoteLinuxDeployStepFactory::displayNameForId(Core::Id id) const
{
    if (id == TarPackageCreationStep::stepId())
        return TarPackageCreationStep::displayName();
    if (id == UploadAndInstallTarPackageStep::stepId())
        return UploadAndInstallTarPackageStep::displayName();
    if (id == GenericDirectUploadStep::stepId())
        return GenericDirectUploadStep::displayName();
    if (id == GenericRemoteLinuxCustomCommandDeploymentStep::stepId())
        return GenericRemoteLinuxCustomCommandDeploymentStep::stepDisplayName();
    if (id == RemoteLinuxCheckForFreeDiskSpaceStep::stepId())
        return RemoteLinuxCheckForFreeDiskSpaceStep::stepDisplayName();
    return QString();
}

bool GenericRemoteLinuxDeployStepFactory::canCreate(BuildStepList *parent, Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

BuildStep *GenericRemoteLinuxDeployStepFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));

    if (id == TarPackageCreationStep::stepId())
        return new TarPackageCreationStep(parent);
    if (id == UploadAndInstallTarPackageStep::stepId())
        return new UploadAndInstallTarPackageStep(parent);
    if (id == GenericDirectUploadStep::stepId())
        return new GenericDirectUploadStep(parent, GenericDirectUploadStep::stepId());
    if (id == GenericRemoteLinuxCustomCommandDeploymentStep::stepId())
        return new GenericRemoteLinuxCustomCommandDeploymentStep(parent);
    if (id == RemoteLinuxCheckForFreeDiskSpaceStep::stepId())
        return new RemoteLinuxCheckForFreeDiskSpaceStep(parent);
    return 0;
}

bool GenericRemoteLinuxDeployStepFactory::canRestore(BuildStepList *parent,
    const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *GenericRemoteLinuxDeployStepFactory::restore(BuildStepList *parent,
    const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));

    BuildStep * const step = create(parent, idFromMap(map));
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool GenericRemoteLinuxDeployStepFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *GenericRemoteLinuxDeployStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    if (TarPackageCreationStep * const other = qobject_cast<TarPackageCreationStep *>(product))
        return new TarPackageCreationStep(parent, other);
    if (UploadAndInstallTarPackageStep * const other = qobject_cast<UploadAndInstallTarPackageStep*>(product))
        return new UploadAndInstallTarPackageStep(parent, other);
    if (GenericDirectUploadStep * const other = qobject_cast<GenericDirectUploadStep *>(product))
        return new GenericDirectUploadStep(parent, other);
    if (GenericRemoteLinuxCustomCommandDeploymentStep * const other = qobject_cast<GenericRemoteLinuxCustomCommandDeploymentStep *>(product))
        return new GenericRemoteLinuxCustomCommandDeploymentStep(parent, other);
    if (RemoteLinuxCheckForFreeDiskSpaceStep * const other = qobject_cast<RemoteLinuxCheckForFreeDiskSpaceStep *>(product))
        return new RemoteLinuxCheckForFreeDiskSpaceStep(parent, other);
    return 0;
}

} // namespace Internal
} // namespace RemoteLinux
