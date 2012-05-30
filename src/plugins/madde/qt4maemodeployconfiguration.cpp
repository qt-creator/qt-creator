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
#include "qt4maemodeployconfiguration.h"

#include "maddeuploadandinstallpackagesteps.h"
#include "maemoconstants.h"
#include "maemodeploybymountsteps.h"
#include "maemodeployconfigurationwidget.h"
#include "maemoinstalltosysrootstep.h"
#include "maemopackagecreationstep.h"
#include "qt4maemotarget.h"

#include <projectexplorer/buildsteplist.h>
#include <qt4projectmanager/qt4target.h>
#include <remotelinux/deployablefile.h>
#include <remotelinux/deployablefilesperprofile.h>
#include <remotelinux/deploymentinfo.h>
#include <remotelinux/remotelinuxcheckforfreediskspacestep.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QString>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {
namespace {
const QString OldDeployConfigId = QLatin1String("2.2MaemoDeployConfig");
} // namespace

Qt4MaemoDeployConfiguration::Qt4MaemoDeployConfiguration(ProjectExplorer::Target *target,
        const Core::Id id, const QString &displayName)
    : RemoteLinuxDeployConfiguration(target, id, displayName)
{
}

Qt4MaemoDeployConfiguration::Qt4MaemoDeployConfiguration(ProjectExplorer::Target *target,
        Qt4MaemoDeployConfiguration *source)
    : RemoteLinuxDeployConfiguration(target, source)
{
}

QString Qt4MaemoDeployConfiguration::localDesktopFilePath(const DeployableFilesPerProFile *proFileInfo) const
{
    QTC_ASSERT(proFileInfo->projectType() == ApplicationTemplate, return QString());

    for (int i = 0; i < proFileInfo->rowCount(); ++i) {
        const DeployableFile &d = proFileInfo->deployableAt(i);
        if (QFileInfo(d.localFilePath).fileName().endsWith(QLatin1String(".desktop")))
            return d.localFilePath;
    }
    return QString();
}


DeployConfigurationWidget *Qt4MaemoDeployConfiguration::configurationWidget() const
{
    return new MaemoDeployConfigurationWidget;
}

Qt4MaemoDeployConfiguration::~Qt4MaemoDeployConfiguration() {}

Core::Id Qt4MaemoDeployConfiguration::fremantleWithPackagingId()
{
    return Core::Id("DeployToFremantleWithPackaging");
}

Core::Id Qt4MaemoDeployConfiguration::fremantleWithoutPackagingId()
{
    return Core::Id("DeployToFremantleWithoutPackaging");
}

Core::Id Qt4MaemoDeployConfiguration::harmattanId()
{
    return Core::Id("DeployToHarmattan");
}

Core::Id Qt4MaemoDeployConfiguration::meegoId()
{
    return Core::Id("DeployToMeego");
}

Qt4MaemoDeployConfigurationFactory::Qt4MaemoDeployConfigurationFactory(QObject *parent)
    : DeployConfigurationFactory(parent)
{ }

QList<Core::Id> Qt4MaemoDeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> ids;
    if (qobject_cast<Qt4Maemo5Target *>(parent)) {
        ids << Qt4MaemoDeployConfiguration::fremantleWithPackagingId()
            << Qt4MaemoDeployConfiguration::fremantleWithoutPackagingId();
    } else if (qobject_cast<Qt4HarmattanTarget *>(parent)) {
        ids << Qt4MaemoDeployConfiguration::harmattanId();
    } else if (qobject_cast<Qt4MeegoTarget *>(parent)) {
        ids << Qt4MaemoDeployConfiguration::meegoId();
    }

    return ids;
}

QString Qt4MaemoDeployConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == Qt4MaemoDeployConfiguration::fremantleWithoutPackagingId())
        return tr("Copy Files to Maemo5 Device");
    else if (id == Qt4MaemoDeployConfiguration::fremantleWithPackagingId())
        return tr("Build Debian Package and Install to Maemo5 Device");
    else if (id == Qt4MaemoDeployConfiguration::harmattanId())
        return tr("Build Debian Package and Install to Harmattan Device");
    else if (id == Qt4MaemoDeployConfiguration::meegoId())
        return tr("Build RPM Package and Install to MeeGo Device");
    return QString();
}

bool Qt4MaemoDeployConfigurationFactory::canCreate(Target *parent,
    const Core::Id id) const
{
    return availableCreationIds(parent).contains(id);
}

DeployConfiguration *Qt4MaemoDeployConfigurationFactory::create(Target *parent,
    const Core::Id id)
{
    Q_ASSERT(canCreate(parent, id));

    const QString displayName = displayNameForId(id);
    DeployConfiguration * const dc = new Qt4MaemoDeployConfiguration(parent, id, displayName);
    if (id == Qt4MaemoDeployConfiguration::fremantleWithoutPackagingId()) {
        dc->stepList()->insertStep(0, new MaemoMakeInstallToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(1, new RemoteLinuxCheckForFreeDiskSpaceStep(dc->stepList()));
        dc->stepList()->insertStep(2, new MaemoCopyFilesViaMountStep(dc->stepList()));
    } else if (id == Qt4MaemoDeployConfiguration::fremantleWithPackagingId()) {
        dc->stepList()->insertStep(0, new MaemoDebianPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoInstallDebianPackageToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(2, new RemoteLinuxCheckForFreeDiskSpaceStep(dc->stepList()));
        dc->stepList()->insertStep(3, new MaemoInstallPackageViaMountStep(dc->stepList()));
    } else if (id == Qt4MaemoDeployConfiguration::harmattanId()) {
        dc->stepList()->insertStep(0, new MaemoDebianPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoInstallDebianPackageToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(2, new RemoteLinuxCheckForFreeDiskSpaceStep(dc->stepList()));
        dc->stepList()->insertStep(3, new MaemoUploadAndInstallPackageStep(dc->stepList()));
    } else if (id == Qt4MaemoDeployConfiguration::meegoId()) {
        dc->stepList()->insertStep(0, new MaemoRpmPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoInstallRpmPackageToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(2, new RemoteLinuxCheckForFreeDiskSpaceStep(dc->stepList()));
        dc->stepList()->insertStep(3, new MeegoUploadAndInstallPackageStep(dc->stepList()));
    }
    return dc;
}

bool Qt4MaemoDeployConfigurationFactory::canRestore(Target *parent,
    const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map))
        || (idFromMap(map) == Core::Id(OldDeployConfigId)
            && qobject_cast<AbstractQt4MaemoTarget *>(parent));
}

DeployConfiguration *Qt4MaemoDeployConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    Core::Id id = idFromMap(map);
    if (id == Core::Id(OldDeployConfigId)) {
        if (qobject_cast<Qt4Maemo5Target *>(parent))
            id = Qt4MaemoDeployConfiguration::fremantleWithPackagingId();
        else if (qobject_cast<Qt4HarmattanTarget *>(parent))
            id = Qt4MaemoDeployConfiguration::harmattanId();
        else if (qobject_cast<Qt4MeegoTarget *>(parent))
            id = Qt4MaemoDeployConfiguration::meegoId();
    }
    Qt4MaemoDeployConfiguration * const dc
        = qobject_cast<Qt4MaemoDeployConfiguration *>(create(parent, id));
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

DeployConfiguration *Qt4MaemoDeployConfigurationFactory::clone(Target *parent,
    DeployConfiguration *product)
{
    if (!canClone(parent, product))
        return 0;
    return new Qt4MaemoDeployConfiguration(parent,
        qobject_cast<Qt4MaemoDeployConfiguration *>(product));
}

} // namespace Internal
} // namespace Madde
