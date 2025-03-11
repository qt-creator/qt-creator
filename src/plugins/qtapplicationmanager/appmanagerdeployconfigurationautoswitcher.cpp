// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerdeployconfigurationautoswitcher.h"

#include "appmanagerconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace AppManager::Internal {

class AppManagerDeployConfigurationAutoSwitcher final : public QObject
{
public:
    AppManagerDeployConfigurationAutoSwitcher();

private:
    void onActiveDeployConfigurationChanged(DeployConfiguration *dc);
    void onActiveRunConfigurationChanged(RunConfiguration *rc);
    void onActiveBuildConfigChanged(BuildConfiguration *bc);

    BuildConfiguration *m_buildConfiguration = nullptr;
    RunConfiguration *m_runConfiguration = nullptr;
    DeployConfiguration *m_deployConfiguration = nullptr;
    QHash<RunConfiguration *, DeployConfiguration *> m_deployConfigurationsUsageHistory;
};

AppManagerDeployConfigurationAutoSwitcher::AppManagerDeployConfigurationAutoSwitcher()
{
    connect(ProjectManager::instance(), &ProjectManager::activeBuildConfigurationChanged,
            this, &AppManagerDeployConfigurationAutoSwitcher::onActiveBuildConfigChanged,
            Qt::UniqueConnection);
    onActiveBuildConfigChanged(activeBuildConfigForActiveProject());
}

void AppManagerDeployConfigurationAutoSwitcher::onActiveDeployConfigurationChanged(DeployConfiguration *deployConfiguration)
{
    if (m_deployConfiguration != deployConfiguration) {
        m_deployConfiguration = deployConfiguration;
        if (deployConfiguration) {
            if (auto runConfiguration = deployConfiguration->buildConfiguration()->activeRunConfiguration()) {
                m_deployConfigurationsUsageHistory.insert(runConfiguration, deployConfiguration);
            }
        }
    }
}

static bool isApplicationManagerRunConfiguration(const RunConfiguration *runConfiguration)
{
    return runConfiguration && (runConfiguration->id() == Constants::RUNCONFIGURATION_ID ||
                                runConfiguration->id() == Constants::RUNANDDEBUGCONFIGURATION_ID);
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
            if (BuildConfiguration * const bc = runConfiguration->buildConfiguration()) {
                const auto stored = m_deployConfigurationsUsageHistory.contains(runConfiguration);
                if (stored) {
                    // deploy selection stored -> restore
                    auto deployConfiguration = m_deployConfigurationsUsageHistory.value(runConfiguration, nullptr);
                    bc->setActiveDeployConfiguration(deployConfiguration);
                } else if (auto activeDeployConfiguration = bc->activeDeployConfiguration()) {
                    // active deploy configuration exists
                    if (isApplicationManagerRunConfiguration(runConfiguration)) {
                        // current run configuration is AM
                        if (!isApplicationManagerDeployConfiguration(activeDeployConfiguration)) {
                            // current deploy configuration is not AM
                            for (auto deployConfiguration : bc->deployConfigurations()) {
                                // find AM deploy configuration
                                if (isApplicationManagerDeployConfiguration(deployConfiguration)) {
                                    // make it active
                                    bc->setActiveDeployConfiguration(deployConfiguration);
                                    break;
                                }
                            }
                        }
                    } else {
                        // current run configuration is not AM
                        if (isApplicationManagerDeployConfiguration(activeDeployConfiguration)) {
                            // current deploy configuration is AM
                            for (auto deployConfiguration : bc->deployConfigurations()) {
                                // find not AM deploy configuration
                                if (!isApplicationManagerDeployConfiguration(deployConfiguration)) {
                                    // make it active
                                    bc->setActiveDeployConfiguration(deployConfiguration);
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

void AppManagerDeployConfigurationAutoSwitcher::onActiveBuildConfigChanged(BuildConfiguration *bc)
{
    if (m_buildConfiguration != bc) {
        if (m_buildConfiguration) {
            disconnect(m_buildConfiguration, nullptr, this, nullptr);
        }
        m_buildConfiguration = bc;
        if (bc) {
            connect(bc, &BuildConfiguration::activeRunConfigurationChanged,
                    this, &AppManagerDeployConfigurationAutoSwitcher::onActiveRunConfigurationChanged);
            connect(bc, &BuildConfiguration::activeDeployConfigurationChanged,
                    this, &AppManagerDeployConfigurationAutoSwitcher::onActiveDeployConfigurationChanged);
        }
        onActiveRunConfigurationChanged(bc ? bc->activeRunConfiguration() : nullptr);
        onActiveDeployConfigurationChanged(bc ? bc->activeDeployConfiguration() : nullptr);
    }
}

void setupAppManagerDeployConfigurationAutoSwitcher()
{
    static AppManagerDeployConfigurationAutoSwitcher theAppManagerDeployConfigurationAutoSwitcher;
}

} // AppManager::Internal
