/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qt4maemodeployconfiguration.h"

#include "deploymentinfo.h"
#include "linuxdeviceconfigurations.h"
#include "maemoconstants.h"
#include "maemodeploybymountstep.h"
#include "maemodeployconfigurationwidget.h"
#include "maemoinstalltosysrootstep.h"
#include "maemopackagecreationstep.h"
#include "maemopertargetdeviceconfigurationlistmodel.h"
#include "maemouploadandinstalldeploystep.h"
#include "qt4maemotarget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <qt4projectmanager/qt4target.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {
namespace {
const QString OldDeployConfigId = QLatin1String("2.2MaemoDeployConfig");
} // namespace

Qt4MaemoDeployConfiguration::Qt4MaemoDeployConfiguration(Target *target,
    const QString &id) : DeployConfiguration(target, id)
{
    // A DeploymentInfo object is only dependent on the active build
    // configuration and therefore can (and should) be shared among all
    // deploy steps. The per-target device configurations model is
    // similarly only dependent on the target.
    const QList<DeployConfiguration *> &deployConfigs
        = this->target()->deployConfigurations();
    foreach (const DeployConfiguration * const dc, deployConfigs) {
        const Qt4MaemoDeployConfiguration * const mdc
            = qobject_cast<const Qt4MaemoDeployConfiguration *>(dc);
        if (mdc) {
            m_deploymentInfo = mdc->deploymentInfo();
            m_devConfModel = mdc->m_devConfModel;
            break;
        }
    }
    if (!m_deploymentInfo) {
        m_deploymentInfo = QSharedPointer<DeploymentInfo>(new DeploymentInfo(qobject_cast<Qt4BaseTarget *>(target)));
        m_devConfModel = QSharedPointer<MaemoPerTargetDeviceConfigurationListModel>
            (new MaemoPerTargetDeviceConfigurationListModel(0, target));
    }

    initialize();
}

Qt4MaemoDeployConfiguration::Qt4MaemoDeployConfiguration(ProjectExplorer::Target *target,
    DeployConfiguration *source) : DeployConfiguration(target, source)
{
    const Qt4MaemoDeployConfiguration * const mdc
        = qobject_cast<Qt4MaemoDeployConfiguration *>(source);
    m_deploymentInfo = mdc->deploymentInfo();
    m_devConfModel = mdc->deviceConfigModel();
    initialize();
}

void Qt4MaemoDeployConfiguration::initialize()
{
    m_deviceConfiguration = deviceConfigModel()->defaultDeviceConfig();
    connect(deviceConfigModel().data(), SIGNAL(updated()),
            SLOT(handleDeviceConfigurationListUpdated()));
}

void Qt4MaemoDeployConfiguration::handleDeviceConfigurationListUpdated()
{
    setDeviceConfig(LinuxDeviceConfigurations::instance()->internalId(m_deviceConfiguration));
}

void Qt4MaemoDeployConfiguration::setDeviceConfig(LinuxDeviceConfiguration::Id internalId)
{
    m_deviceConfiguration = deviceConfigModel()->find(internalId);
    emit deviceConfigurationListChanged();
    emit currentDeviceConfigurationChanged();
}

bool Qt4MaemoDeployConfiguration::fromMap(const QVariantMap &map)
{
    if (!DeployConfiguration::fromMap(map))
        return false;
    setDeviceConfig(map.value(DeviceIdKey, LinuxDeviceConfiguration::InvalidId).toULongLong());
    return true;
}

QVariantMap Qt4MaemoDeployConfiguration::toMap() const
{
    QVariantMap map = DeployConfiguration::toMap();
    map.insert(DeviceIdKey,
        LinuxDeviceConfigurations::instance()->internalId(m_deviceConfiguration));
    return map;
}

void Qt4MaemoDeployConfiguration::setDeviceConfiguration(int index)
{
    const LinuxDeviceConfiguration::ConstPtr &newDevConf = deviceConfigModel()->deviceAt(index);
    if (m_deviceConfiguration != newDevConf) {
        m_deviceConfiguration = newDevConf;
        emit currentDeviceConfigurationChanged();
    }
}

Qt4MaemoDeployConfiguration::~Qt4MaemoDeployConfiguration() {}

DeployConfigurationWidget *Qt4MaemoDeployConfiguration::configurationWidget() const
{
    return new MaemoDeployConfigurationWidget;
}

QSharedPointer<DeploymentInfo> Qt4MaemoDeployConfiguration::deploymentInfo() const
{
    return m_deploymentInfo;
}

QSharedPointer<MaemoPerTargetDeviceConfigurationListModel> Qt4MaemoDeployConfiguration::deviceConfigModel() const
{
    return m_devConfModel;
}

LinuxDeviceConfiguration::ConstPtr Qt4MaemoDeployConfiguration::deviceConfiguration() const
{
    return m_deviceConfiguration;
}

const QString Qt4MaemoDeployConfiguration::FremantleWithPackagingId
    = QLatin1String("DeployToFremantleWithPackaging");
