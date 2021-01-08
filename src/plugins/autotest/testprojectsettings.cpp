/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "testprojectsettings.h"

#include "autotestconstants.h"
#include "testframeworkmanager.h"

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <utils/algorithm.h>

#include <QLoggingCategory>

namespace Autotest {
namespace Internal {

static const char SK_ACTIVE_FRAMEWORKS[]        = "AutoTest.ActiveFrameworks";
static const char SK_RUN_AFTER_BUILD[]          = "AutoTest.RunAfterBuild";
static const char SK_CHECK_STATES[]             = "AutoTest.CheckStates";

static Q_LOGGING_CATEGORY(LOG, "qtc.autotest.frameworkmanager", QtWarningMsg)

TestProjectSettings::TestProjectSettings(ProjectExplorer::Project *project)
    : m_project(project)
{
    load();
    connect(project, &ProjectExplorer::Project::settingsLoaded,
            this, &TestProjectSettings::load);
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings,
            this, &TestProjectSettings::save);
}

TestProjectSettings::~TestProjectSettings()
{
    save();
}

void TestProjectSettings::setUseGlobalSettings(bool useGlobal)
{
    if (m_useGlobalSettings == useGlobal)
        return;
     m_useGlobalSettings = useGlobal;
}

void TestProjectSettings::activateFramework(const Utils::Id &id, bool activate)
{
    ITestFramework *framework = TestFrameworkManager::frameworkForId(id);
    m_activeTestFrameworks[framework] = activate;
    if (!activate)
        framework->resetRootNode();
}

void TestProjectSettings::activateTestTool(const Utils::Id &id, bool activate)
{
    ITestTool *testTool = TestFrameworkManager::testToolForId(id);
    m_activeTestTools[testTool] = activate;
    if (!activate)
        testTool->resetRootNode();
}

void TestProjectSettings::load()
{
    const QVariant useGlobal = m_project->namedSettings(Constants::SK_USE_GLOBAL);
    m_useGlobalSettings = useGlobal.isValid() ? useGlobal.toBool() : true;

    const TestFrameworks registeredFrameworks = TestFrameworkManager::registeredFrameworks();
    qCDebug(LOG) << "Registered frameworks sorted by priority" << registeredFrameworks;
    const TestTools registeredTestTools = TestFrameworkManager::registeredTestTools();
    const QVariant activeFrameworks = m_project->namedSettings(SK_ACTIVE_FRAMEWORKS);

    m_activeTestFrameworks.clear();
    m_activeTestTools.clear();
    if (activeFrameworks.isValid()) {
        const QMap<QString, QVariant> frameworksMap = activeFrameworks.toMap();
        for (ITestFramework *framework : registeredFrameworks) {
            const Utils::Id id = framework->id();
            bool active = frameworksMap.value(id.toString(), framework->active()).toBool();
            m_activeTestFrameworks.insert(framework, active);
        }
        for (ITestTool *testTool : registeredTestTools) {
            const Utils::Id id = testTool->id();
            bool active = frameworksMap.value(id.toString(), testTool->active()).toBool();
            m_activeTestTools.insert(testTool, active);
        }
    } else {
        for (ITestFramework *framework : registeredFrameworks)
            m_activeTestFrameworks.insert(framework, framework->active());
        for (ITestTool *testTool : registeredTestTools)
            m_activeTestTools.insert(testTool, testTool->active());
    }

    const QVariant runAfterBuild = m_project->namedSettings(SK_RUN_AFTER_BUILD);
    m_runAfterBuild = runAfterBuild.isValid() ? RunAfterBuildMode(runAfterBuild.toInt())
                                              : RunAfterBuildMode::None;
    m_checkStateCache.fromSettings(m_project->namedSettings(SK_CHECK_STATES).toMap());
}

void TestProjectSettings::save()
{
    m_project->setNamedSettings(Constants::SK_USE_GLOBAL, m_useGlobalSettings);
    QVariantMap activeFrameworks;
    auto end = m_activeTestFrameworks.cend();
    for (auto it = m_activeTestFrameworks.cbegin(); it != end; ++it)
        activeFrameworks.insert(it.key()->id().toString(), it.value());
    auto endTools = m_activeTestTools.cend();
    for (auto it = m_activeTestTools.cbegin(); it != endTools; ++it)
        activeFrameworks.insert(it.key()->id().toString(), it.value());
    m_project->setNamedSettings(SK_ACTIVE_FRAMEWORKS, activeFrameworks);
    m_project->setNamedSettings(SK_RUN_AFTER_BUILD, int(m_runAfterBuild));
    m_project->setNamedSettings(SK_CHECK_STATES, m_checkStateCache.toSettings());
}

} // namespace Internal
} // namespace Autotest
