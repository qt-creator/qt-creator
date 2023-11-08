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
#include "testsettingspage.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include "boost/boosttestframework.h"
#include "catch/catchtestframework.h"
#include "ctest/ctesttool.h"
#include "gtest/gtestframework.h"
#include "qtest/qttestframework.h"
#include "quick/quicktestframework.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

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
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/processinterface.h>
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
using namespace Utils;

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
    void onDisableTemporarily(bool disable);

    TestSettingsPage m_testSettingPage;

    TestCodeParser m_testCodeParser;
    TestTreeModel m_testTreeModel{&m_testCodeParser};
    TestRunner m_testRunner;
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
    TestFrameworkManager::registerTestFramework(&theQtTestFramework());
    TestFrameworkManager::registerTestFramework(&theQuickTestFramework());
    TestFrameworkManager::registerTestFramework(&theGTestFramework());
    TestFrameworkManager::registerTestFramework(&theBoostTestFramework());
    TestFrameworkManager::registerTestFramework(&theCatchFramework());

    TestFrameworkManager::registerTestTool(&theCTestTool());

    m_resultsPane = TestResultsPane::instance();

    auto panelFactory = new ProjectExplorer::ProjectPanelFactory();
    panelFactory->setPriority(666);
//    panelFactory->setIcon();  // TODO ?
    panelFactory->setDisplayName(Tr::tr("Testing"));
    panelFactory->setCreateWidgetFunction([](ProjectExplorer::Project *project) {
        return new ProjectTestSettingsWidget(project);
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);

    TestFrameworkManager::activateFrameworksAndToolsFromSettings();
    m_testTreeModel.synchronizeTestFrameworks();
    m_testTreeModel.synchronizeTestTools();

    auto sessionManager = ProjectExplorer::ProjectManager::instance();
    connect(sessionManager, &ProjectExplorer::ProjectManager::startupProjectChanged,
            this, [this] { m_runconfigCache.clear(); });

    connect(sessionManager, &ProjectExplorer::ProjectManager::aboutToRemoveProject,
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
    action->setToolTip(Tr::tr("Run All Tests"));
    Command *command = ActionManager::registerAction(action, Constants::ACTION_RUN_ALL_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+A") : Tr::tr("Alt+Shift+T,Alt+A")));
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunAllTriggered, this, TestRunMode::Run));
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(Tr::tr("Run All Tests Without Deployment"), this);
    action->setIcon(Utils::Icons::RUN_SMALL.icon());
    action->setToolTip(Tr::tr("Run All Tests Without Deployment"));
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_ALL_NODEPLOY_ID);
    command->setDefaultKeySequence(
                QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+E") : Tr::tr("Alt+Shift+T,Alt+E")));
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunAllTriggered, this, TestRunMode::RunWithoutDeploy));
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(Tr::tr("&Run Selected Tests"), this);
    action->setIcon(Utils::Icons::RUN_SELECTED.icon());
    action->setToolTip(Tr::tr("Run Selected Tests"));
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_SELECTED_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+R") : Tr::tr("Alt+Shift+T,Alt+R")));
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunSelectedTriggered, this, TestRunMode::Run));
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(Tr::tr("&Run Selected Tests Without Deployment"), this);
    action->setIcon(Utils::Icons::RUN_SELECTED.icon());
    action->setToolTip(Tr::tr("Run Selected Tests Without Deployment"));
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_SELECTED_NODEPLOY_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+W") : Tr::tr("Alt+Shift+T,Alt+W")));
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunSelectedTriggered, this, TestRunMode::RunWithoutDeploy));
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(Tr::tr("Run &Failed Tests"),  this);
    action->setIcon(Icons::RUN_FAILED.icon());
    action->setToolTip(Tr::tr("Run Failed Tests"));
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_FAILED_ID);
    command->setDefaultKeySequence(
                useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+F") : Tr::tr("Alt+Shift+T,Alt+F"));
    connect(action, &QAction::triggered, this, &AutotestPluginPrivate::onRunFailedTriggered);
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(Tr::tr("Run Tests for &Current File"), this);
    action->setIcon(Utils::Icons::RUN_FILE.icon());
    action->setToolTip(Tr::tr("Run Tests for Current File"));
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_FILE_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+C") : Tr::tr("Alt+Shift+T,Alt+C")));
    connect(action, &QAction::triggered, this, &AutotestPluginPrivate::onRunFileTriggered);
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(Tr::tr("Disable Temporarily"), this);
    action->setToolTip(Tr::tr("Disable scanning and other actions until explicitly rescanning, "
                              "re-enabling, or restarting Qt Creator."));
    action->setCheckable(true);
    command = ActionManager::registerAction(action, Constants::ACTION_DISABLE_TMP);
    connect(action, &QAction::triggered, this, &AutotestPluginPrivate::onDisableTemporarily);
    menu->addAction(command);

    action = new QAction(Tr::tr("Re&scan Tests"), this);
    command = ActionManager::registerAction(action, Constants::ACTION_SCAN_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? Tr::tr("Ctrl+Meta+T, Ctrl+Meta+S") : Tr::tr("Alt+Shift+T,Alt+S")));

    connect(action, &QAction::triggered, this, [] {
        if (dd->m_testCodeParser.state() == TestCodeParser::DisabledTemporarily)
            dd->onDisableTemporarily(false);  // Rescan Test should explicitly re-enable
        else
            dd->m_testCodeParser.updateTestTree();
    });
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

