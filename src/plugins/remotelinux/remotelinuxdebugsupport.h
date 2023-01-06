// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

namespace RemoteLinux::Internal {

class RemoteLinuxRunWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    explicit RemoteLinuxRunWorkerFactory(const QList<Utils::Id> &runConfigs);
};

class RemoteLinuxDebugWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    explicit RemoteLinuxDebugWorkerFactory(const QList<Utils::Id> &runConfigs);
};

} // RemoteLinux::Internal