const QString Qt4MaemoDeployConfiguration::FremantleWithoutPackagingId
    = QLatin1String("DeployToFremantleWithoutPackaging");
const QString Qt4MaemoDeployConfiguration::HarmattanId
    = QLatin1String("DeployToHarmattan");
const QString Qt4MaemoDeployConfiguration::MeegoId
    = QLatin1String("DeployToMeego");
const QString Qt4MaemoDeployConfiguration::GenericLinuxId
    = QLatin1String("DeployToGenericLinux");


Qt4MaemoDeployConfigurationFactory::Qt4MaemoDeployConfigurationFactory(QObject *parent)
    : DeployConfigurationFactory(parent)
{ }

QStringList Qt4MaemoDeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    QStringList ids;
    if (qobject_cast<Qt4Maemo5Target *>(parent)) {
        ids << Qt4MaemoDeployConfiguration::FremantleWithPackagingId
            << Qt4MaemoDeployConfiguration::FremantleWithoutPackagingId;
    } else if (qobject_cast<Qt4HarmattanTarget *>(parent)) {
        ids << Qt4MaemoDeployConfiguration::HarmattanId;
    } else if (qobject_cast<Qt4MeegoTarget *>(parent)) {
        ids << Qt4MaemoDeployConfiguration::MeegoId;
    } else if (MaemoGlobal::hasLinuxQt(parent)) {
        ids << Qt4MaemoDeployConfiguration::GenericLinuxId;
    }

    return ids;
}

QString Qt4MaemoDeployConfigurationFactory::displayNameForId(const QString &id) const
{
    if (id == Qt4MaemoDeployConfiguration::FremantleWithoutPackagingId)
        return tr("Copy Files to Maemo5 Device");
    else if (id == Qt4MaemoDeployConfiguration::FremantleWithPackagingId)
        return tr("Build Debian Package and Install to Maemo5 Device");
    else if (id == Qt4MaemoDeployConfiguration::HarmattanId)
        return tr("Build Debian Package and Install to Harmattan Device");
    else if (id == Qt4MaemoDeployConfiguration::MeegoId)
        return tr("Build RPM Package and Install to MeeGo Device");
    else if (id == Qt4MaemoDeployConfiguration::GenericLinuxId)
        return tr("Build Tarball and Install to Linux Host");
    return QString();
}

bool Qt4MaemoDeployConfigurationFactory::canCreate(Target *parent,
    const QString &id) const
{
    return availableCreationIds(parent).contains(id);
}

DeployConfiguration *Qt4MaemoDeployConfigurationFactory::create(Target *parent,
    const QString &id)
{
    Q_ASSERT(canCreate(parent, id));

    DeployConfiguration * const dc
        = new Qt4MaemoDeployConfiguration(parent, id);
    dc->setDefaultDisplayName(displayNameForId(id));

    if (id == Qt4MaemoDeployConfiguration::FremantleWithoutPackagingId) {
        dc->stepList()->insertStep(0, new MaemoMakeInstallToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoMountAndCopyDeployStep(dc->stepList()));
    } else if (id == Qt4MaemoDeployConfiguration::FremantleWithPackagingId) {
        dc->stepList()->insertStep(0, new MaemoDebianPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoInstallDebianPackageToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(2, new MaemoMountAndInstallDeployStep(dc->stepList()));
    } else if (id == Qt4MaemoDeployConfiguration::HarmattanId) {
        dc->stepList()->insertStep(0, new MaemoDebianPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoInstallDebianPackageToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(2, new MaemoUploadAndInstallDpkgPackageStep(dc->stepList()));
    } else if (id == Qt4MaemoDeployConfiguration::MeegoId) {
        dc->stepList()->insertStep(0, new MaemoRpmPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoInstallRpmPackageToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(2, new MaemoUploadAndInstallRpmPackageStep(dc->stepList()));
    } else if (id == Qt4MaemoDeployConfiguration::GenericLinuxId) {
        dc->stepList()->insertStep(0, new MaemoTarPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoUploadAndInstallTarPackageStep(dc->stepList()));
    }
    return dc;
}

bool Qt4MaemoDeployConfigurationFactory::canRestore(Target *parent,
    const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map))
        || (idFromMap(map) == OldDeployConfigId
            && qobject_cast<AbstractQt4MaemoTarget *>(parent));
}

DeployConfiguration *Qt4MaemoDeployConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    QString id = idFromMap(map);
    if (id == OldDeployConfigId) {
        if (qobject_cast<Qt4Maemo5Target *>(parent))
            id = Qt4MaemoDeployConfiguration::FremantleWithPackagingId;
        else if (qobject_cast<Qt4HarmattanTarget *>(parent))
            id = Qt4MaemoDeployConfiguration::HarmattanId;
        else if (qobject_cast<Qt4MeegoTarget *>(parent))
            id = Qt4MaemoDeployConfiguration::MeegoId;
    }
    Qt4MaemoDeployConfiguration * const dc
        = new Qt4MaemoDeployConfiguration(parent, id);
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
    return new Qt4MaemoDeployConfiguration(parent, product);
}

} // namespace Internal
} // namespace RemoteLinux
