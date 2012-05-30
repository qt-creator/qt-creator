/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
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

#include "maemodeploystepfactory.h"

#include "maddeuploadandinstallpackagesteps.h"
#include "maemodeploybymountsteps.h"
#include "maemoinstalltosysrootstep.h"
#include "qt4maemotarget.h"
#include "qt4maemodeployconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <remotelinux/genericdirectuploadstep.h>
#include <remotelinux/remotelinuxcheckforfreediskspacestep.h>
#include <remotelinux/uploadandinstalltarpackagestep.h>

#include <QCoreApplication>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {
namespace {
const QString OldMaemoDeployStepId(QLatin1String("Qt4ProjectManager.MaemoDeployStep"));
} // anonymous namespace

MaemoDeployStepFactory::MaemoDeployStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<Core::Id> MaemoDeployStepFactory::availableCreationIds(BuildStepList *parent) const
{
    QList<Core::Id> ids;
    if (!qobject_cast<Qt4MaemoDeployConfiguration *>(parent->parent()))
        return ids;

    AbstractQt4MaemoTarget * const maemoTarget
        = qobject_cast<AbstractQt4MaemoTarget *>(parent->target());
    if (maemoTarget)
        ids << MaemoMakeInstallToSysrootStep::Id;
    if (qobject_cast<AbstractDebBasedQt4MaemoTarget *>(parent->target())) {
        ids << MaemoInstallDebianPackageToSysrootStep::Id;
        ids << MaemoUploadAndInstallPackageStep::stepId();
    } else if (qobject_cast<AbstractRpmBasedQt4MaemoTarget *>(parent->target())) {
        ids << MaemoInstallRpmPackageToSysrootStep::Id;
        ids << MeegoUploadAndInstallPackageStep::stepId();
    }
    if (qobject_cast<Qt4HarmattanTarget *>(parent->target()))
        ids << GenericDirectUploadStep::stepId();
    if (qobject_cast<Qt4Maemo5Target *>(parent->target()))
        ids << MaemoInstallPackageViaMountStep::stepId()
            << MaemoCopyFilesViaMountStep::stepId();
    ids << RemoteLinuxCheckForFreeDiskSpaceStep::stepId();

    return ids;
}

QString MaemoDeployStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == MaemoInstallPackageViaMountStep::stepId())
        return MaemoInstallPackageViaMountStep::displayName();
    else if (id == MaemoCopyFilesViaMountStep::stepId())
        return MaemoCopyFilesViaMountStep::displayName();
    else if (id == MaemoUploadAndInstallPackageStep::stepId())
        return MaemoUploadAndInstallPackageStep::displayName();
    else if (id == MeegoUploadAndInstallPackageStep::stepId())
        return MeegoUploadAndInstallPackageStep::displayName();
    else if (id == MaemoInstallDebianPackageToSysrootStep::Id)
        return MaemoInstallDebianPackageToSysrootStep::displayName();
    else if (id == MaemoInstallRpmPackageToSysrootStep::Id)
        return MaemoInstallRpmPackageToSysrootStep::displayName();
    else if (id == MaemoCopyToSysrootStep::Id)
        return MaemoCopyToSysrootStep::displayName();
    else if (id == MaemoMakeInstallToSysrootStep::Id)
        return MaemoMakeInstallToSysrootStep::displayName();
    else if (id == GenericDirectUploadStep::stepId())
        return GenericDirectUploadStep::displayName();
    if (id == RemoteLinuxCheckForFreeDiskSpaceStep::stepId())
        return RemoteLinuxCheckForFreeDiskSpaceStep::stepDisplayName();
    return QString();
}

bool MaemoDeployStepFactory::canCreate(BuildStepList *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id) && !parent->contains(id);
}

BuildStep *MaemoDeployStepFactory::create(BuildStepList *parent, const Core::Id id)
{
    const Target * const t = parent->target();

    if (id == MaemoInstallDebianPackageToSysrootStep::Id) {
        return new MaemoInstallDebianPackageToSysrootStep(parent);
    } else if (id == MaemoInstallRpmPackageToSysrootStep::Id) {
        return new MaemoInstallRpmPackageToSysrootStep(parent);
    } else if (id == MaemoCopyToSysrootStep::Id) {
        return new MaemoCopyToSysrootStep(parent);
    } else if (id == MaemoMakeInstallToSysrootStep::Id) {
        return new MaemoMakeInstallToSysrootStep(parent);
    } else if (id == MaemoInstallPackageViaMountStep::stepId()
        || (id == Core::Id(OldMaemoDeployStepId) && qobject_cast< const Qt4Maemo5Target *>(t))) {
        return new MaemoInstallPackageViaMountStep(parent);
    } else if (id == MaemoCopyFilesViaMountStep::stepId()) {
        return new MaemoCopyFilesViaMountStep(parent);
    } else if (id == MaemoUploadAndInstallPackageStep::stepId()
        || (id == Core::Id(OldMaemoDeployStepId) && (qobject_cast<const Qt4HarmattanTarget *>(t)))) {
        return new MaemoUploadAndInstallPackageStep(parent);
    } else if (id == MeegoUploadAndInstallPackageStep::stepId()
        || (id == Core::Id(OldMaemoDeployStepId) && (qobject_cast<const Qt4MeegoTarget *>(t)))) {
        return new MeegoUploadAndInstallPackageStep(parent);
    } else if (id == GenericDirectUploadStep::stepId()) {
        return new GenericDirectUploadStep(parent, id);
    } else if (id == RemoteLinuxCheckForFreeDiskSpaceStep::stepId()) {
        return new RemoteLinuxCheckForFreeDiskSpaceStep(parent);
    }

    return 0;
}

bool MaemoDeployStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map))
        || idFromMap(map) == Core::Id(OldMaemoDeployStepId);
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
    } else if (product->id() == MeegoUploadAndInstallPackageStep::stepId()) {
        return new MeegoUploadAndInstallPackageStep(parent,
            qobject_cast<MeegoUploadAndInstallPackageStep*>(product));
    } else if (product->id() == MaemoInstallDebianPackageToSysrootStep::Id) {
        return new MaemoInstallDebianPackageToSysrootStep(parent,
            qobject_cast<MaemoInstallDebianPackageToSysrootStep *>(product));
    } else if (product->id() == MaemoInstallRpmPackageToSysrootStep::Id) {
        return new MaemoInstallRpmPackageToSysrootStep(parent,
            qobject_cast<MaemoInstallRpmPackageToSysrootStep *>(product));
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
    }

    return 0;
}

} // namespace Internal
} // namespace Madde
