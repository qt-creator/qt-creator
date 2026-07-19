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
// container; fromStore()/toStore() reconcile against currently registered objects.

ActiveTestFrameworksAspect::ActiveTestFrameworksAspect(AspectContainer *container)
    : TypedAspect(container)
{}

void ActiveTestFrameworksAspect::setActive(ITestFramework *framework, bool active)
{
    ActiveTestFrameworks map = value();
    map.insert(framework, active);
    setValue(map);
}

void ActiveTestFrameworksAspect::fromStore(const Store &store)
{
    const TestFrameworks registered = TestFrameworkManager::registeredFrameworks();
    qCDebug(LOG) << "Registered frameworks sorted by priority" << registered;
    ActiveTestFrameworks frameworks;
    for (ITestFramework *framework : registered) {
        const Id id = framework->id();
        frameworks.insert(framework, store.value(id.toKey(), framework->active()).toBool());
    }
    setValue(frameworks);
}

void ActiveTestFrameworksAspect::toStore(Store &store) const
{
    for (auto it = value().cbegin(), end = value().cend(); it != end; ++it)
        store.insert(it.key()->id().toKey(), it.value());
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

void ActiveTestToolsAspect::fromStore(const Store &store)
{
    const TestTools registered = TestFrameworkManager::registeredTestTools();
    ActiveTestTools tools;
    for (ITestTool *testTool : registered) {
        const Id id = testTool->id();
        tools.insert(testTool, store.value(id.toKey(), testTool->active()).toBool());
    }
    setValue(tools);
}

void ActiveTestToolsAspect::toStore(Store &store) const
{
    for (auto it = value().cbegin(), end = value().cend(); it != end; ++it)
        store.insert(it.key()->id().toKey(), it.value());
}

// TestProjectSettings

TestProjectSettings::TestProjectSettings(ProjectExplorer::Project *project)
    : m_project(project)
{
    setAutoApply(true);

    useGlobalSettings.setSettingsPageId(Constants::AUTOTEST_SETTINGS_ID);

    runAfterBuild.addOption(Tr::tr("No Tests"));
    runAfterBuild.addOption(Tr::tr("All", "Run tests after build"));
    runAfterBuild.addOption(Tr::tr("Selected"));
    runAfterBuild.setLabelText(Tr::tr("Automatically run tests after build"));
    runAfterBuild.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);

    load();
    connect(project, &ProjectExplorer::Project::settingsLoaded, this, [this] { load(); });
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings, this, [this] { save(); });
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

    const Store frameworksStore = storeFromVariant(m_project->namedSettings(SK_ACTIVE_FRAMEWORKS));
    activeTestFrameworks.fromStore(frameworksStore);
    activeTestTools.fromStore(frameworksStore);

    const QVariant runAfterBuildVar = m_project->namedSettings(SK_RUN_AFTER_BUILD);
    runAfterBuild.setValue(static_cast<RunAfterBuildMode>(
        runAfterBuildVar.isValid() ? runAfterBuildVar.toInt() : 0));
    checkStateCache.fromSettings(m_project->namedSettings(SK_CHECK_STATES).toMap());
    limitToFilter.setValue(m_project->namedSettings(SK_APPLY_FILTER).toBool());
    pathFilters.setValue(m_project->namedSettings(SK_PATH_FILTERS).toStringList());
}

void TestProjectSettings::save()
{
    m_project->setNamedSettings(Constants::SK_USE_GLOBAL, useGlobalSettings());
    Store frameworksStore;
    activeTestFrameworks.toStore(frameworksStore);
    activeTestTools.toStore(frameworksStore);
    m_project->setNamedSettings(SK_ACTIVE_FRAMEWORKS, variantFromStore(frameworksStore));
    m_project->setNamedSettings(SK_RUN_AFTER_BUILD, int(runAfterBuild()));
    m_project->setNamedSettings(SK_CHECK_STATES, checkStateCache.toSettings());
    m_project->setNamedSettings(SK_APPLY_FILTER, limitToFilter());
    m_project->setNamedSettings(SK_PATH_FILTERS, pathFilters());
}

} // namespace Autotest::Internal
