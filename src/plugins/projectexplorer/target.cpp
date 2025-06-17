// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "target.h"

#include "buildconfiguration.h"
#include "buildinfo.h"
#include "buildmanager.h"
#include "buildsystem.h"
#include "deployconfiguration.h"
#include "devicesupport/devicekitaspects.h"
#include "devicesupport/devicemanager.h"
#include "kit.h"
#include "kitmanager.h"
#include "miniprojecttargetselector.h"
#include "project.h"
#include "projectconfigurationmodel.h"
#include "projectexplorer.h"
#include "projectexplorericons.h"
#include "projectmanager.h"
#include "runconfiguration.h"

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QIcon>

using namespace Utils;

namespace ProjectExplorer {

const char ACTIVE_BC_KEY[] = "ProjectExplorer.Target.ActiveBuildConfiguration";
const char BC_KEY_PREFIX[] = "ProjectExplorer.Target.BuildConfiguration.";
const char BC_COUNT_KEY[] = "ProjectExplorer.Target.BuildConfigurationCount";

const char HAS_PER_BC_DCS[] = "HasPerBcDcs";

static QString formatDeviceInfo(const IDevice::DeviceInfo &input)
{
    const QStringList lines
            = Utils::transform(input, [](const IDevice::DeviceInfoItem &i) {
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
    TargetPrivate(Kit *k)
        : m_kit(k)
    { }

    QIcon m_overlayIcon;
    QList<BuildConfiguration *> m_buildConfigurations;
    QPointer<BuildConfiguration> m_activeBuildConfiguration;
    Kit *const m_kit;
    ProjectConfigurationModel m_buildConfigurationModel;
    bool m_shuttingDown = false;
};


std::unique_ptr<Target> Target::create(Project *parent, Kit *k)
{
    return std::unique_ptr<Target>(new Target(parent, k));
}

Target::Target(Project *project, Kit *k) :
    QObject(project),
    d(std::make_unique<TargetPrivate>(k))
{
    QTC_CHECK(d->m_kit);
    connect(DeviceManager::instance(), &DeviceManager::updated, this, &Target::updateDeviceState);
    KitManager *km = KitManager::instance();
    connect(km, &KitManager::kitUpdated, this, &Target::handleKitUpdates);
    connect(km, &KitManager::kitRemoved, this, &Target::handleKitRemoval);

}

Target::~Target()
{
    qDeleteAll(d->m_buildConfigurations);
}

void Target::handleKitUpdates(Kit *k)
{
    if (k != d->m_kit)
        return;

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
    QTC_ASSERT(d->m_activeBuildConfiguration, return nullptr);
    return d->m_activeBuildConfiguration->buildSystem();
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

    bc->updateDefaultDeployConfigurations();

    // add it
    d->m_buildConfigurations.push_back(bc);

    ProjectExplorerPlugin::targetSelector()->addedBuildConfiguration(bc);
    emit addedBuildConfiguration(bc);
    d->m_buildConfigurationModel.addProjectConfiguration(bc);

    if (!activeBuildConfiguration())
        setActiveBuildConfiguration(bc);

    emit ProjectManager::instance()->buildConfigurationAdded(bc);
}

bool Target::removeBuildConfiguration(BuildConfiguration *bc)
{
    //todo: this might be error prone
    if (!d->m_buildConfigurations.contains(bc))
        return false;

    if (BuildManager::isBuilding(bc))
        return false;

    emit ProjectManager::instance()->aboutToRemoveBuildConfiguration(bc);

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

    emit ProjectManager::instance()->buildConfigurationRemoved(bc);

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

DeployConfiguration *Target::activeDeployConfiguration() const
{
    QTC_ASSERT(activeBuildConfiguration(), return nullptr);
    return activeBuildConfiguration()->activeDeployConfiguration();
}

RunConfiguration *Target::activeRunConfiguration() const
{
    QTC_ASSERT(activeBuildConfiguration(), return nullptr);
    return activeBuildConfiguration()->activeRunConfiguration();
}

void Target::setActiveBuildConfiguration(BuildConfiguration *bc)
{
    if ((!bc && d->m_buildConfigurations.isEmpty()) ||
        (bc && d->m_buildConfigurations.contains(bc) &&
         bc != d->m_activeBuildConfiguration)) {
        d->m_activeBuildConfiguration = bc;
        emit activeBuildConfigurationChanged(bc);
        if (this == project()->activeTarget())
            emit project()->activeBuildConfigurationChanged(bc);
        if (bc == activeBuildConfigForActiveProject())
            emit ProjectManager::instance()->activeBuildConfigurationChanged(bc);
        if (bc == activeBuildConfigForCurrentProject())
            emit ProjectManager::instance()->currentBuildConfigurationChanged(bc);
        ProjectExplorerPlugin::updateActions();
    }
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
}

QString Target::overlayIconToolTip()
{
    IDevice::ConstPtr current = RunDeviceKitAspect::device(kit());
    return current ? formatDeviceInfo(current->deviceInformation()) : QString();
}

Store Target::toMap() const
{
    if (!d->m_kit) // Kit was deleted, target is only around to be copied.
        return {};

    Store map;
    map.insert(displayNameKey(), displayName());
    map.insert(deviceTypeKey(), RunDeviceTypeKitAspect::deviceTypeId(kit()).toSetting());

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

    // Forward compatibility for Qt Creator < 17: Store the active build configuration's
    // deploy and run configurations as the target-global ones. A special tag signifies that
    // we should not read these ourselves.
    map.insert(HAS_PER_BC_DCS, true);
    if (QTC_GUARD(d->m_activeBuildConfiguration))
        d->m_activeBuildConfiguration->storeConfigurationsToMap(map);

    return map;
}

void Target::updateDefaultBuildConfigurations()
{
    BuildConfigurationFactory * bcFactory = BuildConfigurationFactory::find(this);
    if (!bcFactory) {
        qWarning("No build configuration factory found for target id '%s'.", qPrintable(id().toString()));
        return;
    }
    for (const BuildInfo &info : Utils::filtered(
             bcFactory->allAvailableSetups(kit(), project()->projectFilePath()),
             &BuildInfo::enabledByDefault)) {
        if (BuildConfiguration *bc = bcFactory->create(this, info))
            addBuildConfiguration(bc);
    }
    QTC_CHECK(!d->m_buildConfigurations.isEmpty());
}

void Target::updateDefaultRunConfigurations()
{
    for (BuildConfiguration * const bc : std::as_const(d->m_buildConfigurations))
        bc->updateDefaultRunConfigurations();
}

ProjectConfigurationModel *Target::buildConfigurationModel() const
{
    return &d->m_buildConfigurationModel;
}

void Target::updateDeviceState()
{
    IDevice::ConstPtr current = RunDeviceKitAspect::device(kit());

    QIcon overlay;
    static const QIcon disconnected = Icons::DEVICE_DISCONNECTED_INDICATOR_OVERLAY.icon();
    if (!current) {
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

    if (!addConfigurationsFromMap(map, /*setActiveConfigurations=*/true))
        return false;

    return true;
}

bool Target::addConfigurationsFromMap(const Utils::Store &map, bool setActiveConfigurations)
{
    bool ok;
    int bcCount = map.value(BC_COUNT_KEY, 0).toInt(&ok);
    if (!ok || bcCount < 0)
        bcCount = 0;
    int activeConfiguration = map.value(ACTIVE_BC_KEY, 0).toInt(&ok);
    if (!ok || 0 > activeConfiguration || bcCount < activeConfiguration)
        activeConfiguration = 0;
    if (!setActiveConfigurations)
        activeConfiguration = -1;

    const bool hasDeployConfigsPerBuildConfig = map.value(HAS_PER_BC_DCS).toBool();
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

        // Pre-17 backward compatibility: Give each build config the formerly target-global
        // configurations.
        if (!hasDeployConfigsPerBuildConfig) {
            bc->setExtraDataFromMap(map);
            if (!bc->addConfigurationsFromMap(map, true))
                return false;
        }

        addBuildConfiguration(bc);
        if (i == activeConfiguration)
            setActiveBuildConfiguration(bc);
    }
    return true;
}

} // namespace ProjectExplorer
