// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfigurationaspects.h>

namespace AppManager {
namespace Internal {

class AppManagerRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT

public:
    AppManagerRunConfiguration(ProjectExplorer::Target *target, Utils::Id id);

    QString disabledReason() const override;
};

class AppManagerRunConfigurationFactoryPrivate;

class AppManagerRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory
{
    QScopedPointer<AppManagerRunConfigurationFactoryPrivate> d;

public:
    AppManagerRunConfigurationFactory();
    ~AppManagerRunConfigurationFactory() override;

protected:
    QList<ProjectExplorer::RunConfigurationCreationInfo> availableCreators(ProjectExplorer::Target *parent) const override;
};

} // namespace Internal
} // namespace AppManager
