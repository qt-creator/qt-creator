// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "testconfiguration.h"

#include "autotesttr.h"

#include <debugger/debuggerrunconfigurationaspect.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>

#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

class TestRunConfiguration : public ProjectExplorer::RunConfiguration
{
public:
    TestRunConfiguration(ProjectExplorer::Target *parent, TestConfiguration *config)
        : ProjectExplorer::RunConfiguration(parent, "AutoTest.TestRunConfig")
    {
        setDefaultDisplayName(Tr::tr("AutoTest Debug"));

        bool enableQuick = false;
        if (auto debuggable = dynamic_cast<DebuggableTestConfiguration *>(config))
            enableQuick = debuggable->mixedDebugging();

        auto debugAspect = addAspect<Debugger::DebuggerRunConfigurationAspect>(parent);
        debugAspect->setUseQmlDebugger(enableQuick);
        ProjectExplorer::ProjectExplorerPlugin::updateRunActions();
        m_testConfig = config;
    }

    ProjectExplorer::Runnable runnable() const override
    {
        ProjectExplorer::Runnable r;
        QTC_ASSERT(m_testConfig, return r);
        r.command.setExecutable(m_testConfig->executableFilePath());
        r.command.addArgs(m_testConfig->argumentsForTestRunner().join(' '), Utils::CommandLine::Raw);
        r.workingDirectory = m_testConfig->workingDirectory();
        r.environment = m_testConfig->environment();
        return r;
    }

private:
    TestConfiguration *m_testConfig = nullptr;
};

} // namespace Internal
} // namespace Autotest
