/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
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