void AutotestPlugin::initialize()
{
    dd = new AutotestPluginPrivate;
#ifdef WITH_TESTS
    ExtensionSystem::PluginManager::registerScenario("TestModelManagerInterface",
                   [] { return dd->m_loadProjectScenario(); });

    addTest<AutoTestUnitTests>(&dd->m_testTreeModel);
#endif
}

void AutotestPlugin::extensionsInitialized()
{
    ActionContainer *contextMenu = ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT);
    if (!contextMenu) // if QC is started without CppEditor plugin
        return;

    ActionContainer * const runTestMenu = ActionManager::createMenu("Autotest.TestUnderCursor");
    runTestMenu->menu()->setTitle(Tr::tr("Run Test Under Cursor"));
    contextMenu->addSeparator();
    contextMenu->addMenu(runTestMenu);
    contextMenu->addSeparator();

    QAction *action = new QAction(Tr::tr("&Run Test"), this);
    action->setEnabled(false);
    action->setIcon(Utils::Icons::RUN_SMALL.icon());

    Command *command = ActionManager::registerAction(action, Constants::ACTION_RUN_UCURSOR);
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunUnderCursorTriggered, dd, TestRunMode::Run));
    runTestMenu->addAction(command);

    action = new QAction(Tr::tr("Run Test Without Deployment"), this);
    action->setEnabled(false);
    action->setIcon(Utils::Icons::RUN_SMALL.icon());

    command = ActionManager::registerAction(action, Constants::ACTION_RUN_UCURSOR_NODEPLOY);
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunUnderCursorTriggered, dd, TestRunMode::RunWithoutDeploy));
    runTestMenu->addAction(command);

    action = new QAction(Tr::tr("&Debug Test"), this);
    action->setEnabled(false);
    action->setIcon(ProjectExplorer::Icons::DEBUG_START_SMALL.icon());

    command = ActionManager::registerAction(action, Constants::ACTION_RUN_DBG_UCURSOR);
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunUnderCursorTriggered, dd, TestRunMode::Debug));
    runTestMenu->addAction(command);

    action = new QAction(Tr::tr("Debug Test Without Deployment"), this);
    action->setEnabled(false);
    action->setIcon(ProjectExplorer::Icons::DEBUG_START_SMALL.icon());

    command = ActionManager::registerAction(action, Constants::ACTION_RUN_DBG_UCURSOR_NODEPLOY);
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunUnderCursorTriggered, dd, TestRunMode::DebugWithoutDeploy));
    runTestMenu->addAction(command);
}

