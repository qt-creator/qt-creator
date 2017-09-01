/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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
#include <projectexplorer/projectexplorericons.h>
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

static QString formatDeviceInfo(const ProjectExplorer::IDevice::DeviceInfo &input)
{
    const QStringList lines
            = Utils::transform(input, [](const ProjectExplorer::IDevice::DeviceInfoItem &i) {
        return QString::fromLatin1("<b>%1:</b> %2").arg(i.key, i.value);
    });
    return lines.join(QLatin1String("<br>"));
}

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
    QIcon m_overlayIcon;

    QList<BuildConfiguration *> m_buildConfigurations;
    BuildConfiguration *m_activeBuildConfiguration = 0;
    QList<DeployConfiguration *> m_deployConfigurations;
    DeployConfiguration *m_activeDeployConfiguration = 0;
    QList<RunConfiguration *> m_runConfigurations;
    RunConfiguration* m_activeRunConfiguration = 0;
    DeploymentData m_deploymentData;
    BuildTargetInfoList m_appTargets;
    QVariantMap m_pluginSettings;

    Kit *const m_kit;
};

TargetPrivate::TargetPrivate(Kit *k) :
    m_kit(k)
{ }

QList<DeployConfigurationFactory *> TargetPrivate::deployFactories() const
{
    return ExtensionSystem::PluginManager::getObjects<DeployConfigurationFactory>();
}

