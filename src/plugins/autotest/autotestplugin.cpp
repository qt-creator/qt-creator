// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "autotestplugin.h"

#include "autotestconstants.h"
#include "autotesticons.h"
#include "autotesttr.h"
#include "projectsettingswidget.h"
#include "testcodeparser.h"
#include "testframeworkmanager.h"
#include "testnavigationwidget.h"
#include "testprojectsettings.h"
#include "testresultspane.h"
#include "testrunner.h"
#include "testsettings.h"
#include "testsettingspage.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include "boost/boosttestframework.h"
#include "catch/catchframework.h"
#include "ctest/ctesttool.h"
#include "gtest/gtestframework.h"
#include "qtest/qttestframework.h"
#include "quick/quicktestframework.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppmodelmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/algorithm.h>
#include <utils/textutils.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QTextCursor>

#ifdef WITH_TESTS
#include "autotestunittests.h"
#include "loadprojectscenario.h"
#endif

using namespace Core;

namespace Autotest {
namespace Internal {

class AutotestPluginPrivate : public QObject
{
    Q_OBJECT
public:
    AutotestPluginPrivate();
    ~AutotestPluginPrivate() override;

    TestNavigationWidgetFactory m_navigationWidgetFactory;
    TestResultsPane *m_resultsPane = nullptr;
    QMap<QString, ChoicePair> m_runconfigCache;

    void initializeMenuEntries();
    void onRunAllTriggered(TestRunMode mode);
    void onRunSelectedTriggered(TestRunMode mode);
    void onRunFailedTriggered();
    void onRunFileTriggered();
    void onRunUnderCursorTriggered(TestRunMode mode);

    TestSettings m_settings;
    TestSettingsPage m_testSettingPage{&m_settings};

