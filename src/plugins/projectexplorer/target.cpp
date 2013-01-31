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

#include "target.h"

#include "buildtargetinfo.h"
#include "deploymentdata.h"
#include "kit.h"
#include "kitinformation.h"
#include "kitmanager.h"
#include "buildconfiguration.h"
#include "deployconfiguration.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "runconfiguration.h"

#include <limits>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <utils/qtcassert.h>

#include <QDebug>
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
    DeploymentData m_deploymentData;
    BuildTargetInfoList m_appTargets;

    QPixmap m_connectedPixmap;
    QPixmap m_readyToUsePixmap;
    QPixmap m_disconnectedPixmap;

    Kit *m_kit;
};

TargetPrivate::TargetPrivate() :
    m_isEnabled(true),
    m_activeBuildConfiguration(0),
    m_activeDeployConfiguration(0),
    m_activeRunConfiguration(0),
    m_connectedPixmap(QLatin1String(":/projectexplorer/images/DeviceConnected.png")),
    m_readyToUsePixmap(QLatin1String(":/projectexplorer/images/DeviceReadyToUse.png")),
    m_disconnectedPixmap(QLatin1String(":/projectexplorer/images/DeviceDisconnected.png")),
    m_kit(0)
{
}

QList<DeployConfigurationFactory *> TargetPrivate::deployFactories() const
{
    return ExtensionSystem::PluginManager::getObjects<DeployConfigurationFactory>();
}

Target::Target(Project *project, Kit *k) :
    ProjectConfiguration(project, k->id()),
    d(new TargetPrivate)
{
    connect(DeviceManager::instance(), SIGNAL(updated()), this, SLOT(updateDeviceState()));

    d->m_kit = k;

    setDisplayName(d->m_kit->displayName());
    setIcon(d->m_kit->icon());

    KitManager *km = KitManager::instance();
    connect(km, SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(handleKitUpdates(ProjectExplorer::Kit*)));
    connect(km, SIGNAL(kitRemoved(ProjectExplorer::Kit*)),
            this, SLOT(handleKitRemoval(ProjectExplorer::Kit*)));
}

