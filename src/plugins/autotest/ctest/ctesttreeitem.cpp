// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctesttreeitem.h"

#include "ctestconfiguration.h"
#include "ctesttool.h"

#include "../autotestplugin.h"
#include "../testsettings.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <utils/link.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace Autotest {
namespace Internal {

CTestTreeItem::CTestTreeItem(ITestBase *testBase, const QString &name,
                             const FilePath &filepath, Type type)
    : ITestTreeItem(testBase, name, filepath, type)
{
}

QList<ITestConfiguration *> CTestTreeItem::getAllTestConfigurations() const
{
    return testConfigurationsFor({});
}

QList<ITestConfiguration *> CTestTreeItem::getSelectedTestConfigurations() const
{
    QStringList selectedTests;
    forFirstLevelChildren([&selectedTests](ITestTreeItem *it) {
        if (it->checked())
            selectedTests.append(it->name());
    });

    return selectedTests.isEmpty() ? QList<ITestConfiguration *>()
                                   : testConfigurationsFor(selectedTests);
}

QList<ITestConfiguration *> CTestTreeItem::getFailedTestConfigurations() const
{
    QStringList selectedTests;
    forFirstLevelChildren([&selectedTests](ITestTreeItem *it) {
        if (it->data(0, FailedRole).toBool())
            selectedTests.append(it->name());
    });

    return selectedTests.isEmpty() ? QList<ITestConfiguration *>()
                                   : testConfigurationsFor(selectedTests);
}

ITestConfiguration *CTestTreeItem::testConfiguration() const
{
    return testConfigurationsFor({name()}).value(0);
}

QVariant CTestTreeItem::data(int column, int role) const
{
    if (role == Qt::CheckStateRole)
        return checked();
    if (role == LinkRole) {
        QVariant itemLink;
        itemLink.setValue(Link(filePath(), line()));
        return itemLink;
    }
    return ITestTreeItem::data(column, role);
}

QList<ITestConfiguration *> CTestTreeItem::testConfigurationsFor(const QStringList &selected) const
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (!project)
        return {};

    const ProjectExplorer::Target *target = ProjectExplorer::ProjectManager::startupTarget();
    if (!target)
        return {};

    const ProjectExplorer::BuildSystem *buildSystem = target->buildSystem();
    QStringList options{"--timeout", QString::number(testSettings().timeout() / 1000)};
    options << theCTestTool().activeSettingsAsOptions();
    CommandLine command = buildSystem->commandLineForTests(selected, options);
    if (command.executable().isEmpty())
        return {};

    CTestConfiguration *config = new CTestConfiguration(testBase());
    config->setProject(project);
    config->setCommandLine(command);
    const ProjectExplorer::RunConfiguration *runConfig = target->activeRunConfiguration();
    Environment env = Environment::systemEnvironment();
    if (QTC_GUARD(runConfig)) {
        if (auto envAspect = runConfig->aspect<ProjectExplorer::EnvironmentAspect>())
            env = envAspect->environment();
    }
    if (HostOsInfo::isWindowsHost()) {
        env.set("QT_FORCE_STDERR_LOGGING", "1");
        env.set("QT_LOGGING_TO_CONSOLE", "1");
    }
    env.setFallback("CLICOLOR_FORCE", "1");
    config->setEnvironment(env);
    const ProjectExplorer::BuildConfiguration *buildConfig = target->activeBuildConfiguration();
    if (QTC_GUARD(buildConfig))
        config->setWorkingDirectory(buildConfig->buildDirectory());

    if (selected.isEmpty())
        config->setTestCaseCount(testBase()->asTestTool()->rootNode()->childCount());
    else
        config->setTestCaseCount(selected.size());
    return {config};
}

} // namespace Internal
} // namespace Autotest