ExtensionSystem::IPlugin::ShutdownFlag AutotestPlugin::aboutToShutdown()
{
    dd->m_testCodeParser.aboutToShutdown(true);
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

    const FilePath &fileName = document->filePath();
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
    QTC_ASSERT(currentEditor && currentEditor->textDocument(), return);
    const int line = currentEditor->currentLine();
    const FilePath filePath = currentEditor->textDocument()->filePath();

    const CPlusPlus::Snapshot snapshot = CppEditor::CppModelManager::snapshot();
    const CPlusPlus::Document::Ptr doc = snapshot.document(filePath);
    if (doc.isNull()) // not part of C++ snapshot
        return;

    CPlusPlus::Scope *scope = doc->scopeAt(line, currentEditor->currentColumn());
    QTextCursor cursor = currentEditor->editorWidget()->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    const QString text = cursor.selectedText();

    while (scope && scope->asBlock())
        scope = scope->enclosingScope();
    if (scope && scope->asFunction()) { // class, namespace for further stuff?
        const QList<const CPlusPlus::Name *> fullName
                = CPlusPlus::LookupContext::fullyQualifiedName(scope);
        const QString funcName = CPlusPlus::Overview().prettyName(fullName);
        const TestFrameworks active = AutotestPlugin::activeTestFrameworks();
        for (auto framework : active) {
            const QStringList testName = framework->testNameForSymbolName(funcName);
            if (!testName.size())
                continue;
            TestTreeItem *it = framework->rootNode()->findTestByNameAndFile(testName, filePath);
            if (it) {
                const QList<ITestConfiguration *> testsToRun
                        = testItemsToTestConfigurations({ it }, mode);
                if (!testsToRun.isEmpty()) {
                    m_testRunner.runTests(mode, testsToRun);
                    return;
                }
            }
        }
    }

    // general approach
    if (text.isEmpty())
        return; // Do not trigger when no name under cursor

    const QList<ITestTreeItem *> testsItems = m_testTreeModel.testItemsByName(text);
    if (testsItems.isEmpty())
        return; // Wrong location triggered

    // check whether we have been triggered on a test function definition
    QList<ITestTreeItem *> filteredItems = Utils::filtered(testsItems, [&](ITestTreeItem *it){
        return it->line() == line && it->filePath() == filePath;
    });

    if (filteredItems.size() == 0 && testsItems.size() > 1) {
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

void AutotestPluginPrivate::onDisableTemporarily(bool disable)
{
    if (disable) {
        // cancel running parse
        m_testCodeParser.aboutToShutdown(false);
        // clear model
        m_testTreeModel.removeAllTestItems();
        m_testTreeModel.removeAllTestToolItems();
        AutotestPlugin::updateMenuItemsEnabledState();
    } else {
        // re-enable
        m_testCodeParser.setState(TestCodeParser::Idle);
        // trigger scan
        m_testCodeParser.updateTestTree();
    }
}

TestFrameworks AutotestPlugin::activeTestFrameworks()
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    TestFrameworks sorted;
    if (!project || projectSettings(project)->useGlobalSettings()) {
        sorted = Utils::filtered(TestFrameworkManager::registeredFrameworks(),
                                 &ITestFramework::active);
    } else { // we've got custom project settings
        const TestProjectSettings *settings = projectSettings(project);
        const QHash<ITestFramework *, bool> active = settings->activeFrameworks();
        sorted = Utils::filtered(TestFrameworkManager::registeredFrameworks(),
                                 [active](ITestFramework *framework) {
            return active.value(framework, false);
        });
    }
    return sorted;
}

void AutotestPlugin::updateMenuItemsEnabledState()
{
    const ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    const ProjectExplorer::Target *target = project ? project->activeTarget() : nullptr;
    const bool disabled = dd->m_testCodeParser.state() == TestCodeParser::DisabledTemporarily;
    const bool canScan = disabled || (!dd->m_testRunner.isTestRunning()
                                      && dd->m_testCodeParser.state() == TestCodeParser::Idle);
    const bool hasTests = dd->m_testTreeModel.hasTests();
    // avoid expensive call to PE::canRunStartupProject() - limit to minimum necessary checks
    const bool canRun = !disabled && hasTests && canScan
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

bool ChoicePair::matches(const ProjectExplorer::RunConfiguration *rc) const
{
    return rc && rc->displayName() == displayName && rc->runnable().command.executable() == executable;
}

} // Internal
} // Autotest

#include "autotestplugin.moc"
