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

#include "target.h"

#include "toolchain.h"
#include "buildconfiguration.h"
#include "deployconfiguration.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "runconfiguration.h"
#include "toolchainmanager.h"

#include <limits>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <utils/qtcassert.h>

#include <QIcon>
#include <QPainter>

namespace {
const char ACTIVE_BC_KEY[] = "ProjectExplorer.Target.ActiveBuildConfiguration";
const char BC_KEY_PREFIX[] = "ProjectExplorer.Target.BuildConfiguration.";
const char BC_COUNT_KEY[] = "ProjectExplorer.Target.BuildConfigurationCount";

const char ACTIVE_DC_KEY[] = "ProjectExplorer.Target.ActiveDeployConfiguration";
const char DC_KEY_PREFIX[] = "ProjectExplorer.Target.DeployConfiguration.";
const char DC_COUNT_KEY[] = "ProjectExplorer.Target.DeployConfigurationCount";

const char ACTIVE_RC_KEY[] = "ProjectExplorer.Target.ActiveRunConfiguration";
const char RC_KEY_PREFIX[] = "ProjectExplorer.Target.RunConfiguration.";
const char RC_COUNT_KEY[] = "ProjectExplorer.Target.RunConfigurationCount";

} // namespace

// -------------------------------------------------------------------------
// Target
// -------------------------------------------------------------------------

namespace ProjectExplorer {

class TargetPrivate
{
public:
    TargetPrivate();

    QList<DeployConfigurationFactory *> deployFactories() const;

    bool m_isEnabled;
    QIcon m_icon;
    QIcon m_overlayIcon;
    QString m_toolTip;

    QList<BuildConfiguration *> m_buildConfigurations;
    BuildConfiguration *m_activeBuildConfiguration;
    QList<DeployConfiguration *> m_deployConfigurations;
    DeployConfiguration *m_activeDeployConfiguration;
    QList<RunConfiguration *> m_runConfigurations;
    RunConfiguration* m_activeRunConfiguration;

