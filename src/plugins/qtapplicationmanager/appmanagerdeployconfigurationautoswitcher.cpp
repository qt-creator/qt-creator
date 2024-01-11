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

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace AppManager::Internal {

class AppManagerDeployConfigurationAutoSwitcher final : public QObject
{
public:
    AppManagerDeployConfigurationAutoSwitcher();

private:
    void onActiveDeployConfigurationChanged(DeployConfiguration *dc);
    void onActiveRunConfigurationChanged(RunConfiguration *rc);
    void onActiveTargetChanged(Target *target);
    void onStartupProjectChanged(Project *project);

    Project *m_project = nullptr;
    Target *m_target = nullptr;
    RunConfiguration *m_runConfiguration = nullptr;
    DeployConfiguration *m_deployConfiguration = nullptr;
    QHash<RunConfiguration *, DeployConfiguration *> m_deployConfigurationsUsageHistory;
};

AppManagerDeployConfigurationAutoSwitcher::AppManagerDeployConfigurationAutoSwitcher()
{
    ProjectManager *projectManager = ProjectManager::instance();
    QTC_ASSERT(projectManager, return);

    connect(projectManager, &ProjectManager::startupProjectChanged,
            this, &AppManagerDeployConfigurationAutoSwitcher::onStartupProjectChanged, Qt::UniqueConnection);
    onStartupProjectChanged(projectManager->startupProject());
}

void AppManagerDeployConfigurationAutoSwitcher::onActiveDeployConfigurationChanged(DeployConfiguration *deployConfiguration)
{
    if (m_deployConfiguration != deployConfiguration) {
        m_deployConfiguration = deployConfiguration;
        if (deployConfiguration && deployConfiguration->target()) {
            if (auto runConfiguration = deployConfiguration->target()->activeRunConfiguration()) {
                m_deployConfigurationsUsageHistory.insert(runConfiguration, deployConfiguration);
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
    if (m_runConfiguration != runConfiguration) {
        m_runConfiguration = runConfiguration;
        if (runConfiguration) {
            if (auto target = runConfiguration->target()) {
                const auto stored = m_deployConfigurationsUsageHistory.contains(runConfiguration);
                if (stored) {
                    // deploy selection stored -> restore
                    auto deployConfiguration = m_deployConfigurationsUsageHistory.value(runConfiguration, nullptr);
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
    if (m_target != target) {
        if (m_target) {
            disconnect(m_target, nullptr, this, nullptr);
        }
        m_target = target;
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
    if (m_project != project) {
        if (m_project) {
            disconnect(m_project, nullptr, this, nullptr);
        }
        m_project = project;
        if (project) {
            connect(project, &Project::activeTargetChanged,
                    this, &AppManagerDeployConfigurationAutoSwitcher::onActiveTargetChanged);
        }
        onActiveTargetChanged(project ? project->activeTarget() : nullptr);
    }
}

void setupAppManagerDeployConfigurationAutoSwitcher()
{
    static AppManagerDeployConfigurationAutoSwitcher theAppManagerDeployConfigurationAutoSwitcher;
}

} // AppManager::Internal
