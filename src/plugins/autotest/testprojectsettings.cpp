// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testprojectsettings.h"

#include "autotestconstants.h"
#include "testcodeparser.h"
#include "testframeworkmanager.h"
#include "testtreemodel.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <utils/algorithm.h>

#include <QLoggingCategory>

using namespace Utils;

namespace Autotest {
namespace Internal {

static const char SK_ACTIVE_FRAMEWORKS[]        = "AutoTest.ActiveFrameworks";
static const char SK_RUN_AFTER_BUILD[]          = "AutoTest.RunAfterBuild";
static const char SK_CHECK_STATES[]             = "AutoTest.CheckStates";

static Q_LOGGING_CATEGORY(LOG, "qtc.autotest.projectsettings", QtWarningMsg)

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

void TestProjectSettings::activateFramework(const Id &id, bool activate)
{
    ITestFramework *framework = TestFrameworkManager::frameworkForId(id);
    m_activeTestFrameworks[framework] = activate;
    if (TestTreeModel::instance()->parser()->isParsing())
        framework->rootNode()->markForRemoval(!activate);
    else if (!activate)
        framework->resetRootNode();
}

void TestProjectSettings::activateTestTool(const Id &id, bool activate)
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
        const Store frameworksMap = storeFromVariant(activeFrameworks);
        for (ITestFramework *framework : registeredFrameworks) {
            const Id id = framework->id();
            bool active = frameworksMap.value(id.toKey(), framework->active()).toBool();
            m_activeTestFrameworks.insert(framework, active);
        }
        for (ITestTool *testTool : registeredTestTools) {
            const Id id = testTool->id();
            bool active = frameworksMap.value(id.toKey(), testTool->active()).toBool();
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
    m_project->setNamedSettings(SK_CHECK_STATES, m_checkStateCache.toSettings(Qt::Checked));
}

} // namespace Internal
} // namespace Autotest