    QPixmap m_connectedPixmap;
    QPixmap m_readyToUsePixmap;
    QPixmap m_disconnectedPixmap;
};

TargetPrivate::TargetPrivate() :
    m_isEnabled(true),
    m_activeBuildConfiguration(0),
    m_activeDeployConfiguration(0),
    m_activeRunConfiguration(0),
    m_connectedPixmap(QLatin1String(":/projectexplorer/images/DeviceConnected.png")),
    m_readyToUsePixmap(QLatin1String(":/projectexplorer/images/DeviceReadyToUse.png")),
    m_disconnectedPixmap(QLatin1String(":/projectexplorer/images/DeviceDisconnected.png"))
{
}

QList<DeployConfigurationFactory *> TargetPrivate::deployFactories() const
{
    return ExtensionSystem::PluginManager::getObjects<DeployConfigurationFactory>();
}


Target::Target(Project *project, const Core::Id id) :
    ProjectConfiguration(project, id),
    d(new TargetPrivate)
{
    connect(DeviceManager::instance(), SIGNAL(updated()), this, SLOT(updateDeviceState()));
}

Target::~Target()
{
    qDeleteAll(d->m_buildConfigurations);
    qDeleteAll(d->m_runConfigurations);
    delete d;
}

void Target::changeEnvironment()
{
    BuildConfiguration *bc = qobject_cast<BuildConfiguration *>(sender());
    if (bc == activeBuildConfiguration())
        emit environmentChanged();
}

void Target::changeBuildConfigurationEnabled()
{
    BuildConfiguration *bc = qobject_cast<BuildConfiguration *>(sender());
    if (bc == activeBuildConfiguration())
        emit buildConfigurationEnabledChanged();
}

void Target::changeDeployConfigurationEnabled()
{
    DeployConfiguration *dc = qobject_cast<DeployConfiguration *>(sender());
    if (dc == activeDeployConfiguration())
        emit deployConfigurationEnabledChanged();
}

void Target::changeRunConfigurationEnabled()
{
    RunConfiguration *rc = qobject_cast<RunConfiguration *>(sender());
    if (rc == activeRunConfiguration())
        emit runConfigurationEnabledChanged();
}

Project *Target::project() const
{
    return static_cast<Project *>(parent());
}

void Target::addBuildConfiguration(BuildConfiguration *configuration)
{
    QTC_ASSERT(configuration && !d->m_buildConfigurations.contains(configuration), return);
    Q_ASSERT(configuration->target() == this);

    if (!buildConfigurationFactory())
        return;

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = configuration->displayName();
    QStringList displayNames;
    foreach (const BuildConfiguration *bc, d->m_buildConfigurations)
        displayNames << bc->displayName();
    configurationDisplayName = Project::makeUnique(configurationDisplayName, displayNames);
    if (configurationDisplayName != configuration->displayName()) {
        if (configuration->usesDefaultDisplayName()) {
            configuration->setDefaultDisplayName(configurationDisplayName);
        } else {
            configuration->setDisplayName(configurationDisplayName);
        }
    }

    // Make sure we have a sane tool chain if at all possible
    if (!configuration->toolChain()
            || !possibleToolChains(configuration).contains(configuration->toolChain()))
        configuration->setToolChain(preferredToolChain(configuration));

    // add it
    d->m_buildConfigurations.push_back(configuration);

    emit addedBuildConfiguration(configuration);

    connect(configuration, SIGNAL(environmentChanged()),
            SLOT(changeEnvironment()));

    connect(configuration, SIGNAL(enabledChanged()),
            this, SLOT(changeBuildConfigurationEnabled()));

    if (!activeBuildConfiguration())
        setActiveBuildConfiguration(configuration);
}

bool Target::removeBuildConfiguration(BuildConfiguration *configuration)
{
    //todo: this might be error prone
    if (!d->m_buildConfigurations.contains(configuration))
        return false;

    ProjectExplorer::BuildManager *bm =
            ProjectExplorer::ProjectExplorerPlugin::instance()->buildManager();
    if (bm->isBuilding(configuration))
        return false;

    d->m_buildConfigurations.removeOne(configuration);

    emit removedBuildConfiguration(configuration);

    if (activeBuildConfiguration() == configuration) {
        if (d->m_buildConfigurations.isEmpty())
            setActiveBuildConfiguration(0);
        else
            setActiveBuildConfiguration(d->m_buildConfigurations.at(0));
    }

    delete configuration;
    return true;
}

QList<BuildConfiguration *> Target::buildConfigurations() const
{
    return d->m_buildConfigurations;
}

BuildConfiguration *Target::activeBuildConfiguration() const
{
    return d->m_activeBuildConfiguration;
}

void Target::setActiveBuildConfiguration(BuildConfiguration *configuration)
{
    if ((!configuration && d->m_buildConfigurations.isEmpty()) ||
        (configuration && d->m_buildConfigurations.contains(configuration) &&
         configuration != d->m_activeBuildConfiguration)) {
        d->m_activeBuildConfiguration = configuration;
        emit activeBuildConfigurationChanged(d->m_activeBuildConfiguration);
        emit environmentChanged();
        emit buildConfigurationEnabledChanged();
    }
}

void Target::addDeployConfiguration(DeployConfiguration *dc)
{
    QTC_ASSERT(dc && !d->m_deployConfigurations.contains(dc), return);
    Q_ASSERT(dc->target() == this);

    if (d->deployFactories().isEmpty())
        return;

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = dc->displayName();
    QStringList displayNames;
    foreach (const DeployConfiguration *current, d->m_deployConfigurations)
        displayNames << current->displayName();
    configurationDisplayName = Project::makeUnique(configurationDisplayName, displayNames);
    dc->setDisplayName(configurationDisplayName);

    // add it
    d->m_deployConfigurations.push_back(dc);

    connect(dc, SIGNAL(enabledChanged()), this, SLOT(changeDeployConfigurationEnabled()));

    emit addedDeployConfiguration(dc);

    if (!d->m_activeDeployConfiguration)
        setActiveDeployConfiguration(dc);
    Q_ASSERT(activeDeployConfiguration());
}

bool Target::removeDeployConfiguration(DeployConfiguration *dc)
{
    //todo: this might be error prone
    if (!d->m_deployConfigurations.contains(dc))
        return false;

    ProjectExplorer::BuildManager *bm =
            ProjectExplorer::ProjectExplorerPlugin::instance()->buildManager();
    if (bm->isBuilding(dc))
        return false;

    d->m_deployConfigurations.removeOne(dc);

    emit removedDeployConfiguration(dc);

    if (activeDeployConfiguration() == dc) {
        if (d->m_deployConfigurations.isEmpty())
            setActiveDeployConfiguration(0);
        else
            setActiveDeployConfiguration(d->m_deployConfigurations.at(0));
    }

    delete dc;
    return true;
}

QList<DeployConfiguration *> Target::deployConfigurations() const
{
    return d->m_deployConfigurations;
}

DeployConfiguration *Target::activeDeployConfiguration() const
{
    return d->m_activeDeployConfiguration;
}

void Target::setActiveDeployConfiguration(DeployConfiguration *dc)
{
    if ((!dc && d->m_deployConfigurations.isEmpty()) ||
        (dc && d->m_deployConfigurations.contains(dc) &&
         dc != d->m_activeDeployConfiguration)) {
        d->m_activeDeployConfiguration = dc;
        emit activeDeployConfigurationChanged(d->m_activeDeployConfiguration);
        emit deployConfigurationEnabledChanged();
    }
    updateDeviceState();
}

QList<Core::Id> Target::availableDeployConfigurationIds()
{
    QList<Core::Id> ids;
    foreach (const DeployConfigurationFactory * const factory, d->deployFactories())
        ids << factory->availableCreationIds(this);
    return ids;
}

QString Target::displayNameForDeployConfigurationId(Core::Id &id)
{
    foreach (const DeployConfigurationFactory * const factory, d->deployFactories()) {
        if (factory->availableCreationIds(this).contains(id))
            return factory->displayNameForId(id);
    }
    return QString();
}

DeployConfiguration *Target::createDeployConfiguration(Core::Id id)
{
    foreach (DeployConfigurationFactory * const factory, d->deployFactories()) {
        if (factory->canCreate(this, id))
            return factory->create(this, id);
    }
    return 0;
}

QList<RunConfiguration *> Target::runConfigurations() const
{
    return d->m_runConfigurations;
}

void Target::addRunConfiguration(RunConfiguration* runConfiguration)
{
    QTC_ASSERT(runConfiguration && !d->m_runConfigurations.contains(runConfiguration), return);
    Q_ASSERT(runConfiguration->target() == this);

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = runConfiguration->displayName();
    QStringList displayNames;
    foreach (const RunConfiguration *rc, d->m_runConfigurations)
        displayNames << rc->displayName();
    configurationDisplayName = Project::makeUnique(configurationDisplayName, displayNames);
    runConfiguration->setDisplayName(configurationDisplayName);

    d->m_runConfigurations.push_back(runConfiguration);

    connect(runConfiguration, SIGNAL(enabledChanged()), this, SLOT(changeRunConfigurationEnabled()));

    emit addedRunConfiguration(runConfiguration);

    if (!activeRunConfiguration())
        setActiveRunConfiguration(runConfiguration);
}

void Target::removeRunConfiguration(RunConfiguration* runConfiguration)
{
    QTC_ASSERT(runConfiguration && d->m_runConfigurations.contains(runConfiguration), return);

    d->m_runConfigurations.removeOne(runConfiguration);

    if (activeRunConfiguration() == runConfiguration) {
        if (d->m_runConfigurations.isEmpty())
            setActiveRunConfiguration(0);
        else
            setActiveRunConfiguration(d->m_runConfigurations.at(0));
    }

    emit removedRunConfiguration(runConfiguration);
    delete runConfiguration;
}

RunConfiguration* Target::activeRunConfiguration() const
{
    return d->m_activeRunConfiguration;
}

void Target::setActiveRunConfiguration(RunConfiguration* configuration)
{
    if ((!configuration && d->m_runConfigurations.isEmpty()) ||
        (configuration && d->m_runConfigurations.contains(configuration) &&
         configuration != d->m_activeRunConfiguration)) {
        d->m_activeRunConfiguration = configuration;
        emit activeRunConfigurationChanged(d->m_activeRunConfiguration);
        emit runConfigurationEnabledChanged();
    }
    updateDeviceState();
}

bool Target::isEnabled() const
{
    return d->m_isEnabled;
}

QIcon Target::icon() const
{
    return d->m_icon;
}

void Target::setIcon(QIcon icon)
{
    d->m_icon = icon;
    emit iconChanged();
}

QIcon Target::overlayIcon() const
{
    return d->m_overlayIcon;
}

void Target::setOverlayIcon(QIcon icon)
{
    d->m_overlayIcon = icon;
    emit overlayIconChanged();
}

QString Target::toolTip() const
{
    return d->m_toolTip;
}

void Target::setToolTip(const QString &text)
{
    d->m_toolTip = text;
    emit toolTipChanged();
}

QList<ToolChain *> Target::possibleToolChains(BuildConfiguration *) const
{
    QList<ToolChain *> tcList = ToolChainManager::instance()->toolChains();
    QList<ToolChain *> result;
    foreach (ToolChain *tc, tcList) {
        QList<Core::Id> restricted = tc->restrictedToTargets();
        if (restricted.isEmpty() || restricted.contains(id()))
            result.append(tc);
    }
    return result;
}

ToolChain *Target::preferredToolChain(BuildConfiguration *bc) const
{
    QList<ToolChain *> tcs = possibleToolChains(bc);
    if (tcs.isEmpty())
        return 0;
    return tcs.at(0);
}

QVariantMap Target::toMap() const
{
    const QList<BuildConfiguration *> bcs = buildConfigurations();

    QVariantMap map(ProjectConfiguration::toMap());
    map.insert(QLatin1String(ACTIVE_BC_KEY), bcs.indexOf(d->m_activeBuildConfiguration));
    map.insert(QLatin1String(BC_COUNT_KEY), bcs.size());
    for (int i = 0; i < bcs.size(); ++i)
        map.insert(QString::fromLatin1(BC_KEY_PREFIX) + QString::number(i), bcs.at(i)->toMap());

    const QList<DeployConfiguration *> dcs = deployConfigurations();
    map.insert(QLatin1String(ACTIVE_DC_KEY), dcs.indexOf(d->m_activeDeployConfiguration));
    map.insert(QLatin1String(DC_COUNT_KEY), dcs.size());
    for (int i = 0; i < dcs.size(); ++i)
        map.insert(QString::fromLatin1(DC_KEY_PREFIX) + QString::number(i), dcs.at(i)->toMap());

    const QList<RunConfiguration *> rcs = runConfigurations();
    map.insert(QLatin1String(ACTIVE_RC_KEY), rcs.indexOf(d->m_activeRunConfiguration));
    map.insert(QLatin1String(RC_COUNT_KEY), rcs.size());
    for (int i = 0; i < rcs.size(); ++i)
        map.insert(QString::fromLatin1(RC_KEY_PREFIX) + QString::number(i), rcs.at(i)->toMap());

    return map;
}

static QString formatToolTip(const IDevice::DeviceInfo &input)
{
    QStringList lines;
    foreach (const IDevice::DeviceInfoItem &item, input)
        lines << QString::fromLatin1("<b>%1:</b> %2").arg(item.key, item.value);
    return lines.join(QLatin1String("<br>"));
}

void Target::updateDeviceState()
{
    IDevice::ConstPtr current = currentDevice();

    QPixmap overlay;
    if (current.isNull()) {
        overlay = d->m_disconnectedPixmap;
    } else {
        switch (current->deviceState()) {
        case IDevice::DeviceStateUnknown:
            setOverlayIcon(QIcon());
            setToolTip(QString());
            return;
        case IDevice::DeviceReadyToUse:
            overlay = d->m_readyToUsePixmap;
            break;
        case IDevice::DeviceConnected:
            overlay = d->m_connectedPixmap;
            break;
        case IDevice::DeviceDisconnected:
            overlay = d->m_disconnectedPixmap;
            break;
        default:
            break;
        }
    }

    static const int TARGET_OVERLAY_ORIGINAL_SIZE = 32;

    double factor = Core::Constants::TARGET_ICON_SIZE / (double)TARGET_OVERLAY_ORIGINAL_SIZE;
    QSize overlaySize(overlay.size().width()*factor, overlay.size().height()*factor);
    QPixmap pixmap(Core::Constants::TARGET_ICON_SIZE, Core::Constants::TARGET_ICON_SIZE);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.drawPixmap(Core::Constants::TARGET_ICON_SIZE - overlaySize.width(),
                       Core::Constants::TARGET_ICON_SIZE - overlaySize.height(),
                       overlay.scaled(overlaySize));

    setOverlayIcon(QIcon(pixmap));
    setToolTip(current.isNull() ? QString() : formatToolTip(current->deviceInformation()));
}

ProjectExplorer::IDevice::ConstPtr Target::currentDevice() const
{
    return DeviceManager::instance()->find(ProjectExplorer::Constants::DESKTOP_DEVICE_ID);
}

void Target::setEnabled(bool enabled)
{
    if (enabled == d->m_isEnabled)
        return;

    d->m_isEnabled = enabled;
    emit targetEnabled(d->m_isEnabled);
}

bool Target::fromMap(const QVariantMap &map)
{
    if (!ProjectConfiguration::fromMap(map))
        return false;

    bool ok;
    int bcCount = map.value(QLatin1String(BC_COUNT_KEY), 0).toInt(&ok);
    if (!ok || bcCount < 0)
        bcCount = 0;
    int activeConfiguration = map.value(QLatin1String(ACTIVE_BC_KEY), 0).toInt(&ok);
    if (!ok || activeConfiguration < 0)
        activeConfiguration = 0;
    if (0 > activeConfiguration || bcCount < activeConfiguration)
        activeConfiguration = 0;

    for (int i = 0; i < bcCount; ++i) {
        const QString key = QString::fromLatin1(BC_KEY_PREFIX) + QString::number(i);
        if (!map.contains(key))
            return false;
        BuildConfiguration *bc = buildConfigurationFactory()->restore(this, map.value(key).toMap());
        if (!bc)
            continue;
        addBuildConfiguration(bc);
        if (i == activeConfiguration)
            setActiveBuildConfiguration(bc);
    }
    if (buildConfigurations().isEmpty() && buildConfigurationFactory())
        return false;

    int dcCount = map.value(QLatin1String(DC_COUNT_KEY), 0).toInt(&ok);
    if (!ok || dcCount < 0)
        dcCount = 0;
    activeConfiguration = map.value(QLatin1String(ACTIVE_DC_KEY), 0).toInt(&ok);
    if (!ok || activeConfiguration < 0)
        activeConfiguration = 0;
    if (0 > activeConfiguration || dcCount < activeConfiguration)
        activeConfiguration = 0;

    for (int i = 0; i < dcCount; ++i) {
        const QString key = QString::fromLatin1(DC_KEY_PREFIX) + QString::number(i);
        if (!map.contains(key))
            return false;
        DeployConfiguration *dc = 0;
        foreach (DeployConfigurationFactory * const factory, d->deployFactories()) {
            QVariantMap valueMap = map.value(key).toMap();
            if (factory->canRestore(this, valueMap)) {
                dc = factory->restore(this, valueMap);
                break;
            }
        }
        if (!dc)
            continue;
        addDeployConfiguration(dc);
        if (i == activeConfiguration)
            setActiveDeployConfiguration(dc);
    }

    int rcCount = map.value(QLatin1String(RC_COUNT_KEY), 0).toInt(&ok);
    if (!ok || rcCount < 0)
        rcCount = 0;
    activeConfiguration = map.value(QLatin1String(ACTIVE_RC_KEY), 0).toInt(&ok);
    if (!ok || activeConfiguration < 0)
        activeConfiguration = 0;
    if (0 > activeConfiguration || rcCount < activeConfiguration)
        activeConfiguration = 0;

    for (int i = 0; i < rcCount; ++i) {
        const QString key = QString::fromLatin1(RC_KEY_PREFIX) + QString::number(i);
        if (!map.contains(key))
            return false;

        QVariantMap valueMap = map.value(key).toMap();
        IRunConfigurationFactory *factory = IRunConfigurationFactory::restoreFactory(this, valueMap);
        if (!factory)
            continue; // Skip RCs we do not know about.)

        RunConfiguration *rc = factory->restore(this, valueMap);
        if (!rc)
            continue;
        addRunConfiguration(rc);
        if (i == activeConfiguration)
            setActiveRunConfiguration(rc);
    }
    // Ignore missing RCs: We will just populate them using the default ones.

    return true;
}

// -------------------------------------------------------------------------
// ITargetFactory
// -------------------------------------------------------------------------

ITargetFactory::ITargetFactory(QObject *parent) :
    QObject(parent)
{
    connect(ToolChainManager::instance(), SIGNAL(toolChainsChanged()),
            this, SIGNAL(canCreateTargetIdsChanged()));
}

} // namespace ProjectExplorer