    TestCodeParser m_testCodeParser;
    TestTreeModel m_testTreeModel{&m_testCodeParser};
    TestRunner m_testRunner;
    TestFrameworkManager m_frameworkManager;
#ifdef WITH_TESTS
    LoadProjectScenario m_loadProjectScenario{&m_testTreeModel};
#endif
};

static AutotestPluginPrivate *dd = nullptr;
static QHash<ProjectExplorer::Project *, TestProjectSettings *> s_projectSettings;

AutotestPlugin::AutotestPlugin()
{
    // needed to be used in QueuedConnection connects
    qRegisterMetaType<TestResult>();
    qRegisterMetaType<TestTreeItem *>();
    qRegisterMetaType<TestCodeLocationAndType>();
    // warm up meta type system to be able to read Qt::CheckState with persistent settings
    qRegisterMetaType<Qt::CheckState>();
}

AutotestPlugin::~AutotestPlugin()
{
    delete dd;
    dd = nullptr;
}

AutotestPluginPrivate::AutotestPluginPrivate()
{
    dd = this; // Needed as the code below access it via the static plugin interface
    initializeMenuEntries();
    m_frameworkManager.registerTestFramework(new QtTestFramework);
    m_frameworkManager.registerTestFramework(new QuickTestFramework);
    m_frameworkManager.registerTestFramework(new GTestFramework);
    m_frameworkManager.registerTestFramework(new BoostTestFramework);
    m_frameworkManager.registerTestFramework(new CatchFramework);

    m_frameworkManager.registerTestTool(new CTestTool);

    m_frameworkManager.synchronizeSettings(ICore::settings());
    m_resultsPane = TestResultsPane::instance();

    auto panelFactory = new ProjectExplorer::ProjectPanelFactory();
    panelFactory->setPriority(666);
//    panelFactory->setIcon();  // TODO ?
    panelFactory->setDisplayName(Tr::tr("Testing"));
    panelFactory->setCreateWidgetFunction([](ProjectExplorer::Project *project) {
        return new ProjectTestSettingsWidget(project);
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);

    TestFrameworkManager::activateFrameworksAndToolsFromSettings(&m_settings);
    m_testTreeModel.synchronizeTestFrameworks();
    m_testTreeModel.synchronizeTestTools();

    auto sessionManager = ProjectExplorer::SessionManager::instance();
    connect(sessionManager, &ProjectExplorer::SessionManager::startupProjectChanged,
            this, [this] { m_runconfigCache.clear(); });

    connect(sessionManager, &ProjectExplorer::SessionManager::aboutToRemoveProject,
            this, [](ProjectExplorer::Project *project) {
        const auto it = s_projectSettings.constFind(project);
        if (it != s_projectSettings.constEnd()) {
            delete it.value();
            s_projectSettings.erase(it);
        }
    });
}

AutotestPluginPrivate::~AutotestPluginPrivate()
{
    if (!s_projectSettings.isEmpty()) {
        qDeleteAll(s_projectSettings);
        s_projectSettings.clear();
    }

    delete m_resultsPane;
}

TestSettings *AutotestPlugin::settings()
{
    return &dd->m_settings;
}

TestProjectSettings *AutotestPlugin::projectSettings(ProjectExplorer::Project *project)
{
    auto &settings = s_projectSettings[project];
    if (!settings)
        settings = new TestProjectSettings(project);

    return settings;
}

void AutotestPluginPrivate::initializeMenuEntries()
{
    ActionContainer *menu = ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(Tr::tr("&Tests"));
    menu->setOnAllDisabledBehavior(ActionContainer::Show);

    QAction *action = new QAction(Tr::tr("Run &All Tests"), this);
    action->setIcon(Utils::Icons::RUN_SMALL.icon());
    action->setToolTip(Tr::tr("Run all tests"));
    Command *command = ActionManager::registerAction(action, Constants::ACTION_RUN_ALL_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+A") : Tr::tr("Alt+Shift+T,Alt+A")));
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunAllTriggered, this, TestRunMode::Run));
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(Tr::tr("Run All Tests Without Deployment"), this);
    action->setIcon(Utils::Icons::RUN_SMALL.icon());
    action->setToolTip(Tr::tr("Run all tests without deployment"));
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_ALL_NODEPLOY_ID);
    command->setDefaultKeySequence(
                QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+E") : Tr::tr("Alt+Shift+T,Alt+E")));
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunAllTriggered, this, TestRunMode::RunWithoutDeploy));
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(Tr::tr("&Run Selected Tests"), this);
    action->setIcon(Utils::Icons::RUN_SELECTED.icon());
    action->setToolTip(Tr::tr("Run selected tests"));
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_SELECTED_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+R") : Tr::tr("Alt+Shift+T,Alt+R")));
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunSelectedTriggered, this, TestRunMode::Run));
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(Tr::tr("&Run Selected Tests Without Deployment"), this);
    action->setIcon(Utils::Icons::RUN_SELECTED.icon());
    action->setToolTip(Tr::tr("Run selected tests"));
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_SELECTED_NODEPLOY_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+W") : Tr::tr("Alt+Shift+T,Alt+W")));
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunSelectedTriggered, this, TestRunMode::RunWithoutDeploy));
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(Tr::tr("Run &Failed Tests"),  this);
    action->setIcon(Icons::RUN_FAILED.icon());
    action->setToolTip(Tr::tr("Run failed tests"));
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_FAILED_ID);
    command->setDefaultKeySequence(
                useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+F") : Tr::tr("Alt+Shift+T,Alt+F"));
    connect(action, &QAction::triggered, this, &AutotestPluginPrivate::onRunFailedTriggered);
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(Tr::tr("Run Tests for &Current File"), this);
    action->setIcon(Utils::Icons::RUN_FILE.icon());
    action->setToolTip(Tr::tr("Run tests for current file"));
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_FILE_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+C") : Tr::tr("Alt+Shift+T,Alt+C")));
    connect(action, &QAction::triggered, this, &AutotestPluginPrivate::onRunFileTriggered);
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(Tr::tr("Re&scan Tests"), this);
    command = ActionManager::registerAction(action, Constants::ACTION_SCAN_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+S") : Tr::tr("Alt+Shift+T,Alt+S")));

    connect(action, &QAction::triggered, this, [] { dd->m_testCodeParser.updateTestTree(); });
    menu->addAction(command);

    ActionContainer *toolsMenu = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsMenu->addMenu(menu);
    using namespace ProjectExplorer;
    connect(BuildManager::instance(), &BuildManager::buildStateChanged,
            this, &AutotestPlugin::updateMenuItemsEnabledState);
    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            this, &AutotestPlugin::updateMenuItemsEnabledState);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
            this, &AutotestPlugin::updateMenuItemsEnabledState);
    connect(&dd->m_testTreeModel, &TestTreeModel::testTreeModelChanged,
            this, &AutotestPlugin::updateMenuItemsEnabledState);
}

bool AutotestPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    dd = new AutotestPluginPrivate;
#ifdef WITH_TESTS
    ExtensionSystem::PluginManager::registerScenario("TestModelManagerInterface",
                   [] { return dd->m_loadProjectScenario(); });
#endif
    return true;
}

