// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debuggerruncontrol.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

namespace Qdb {

class QdbDeviceInferiorRunner;

class QdbDeviceDebugSupport : public Debugger::DebuggerRunTool
{
public:
    QdbDeviceDebugSupport(ProjectExplorer::RunControl *runControl);

private:
    void start();
    void stop();

    QdbDeviceInferiorRunner *m_debuggee = nullptr;
};

class QdbDeviceQmlToolingSupport : public ProjectExplorer::RunWorker
{
public:
    QdbDeviceQmlToolingSupport(ProjectExplorer::RunControl *runControl);

private:
    void start() override;

    QdbDeviceInferiorRunner *m_runner = nullptr;
    ProjectExplorer::RunWorker *m_worker = nullptr;
};

class QdbDevicePerfProfilerSupport : public ProjectExplorer::RunWorker
{
public:
    QdbDevicePerfProfilerSupport(ProjectExplorer::RunControl *runControl);

private:
    void start() override;

    QdbDeviceInferiorRunner *m_profilee = nullptr;
};

} // namespace Qdb
