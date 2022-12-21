// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "qmakeprojectmanager/qmakebuildconfiguration.h"
#include <cmakeprojectmanager/cmakebuildconfiguration.h>
#include <utils/aspects.h>

namespace Ios {
namespace Internal {

class IosQmakeBuildConfiguration : public QmakeProjectManager::QmakeBuildConfiguration
{
    Q_OBJECT

public:
    IosQmakeBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);

private:
    QList<ProjectExplorer::NamedWidget *> createSubConfigWidgets() override;
    bool fromMap(const QVariantMap &map) override;

    void updateQmakeCommand();

    Utils::StringAspect *m_signingIdentifier = nullptr;
    Utils::BoolAspect *m_autoManagedSigning = nullptr;
};

class IosQmakeBuildConfigurationFactory : public QmakeProjectManager::QmakeBuildConfigurationFactory
{
public:
    IosQmakeBuildConfigurationFactory();
};

class IosCMakeBuildConfiguration : public CMakeProjectManager::CMakeBuildConfiguration
{
    Q_OBJECT

public:
    IosCMakeBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);

private:
    QList<ProjectExplorer::NamedWidget *> createSubConfigWidgets() override;
    bool fromMap(const QVariantMap &map) override;

    CMakeProjectManager::CMakeConfig signingFlags() const final;

    Utils::StringAspect *m_signingIdentifier = nullptr;
    Utils::BoolAspect *m_autoManagedSigning = nullptr;
};

class IosCMakeBuildConfigurationFactory : public CMakeProjectManager::CMakeBuildConfigurationFactory
{
public:
    IosCMakeBuildConfigurationFactory();
};

} // namespace Internal
} // namespace Ios
