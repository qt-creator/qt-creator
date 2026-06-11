// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testprojectsettings.h"

#include "autotestconstants.h"
#include "autotesttr.h"
#include "testcodeparser.h"
#include "testframeworkmanager.h"
#include "testtreemodel.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <utils/algorithm.h>

#include <QLoggingCategory>

using namespace Utils;

namespace Autotest::Internal {

static const char SK_ACTIVE_FRAMEWORKS[]        = "AutoTest.ActiveFrameworks";
static const char SK_RUN_AFTER_BUILD[]          = "AutoTest.RunAfterBuild";
static const char SK_CHECK_STATES[]             = "AutoTest.CheckStates";
static const char SK_APPLY_FILTER[]             = "AutoTest.ApplyFilter";
static const char SK_PATH_FILTERS[]             = "AutoTest.PathFilters";

static Q_LOGGING_CATEGORY(LOG, "qtc.autotest.projectsettings", QtWarningMsg)

TestProjectSettings::TestProjectSettings(ProjectExplorer::Project *project)
    : m_project(project)
{
    setAutoApply(true);

    useGlobalSettings.setDefaultValue(true);

    runAfterBuild.addOption(Tr::tr("No Tests"));
    runAfterBuild.addOption(Tr::tr("All", "Run tests after build"));
    runAfterBuild.addOption(Tr::tr("Selected"));
    runAfterBuild.setLabelText(Tr::tr("Automatically run tests after build"));
    runAfterBuild.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);

    load();
    connect(project, &ProjectExplorer::Project::settingsLoaded, this, [this] { load(); });
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings, this, [this] { save(); });
}

RunAfterBuildMode TestProjectSettings::runAfterBuildMode() const
{
    return static_cast<RunAfterBuildMode>(runAfterBuild());
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
    useGlobalSettings.setValue(useGlobal.isValid() ? useGlobal.toBool() : true);

    const TestFrameworks registeredFrameworks = TestFrameworkManager::registeredFrameworks();
    qCDebug(LOG) << "Registered frameworks sorted by priority" << registeredFrameworks;
    const TestTools registeredTestTools = TestFrameworkManager::registeredTestTools();
    const QVariant activeFrameworksVar = m_project->namedSettings(SK_ACTIVE_FRAMEWORKS);

    m_activeTestFrameworks.clear();
    m_activeTestTools.clear();
    if (activeFrameworksVar.isValid()) {
        const Store frameworksMap = storeFromVariant(activeFrameworksVar);
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

    const QVariant runAfterBuildVar = m_project->namedSettings(SK_RUN_AFTER_BUILD);
    runAfterBuild.setValue(runAfterBuildVar.isValid() ? runAfterBuildVar.toInt() : 0);
    m_checkStateCache.fromSettings(m_project->namedSettings(SK_CHECK_STATES).toMap());
    limitToFilter.setValue(m_project->namedSettings(SK_APPLY_FILTER).toBool());
    m_pathFilters = m_project->namedSettings(SK_PATH_FILTERS).toStringList();
}

void TestProjectSettings::save()
{
    m_project->setNamedSettings(Constants::SK_USE_GLOBAL, useGlobalSettings());
    QVariantMap activeFrameworksMap;
    auto end = m_activeTestFrameworks.cend();
    for (auto it = m_activeTestFrameworks.cbegin(); it != end; ++it)
        activeFrameworksMap.insert(it.key()->id().toString(), it.value());
    auto endTools = m_activeTestTools.cend();
    for (auto it = m_activeTestTools.cbegin(); it != endTools; ++it)
        activeFrameworksMap.insert(it.key()->id().toString(), it.value());
    m_project->setNamedSettings(SK_ACTIVE_FRAMEWORKS, activeFrameworksMap);
    m_project->setNamedSettings(SK_RUN_AFTER_BUILD, runAfterBuild());
    m_project->setNamedSettings(SK_CHECK_STATES, m_checkStateCache.toSettings(Qt::Checked));
    m_project->setNamedSettings(SK_APPLY_FILTER, limitToFilter());
    m_project->setNamedSettings(SK_PATH_FILTERS, m_pathFilters);
}

} // namespace Autotest::Internal
