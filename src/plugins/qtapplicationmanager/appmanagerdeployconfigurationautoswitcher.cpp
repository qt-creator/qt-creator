// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerdeployconfigurationautoswitcher.h"

#include "appmanagerconstants.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace AppManager {
namespace Internal {

class AppManagerDeployConfigurationAutoSwitcherPrivate
{
public:
    Project *project = nullptr;
    Target *target = nullptr;
    RunConfiguration *runConfiguration = nullptr;
    DeployConfiguration *deployConfiguration = nullptr;
    QHash<RunConfiguration*,DeployConfiguration*> deployConfigurationsUsageHistory;
};

AppManagerDeployConfigurationAutoSwitcher::AppManagerDeployConfigurationAutoSwitcher(QObject *parent)
    : QObject(parent)
    , d(new AppManagerDeployConfigurationAutoSwitcherPrivate())
{
}

AppManagerDeployConfigurationAutoSwitcher::~AppManagerDeployConfigurationAutoSwitcher() = default;

void AppManagerDeployConfigurationAutoSwitcher::onActiveDeployConfigurationChanged(DeployConfiguration *deployConfiguration)
{
    if (d->deployConfiguration != deployConfiguration) {
        d->deployConfiguration = deployConfiguration;
        if (deployConfiguration && deployConfiguration->target()) {
            if (auto runConfiguration = deployConfiguration->target()->activeRunConfiguration()) {
                d->deployConfigurationsUsageHistory.insert(runConfiguration, deployConfiguration);
            }
        }
    }
}

static bool isApplicationManagerRunConfiguration(const RunConfiguration *runConfiguration)
{
    return runConfiguration && runConfiguration->id() == Constants::RUNCONFIGURATION_ID;
}

static bool isApplicationManagerDeployConfiguration(const DeployConfiguration *deployConfiguration)
{
    return deployConfiguration && deployConfiguration->id() == Constants::DEPLOYCONFIGURATION_ID;
}

void AppManagerDeployConfigurationAutoSwitcher::onActiveRunConfigurationChanged(RunConfiguration *runConfiguration)
{
    if (d->runConfiguration != runConfiguration) {
        d->runConfiguration = runConfiguration;
        if (runConfiguration) {
            if (auto target = runConfiguration->target()) {
                const auto stored = d->deployConfigurationsUsageHistory.contains(runConfiguration);
                if (stored) {
                    // deploy selection stored -> restore
                    auto deployConfiguration = d->deployConfigurationsUsageHistory.value(runConfiguration, nullptr);
                    target->setActiveDeployConfiguration(deployConfiguration, SetActive::NoCascade);
                } else if (auto activeDeployConfiguration = target->activeDeployConfiguration()) {
                    // active deploy configuration exists
                    if (isApplicationManagerRunConfiguration(runConfiguration)) {
                        // current run configuration is AM
                        if (!isApplicationManagerDeployConfiguration(activeDeployConfiguration)) {
                            // current deploy configuration is not AM
                            for (auto deployConfiguration : target->deployConfigurations()) {
                                // find AM deploy configuration
                                if (isApplicationManagerDeployConfiguration(deployConfiguration)) {
                                    // make it active
                                    target->setActiveDeployConfiguration(deployConfiguration, SetActive::NoCascade);
                                    break;
                                }
                            }
                        }
                    } else {
                        // current run configuration is not AM
                        if (isApplicationManagerDeployConfiguration(activeDeployConfiguration)) {
                            // current deploy configuration is AM
                            for (auto deployConfiguration : target->deployConfigurations()) {
                                // find not AM deploy configuration
                                if (!isApplicationManagerDeployConfiguration(deployConfiguration)) {
                                    // make it active
                                    target->setActiveDeployConfiguration(deployConfiguration, SetActive::NoCascade);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void AppManagerDeployConfigurationAutoSwitcher::onActiveTargetChanged(Target *target)
{
    if (d->target != target) {
        if (d->target) {
            disconnect(d->target, nullptr, this, nullptr);
        }
        d->target = target;
        if (target) {
            connect(target, &Target::activeRunConfigurationChanged,
                    this, &AppManagerDeployConfigurationAutoSwitcher::onActiveRunConfigurationChanged);
            connect(target, &Target::activeDeployConfigurationChanged,
                    this, &AppManagerDeployConfigurationAutoSwitcher::onActiveDeployConfigurationChanged);
        }
        onActiveRunConfigurationChanged(target ? target->activeRunConfiguration() : nullptr);
        onActiveDeployConfigurationChanged(target ? target->activeDeployConfiguration() : nullptr);
    }
}

void AppManagerDeployConfigurationAutoSwitcher::onStartupProjectChanged(Project *project)
{
    if (d->project != project) {
        if (d->project) {
            disconnect(d->project, nullptr, this, nullptr);
        }
        d->project = project;
        if (project) {
            connect(project, &Project::activeTargetChanged,
                    this, &AppManagerDeployConfigurationAutoSwitcher::onActiveTargetChanged);
        }
        onActiveTargetChanged(project ? project->activeTarget() : nullptr);
    }
}

void AppManagerDeployConfigurationAutoSwitcher::initialize()
{
    if (auto projectManager = ProjectManager::instance()) {
        connect(projectManager, &ProjectManager::startupProjectChanged,
                this, &AppManagerDeployConfigurationAutoSwitcher::onStartupProjectChanged, Qt::UniqueConnection);
        onStartupProjectChanged(projectManager->startupProject());
    }
}

} // namespace Internal
} // namespace AppManager
