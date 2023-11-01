// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "target.h"

#include "buildconfiguration.h"
#include "buildinfo.h"
#include "buildmanager.h"
#include "buildsystem.h"
#include "buildtargetinfo.h"
#include "deployconfiguration.h"
#include "deploymentdata.h"
#include "devicesupport/devicemanager.h"
#include "kit.h"
#include "kitaspects.h"
#include "kitmanager.h"
#include "miniprojecttargetselector.h"
#include "project.h"
#include "projectconfigurationmodel.h"
#include "projectexplorer.h"
#include "projectexplorericons.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "runconfiguration.h"

#include <coreplugin/coreconstants.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDebug>
#include <QIcon>
#include <QPainter>

#include <limits>

using namespace Utils;

namespace ProjectExplorer {

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

// -------------------------------------------------------------------------
// Target
// -------------------------------------------------------------------------

class TargetPrivate
{
public:
    TargetPrivate(Target *t, Kit *k) :
        m_kit(k),
        m_buildConfigurationModel(t),
        m_deployConfigurationModel(t),
        m_runConfigurationModel(t)
    { }

    ~TargetPrivate()
    {
        delete m_buildSystem;
    }

    QIcon m_overlayIcon;

    QList<BuildConfiguration *> m_buildConfigurations;
    QPointer<BuildConfiguration> m_activeBuildConfiguration;
    QList<DeployConfiguration *> m_deployConfigurations;
    DeployConfiguration *m_activeDeployConfiguration = nullptr;
    QList<RunConfiguration *> m_runConfigurations;
    RunConfiguration* m_activeRunConfiguration = nullptr;
    Store m_pluginSettings;

    Kit *const m_kit;
    MacroExpander m_macroExpander;
    BuildSystem *m_buildSystem = nullptr;

    ProjectConfigurationModel m_buildConfigurationModel;
    ProjectConfigurationModel m_deployConfigurationModel;
    ProjectConfigurationModel m_runConfigurationModel;

