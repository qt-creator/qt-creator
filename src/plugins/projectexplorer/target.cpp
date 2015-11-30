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

#include "target.h"

#include "buildinfo.h"
#include "buildtargetinfo.h"
#include "deploymentdata.h"
#include "kit.h"
#include "kitinformation.h"
#include "kitmanager.h"
#include "buildconfiguration.h"
#include "deployconfiguration.h"
#include "project.h"
#include "runconfiguration.h"
#include "session.h"

#include <limits>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

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
const char PLUGIN_SETTINGS_KEY[] = "ProjectExplorer.Target.PluginSettings";

} // namespace

// -------------------------------------------------------------------------
// Target
// -------------------------------------------------------------------------

namespace ProjectExplorer {

class TargetPrivate
{
public:
    TargetPrivate(Kit *k);

    QList<DeployConfigurationFactory *> deployFactories() const;

    bool m_isEnabled = true;
    QIcon m_icon;
    QIcon m_overlayIcon;
    QString m_toolTip;

    QList<BuildConfiguration *> m_buildConfigurations;
    BuildConfiguration *m_activeBuildConfiguration = 0;
    QList<DeployConfiguration *> m_deployConfigurations;
    DeployConfiguration *m_activeDeployConfiguration = 0;
    QList<RunConfiguration *> m_runConfigurations;
    RunConfiguration* m_activeRunConfiguration = 0;
    DeploymentData m_deploymentData;
    BuildTargetInfoList m_appTargets;
    QVariantMap m_pluginSettings;

    QPixmap m_connectedPixmap;
    QPixmap m_readyToUsePixmap;
    QPixmap m_disconnectedPixmap;