void AutotestPlugin::extensionsInitialized()
{
    ActionContainer *contextMenu = ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT);
    if (!contextMenu) // if QC is started without CppEditor plugin
        return;

    QAction *action = new QAction(Tr::tr("&Run Test Under Cursor"), this);
    action->setEnabled(false);
    action->setIcon(Utils::Icons::RUN_SMALL.icon());

    Command *command = ActionManager::registerAction(action, Constants::ACTION_RUN_UCURSOR);
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunUnderCursorTriggered, dd, TestRunMode::Run));
    contextMenu->addSeparator();
    contextMenu->addAction(command);

    action = new QAction(Tr::tr("Run Test Under Cursor Without Deployment"), this);
    action->setEnabled(false);
    action->setIcon(Utils::Icons::RUN_SMALL.icon());

    command = ActionManager::registerAction(action, Constants::ACTION_RUN_UCURSOR_NODEPLOY);
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunUnderCursorTriggered, dd, TestRunMode::RunWithoutDeploy));
    contextMenu->addAction(command);

    action = new QAction(Tr::tr("&Debug Test Under Cursor"), this);
    action->setEnabled(false);
    action->setIcon(ProjectExplorer::Icons::DEBUG_START_SMALL.icon());

    command = ActionManager::registerAction(action, Constants::ACTION_RUN_DBG_UCURSOR);
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunUnderCursorTriggered, dd, TestRunMode::Debug));
    contextMenu->addAction(command);

    action = new QAction(Tr::tr("Debug Test Under Cursor Without Deployment"), this);
    action->setEnabled(false);
    action->setIcon(ProjectExplorer::Icons::DEBUG_START_SMALL.icon());

    command = ActionManager::registerAction(action, Constants::ACTION_RUN_DBG_UCURSOR_NODEPLOY);
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunUnderCursorTriggered, dd, TestRunMode::DebugWithoutDeploy));
    contextMenu->addAction(command);
    contextMenu->addSeparator();
}

ExtensionSystem::IPlugin::ShutdownFlag AutotestPlugin::aboutToShutdown()
{
    dd->m_testCodeParser.aboutToShutdown();
    dd->m_testTreeModel.disconnect();
    return SynchronousShutdown;
}

void AutotestPluginPrivate::onRunAllTriggered(TestRunMode mode)
{
    m_testRunner.runTests(mode, m_testTreeModel.getAllTestCases());
}

void AutotestPluginPrivate::onRunSelectedTriggered(TestRunMode mode)
{
    m_testRunner.runTests(mode, m_testTreeModel.getSelectedTests());
}

void AutotestPluginPrivate::onRunFailedTriggered()
{
    const QList<ITestConfiguration *> failed = m_testTreeModel.getFailedTests();
    if (failed.isEmpty()) // the framework might not be able to provide them
        return;
    m_testRunner.runTests(TestRunMode::Run, failed);
}

void AutotestPluginPrivate::onRunFileTriggered()
{
    const IDocument *document = EditorManager::currentDocument();
    if (!document)
        return;

    const Utils::FilePath &fileName = document->filePath();
    if (fileName.isEmpty())
        return;

    const QList<ITestConfiguration *> tests = m_testTreeModel.getTestsForFile(fileName);
    if (tests.isEmpty())
        return;

    m_testRunner.runTests(TestRunMode::Run, tests);
}

static QList<ITestConfiguration *> testItemsToTestConfigurations(const QList<ITestTreeItem *> &items,
                                                                TestRunMode mode)
{
    QList<ITestConfiguration *> configs;
    for (const ITestTreeItem * item : items) {
        if (ITestConfiguration *currentConfig = item->asConfiguration(mode))
            configs << currentConfig;
    }
    return configs;
}