Target::~Target()
{
    qDeleteAll(d->m_buildConfigurations);
    qDeleteAll(d->m_deployConfigurations);
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

void Target::onBuildDirectoryChanged()
{
    BuildConfiguration *bc = qobject_cast<BuildConfiguration *>(sender());
    if (bc)
        emit buildDirectoryChanged();
}

void Target::handleKitUpdates(Kit *k)
{
    if (k != d->m_kit)
        return;

    setDisplayName(k->displayName());
    setIcon(k->icon());
    updateDefaultDeployConfigurations();
    updateDeviceState(); // in case the device changed...
    emit kitChanged();
}

void Target::handleKitRemoval(Kit *k)
{
    if (k != d->m_kit)
        return;
    d->m_kit = 0;
    project()->removeTarget(this);
}

Project *Target::project() const
{
    return static_cast<Project *>(parent());
}

Kit *Target::kit() const
{
    return d->m_kit;
}

void Target::addBuildConfiguration(BuildConfiguration *configuration)
{
    QTC_ASSERT(configuration && !d->m_buildConfigurations.contains(configuration), return);
    Q_ASSERT(configuration->target() == this);

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = configuration->displayName();
    QStringList displayNames;
    foreach (const BuildConfiguration *bc, d->m_buildConfigurations)
        displayNames << bc->displayName();
    configurationDisplayName = Project::makeUnique(configurationDisplayName, displayNames);
    if (configurationDisplayName != configuration->displayName()) {
        if (configuration->usesDefaultDisplayName())
            configuration->setDefaultDisplayName(configurationDisplayName);
        else
            configuration->setDisplayName(configurationDisplayName);
    }

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
        emit buildDirectoryChanged();
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

void Target::setDeploymentData(const DeploymentData &deploymentData)
{
    if (d->m_deploymentData != deploymentData) {
        d->m_deploymentData = deploymentData;
        emit deploymentDataChanged();
    }
}

DeploymentData Target::deploymentData() const
{
    return d->m_deploymentData;
}

void Target::setApplicationTargets(const BuildTargetInfoList &appTargets)
{
    if (d->m_appTargets != appTargets) {
        d->m_appTargets = appTargets;
        emit applicationTargetsChanged();
    }
}

BuildTargetInfoList Target::applicationTargets() const
{
    return d->m_appTargets;
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

QVariantMap Target::toMap() const
{
    if (!d->m_kit) // Kit was deleted, target is only around to be copied.
        return QVariantMap();

    QVariantMap map(ProjectConfiguration::toMap());

    const QList<BuildConfiguration *> bcs = buildConfigurations();
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

void Target::updateDefaultBuildConfigurations()
{
    IBuildConfigurationFactory *bcFactory = IBuildConfigurationFactory::find(this);
    if (!bcFactory) {
        qWarning("No build configuration factory found for target id '%s'.", qPrintable(id().toString()));
        return;
    }
    QList<Core::Id> bcIds = bcFactory->availableCreationIds(this);
    foreach (Core::Id id, bcIds) {
        if (!bcFactory->canCreate(this, id))
            continue;
        BuildConfiguration *bc = bcFactory->create(this, id, tr("Default build"));
        if (!bc)
            continue;
        QTC_CHECK(bc->id() == id);
        addBuildConfiguration(bc);
    }
}

void Target::updateDefaultDeployConfigurations()
{
    DeployConfigurationFactory *dcFactory = DeployConfigurationFactory::find(this);
    if (!dcFactory) {
        qWarning("No deployment configuration factory found for target id '%s'.", qPrintable(id().toString()));
        return;
    }
    QList<Core::Id> dcIds = dcFactory->availableCreationIds(this);
    QList<DeployConfiguration *> dcList = deployConfigurations();

    foreach (DeployConfiguration *dc, dcList) {
        if (dcIds.contains(dc->id()))
            dcIds.removeOne(dc->id());
        else
            removeDeployConfiguration(dc);
    }

    foreach (Core::Id id, dcIds) {
        if (!dcFactory->canCreate(this, id))
            continue;
        DeployConfiguration *dc = dcFactory->create(this, id);
        if (dc) {
            QTC_CHECK(dc->id() == id);
            addDeployConfiguration(dc);
        }
    }
}

void Target::updateDefaultRunConfigurations()
{
    QList<IRunConfigurationFactory *> rcFactories = IRunConfigurationFactory::find(this);
    if (rcFactories.isEmpty()) {
        qWarning("No run configuration factory found for target id '%s'.", qPrintable(id().toString()));
        return;
    }

    QList<RunConfiguration *> existingConfigured; // Existing configured RCs
    QList<RunConfiguration *> existingUnconfigured; // Existing unconfigured RCs
    QList<RunConfiguration *> newConfigured; // NEW configured Rcs
    QList<RunConfiguration *> newUnconfigured; // NEW unconfigured RCs


    // sort existing RCs into configured/unconfigured.
    foreach (RunConfiguration *rc, runConfigurations()) {
        if (!rc->isConfigured() && rc != activeRunConfiguration())
            existingUnconfigured << rc;
        else
            existingConfigured << rc;
    }
    int configuredCount = existingConfigured.count();

    // find all RC ids that can get created:
    QList<Core::Id> factoryIds;
    foreach (IRunConfigurationFactory *rcFactory, rcFactories)
        factoryIds.append(rcFactory->availableCreationIds(this));

    // Put outdated RCs into toRemove, do not bother with factories
    // that produce already existing RCs
    QList<RunConfiguration *> toRemove;
    QList<Core::Id> toIgnore;
    foreach (RunConfiguration *rc, existingConfigured) {
        if (factoryIds.contains(rc->id()))
            toIgnore.append(rc->id()); // Already there
        else
            toRemove << rc;
    }
    foreach (Core::Id i, toIgnore)
        factoryIds.removeAll(i);
    configuredCount -= toRemove.count();

    // Create new RCs and put them into newConfigured/newUnconfigured
    foreach (Core::Id id, factoryIds) {
        IRunConfigurationFactory *factory = 0;
        foreach (IRunConfigurationFactory *i, rcFactories) {
            if (i->canCreate(this, id)) {
                factory = i;
                break;
            }
        }
        if (!factory)
            continue;

        RunConfiguration *rc = factory->create(this, id);
        if (!rc)
            continue;
        QTC_CHECK(rc->id() == id);
        if (!rc->isConfigured())
            newUnconfigured << rc;
        else
            newConfigured << rc;
    }
    configuredCount += newConfigured.count();

    // Decide what to do with the different categories:
    bool removeExistingUnconfigured = false;
    if (configuredCount > 0) {
        // new non-Custom Executable RCs were added
        removeExistingUnconfigured = true;
        qDeleteAll(newUnconfigured);
        newUnconfigured.clear();
    } else {
        // no new RCs, use old or new CERCs?
        if (!existingUnconfigured.isEmpty()) {
            qDeleteAll(newUnconfigured);
            newUnconfigured.clear();
        }
    }

    // Do actual changes:
    foreach (RunConfiguration *rc, toRemove)
        removeRunConfiguration(rc);

    if (removeExistingUnconfigured) {
        foreach (RunConfiguration *rc, existingUnconfigured)
            removeRunConfiguration(rc);
        existingUnconfigured.clear();
    }

    foreach (RunConfiguration *rc, newConfigured)
        addRunConfiguration(rc);
    foreach (RunConfiguration *rc, newUnconfigured)
        addRunConfiguration(rc);
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
    IDevice::ConstPtr current = DeviceKitInformation::device(kit());

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

    d->m_kit = KitManager::instance()->find(id());
    if (!d->m_kit)
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
        const QVariantMap valueMap = map.value(key).toMap();
        IBuildConfigurationFactory *factory = IBuildConfigurationFactory::find(this, valueMap);
        if (!factory) {
            qWarning("No factory found to restore build configuration!");
            continue;
        }
        BuildConfiguration *bc = factory->restore(this, valueMap);
        if (!bc) {
            qWarning("Failed '%s' to restore build configuration!", qPrintable(factory->objectName()));
            continue;
        }
        QTC_CHECK(bc->id() == ProjectExplorer::idFromMap(valueMap));
        addBuildConfiguration(bc);
        if (i == activeConfiguration)
            setActiveBuildConfiguration(bc);
    }
    if (buildConfigurations().isEmpty() && IBuildConfigurationFactory::find(this))
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
        QVariantMap valueMap = map.value(key).toMap();
        DeployConfigurationFactory *factory = DeployConfigurationFactory::find(this, valueMap);
        if (!factory) {
            Core::Id id = idFromMap(valueMap);
            qWarning("No factory found to restore deployment configuration of id '%s'!",
                     id.isValid() ? qPrintable(id.toString()) : "UNKNOWN");
            continue;
        }
        DeployConfiguration *dc = factory->restore(this, valueMap);
        if (!dc) {
            qWarning("Factory '%s' failed to restore deployment configuration!", qPrintable(factory->objectName()));
            continue;
        }
        QTC_CHECK(dc->id() == ProjectExplorer::idFromMap(valueMap));
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

        // Ignore missing RCs: We will just populate them using the default ones.
        QVariantMap valueMap = map.value(key).toMap();
        IRunConfigurationFactory *factory = IRunConfigurationFactory::find(this, valueMap);
        if (!factory)
            continue;
        RunConfiguration *rc = factory->restore(this, valueMap);
        if (!rc)
            continue;
        QTC_CHECK(rc->id() == ProjectExplorer::idFromMap(valueMap));
        addRunConfiguration(rc);
        if (i == activeConfiguration)
            setActiveRunConfiguration(rc);
    }

    return true;
}

} // namespace ProjectExplorer
