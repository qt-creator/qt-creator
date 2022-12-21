// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "runcontrol.h"

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT CustomExecutableRunConfiguration : public RunConfiguration
{
    Q_OBJECT

public:
    CustomExecutableRunConfiguration(Target *target, Utils::Id id);
    explicit CustomExecutableRunConfiguration(Target *target);

    QString defaultDisplayName() const;

private:
    Runnable runnable() const override;
    bool isEnabled() const override;
    Tasks checkForIssues() const override;

    void configurationDialogFinished();
    Utils::FilePath executable() const;
};

class CustomExecutableRunConfigurationFactory : public FixedRunConfigurationFactory
{
public:
    CustomExecutableRunConfigurationFactory();
};

class CustomExecutableRunWorkerFactory : public RunWorkerFactory
{
public:
    CustomExecutableRunWorkerFactory();
};

} // namespace ProjectExplorer