    bool m_shuttingDown = false;
};


Target::Target(Project *project, Kit *k, _constructor_tag) :
    QObject(project),
    d(std::make_unique<TargetPrivate>(this, k))
{
    // Note: nullptr is a valid state for the per-buildConfig systems.
    d->m_buildSystem = project->createBuildSystem(this);

    QTC_CHECK(d->m_kit);
    connect(DeviceManager::instance(), &DeviceManager::updated, this, &Target::updateDeviceState);

    connect(this, &Target::parsingStarted, this, [this, project] {
        emit project->anyParsingStarted(this);
    });

    connect(this, &Target::parsingFinished, this, [this, project](bool success) {
        if (success && this == ProjectManager::startupTarget())
            updateDefaultRunConfigurations();
        // For testing.
        emit ProjectManager::instance()->projectFinishedParsing(project);
        emit project->anyParsingFinished(this, success);
    }, Qt::QueuedConnection); // Must wait for run configs to change their enabled state.

    KitManager *km = KitManager::instance();
    connect(km, &KitManager::kitUpdated, this, &Target::handleKitUpdates);
    connect(km, &KitManager::kitRemoved, this, &Target::handleKitRemoval);

    d->m_macroExpander.setDisplayName(Tr::tr("Target Settings"));
    d->m_macroExpander.setAccumulating(true);

    d->m_macroExpander.registerSubProvider([this] { return kit()->macroExpander(); });

    d->m_macroExpander.registerVariable("sourceDir", Tr::tr("Source directory"),
            [project] { return project->projectDirectory().toUserOutput(); });
    d->m_macroExpander.registerVariable("BuildSystem:Name", Tr::tr("Build system"), [this] {
        if (const BuildSystem * const bs = buildSystem())
            return bs->name();
        return QString();
    });

    d->m_macroExpander.registerVariable("Project:Name",
            Tr::tr("Name of current project"),
            [project] { return project->displayName(); });
}

Target::~Target()
{
    qDeleteAll(d->m_buildConfigurations);
    qDeleteAll(d->m_deployConfigurations);
    qDeleteAll(d->m_runConfigurations);
}

void Target::handleKitUpdates(Kit *k)
{
    if (k != d->m_kit)
        return;

    updateDefaultDeployConfigurations();
    updateDeviceState(); // in case the device changed...

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

void Target::markAsShuttingDown()
{
    d->m_shuttingDown = true;
}

bool Target::isShuttingDown() const
{
    return d->m_shuttingDown;
}

Project *Target::project() const
{
    return static_cast<Project *>(parent());
}

Kit *Target::kit() const
{
    return d->m_kit;
}

BuildSystem *Target::buildSystem() const
{
    if (d->m_activeBuildConfiguration)
        return d->m_activeBuildConfiguration->buildSystem();

    return d->m_buildSystem;
}

BuildSystem *Target::fallbackBuildSystem() const
{
    return d->m_buildSystem;
}

DeploymentData Target::deploymentData() const
{
    const DeployConfiguration * const dc = activeDeployConfiguration();
    if (dc && dc->usesCustomDeploymentData())
        return dc->customDeploymentData();
    return buildSystemDeploymentData();
}

DeploymentData Target::buildSystemDeploymentData() const
{
    QTC_ASSERT(buildSystem(), return {});
    return buildSystem()->deploymentData();
}

BuildTargetInfo Target::buildTarget(const QString &buildKey) const
{
    QTC_ASSERT(buildSystem(), return {});
    return buildSystem()->buildTarget(buildKey);
}

QString Target::activeBuildKey() const
{
    // Should not happen. If it does, return a buildKey that wont be found in
    // the project tree, so that the project()->findNodeForBuildKey(buildKey)
    // returns null.
    QTC_ASSERT(d->m_activeRunConfiguration, return QString(QChar(0)));
    return d->m_activeRunConfiguration->buildKey();
}

void Target::setActiveBuildConfiguration(BuildConfiguration *bc, SetActive cascade)
{
    QTC_ASSERT(project(), return);

    if (project()->isShuttingDown() || isShuttingDown())
        return;

    setActiveBuildConfiguration(bc);

    if (!bc)
        return;
    if (cascade != SetActive::Cascade || !ProjectManager::isProjectConfigurationCascading())
        return;

    Id kitId = kit()->id();
    QString name = bc->displayName(); // We match on displayname
    for (Project *otherProject : ProjectManager::projects()) {
        if (otherProject == project())
            continue;
        Target *otherTarget = otherProject->activeTarget();
        if (!otherTarget || otherTarget->kit()->id() != kitId)
            continue;

        for (BuildConfiguration *otherBc : otherTarget->buildConfigurations()) {
            if (otherBc->displayName() == name) {
                otherTarget->setActiveBuildConfiguration(otherBc);
                break;
            }
        }
    }
}

void Target::setActiveDeployConfiguration(DeployConfiguration *dc, SetActive cascade)
{
    QTC_ASSERT(project(), return);

    if (project()->isShuttingDown() || isShuttingDown())
        return;

    setActiveDeployConfiguration(dc);

    if (!dc)
        return;
    if (cascade != SetActive::Cascade || !ProjectManager::isProjectConfigurationCascading())
        return;

    Id kitId = kit()->id();
    QString name = dc->displayName(); // We match on displayname
    for (Project *otherProject : ProjectManager::projects()) {
        if (otherProject == project())
            continue;
        Target *otherTarget = otherProject->activeTarget();
        if (!otherTarget || otherTarget->kit()->id() != kitId)
            continue;

        for (DeployConfiguration *otherDc : otherTarget->deployConfigurations()) {
            if (otherDc->displayName() == name) {
                otherTarget->setActiveDeployConfiguration(otherDc);
                break;
            }
        }
    }
}

Utils::Id Target::id() const
{
    return d->m_kit->id();
}

QString Target::displayName() const
{
    return d->m_kit->displayName();
}

QString Target::toolTip() const
{
    return d->m_kit->toHtml();
}

Key Target::displayNameKey()
{
    return "ProjectExplorer.ProjectConfiguration.DisplayName";
}

Key Target::deviceTypeKey()
{
    return "DeviceType";
}

void Target::addBuildConfiguration(BuildConfiguration *bc)
{
    QTC_ASSERT(bc && !d->m_buildConfigurations.contains(bc), return);
    Q_ASSERT(bc->target() == this);

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = bc->displayName();
    QStringList displayNames = Utils::transform(d->m_buildConfigurations, &BuildConfiguration::displayName);
    configurationDisplayName = Utils::makeUniquelyNumbered(configurationDisplayName, displayNames);
    if (configurationDisplayName != bc->displayName()) {
        if (bc->usesDefaultDisplayName())
            bc->setDefaultDisplayName(configurationDisplayName);
        else
            bc->setDisplayName(configurationDisplayName);
    }

    // add it
    d->m_buildConfigurations.push_back(bc);

    ProjectExplorerPlugin::targetSelector()->addedBuildConfiguration(bc);
    emit addedBuildConfiguration(bc);
    d->m_buildConfigurationModel.addProjectConfiguration(bc);

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

    if (activeBuildConfiguration() == bc) {
        if (d->m_buildConfigurations.isEmpty())
            setActiveBuildConfiguration(nullptr, SetActive::Cascade);
        else
            setActiveBuildConfiguration(d->m_buildConfigurations.at(0), SetActive::Cascade);
    }

    emit removedBuildConfiguration(bc);
    ProjectExplorerPlugin::targetSelector()->removedBuildConfiguration(bc);
    d->m_buildConfigurationModel.removeProjectConfiguration(bc);

    delete bc;
    return true;
}

const QList<BuildConfiguration *> Target::buildConfigurations() const
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
        emit activeBuildConfigurationChanged(d->m_activeBuildConfiguration);
        ProjectExplorerPlugin::updateActions();
    }
}

