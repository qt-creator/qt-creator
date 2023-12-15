// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/runconfiguration.h>

namespace AppManager {
namespace Internal {

class AppManagerDeployConfigurationAutoSwitcherPrivate;

class AppManagerDeployConfigurationAutoSwitcher : public QObject
{
    Q_OBJECT

    QScopedPointer<AppManagerDeployConfigurationAutoSwitcherPrivate> d;

public:
    AppManagerDeployConfigurationAutoSwitcher(QObject *parent = nullptr);
    ~AppManagerDeployConfigurationAutoSwitcher() override;

    void initialize();

private:
    void onActiveDeployConfigurationChanged(ProjectExplorer::DeployConfiguration *dc);
    void onActiveRunConfigurationChanged(ProjectExplorer::RunConfiguration *rc);
    void onActiveTargetChanged(ProjectExplorer::Target *target);
    void onStartupProjectChanged(ProjectExplorer::Project *project);
};

} // namespace Internal
} // namespace AppManager
