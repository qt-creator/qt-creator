// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <cmakeprojectmanager/cmakebuildconfiguration.h>

namespace Ios::Internal {

class IosQmakeBuildConfigurationFactory : public QmakeProjectManager::QmakeBuildConfigurationFactory
{
public:
    IosQmakeBuildConfigurationFactory();
};

class IosCMakeBuildConfigurationFactory : public CMakeProjectManager::CMakeBuildConfigurationFactory
{
public:
    IosCMakeBuildConfigurationFactory();
};

} // Ios::Internal
