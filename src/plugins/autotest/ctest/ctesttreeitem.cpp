/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "ctesttreeitem.h"

#include "ctestconfiguration.h"

#include "../autotestplugin.h"
#include "../itestframework.h"
#include "../testsettings.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/link.h>
#include  <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

CTestTreeItem::CTestTreeItem(ITestBase *testBase, const QString &name,
                             const Utils::FilePath &filepath, Type type)
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
        itemLink.setValue(Utils::Link(filePath(), line()));
        return itemLink;
    }
    return ITestTreeItem::data(column, role);
}

QList<ITestConfiguration *> CTestTreeItem::testConfigurationsFor(const QStringList &selected) const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project)
        return {};

    const ProjectExplorer::Target *target = project->targets().value(0);
    if (!target)
        return {};

    const ProjectExplorer::BuildSystem *buildSystem = target->buildSystem();
    QStringList options{"--timeout", QString::number(AutotestPlugin::settings()->timeout / 1000)};
    // TODO add ctest options from settings?
    options << "--output-on-failure";
    Utils::CommandLine command = buildSystem->commandLineForTests(selected, options);
    if (command.executable().isEmpty())
        return {};

    CTestConfiguration *config = new CTestConfiguration(testBase());
    config->setProject(project);
    config->setCommandLine(command);
    const ProjectExplorer::RunConfiguration *runConfig = target->activeRunConfiguration();
    if (QTC_GUARD(runConfig)) {
        if (auto envAspect = runConfig->aspect<ProjectExplorer::EnvironmentAspect>())
            config->setEnvironment(envAspect->environment());
        else
            config->setEnvironment(Utils::Environment::systemEnvironment());
    }
    const ProjectExplorer::BuildConfiguration *buildConfig = target->activeBuildConfiguration();
    if (QTC_GUARD(buildConfig))
        config->setWorkingDirectory(buildConfig->buildDirectory().toString());

    if (selected.isEmpty())
        config->setTestCaseCount(testBase()->asTestTool()->rootNode()->childCount());
    else
        config->setTestCaseCount(selected.size());
    return {config};
}

} // namespace Internal
} // namespace Autotest