void AutotestPluginPrivate::onRunUnderCursorTriggered(TestRunMode mode)
{
    TextEditor::BaseTextEditor *currentEditor = TextEditor::BaseTextEditor::currentTextEditor();
    QTextCursor cursor = currentEditor->editorWidget()->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    const QString text = cursor.selectedText();
    if (text.isEmpty())
        return; // Do not trigger when no name under cursor

    const QList<ITestTreeItem *> testsItems = m_testTreeModel.testItemsByName(text);
    if (testsItems.isEmpty())
        return; // Wrong location triggered

    // check whether we have been triggered on a test function definition
    const int line = currentEditor->currentLine();
    const Utils::FilePath &filePath = currentEditor->textDocument()->filePath();
    QList<ITestTreeItem *> filteredItems = Utils::filtered(testsItems, [&](ITestTreeItem *it){
        return it->line() == line && it->filePath() == filePath;
    });

    if (filteredItems.size() == 0 && testsItems.size() > 1) {
        const CPlusPlus::Snapshot snapshot = CppEditor::CppModelManager::instance()->snapshot();
        const CPlusPlus::Document::Ptr doc = snapshot.document(filePath);
        if (!doc.isNull()) {
            CPlusPlus::Scope *scope = doc->scopeAt(line, currentEditor->currentColumn());
            if (scope->asClass()) {
                const QList<const CPlusPlus::Name *> fullName
                        = CPlusPlus::LookupContext::fullyQualifiedName(scope);
                const QString className = CPlusPlus::Overview().prettyName(fullName);

                filteredItems = Utils::filtered(testsItems,
                                                [&text, &className](ITestTreeItem *it){
                    return it->name() == text
                            && static_cast<ITestTreeItem *>(it->parent())->name() == className;
                });
            }
        }
    }
    if ((filteredItems.size() != 1 && testsItems.size() > 1)
            && (mode == TestRunMode::Debug || mode == TestRunMode::DebugWithoutDeploy)) {
        MessageManager::writeFlashing(Tr::tr("Cannot debug multiple tests at once."));
        return;
    }
    const QList<ITestConfiguration *> testsToRun = testItemsToTestConfigurations(
                filteredItems.size() == 1 ? filteredItems : testsItems, mode);

    if (testsToRun.isEmpty()) {
        MessageManager::writeFlashing(Tr::tr("Selected test was not found (%1).").arg(text));
        return;
    }

    m_testRunner.runTests(mode, testsToRun);
}

void AutotestPlugin::updateMenuItemsEnabledState()
{
    const ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    const ProjectExplorer::Target *target = project ? project->activeTarget() : nullptr;
    const bool canScan = !dd->m_testRunner.isTestRunning()
            && dd->m_testCodeParser.state() == TestCodeParser::Idle;
    const bool hasTests = dd->m_testTreeModel.hasTests();
    // avoid expensive call to PE::canRunStartupProject() - limit to minimum necessary checks
    const bool canRun = hasTests && canScan
            && project && !project->needsConfiguration()
            && target && target->activeRunConfiguration()
            && !ProjectExplorer::BuildManager::isBuilding();
    const bool canRunFailed = canRun && dd->m_testTreeModel.hasFailedTests();

    ActionManager::command(Constants::ACTION_RUN_ALL_ID)->action()->setEnabled(canRun);
    ActionManager::command(Constants::ACTION_RUN_SELECTED_ID)->action()->setEnabled(canRun);
    ActionManager::command(Constants::ACTION_RUN_ALL_NODEPLOY_ID)->action()->setEnabled(canRun);
    ActionManager::command(Constants::ACTION_RUN_SELECTED_NODEPLOY_ID)->action()->setEnabled(canRun);
    ActionManager::command(Constants::ACTION_RUN_FAILED_ID)->action()->setEnabled(canRunFailed);
    ActionManager::command(Constants::ACTION_RUN_FILE_ID)->action()->setEnabled(canRun);
    ActionManager::command(Constants::ACTION_SCAN_ID)->action()->setEnabled(canScan);

    ActionContainer *contextMenu = ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT);
    if (!contextMenu)
        return; // When no context menu, actions do not exists

    ActionManager::command(Constants::ACTION_RUN_UCURSOR)->action()->setEnabled(canRun);
    ActionManager::command(Constants::ACTION_RUN_UCURSOR_NODEPLOY)->action()->setEnabled(canRun);
    ActionManager::command(Constants::ACTION_RUN_DBG_UCURSOR)->action()->setEnabled(canRun);
    ActionManager::command(Constants::ACTION_RUN_DBG_UCURSOR_NODEPLOY)->action()->setEnabled(canRun);
}

void AutotestPlugin::cacheRunConfigChoice(const QString &buildTargetKey, const ChoicePair &choice)
{
    if (dd)
        dd->m_runconfigCache.insert(buildTargetKey, choice);
}

ChoicePair AutotestPlugin::cachedChoiceFor(const QString &buildTargetKey)
{
    return dd ? dd->m_runconfigCache.value(buildTargetKey) : ChoicePair();
}

void AutotestPlugin::clearChoiceCache()
{
    if (dd)
        dd->m_runconfigCache.clear();
}

void AutotestPlugin::popupResultsPane()
{
    if (dd)
        dd->m_resultsPane->popup(Core::IOutputPane::NoModeSwitch);
}

QVector<QObject *> AutotestPlugin::createTestObjects() const
{
    QVector<QObject *> tests;
#ifdef WITH_TESTS
    tests << new AutoTestUnitTests(&dd->m_testTreeModel);
#endif
    return tests;
}

bool ChoicePair::matches(const ProjectExplorer::RunConfiguration *rc) const
{
    return rc && rc->displayName() == displayName && rc->runnable().command.executable() == executable;
}

} // Internal
} // Autotest

#include "autotestplugin.moc"
