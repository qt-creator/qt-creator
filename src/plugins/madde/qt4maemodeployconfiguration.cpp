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
#include <remotelinux/deploymentsettingsassistant.h>
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtCore/QString>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {
namespace {
const QString OldDeployConfigId = QLatin1String("2.2MaemoDeployConfig");
} // namespace

Qt4MaemoDeployConfiguration::Qt4MaemoDeployConfiguration(ProjectExplorer::Target *target,
        const QString &id, const QString &displayName, const QString &supportedOsType)
    : RemoteLinuxDeployConfiguration(target, id, displayName, supportedOsType)
{
    const QList<DeployConfiguration *> &deployConfigs = target->deployConfigurations();
    foreach (const DeployConfiguration * const dc, deployConfigs) {
        const Qt4MaemoDeployConfiguration * const mdc
            = qobject_cast<const Qt4MaemoDeployConfiguration *>(dc);
        if (mdc) {
            m_deploymentSettingsAssistant = mdc->deploymentSettingsAssistant();
            break;
        }
    }
    if (!m_deploymentSettingsAssistant) {
        QString qmakeScope;
        if (supportedOsType == QLatin1String(Maemo5OsType))
            qmakeScope = QLatin1String("maemo5");
        else if (supportedOsType == QLatin1String(HarmattanOsType))
            qmakeScope = QLatin1String("contains(MEEGO_EDITION,harmattan)");
        else if (supportedOsType == QLatin1String(MeeGoOsType))
            qmakeScope = QLatin1String("!isEmpty(MEEGO_VERSION_MAJOR):!contains(MEEGO_EDITION,harmattan)");
        else
            qDebug("%s: Unexpected OS type %s", Q_FUNC_INFO, qPrintable(supportedOsType));
        m_deploymentSettingsAssistant = QSharedPointer<DeploymentSettingsAssistant>
            (new DeploymentSettingsAssistant(qmakeScope, QLatin1String("/opt"), deploymentInfo()));
    }
}

Qt4MaemoDeployConfiguration::Qt4MaemoDeployConfiguration(ProjectExplorer::Target *target,
        Qt4MaemoDeployConfiguration *source)
    : RemoteLinuxDeployConfiguration(target, source)
{
    m_deploymentSettingsAssistant = source->deploymentSettingsAssistant();
}

QSharedPointer<DeploymentSettingsAssistant> Qt4MaemoDeployConfiguration::deploymentSettingsAssistant() const
{
    return m_deploymentSettingsAssistant;
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

QString Qt4MaemoDeployConfiguration::fremantleWithPackagingId()
{
    return QLatin1String("DeployToFremantleWithPackaging");
}

QString Qt4MaemoDeployConfiguration::fremantleWithoutPackagingId()
{
    return QLatin1String("DeployToFremantleWithoutPackaging");
}

QString Qt4MaemoDeployConfiguration::harmattanId()
{
    return QLatin1String("DeployToHarmattan");
}

QString Qt4MaemoDeployConfiguration::meegoId()
{
    return QLatin1String("DeployToMeego");
}


Qt4MaemoDeployConfigurationFactory::Qt4MaemoDeployConfigurationFactory(QObject *parent)
    : DeployConfigurationFactory(parent)
{ }

QStringList Qt4MaemoDeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    QStringList ids;
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

QString Qt4MaemoDeployConfigurationFactory::displayNameForId(const QString &id) const
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
    const QString &id) const
{
    return availableCreationIds(parent).contains(id);
}

DeployConfiguration *Qt4MaemoDeployConfigurationFactory::create(Target *parent,
    const QString &id)
{
    Q_ASSERT(canCreate(parent, id));

    DeployConfiguration *dc = 0;
    const QString displayName = displayNameForId(id);
    if (id == Qt4MaemoDeployConfiguration::fremantleWithoutPackagingId()) {
        dc = new Qt4MaemoDeployConfiguration(parent, id, displayName, QLatin1String(Maemo5OsType));
        dc->stepList()->insertStep(0, new MaemoMakeInstallToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoCopyFilesViaMountStep(dc->stepList()));
    } else if (id == Qt4MaemoDeployConfiguration::fremantleWithPackagingId()) {
        dc = new Qt4MaemoDeployConfiguration(parent, id, displayName, QLatin1String(Maemo5OsType));
        dc->stepList()->insertStep(0, new MaemoDebianPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoInstallDebianPackageToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(2, new MaemoInstallPackageViaMountStep(dc->stepList()));
    } else if (id == Qt4MaemoDeployConfiguration::harmattanId()) {
        dc = new Qt4MaemoDeployConfiguration(parent, id, displayName,
            QLatin1String(HarmattanOsType));
        dc->stepList()->insertStep(0, new MaemoDebianPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoInstallDebianPackageToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(2, new MaemoUploadAndInstallPackageStep(dc->stepList()));
    } else if (id == Qt4MaemoDeployConfiguration::meegoId()) {
        dc = new Qt4MaemoDeployConfiguration(parent, id, displayName, QLatin1String(MeeGoOsType));
        dc->stepList()->insertStep(0, new MaemoRpmPackageCreationStep(dc->stepList()));
        dc->stepList()->insertStep(1, new MaemoInstallRpmPackageToSysrootStep(dc->stepList()));
        dc->stepList()->insertStep(2, new MeegoUploadAndInstallPackageStep(dc->stepList()));
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