void Target::addDeployConfiguration(DeployConfiguration *dc)
{
    QTC_ASSERT(dc && !d->m_deployConfigurations.contains(dc), return);
    Q_ASSERT(dc->target() == this);

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = dc->displayName();
    QStringList displayNames = Utils::transform(d->m_deployConfigurations, &DeployConfiguration::displayName);
    configurationDisplayName = Utils::makeUniquelyNumbered(configurationDisplayName, displayNames);
    dc->setDisplayName(configurationDisplayName);

    // add it
    d->m_deployConfigurations.push_back(dc);

    ProjectExplorerPlugin::targetSelector()->addedDeployConfiguration(dc);
    d->m_deployConfigurationModel.addProjectConfiguration(dc);
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

    if (activeDeployConfiguration() == dc) {
        if (d->m_deployConfigurations.isEmpty())
            setActiveDeployConfiguration(nullptr, SetActive::Cascade);
        else
            setActiveDeployConfiguration(d->m_deployConfigurations.at(0), SetActive::Cascade);
    }

    ProjectExplorerPlugin::targetSelector()->removedDeployConfiguration(dc);
    d->m_deployConfigurationModel.removeProjectConfiguration(dc);
    emit removedDeployConfiguration(dc);

    delete dc;
    return true;
}

const QList<DeployConfiguration *> Target::deployConfigurations() const
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
    }
    updateDeviceState();
}

const QList<RunConfiguration *> Target::runConfigurations() const
{
    return d->m_runConfigurations;
}

void Target::addRunConfiguration(RunConfiguration *rc)
{
    QTC_ASSERT(rc && !d->m_runConfigurations.contains(rc), return);
    Q_ASSERT(rc->target() == this);

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = rc->displayName();
    if (!configurationDisplayName.isEmpty()) {
        QStringList displayNames = Utils::transform(d->m_runConfigurations,
                                                    &RunConfiguration::displayName);
        configurationDisplayName = Utils::makeUniquelyNumbered(configurationDisplayName,
                                                               displayNames);
        rc->setDisplayName(configurationDisplayName);
    }

    d->m_runConfigurations.push_back(rc);

    ProjectExplorerPlugin::targetSelector()->addedRunConfiguration(rc);
    d->m_runConfigurationModel.addProjectConfiguration(rc);
    emit addedRunConfiguration(rc);

    if (!activeRunConfiguration())
        setActiveRunConfiguration(rc);
}

void Target::removeRunConfiguration(RunConfiguration *rc)
{
    QTC_ASSERT(rc && d->m_runConfigurations.contains(rc), return);

    d->m_runConfigurations.removeOne(rc);

    if (activeRunConfiguration() == rc) {
        if (d->m_runConfigurations.isEmpty())
            setActiveRunConfiguration(nullptr);
        else
            setActiveRunConfiguration(d->m_runConfigurations.at(0));
    }

    emit removedRunConfiguration(rc);
    ProjectExplorerPlugin::targetSelector()->removedRunConfiguration(rc);
    d->m_runConfigurationModel.removeProjectConfiguration(rc);

    delete rc;
}

