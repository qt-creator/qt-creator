// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runcontrol.h>

namespace Qdb::Internal {

class QdbRunWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    explicit QdbRunWorkerFactory(const QList<Utils::Id> &runConfigs);
};

class QdbDebugWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    explicit QdbDebugWorkerFactory(const QList<Utils::Id> &runConfigs);
};

class QdbQmlToolingWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    explicit QdbQmlToolingWorkerFactory(const QList<Utils::Id> &runConfigs);
};

class QdbPerfProfilerWorkerFactory final : public ProjectExplorer::RunWorkerFactory
{
public:
    QdbPerfProfilerWorkerFactory();
};

} // Qdb::Internal