    Kit *const m_kit;
};

TargetPrivate::TargetPrivate(Kit *k) :
    m_connectedPixmap(QLatin1String(":/projectexplorer/images/DeviceConnected.png")),
    m_readyToUsePixmap(QLatin1String(":/projectexplorer/images/DeviceReadyToUse.png")),
    m_disconnectedPixmap(QLatin1String(":/projectexplorer/images/DeviceDisconnected.png")),
    m_kit(k)
{ }

QList<DeployConfigurationFactory *> TargetPrivate::deployFactories() const
{
    return ExtensionSystem::PluginManager::getObjects<DeployConfigurationFactory>();
}

Target::Target(Project *project, Kit *k) :
    ProjectConfiguration(project, k->id()),
    d(new TargetPrivate(k))
{
    QTC_CHECK(d->m_kit);
    connect(DeviceManager::instance(), &DeviceManager::updated, this, &Target::updateDeviceState);

    setDisplayName(d->m_kit->displayName());
    setIcon(d->m_kit->icon());

    KitManager *km = KitManager::instance();
    connect(km, &KitManager::kitUpdated, this, &Target::handleKitUpdates);
    connect(km, &KitManager::kitRemoved, this, &Target::handleKitRemoval);

    Utils::MacroExpander *expander = macroExpander();
    expander->setDisplayName(tr("Target Settings"));
    expander->setAccumulating(true);

    expander->registerSubProvider([this] { return kit()->macroExpander(); });

    expander->registerVariable("sourceDir", tr("Source directory"),
            [project] { return project->projectDirectory().toUserOutput(); });

    // Legacy support.
    expander->registerVariable(Constants::VAR_CURRENTPROJECT_NAME,
            QCoreApplication::translate("ProjectExplorer", "Name of current project"),
            [project] { return project->displayName(); },
            false);

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
    if (bc && activeBuildConfiguration() == bc)
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
    QStringList displayNames = Utils::transform(d->m_buildConfigurations, &BuildConfiguration::displayName);
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

    connect(configuration, &BuildConfiguration::environmentChanged,
            this, &Target::changeEnvironment);
    connect(configuration, &BuildConfiguration::enabledChanged,
            this, &Target::changeBuildConfigurationEnabled);
    connect(configuration, &BuildConfiguration::buildDirectoryChanged,
            this, &Target::onBuildDirectoryChanged);

    if (!activeBuildConfiguration())
        setActiveBuildConfiguration(configuration);
}

bool Target::removeBuildConfiguration(BuildConfiguration *configuration)
{
    //todo: this might be error prone
    if (!d->m_buildConfigurations.contains(configuration))
        return false;

    if (BuildManager::isBuilding(configuration))
        return false;

    d->m_buildConfigurations.removeOne(configuration);

    emit removedBuildConfiguration(configuration);

    if (activeBuildConfiguration() == configuration) {
        if (d->m_buildConfigurations.isEmpty())
            SessionManager::setActiveBuildConfiguration(this, 0, SetActive::Cascade);
        else
            SessionManager::setActiveBuildConfiguration(this, d->m_buildConfigurations.at(0), SetActive::Cascade);
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
    QStringList displayNames = Utils::transform(d->m_deployConfigurations, &DeployConfiguration::displayName);
    configurationDisplayName = Project::makeUnique(configurationDisplayName, displayNames);
    dc->setDisplayName(configurationDisplayName);

    // add it
    d->m_deployConfigurations.push_back(dc);

    connect(dc, &DeployConfiguration::enabledChanged,
            this, &Target::changeDeployConfigurationEnabled);

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

    if (BuildManager::isBuilding(dc))
        return false;

    d->m_deployConfigurations.removeOne(dc);

    emit removedDeployConfiguration(dc);

    if (activeDeployConfiguration() == dc) {
        if (d->m_deployConfigurations.isEmpty())
            SessionManager::setActiveDeployConfiguration(this, 0, SetActive::Cascade);
        else
            SessionManager::setActiveDeployConfiguration(this, d->m_deployConfigurations.at(0),
                                                         SetActive::Cascade);
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
    runConfiguration->addExtraAspects();

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = runConfiguration->displayName();
    QStringList displayNames = Utils::transform(d->m_runConfigurations, &RunConfiguration::displayName);
    configurationDisplayName = Project::makeUnique(configurationDisplayName, displayNames);
    runConfiguration->setDisplayName(configurationDisplayName);

    d->m_runConfigurations.push_back(runConfiguration);

    connect(runConfiguration, &RunConfiguration::enabledChanged,
            this, &Target::changeRunConfigurationEnabled);

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

    map.insert(QLatin1String(PLUGIN_SETTINGS_KEY), d->m_pluginSettings);

    return map;
}

void Target::updateDefaultBuildConfigurations()
{
    IBuildConfigurationFactory *bcFactory = IBuildConfigurationFactory::find(this);
    if (!bcFactory) {
        qWarning("No build configuration factory found for target id '%s'.", qPrintable(id().toString()));
        return;
    }
    QList<BuildInfo *> infoList = bcFactory->availableSetups(this->kit(), project()->projectFilePath().toString());
    foreach (BuildInfo *info, infoList) {
        BuildConfiguration *bc = bcFactory->create(this, info);
        if (!bc)
            continue;
        addBuildConfiguration(bc);
    }
    qDeleteAll(infoList);
}

void Target::updateDefaultDeployConfigurations()
{
    QList<DeployConfigurationFactory *> dcFactories = DeployConfigurationFactory::find(this);
    if (dcFactories.isEmpty()) {
        qWarning("No deployment configuration factory found for target id '%s'.", qPrintable(id().toString()));
        return;
    }

    QList<Core::Id> dcIds;
    foreach (DeployConfigurationFactory *dcFactory, dcFactories)
        dcIds.append(dcFactory->availableCreationIds(this));

    QList<DeployConfiguration *> dcList = deployConfigurations();
    QList<Core::Id> toCreate = dcIds;

    foreach (DeployConfiguration *dc, dcList) {
        if (dcIds.contains(dc->id()))
            toCreate.removeOne(dc->id());
        else
            removeDeployConfiguration(dc);
    }

    foreach (Core::Id id, toCreate) {
        foreach (DeployConfigurationFactory *dcFactory, dcFactories) {
            if (dcFactory->canCreate(this, id)) {
                DeployConfiguration *dc = dcFactory->create(this, id);
                if (dc) {
                    QTC_CHECK(dc->id() == id);
                    addDeployConfiguration(dc);
                }
            }
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
        if (!rc->isConfigured())
            existingUnconfigured << rc;
        else
            existingConfigured << rc;
    }
    int configuredCount = existingConfigured.count();

    // find all RC ids that can get created:
    QList<Core::Id> availableFactoryIds;
    foreach (IRunConfigurationFactory *rcFactory, rcFactories)
        availableFactoryIds.append(rcFactory->availableCreationIds(this));

    QList<Core::Id> autoCreateFactoryIds;
    foreach (IRunConfigurationFactory *rcFactory, rcFactories)
        autoCreateFactoryIds.append(rcFactory->availableCreationIds(this,
                                                                    IRunConfigurationFactory::AutoCreate));

    // Put outdated RCs into toRemove, do not bother with factories
    // that produce already existing RCs
    QList<RunConfiguration *> toRemove;
    QList<Core::Id> toIgnore;
    foreach (RunConfiguration *rc, existingConfigured) {
        if (availableFactoryIds.contains(rc->id()))
            toIgnore.append(rc->id()); // Already there
        else
            toRemove << rc;
    }
    foreach (Core::Id i, toIgnore)
        autoCreateFactoryIds.removeAll(i);
    configuredCount -= toRemove.count();

    // Create new RCs and put them into newConfigured/newUnconfigured
    foreach (Core::Id id, autoCreateFactoryIds) {
        IRunConfigurationFactory *factory = Utils::findOrDefault(rcFactories,
                                                          [this, id] (IRunConfigurationFactory *i) {
                                                                return i->canCreate(this, id);
                                                          });
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
    foreach (RunConfiguration *rc, newConfigured)
        addRunConfiguration(rc);
    foreach (RunConfiguration *rc, newUnconfigured)
        addRunConfiguration(rc);

    // Generate complete list of RCs to remove later:
    QList<RunConfiguration *> removalList;
    foreach (RunConfiguration *rc, toRemove) {
        removalList << rc;
        existingConfigured.removeOne(rc); // make sure to also remove them from existingConfigured!
    }

    if (removeExistingUnconfigured) {
        removalList.append(existingUnconfigured);
        existingUnconfigured.clear();
    }

    // Make sure a configured RC will be active after we delete the RCs:
    RunConfiguration *active = activeRunConfiguration();
    if (removalList.contains(active)) {
        if (!existingConfigured.isEmpty()) {
            setActiveRunConfiguration(existingConfigured.at(0));
        } else if (!newConfigured.isEmpty()) {
            RunConfiguration *selected = newConfigured.at(0);
            // Try to find a runconfiguration that matches the project name. That is a good
            // candidate for something to run initially.
            selected = Utils::findOr(newConfigured, selected,
                                     Utils::equal(&RunConfiguration::displayName, project()->displayName()));
            setActiveRunConfiguration(selected);
        } else if (!newUnconfigured.isEmpty()){
            setActiveRunConfiguration(newUnconfigured.at(0));
        } else {
            if (!removalList.isEmpty())
                setActiveRunConfiguration(removalList.last());
            // Nothing will be left after removal: We set this to the last of in the removal list
            // since that gives us the minimum number of signals (one signal for the change here and
            // one more when the last RC is removed and the active RC becomes 0).
        }
    }

    // Remove the RCs that are no longer needed:
    foreach (RunConfiguration *rc, removalList)
        removeRunConfiguration(rc);
}

QVariant Target::namedSettings(const QString &name) const
{
    return d->m_pluginSettings.value(name);
}

void Target::setNamedSettings(const QString &name, const QVariant &value)
{
    if (value.isNull())
        d->m_pluginSettings.remove(name);
    else
        d->m_pluginSettings.insert(name, value);
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

    QTC_ASSERT(d->m_kit == KitManager::find(id()), return false);

    setDisplayName(d->m_kit->displayName()); // Overwrite displayname read from file
    setDefaultDisplayName(d->m_kit->displayName());

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

    if (map.contains(QLatin1String(PLUGIN_SETTINGS_KEY)))
        d->m_pluginSettings = map.value(QLatin1String(PLUGIN_SETTINGS_KEY)).toMap();

    return true;
}

} // namespace ProjectExplorer