void Target::removeAllRunConfigurations()
{
    QList<RunConfiguration *> runConfigs = d->m_runConfigurations;
    d->m_runConfigurations.clear();
    setActiveRunConfiguration(nullptr);
    while (!runConfigs.isEmpty()) {
        RunConfiguration * const rc = runConfigs.takeFirst();
        emit removedRunConfiguration(rc);
        ProjectExplorerPlugin::targetSelector()->removedRunConfiguration(rc);
        d->m_runConfigurationModel.removeProjectConfiguration(rc);
        delete rc;
    }
}

RunConfiguration *Target::activeRunConfiguration() const
{
    return d->m_activeRunConfiguration;
}

void Target::setActiveRunConfiguration(RunConfiguration *rc)
{
    if (isShuttingDown())
        return;

    if ((!rc && d->m_runConfigurations.isEmpty()) ||
        (rc && d->m_runConfigurations.contains(rc) &&
         rc != d->m_activeRunConfiguration)) {
        d->m_activeRunConfiguration = rc;
        emit activeRunConfigurationChanged(d->m_activeRunConfiguration);
        ProjectExplorerPlugin::updateActions();
    }
    updateDeviceState();
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
    IDevice::ConstPtr current = DeviceKitAspect::device(kit());
    return current.isNull() ? QString() : formatDeviceInfo(current->deviceInformation());
}

Store Target::toMap() const
{
    if (!d->m_kit) // Kit was deleted, target is only around to be copied.
        return {};

    Store map;
    map.insert(displayNameKey(), displayName());
    map.insert(deviceTypeKey(), DeviceTypeKitAspect::deviceTypeId(kit()).toSetting());

    {
        // FIXME: For compatibility within the 4.11 cycle, remove this block later.
        // This is only read by older versions of Creator, but even there not actively used.
        const char CONFIGURATION_ID_KEY[] = "ProjectExplorer.ProjectConfiguration.Id";
        const char DEFAULT_DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DefaultDisplayName";
        map.insert(CONFIGURATION_ID_KEY, id().toSetting());
        map.insert(DEFAULT_DISPLAY_NAME_KEY, displayName());
    }

    const QList<BuildConfiguration *> bcs = buildConfigurations();
    map.insert(ACTIVE_BC_KEY, bcs.indexOf(d->m_activeBuildConfiguration));
    map.insert(BC_COUNT_KEY, bcs.size());
    for (int i = 0; i < bcs.size(); ++i) {
        Store data;
        bcs.at(i)->toMap(data);
        map.insert(numberedKey(BC_KEY_PREFIX, i), variantFromStore(data));
    }

    const QList<DeployConfiguration *> dcs = deployConfigurations();
    map.insert(ACTIVE_DC_KEY, dcs.indexOf(d->m_activeDeployConfiguration));
    map.insert(DC_COUNT_KEY, dcs.size());
    for (int i = 0; i < dcs.size(); ++i) {
        Store data;
        dcs.at(i)->toMap(data);
        map.insert(numberedKey(DC_KEY_PREFIX, i), variantFromStore(data));
    }

    const QList<RunConfiguration *> rcs = runConfigurations();
    map.insert(ACTIVE_RC_KEY, rcs.indexOf(d->m_activeRunConfiguration));
    map.insert(RC_COUNT_KEY, rcs.size());
    for (int i = 0; i < rcs.size(); ++i) {
        Store data;
        rcs.at(i)->toMap(data);
        map.insert(numberedKey(RC_KEY_PREFIX, i), variantFromStore(data));
    }

    if (!d->m_pluginSettings.isEmpty())
        map.insert(PLUGIN_SETTINGS_KEY, variantFromStore(d->m_pluginSettings));

    return map;
}

void Target::updateDefaultBuildConfigurations()
{
    BuildConfigurationFactory *bcFactory = BuildConfigurationFactory::find(this);
    if (!bcFactory) {
        qWarning("No build configuration factory found for target id '%s'.", qPrintable(id().toString()));
        return;
    }
    for (const BuildInfo &info : bcFactory->allAvailableSetups(kit(), project()->projectFilePath())) {
        if (BuildConfiguration *bc = bcFactory->create(this, info))
            addBuildConfiguration(bc);
    }
}

