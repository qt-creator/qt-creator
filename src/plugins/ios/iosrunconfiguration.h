// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iosconstants.h"
#include "iosconfigurations.h"
#include "iossimulator.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <utils/fileutils.h>

#include <QComboBox>
#include <QStandardItemModel>

namespace Ios::Internal {

class IosRunConfiguration;

class IosDeviceTypeAspect : public Utils::BaseAspect
{
    Q_OBJECT

public:
    explicit IosDeviceTypeAspect(Utils::AspectContainer *container,
                                 IosRunConfiguration *runConfiguration);

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;
    void addToLayout(Layouting::LayoutItem &parent) override;

    IosDeviceType deviceType() const;
    void setDeviceType(const IosDeviceType &deviceType);

    void updateValues();
    void setDeviceTypeIndex(int devIndex);
    void deviceChanges();
    void updateDeviceType();

    class Data : public Utils::BaseAspect::Data
    {
    public:
        Utils::FilePath bundleDirectory;
        IosDeviceType deviceType;
        QString applicationName;
        Utils::FilePath localExecutable;
    };

private:
    Utils::FilePath bundleDirectory() const;
    QString applicationName() const;
    Utils::FilePath localExecutable() const;

    IosDeviceType m_deviceType;
    IosRunConfiguration *m_runConfiguration = nullptr;
    QStandardItemModel m_deviceTypeModel;
    QLabel *m_deviceTypeLabel = nullptr;
    QComboBox *m_deviceTypeComboBox = nullptr;
};

class IosRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT // FIXME: Used in  IosDsymBuildStep

public:
    IosRunConfiguration(ProjectExplorer::Target *target, Utils::Id id);

    QString applicationName() const;
    Utils::FilePath bundleDirectory() const;
    Utils::FilePath localExecutable() const;
    QString disabledReason() const override;
    IosDeviceType deviceType() const;

private:
    bool isEnabled() const final;

    ProjectExplorer::ExecutableAspect executable{this};
    ProjectExplorer::ArgumentsAspect arguments{this};
    IosDeviceTypeAspect iosDeviceType;
};

class IosRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory
{
public:
    IosRunConfigurationFactory();
};

} // Ios::Internal
