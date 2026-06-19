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

// ActiveTestFrameworksAspect

// Holds the per-project "active" flag for each registered test framework/tool.
// Keyed by pointer (registered at runtime), so not serialized through the
// container; TestProjectSettings reconciles in load()/save().

ActiveTestFrameworksAspect::ActiveTestFrameworksAspect(AspectContainer *container)
    : TypedAspect(container)
{}

void ActiveTestFrameworksAspect::setActive(ITestFramework *framework, bool active)
{
    ActiveTestFrameworks map = value();
    map.insert(framework, active);
    setValue(map);
}

// ActiveTestToolsAspect

ActiveTestToolsAspect::ActiveTestToolsAspect(AspectContainer *container)
    : TypedAspect(container)
{}

void ActiveTestToolsAspect::setActive(ITestTool *testTool, bool active)
{
    ActiveTestTools map = value();
    map.insert(testTool, active);
    setValue(map);
}

// TestProjectSettings

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
    activeTestFrameworks.setActive(framework, activate);
    if (!activate) {
        TestTreeItem *root = framework->rootNode();
        if (root->model())
            TestTreeModel::instance()->takeItem(root);
    }
}

void TestProjectSettings::activateTestTool(const Id &id, bool activate)
{
    ITestTool *testTool = TestFrameworkManager::testToolForId(id);
    activeTestTools.setActive(testTool, activate);
    if (!activate) {
        ITestTreeItem *root = testTool->rootNode();
        if (root->model())
            TestTreeModel::instance()->takeItem(root);
    }
}

void TestProjectSettings::load()
{
    const QVariant useGlobal = m_project->namedSettings(Constants::SK_USE_GLOBAL);
    useGlobalSettings.setValue(useGlobal.isValid() ? useGlobal.toBool() : true);

    const TestFrameworks registeredFrameworks = TestFrameworkManager::registeredFrameworks();
    qCDebug(LOG) << "Registered frameworks sorted by priority" << registeredFrameworks;
    const TestTools registeredTestTools = TestFrameworkManager::registeredTestTools();
    const QVariant activeFrameworksVar = m_project->namedSettings(SK_ACTIVE_FRAMEWORKS);

    QHash<ITestFramework *, bool> frameworks;
    QHash<ITestTool *, bool> tools;
    if (activeFrameworksVar.isValid()) {
        const Store frameworksMap = storeFromVariant(activeFrameworksVar);
        for (ITestFramework *framework : registeredFrameworks) {
            const Id id = framework->id();
            frameworks.insert(framework, frameworksMap.value(id.toKey(), framework->active()).toBool());
        }
        for (ITestTool *testTool : registeredTestTools) {
            const Id id = testTool->id();
            tools.insert(testTool, frameworksMap.value(id.toKey(), testTool->active()).toBool());
        }
    } else {
        for (ITestFramework *framework : registeredFrameworks)
            frameworks.insert(framework, framework->active());
        for (ITestTool *testTool : registeredTestTools)
            tools.insert(testTool, testTool->active());
    }
    activeTestFrameworks.setValue(frameworks);
    activeTestTools.setValue(tools);

    const QVariant runAfterBuildVar = m_project->namedSettings(SK_RUN_AFTER_BUILD);
    runAfterBuild.setValue(runAfterBuildVar.isValid() ? runAfterBuildVar.toInt() : 0);
    m_checkStateCache.fromSettings(m_project->namedSettings(SK_CHECK_STATES).toMap());
    limitToFilter.setValue(m_project->namedSettings(SK_APPLY_FILTER).toBool());
    pathFilters.setValue(m_project->namedSettings(SK_PATH_FILTERS).toStringList());
}

void TestProjectSettings::save()
{
    m_project->setNamedSettings(Constants::SK_USE_GLOBAL, useGlobalSettings());
    QVariantMap activeFrameworksMap;
    const QHash<ITestFramework *, bool> frameworks = activeTestFrameworks();
    auto end = frameworks.cend();
    for (auto it = frameworks.cbegin(); it != end; ++it)
        activeFrameworksMap.insert(it.key()->id().toString(), it.value());
    const QHash<ITestTool *, bool> tools = activeTestTools();
    for (auto it = tools.cbegin(), end = tools.cend(); it != end; ++it)
        activeFrameworksMap.insert(it.key()->id().toString(), it.value());
    m_project->setNamedSettings(SK_ACTIVE_FRAMEWORKS, activeFrameworksMap);
    m_project->setNamedSettings(SK_RUN_AFTER_BUILD, runAfterBuild());
    m_project->setNamedSettings(SK_CHECK_STATES, m_checkStateCache.toSettings());
    m_project->setNamedSettings(SK_APPLY_FILTER, limitToFilter());
    m_project->setNamedSettings(SK_PATH_FILTERS, pathFilters());
}

} // namespace Autotest::Internal