void Target::updateDefaultDeployConfigurations()
{
    const QList<DeployConfigurationFactory *> dcFactories = DeployConfigurationFactory::find(this);
    if (dcFactories.isEmpty()) {
        qWarning("No deployment configuration factory found for target id '%s'.", qPrintable(id().toString()));
        return;
    }

    QList<Utils::Id> dcIds;
    for (const DeployConfigurationFactory *dcFactory : dcFactories)
        dcIds.append(dcFactory->creationId());

    const QList<DeployConfiguration *> dcList = deployConfigurations();
    QList<Utils::Id> toCreate = dcIds;

    for (DeployConfiguration *dc : dcList) {
        if (dcIds.contains(dc->id()))
            toCreate.removeOne(dc->id());
        else
            removeDeployConfiguration(dc);
    }

    for (Utils::Id id : std::as_const(toCreate)) {
        for (DeployConfigurationFactory *dcFactory : dcFactories) {
            if (dcFactory->creationId() == id) {
                DeployConfiguration *dc = dcFactory->create(this);
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
    // Manual and Auto
    const QList<RunConfigurationCreationInfo> creators
            = RunConfigurationFactory::creatorsForTarget(this);

    if (creators.isEmpty()) {
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

    // Put outdated RCs into toRemove, do not bother with factories
    // that produce already existing RCs
    QList<RunConfiguration *> toRemove;
    QList<RunConfigurationCreationInfo> existing;
    for (RunConfiguration *rc : std::as_const(existingConfigured)) {
        bool present = false;
        for (const RunConfigurationCreationInfo &item : creators) {
            QString buildKey = rc->buildKey();
            if (item.factory->runConfigurationId() == rc->id() && item.buildKey == buildKey) {
                existing.append(item);
                present = true;
            }
        }
        if (!present && !rc->isCustomized())
            toRemove.append(rc);
    }
    configuredCount -= toRemove.count();

    bool removeExistingUnconfigured = false;
    if (ProjectExplorerPlugin::projectExplorerSettings().automaticallyCreateRunConfigurations) {
        // Create new "automatic" RCs and put them into newConfigured/newUnconfigured
        for (const RunConfigurationCreationInfo &item : creators) {
            if (item.creationMode == RunConfigurationCreationInfo::ManualCreationOnly)
                continue;
            bool exists = false;
            for (const RunConfigurationCreationInfo &ex : existing) {
                if (ex.factory == item.factory && ex.buildKey == item.buildKey)
                    exists = true;
            }
            if (exists)
                continue;

            RunConfiguration *rc = item.create(this);
            if (!rc)
                continue;
            QTC_CHECK(rc->id() == item.factory->runConfigurationId());
            if (!rc->isConfigured())
                newUnconfigured << rc;
            else
                newConfigured << rc;
        }
        configuredCount += newConfigured.count();

        // Decide what to do with the different categories:
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
    }

    // Do actual changes:
    for (RunConfiguration *rc : std::as_const(newConfigured))
        addRunConfiguration(rc);
    for (RunConfiguration *rc : std::as_const(newUnconfigured))
        addRunConfiguration(rc);

    // Generate complete list of RCs to remove later:
    QList<RunConfiguration *> removalList;
    for (RunConfiguration *rc : std::as_const(toRemove)) {
        removalList << rc;
        existingConfigured.removeOne(rc); // make sure to also remove them from existingConfigured!
    }

    if (removeExistingUnconfigured) {
        removalList.append(existingUnconfigured);
        existingUnconfigured.clear();
    }

    // Make sure a configured RC will be active after we delete the RCs:
    RunConfiguration *active = activeRunConfiguration();
    if (active && (removalList.contains(active) || !active->isEnabled())) {
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
    for (RunConfiguration *rc : std::as_const(removalList))
        removeRunConfiguration(rc);

    emit runConfigurationsUpdated();
}

QVariant Target::namedSettings(const Key &name) const
{
    return d->m_pluginSettings.value(name);
}

void Target::setNamedSettings(const Key &name, const QVariant &value)
{
    if (value.isNull())
        d->m_pluginSettings.remove(name);
    else
        d->m_pluginSettings.insert(name, value);
}

QVariant Target::additionalData(Utils::Id id) const
{
    if (const BuildSystem *bs = buildSystem())
        return bs->additionalData(id);

    return {};
}

MacroExpander *Target::macroExpander() const
{
    return &d->m_macroExpander;
}

ProjectConfigurationModel *Target::buildConfigurationModel() const
{
    return &d->m_buildConfigurationModel;
}

ProjectConfigurationModel *Target::deployConfigurationModel() const
{
    return &d->m_deployConfigurationModel;
}

ProjectConfigurationModel *Target::runConfigurationModel() const
{
    return &d->m_runConfigurationModel;
}

void Target::updateDeviceState()
{
    IDevice::ConstPtr current = DeviceKitAspect::device(kit());

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

bool Target::fromMap(const Store &map)
{
    QTC_ASSERT(d->m_kit == KitManager::kit(id()), return false);

    bool ok;
    int bcCount = map.value(BC_COUNT_KEY, 0).toInt(&ok);
    if (!ok || bcCount < 0)
        bcCount = 0;
    int activeConfiguration = map.value(ACTIVE_BC_KEY, 0).toInt(&ok);
    if (!ok || activeConfiguration < 0)
        activeConfiguration = 0;
    if (0 > activeConfiguration || bcCount < activeConfiguration)
        activeConfiguration = 0;

    for (int i = 0; i < bcCount; ++i) {
        const Key key = numberedKey(BC_KEY_PREFIX, i);
        if (!map.contains(key))
            return false;
        const Store valueMap = storeFromVariant(map.value(key));
        BuildConfiguration *bc = BuildConfigurationFactory::restore(this, valueMap);
        if (!bc) {
            qWarning("No factory found to restore build configuration!");
            continue;
        }
        QTC_CHECK(bc->id() == ProjectExplorer::idFromMap(valueMap));
        addBuildConfiguration(bc);
        if (i == activeConfiguration)
            setActiveBuildConfiguration(bc);
    }
    if (buildConfigurations().isEmpty() && BuildConfigurationFactory::find(this))
        return false;

    int dcCount = map.value(DC_COUNT_KEY, 0).toInt(&ok);
    if (!ok || dcCount < 0)
        dcCount = 0;
    activeConfiguration = map.value(ACTIVE_DC_KEY, 0).toInt(&ok);
    if (!ok || activeConfiguration < 0)
        activeConfiguration = 0;
    if (0 > activeConfiguration || dcCount < activeConfiguration)
        activeConfiguration = 0;

    for (int i = 0; i < dcCount; ++i) {
        const Key key = numberedKey(DC_KEY_PREFIX, i);
        if (!map.contains(key))
            return false;
        Store valueMap = storeFromVariant(map.value(key));
        DeployConfiguration *dc = DeployConfigurationFactory::restore(this, valueMap);
        if (!dc) {
            Utils::Id id = idFromMap(valueMap);
            qWarning("No factory found to restore deployment configuration of id '%s'!",
                     id.isValid() ? qPrintable(id.toString()) : "UNKNOWN");
            continue;
        }
        QTC_CHECK(dc->id() == ProjectExplorer::idFromMap(valueMap));
        addDeployConfiguration(dc);
        if (i == activeConfiguration)
            setActiveDeployConfiguration(dc);
    }

    int rcCount = map.value(RC_COUNT_KEY, 0).toInt(&ok);
    if (!ok || rcCount < 0)
        rcCount = 0;
    activeConfiguration = map.value(ACTIVE_RC_KEY, 0).toInt(&ok);
    if (!ok || activeConfiguration < 0)
        activeConfiguration = 0;
    if (0 > activeConfiguration || rcCount < activeConfiguration)
        activeConfiguration = 0;

    for (int i = 0; i < rcCount; ++i) {
        const Key key = numberedKey(RC_KEY_PREFIX, i);
        if (!map.contains(key))
            return false;

        // Ignore missing RCs: We will just populate them using the default ones.
        Store valueMap = storeFromVariant(map.value(key));
        RunConfiguration *rc = RunConfigurationFactory::restore(this, valueMap);
        if (!rc)
            continue;
        const Utils::Id theIdFromMap = ProjectExplorer::idFromMap(valueMap);
        if (!theIdFromMap.name().contains("///::///")) { // Hack for cmake 4.10 -> 4.11
            QTC_CHECK(rc->id().withSuffix(rc->buildKey()) == theIdFromMap);
        }
        addRunConfiguration(rc);
        if (i == activeConfiguration)
            setActiveRunConfiguration(rc);
    }

    if (map.contains(PLUGIN_SETTINGS_KEY))
        d->m_pluginSettings = storeFromVariant(map.value(PLUGIN_SETTINGS_KEY));

    return true;
}

} // namespace ProjectExplorer
