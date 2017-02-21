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

#include "autotestplugin.h"
#include "testconfiguration.h"

#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runnables.h>
#include <utils/qtcassert.h>
#include <debugger/debuggerrunconfigurationaspect.h>

#include <QCoreApplication>

namespace Autotest {
namespace Internal {

class TestRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(Autotest::Internal::TestRunConfiguration)

public:
    TestRunConfiguration(ProjectExplorer::Target *parent, TestConfiguration *config)
        : ProjectExplorer::RunConfiguration(parent, "AutoTest.TestRunConfig")
    {
        setDefaultDisplayName(tr("AutoTest Debug"));

        // disable QmlDebugger that is enabled by default
        // might change if debugging QuickTest gets enabled
        if (auto debugAspect = extraAspect<Debugger::DebuggerRunConfigurationAspect>())
            debugAspect->setUseQmlDebugger(false);
        m_testConfig = config;
    }

    ProjectExplorer::Runnable runnable() const override
    {
        ProjectExplorer::StandardRunnable r;
        QTC_ASSERT(m_testConfig, return r);
        r.executable = m_testConfig->executableFilePath();
        r.commandLineArguments = m_testConfig->argumentsForTestRunner().join(' ');
        r.workingDirectory = m_testConfig->workingDirectory();
        r.environment = m_testConfig->environment();
        r.runMode = ProjectExplorer::ApplicationLauncher::Gui;
        r.device = ProjectExplorer::DeviceManager::instance()->defaultDevice(
                    ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
        return r;
    }

private:
    QWidget *createConfigurationWidget() override { return 0; }
    TestConfiguration *m_testConfig = 0;
};

} // namespace Internal
} // namespace Autotest
