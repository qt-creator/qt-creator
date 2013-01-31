/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "maemodeploystepfactory.h"

#include "maemoconstants.h"
#include "maddeqemustartstep.h"
#include "maddeuploadandinstallpackagesteps.h"
#include "maemodeploybymountsteps.h"
#include "maemoinstalltosysrootstep.h"
#include "qt4maemodeployconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <remotelinux/genericdirectuploadstep.h>
#include <remotelinux/remotelinuxcheckforfreediskspacestep.h>
#include <remotelinux/uploadandinstalltarpackagestep.h>

#include <QCoreApplication>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

const char OldMaemoDeployStepId[] = "Qt4ProjectManager.MaemoDeployStep";

MaemoDeployStepFactory::MaemoDeployStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<Core::Id> MaemoDeployStepFactory::availableCreationIds(BuildStepList *parent) const
{
    QList<Core::Id> ids;
    if (!qobject_cast<Qt4MaemoDeployConfiguration *>(parent->parent()))
        return ids;

    QString platform;
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(parent->target()->kit());
    if (version)
        platform = version->platformName();

    if (platform == QLatin1String(QtSupport::Constants::MAEMO_FREMANTLE_PLATFORM)) {
        ids << MaemoMakeInstallToSysrootStep::Id
            << MaemoInstallDebianPackageToSysrootStep::Id
            << MaemoUploadAndInstallPackageStep::stepId()
            << MaemoInstallPackageViaMountStep::stepId()
            << MaemoCopyFilesViaMountStep::stepId()
            << MaddeQemuStartStep::stepId();
    } else if (platform == QLatin1String(QtSupport::Constants::MEEGO_HARMATTAN_PLATFORM)) {
        ids << MaemoMakeInstallToSysrootStep::Id
            << MaemoInstallDebianPackageToSysrootStep::Id
            << MaemoUploadAndInstallPackageStep::stepId()
            << GenericDirectUploadStep::stepId()
            << MaddeQemuStartStep::stepId();
    }

    return ids;
}

QString MaemoDeployStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == MaemoInstallPackageViaMountStep::stepId())
        return MaemoInstallPackageViaMountStep::displayName();
    if (id == MaemoCopyFilesViaMountStep::stepId())
        return MaemoCopyFilesViaMountStep::displayName();
    if (id == MaemoUploadAndInstallPackageStep::stepId())
        return MaemoUploadAndInstallPackageStep::displayName();
    if (id == MaemoInstallDebianPackageToSysrootStep::Id)
        return MaemoInstallDebianPackageToSysrootStep::displayName();
    if (id == MaemoCopyToSysrootStep::Id)
        return MaemoCopyToSysrootStep::displayName();
    if (id == MaemoMakeInstallToSysrootStep::Id)
        return MaemoMakeInstallToSysrootStep::displayName();
    if (id == GenericDirectUploadStep::stepId())
        return GenericDirectUploadStep::displayName();
    if (id == RemoteLinuxCheckForFreeDiskSpaceStep::stepId())
        return RemoteLinuxCheckForFreeDiskSpaceStep::stepDisplayName();
    if (id == MaddeQemuStartStep::stepId())
        return MaddeQemuStartStep::stepDisplayName();
    return QString();
}

bool MaemoDeployStepFactory::canCreate(BuildStepList *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id) && !parent->contains(id);
}

BuildStep *MaemoDeployStepFactory::create(BuildStepList *parent, const Core::Id id)
{
    Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(parent->target()->kit());

    if (id == MaemoInstallDebianPackageToSysrootStep::Id)
        return new MaemoInstallDebianPackageToSysrootStep(parent);
    if (id == MaemoCopyToSysrootStep::Id)
        return new MaemoCopyToSysrootStep(parent);
    if (id == MaemoMakeInstallToSysrootStep::Id)
        return new MaemoMakeInstallToSysrootStep(parent);
    if (id == MaemoInstallPackageViaMountStep::stepId()
        || (id == OldMaemoDeployStepId && deviceType == Maemo5OsType))
        return new MaemoInstallPackageViaMountStep(parent);
    if (id == MaemoCopyFilesViaMountStep::stepId())
        return new MaemoCopyFilesViaMountStep(parent);
    if (id == MaemoUploadAndInstallPackageStep::stepId()
        || (id == OldMaemoDeployStepId && deviceType == HarmattanOsType))
        return new MaemoUploadAndInstallPackageStep(parent);
    if (id == GenericDirectUploadStep::stepId())
        return new GenericDirectUploadStep(parent, id);
    if (id == RemoteLinuxCheckForFreeDiskSpaceStep::stepId())
        return new RemoteLinuxCheckForFreeDiskSpaceStep(parent);
    if (id == MaddeQemuStartStep::stepId())
        return new MaddeQemuStartStep(parent);
    return 0;
}

bool MaemoDeployStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map)) || idFromMap(map) == OldMaemoDeployStepId;
}

BuildStep *MaemoDeployStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    Q_ASSERT(canRestore(parent, map));
    BuildStep * const step = create(parent, idFromMap(map));
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
    if (product->id() == MaemoInstallPackageViaMountStep::stepId()) {
        return new MaemoInstallPackageViaMountStep(parent,
            qobject_cast<MaemoInstallPackageViaMountStep *>(product));
    } else if (product->id() == MaemoCopyFilesViaMountStep::stepId()) {
        return new MaemoCopyFilesViaMountStep(parent,
            qobject_cast<MaemoCopyFilesViaMountStep *>(product));
    } else if (product->id() == MaemoUploadAndInstallPackageStep::stepId()) {
        return new MaemoUploadAndInstallPackageStep(parent,
            qobject_cast<MaemoUploadAndInstallPackageStep*>(product));
    } else if (product->id() == MaemoInstallDebianPackageToSysrootStep::Id) {
        return new MaemoInstallDebianPackageToSysrootStep(parent,
            qobject_cast<MaemoInstallDebianPackageToSysrootStep *>(product));
    } else if (product->id() == MaemoCopyToSysrootStep::Id) {
        return new MaemoCopyToSysrootStep(parent,
            qobject_cast<MaemoCopyToSysrootStep *>(product));
    } else if (product->id() == MaemoMakeInstallToSysrootStep::Id) {
        return new MaemoMakeInstallToSysrootStep(parent,
            qobject_cast<MaemoMakeInstallToSysrootStep *>(product));
    } else if (product->id() == GenericDirectUploadStep::stepId()) {
        return new GenericDirectUploadStep(parent,
             qobject_cast<GenericDirectUploadStep *>(product));
    } else if (RemoteLinuxCheckForFreeDiskSpaceStep * const other = qobject_cast<RemoteLinuxCheckForFreeDiskSpaceStep *>(product)) {
        return new RemoteLinuxCheckForFreeDiskSpaceStep(parent, other);
    } else if (MaddeQemuStartStep * const other = qobject_cast<MaddeQemuStartStep *>(product)) {
        return new MaddeQemuStartStep(parent, other);
    }

    return 0;
}

} // namespace Internal
} // namespace Madde