Target::Target(Project *project, Kit *k) :
    ProjectConfiguration(project),
    d(new TargetPrivate(k))
{
    initialize(k->id());
    QTC_CHECK(d->m_kit);
    connect(DeviceManager::instance(), &DeviceManager::updated, this, &Target::updateDeviceState);

    setDisplayName(d->m_kit->displayName());
    setToolTip(d->m_kit->toHtml());

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

void Target::handleKitUpdates(Kit *k)
{
    if (k != d->m_kit)
        return;

    setDisplayName(k->displayName());
    updateDefaultDeployConfigurations();
    updateDeviceState(); // in case the device changed...
    setToolTip(k->toHtml());

    emit iconChanged();
    emit kitChanged();
}

void Target::handleKitRemoval(Kit *k)
{
    if (k != d->m_kit)
        return;
    project()->removeTarget(this);
}

bool Target::isActive() const
{
    return project()->activeTarget() == this;
}

Project *Target::project() const
{
    return static_cast<Project *>(parent());
}

Kit *Target::kit() const
{
    return d->m_kit;
}

void Target::addBuildConfiguration(BuildConfiguration *bc)
{
    QTC_ASSERT(bc && !d->m_buildConfigurations.contains(bc), return);
    Q_ASSERT(bc->target() == this);

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = bc->displayName();
    QStringList displayNames = Utils::transform(d->m_buildConfigurations, &BuildConfiguration::displayName);
    configurationDisplayName = Project::makeUnique(configurationDisplayName, displayNames);
    if (configurationDisplayName != bc->displayName()) {
        if (bc->usesDefaultDisplayName())
            bc->setDefaultDisplayName(configurationDisplayName);
        else
            bc->setDisplayName(configurationDisplayName);
    }

    // add it
    d->m_buildConfigurations.push_back(bc);

    emit addedProjectConfiguration(bc);
    emit addedBuildConfiguration(bc);

    if (!activeBuildConfiguration())
        setActiveBuildConfiguration(bc);
}

bool Target::removeBuildConfiguration(BuildConfiguration *bc)
{
    //todo: this might be error prone
    if (!d->m_buildConfigurations.contains(bc))
        return false;

    if (BuildManager::isBuilding(bc))
        return false;

    d->m_buildConfigurations.removeOne(bc);
    emit aboutToRemoveProjectConfiguration(bc);
    d->m_buildConfigurations.removeOne(bc);


    if (activeBuildConfiguration() == bc) {
        if (d->m_buildConfigurations.isEmpty())
            SessionManager::setActiveBuildConfiguration(this, nullptr, SetActive::Cascade);
        else
            SessionManager::setActiveBuildConfiguration(this, d->m_buildConfigurations.at(0), SetActive::Cascade);
    }

    emit removedBuildConfiguration(bc);
    emit removedProjectConfiguration(bc);

    delete bc;
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

void Target::setActiveBuildConfiguration(BuildConfiguration *bc)
{
    if ((!bc && d->m_buildConfigurations.isEmpty()) ||
        (bc && d->m_buildConfigurations.contains(bc) &&
         bc != d->m_activeBuildConfiguration)) {
        d->m_activeBuildConfiguration = bc;
        emit activeProjectConfigurationChanged(d->m_activeBuildConfiguration);
        emit activeBuildConfigurationChanged(d->m_activeBuildConfiguration);
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

    emit addedProjectConfiguration(dc);
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

    emit aboutToRemoveProjectConfiguration(dc);
    d->m_deployConfigurations.removeOne(dc);

    if (activeDeployConfiguration() == dc) {
        if (d->m_deployConfigurations.isEmpty())
            SessionManager::setActiveDeployConfiguration(this, nullptr, SetActive::Cascade);
        else
            SessionManager::setActiveDeployConfiguration(this, d->m_deployConfigurations.at(0),
                                                         SetActive::Cascade);
    }

    emit removedProjectConfiguration(dc);
    emit removedDeployConfiguration(dc);

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
        emit activeProjectConfigurationChanged(d->m_activeDeployConfiguration);
        emit activeDeployConfigurationChanged(d->m_activeDeployConfiguration);
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

QList<ProjectConfiguration *> Target::projectConfigurations() const
{
    QList<ProjectConfiguration *> result;
    result.append(Utils::transform(buildConfigurations(), [](BuildConfiguration *bc) { return qobject_cast<ProjectConfiguration *>(bc); }));
    result.append(Utils::transform(deployConfigurations(), [](DeployConfiguration *dc) { return qobject_cast<ProjectConfiguration *>(dc); }));
    result.append(Utils::transform(runConfigurations(), [](RunConfiguration *rc) { return qobject_cast<ProjectConfiguration *>(rc); }));
    return result;
}

QList<RunConfiguration *> Target::runConfigurations() const
{
    return d->m_runConfigurations;
}

void Target::addRunConfiguration(RunConfiguration *rc)
{
    QTC_ASSERT(rc && !d->m_runConfigurations.contains(rc), return);
    Q_ASSERT(rc->target() == this);

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = rc->displayName();
    QStringList displayNames = Utils::transform(d->m_runConfigurations, &RunConfiguration::displayName);
    configurationDisplayName = Project::makeUnique(configurationDisplayName, displayNames);
    rc->setDisplayName(configurationDisplayName);

    d->m_runConfigurations.push_back(rc);

    emit addedProjectConfiguration(rc);
    emit addedRunConfiguration(rc);

    if (!activeRunConfiguration())
        setActiveRunConfiguration(rc);
}

void Target::removeRunConfiguration(RunConfiguration *rc)
{
    QTC_ASSERT(rc && d->m_runConfigurations.contains(rc), return);

    emit aboutToRemoveProjectConfiguration(rc);
    d->m_runConfigurations.removeOne(rc);

    if (activeRunConfiguration() == rc) {
        if (d->m_runConfigurations.isEmpty())
            setActiveRunConfiguration(nullptr);
        else
            setActiveRunConfiguration(d->m_runConfigurations.at(0));
    }

    emit removedRunConfiguration(rc);
    emit removedProjectConfiguration(rc);

    delete rc;
}

RunConfiguration *Target::activeRunConfiguration() const
{
    return d->m_activeRunConfiguration;
}

void Target::setActiveRunConfiguration(RunConfiguration *rc)
{
    if ((!rc && d->m_runConfigurations.isEmpty()) ||
        (rc && d->m_runConfigurations.contains(rc) &&
         rc != d->m_activeRunConfiguration)) {
        d->m_activeRunConfiguration = rc;
        emit activeProjectConfigurationChanged(d->m_activeRunConfiguration);
        emit activeRunConfigurationChanged(d->m_activeRunConfiguration);
    }
    updateDeviceState();
}

bool Target::isEnabled() const
{
    return d->m_isEnabled;
}

QIcon Target::icon() const
{
    return d->m_kit->icon();
}

QIcon Target::overlayIcon() const
{
    return d->m_overlayIcon;
}

void Target::setOverlayIcon(const QIcon &icon)
{
    d->m_overlayIcon = icon;
    emit overlayIconChanged();
}

QString Target::overlayIconToolTip()
{
    IDevice::ConstPtr current = DeviceKitInformation::device(kit());
    return current.isNull() ? QString() : formatDeviceInfo(current->deviceInformation());
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
    std::tie(existingConfigured, existingUnconfigured)
            = Utils::partition(runConfigurations(),
                               [](const RunConfiguration *rc) { return rc->isConfigured(); });
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
        else if (project()->knowsAllBuildExecutables())
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
    if (removalList.contains(active) || !active->isEnabled()) {
        RunConfiguration *newConfiguredDefault = newConfigured.isEmpty() ? nullptr : newConfigured.at(0);

        RunConfiguration *rc
                = Utils::findOrDefault(existingConfigured,
                                       [](RunConfiguration *rc) { return rc->isEnabled(); });
        if (!rc) {
            rc = Utils::findOr(newConfigured, newConfiguredDefault,
                               Utils::equal(&RunConfiguration::displayName, project()->displayName()));
        }
        if (!rc)
            rc = newUnconfigured.isEmpty() ? nullptr : newUnconfigured.at(0);
        if (!rc) {
            // No RCs will be deleted, so use the one that will emit the minimum number of signals.
            // One signal will be emitted from the next setActiveRunConfiguration, another one
            // when the RC gets removed (and the activeRunConfiguration turns into a nullptr).
            rc = removalList.isEmpty() ? nullptr : removalList.last();
        }

        if (rc)
            setActiveRunConfiguration(rc);
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

void Target::updateDeviceState()
{
    IDevice::ConstPtr current = DeviceKitInformation::device(kit());

    QIcon overlay;
    static const QIcon disconnected = Icons::DEVICE_DISCONNECTED_INDICATOR_OVERLAY.icon();
    if (current.isNull()) {
        overlay = disconnected;
    } else {
        switch (current->deviceState()) {
        case IDevice::DeviceStateUnknown:
            overlay = QIcon();
            return;
        case IDevice::DeviceReadyToUse: {
            static const QIcon ready = Icons::DEVICE_READY_INDICATOR_OVERLAY.icon();
            overlay = ready;
            break;
        }
        case IDevice::DeviceConnected: {
            static const QIcon connected = Icons::DEVICE_CONNECTED_INDICATOR_OVERLAY.icon();
            overlay = connected;
            break;
        }
        case IDevice::DeviceDisconnected:
            overlay = disconnected;
            break;
        default:
            break;
        }
    }

    setOverlayIcon(overlay);
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

    QTC_ASSERT(d->m_kit == KitManager::kit(id()), return false);

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
