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
#include "qtest/datataglocatorfilter.h"
#include "qtest/qttestframework.h"
#include "quick/quicktestframework.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppmodelmanager.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectmanager.h>
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
using namespace ProjectExplorer;
using namespace Utils;

namespace Autotest::Internal {

class AutotestPluginPrivate final : public QObject
{
    Q_OBJECT
public:
    AutotestPluginPrivate();
    ~AutotestPluginPrivate() final;

    TestResultsPane *m_resultsPane = nullptr;
    QMap<QString, ChoicePair> m_runconfigCache;

    void initializeMenuEntries();
    void onRunAllTriggered(TestRunMode mode);
    void onRunSelectedTriggered(TestRunMode mode);
    void onRunFailedTriggered();
    void onRunFileTriggered();
    void onRunUnderCursorTriggered(TestRunMode mode);
    void onDisableTemporarily(bool disable);

    TestCodeParser m_testCodeParser;
    TestTreeModel m_testTreeModel{&m_testCodeParser};
    TestRunner m_testRunner;
    DataTagLocatorFilter m_dataTagLocatorFilter;
#ifdef WITH_TESTS
    LoadProjectScenario m_loadProjectScenario{&m_testTreeModel};
#endif
};

static AutotestPluginPrivate *dd = nullptr;
static QHash<Project *, TestProjectSettings *> s_projectSettings;

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

    setupAutotestProjectPanel();

    TestFrameworkManager::activateFrameworksAndToolsFromSettings();
    m_testTreeModel.synchronizeTestFrameworks();
    m_testTreeModel.synchronizeTestTools();

    auto projectManager = ProjectManager::instance();
    connect(projectManager, &ProjectManager::startupProjectChanged,
            this, [this] { m_runconfigCache.clear(); });

    connect(projectManager, &ProjectManager::aboutToRemoveProject, this, [](Project *project) {
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

TestProjectSettings *projectSettings(Project *project)
{
    auto &settings = s_projectSettings[project];
    if (!settings)
        settings = new TestProjectSettings(project);

    return settings;
}

void AutotestPluginPrivate::initializeMenuEntries()
{
    const Id menuId = Constants::MENU_ID;

    MenuBuilder(menuId)
        .setTitle(Tr::tr("&Tests"))
        .setOnAllDisabledBehavior(ActionContainer::Show)
        .addToContainer(Core::Constants::M_TOOLS);

    ActionBuilder(this, Constants::ACTION_RUN_ALL_ID)
        .setText(Tr::tr("Run &All Tests"))
        .setIcon(Utils::Icons::RUN_SMALL.icon())
        .setToolTip(Tr::tr("Run All Tests"))
        .setDefaultKeySequence(Tr::tr("Ctrl+Meta+T, Ctrl+Meta+A"), Tr::tr("Alt+Shift+T,Alt+A"))
        .addToContainer(menuId)
        .setEnabled(false)
        .addOnTriggered(this, [this] { onRunAllTriggered(TestRunMode::Run); });

    ActionBuilder(this, Constants::ACTION_RUN_ALL_NODEPLOY_ID)
        .setText(Tr::tr("Run All Tests Without Deployment"))
        .setIcon(Utils::Icons::RUN_SMALL.icon())
        .setToolTip(Tr::tr("Run All Tests Without Deployment"))
        .setDefaultKeySequence(Tr::tr("Ctrl+Meta+T, Ctrl+Meta+E"), Tr::tr("Alt+Shift+T,Alt+E"))
        .addToContainer(menuId)
        .setEnabled(false)
        .addOnTriggered(this, [this] { onRunAllTriggered(TestRunMode::RunWithoutDeploy); });

    ActionBuilder(this, Constants::ACTION_RUN_SELECTED_ID)
        .setText(Tr::tr("&Run Selected Tests"))
        .setIcon(Utils::Icons::RUN_SELECTED.icon())
        .setToolTip(Tr::tr("Run Selected Tests"))
        .setDefaultKeySequence(Tr::tr("Ctrl+Meta+T, Ctrl+Meta+R"), Tr::tr("Alt+Shift+T,Alt+R"))
        .addToContainer(menuId)
        .setEnabled(false)
        .addOnTriggered(this, [this] { onRunSelectedTriggered(TestRunMode::Run); });

    ActionBuilder(this, Constants::ACTION_RUN_SELECTED_NODEPLOY_ID)
        .setText(Tr::tr("&Run Selected Tests Without Deployment"))
        .setIcon(Utils::Icons::RUN_SELECTED.icon())
        .setToolTip(Tr::tr("Run Selected Tests Without Deployment"))
        .setDefaultKeySequence(Tr::tr("Ctrl+Meta+T, Ctrl+Meta+W"), Tr::tr("Alt+Shift+T,Alt+W"))
        .addToContainer(menuId)
        .setEnabled(false)
        .addOnTriggered(this, [this] { onRunSelectedTriggered(TestRunMode::RunWithoutDeploy); });

    ActionBuilder(this, Constants::ACTION_RUN_FAILED_ID)
        .setText(Tr::tr("Run &Failed Tests"))
        .setIcon(Icons::RUN_FAILED.icon())
        .setToolTip(Tr::tr("Run Failed Tests"))
        .setDefaultKeySequence(Tr::tr("Ctrl+Meta+T, Ctrl+Meta+F"), Tr::tr("Alt+Shift+T,Alt+F"))
        .addToContainer(menuId)
        .setEnabled(false)
        .addOnTriggered(this, [this] { onRunFailedTriggered(); });

    ActionBuilder(this, Constants::ACTION_RUN_FILE_ID)
        .setText(Tr::tr("Run Tests for &Current File"))
        .setIcon(Utils::Icons::RUN_FILE.icon())
        .setToolTip(Tr::tr("Run Tests for Current File"))
        .setDefaultKeySequence(Tr::tr("Ctrl+Meta+T, Ctrl+Meta+C"), Tr::tr("Alt+Shift+T,Alt+C"))
        .addToContainer(menuId)
        .setEnabled(false)
        .addOnTriggered(this, [this] { onRunFileTriggered(); });

    ActionBuilder(this, Constants::ACTION_DISABLE_TMP)
        .setText(Tr::tr("Disable Temporarily"))
        .setToolTip(Tr::tr("Disable scanning and other actions until explicitly rescanning, "
                           "re-enabling, or restarting Qt Creator."))
        .setCheckable(true)
        .addToContainer(menuId)
        .addOnTriggered(this, [this](bool on) { onDisableTemporarily(on); });

    ActionBuilder(this, Constants::ACTION_SCAN_ID)
        .setText(Tr::tr("Re&scan Tests"))
        .setDefaultKeySequence(Tr::tr("Ctrl+Meta+T, Ctrl+Meta+S"), Tr::tr("Alt+Shift+T,Alt+S"))
        .addToContainer(menuId)
        .addOnTriggered(this, [] {
            if (dd->m_testCodeParser.state() == TestCodeParser::DisabledTemporarily)
                dd->onDisableTemporarily(false);  // Rescan Test should explicitly re-enable
            else
                dd->m_testCodeParser.updateTestTree();
        });

    connect(BuildManager::instance(), &BuildManager::buildStateChanged,
            this, &updateMenuItemsEnabledState);
    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            this, &updateMenuItemsEnabledState);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
            this, &updateMenuItemsEnabledState);
    connect(&dd->m_testTreeModel, &TestTreeModel::testTreeModelChanged,
            this, &updateMenuItemsEnabledState);
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
        const TestFrameworks active = activeTestFrameworks();
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
        updateMenuItemsEnabledState();
    } else {
        // re-enable
        m_testCodeParser.setState(TestCodeParser::Idle);
        // trigger scan
        m_testCodeParser.updateTestTree();
    }
}

TestFrameworks activeTestFrameworks()
{
    Project *project = ProjectManager::startupProject();
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

void updateMenuItemsEnabledState()
{
    const Project *project = ProjectManager::startupProject();
    const Target *target = project ? project->activeTarget() : nullptr;
    const bool disabled = dd->m_testCodeParser.state() == TestCodeParser::DisabledTemporarily;
    const bool canScan = disabled || (!dd->m_testRunner.isTestRunning()
                                      && dd->m_testCodeParser.state() == TestCodeParser::Idle);
    const bool hasTests = dd->m_testTreeModel.hasTests();
    // avoid expensive call to PE::canRunStartupProject() - limit to minimum necessary checks
    const bool canRun = !disabled && hasTests && canScan
            && project && !project->needsConfiguration()
            && target && target->activeRunConfiguration()
            && !BuildManager::isBuilding();
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

void cacheRunConfigChoice(const QString &buildTargetKey, const ChoicePair &choice)
{
    if (dd)
        dd->m_runconfigCache.insert(buildTargetKey, choice);
}

ChoicePair cachedChoiceFor(const QString &buildTargetKey)
{
    return dd ? dd->m_runconfigCache.value(buildTargetKey) : ChoicePair();
}

void clearChoiceCache()
{
    if (dd)
        dd->m_runconfigCache.clear();
}

void popupResultsPane()
{
    if (dd)
        dd->m_resultsPane->popup(Core::IOutputPane::NoModeSwitch);
}

QString wildcardPatternFromString(const QString &original)
{
    QString pattern = original;
    pattern.replace('\\', "\\\\");
    pattern.replace('.', "\\.");
    pattern.replace('^', "\\^").replace('$', "\\$");
    pattern.replace('(', "\\(").replace(')', "\\)");
    pattern.replace('[', "\\[").replace(']', "\\]");
    pattern.replace('{', "\\{").replace('}', "\\}");
    pattern.replace('+', "\\+");
    pattern.replace('*', ".*");
    pattern.replace('?', '.');
    return pattern;
}

bool ChoicePair::matches(const RunConfiguration *rc) const
{
    return rc && rc->displayName() == displayName && rc->runnable().command.executable() == executable;
}

// AutotestPlugin

class AutotestPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "AutoTest.json")

public:
    AutotestPlugin()
    {
        // needed to be used in QueuedConnection connects
        qRegisterMetaType<TestResult>();
        qRegisterMetaType<TestTreeItem *>();
        qRegisterMetaType<TestCodeLocationAndType>();
        // warm up meta type system to be able to read Qt::CheckState with persistent settings
        qRegisterMetaType<Qt::CheckState>();

        setupTestNavigationWidgetFactory();
    }

    ~AutotestPlugin() final
    {
        delete dd;
        dd = nullptr;
    }

    void initialize() final
    {
        setupTestSettingsPage();

        dd = new AutotestPluginPrivate;
    #ifdef WITH_TESTS
        ExtensionSystem::PluginManager::registerScenario("TestModelManagerInterface",
                       [] { return dd->m_loadProjectScenario(); });

        addTestCreator(createAutotestUnitTests);
    #endif
    }

    void extensionsInitialized()
    {
        ActionContainer *contextMenu = ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT);
        if (!contextMenu) // if QC is started without CppEditor plugin
            return;

        const Id menuId = "Autotest.TestUnderCursor";
        ActionContainer * const runTestMenu = ActionManager::createMenu(menuId);
        runTestMenu->menu()->setTitle(Tr::tr("Run Test Under Cursor"));
        contextMenu->addSeparator();
        contextMenu->addMenu(runTestMenu);
        contextMenu->addSeparator();

        ActionBuilder(this, Constants::ACTION_RUN_UCURSOR)
            .setText(Tr::tr("&Run Test"))
            .setEnabled(false)
            .setIcon(Utils::Icons::RUN_SMALL.icon())
            .addToContainer(menuId)
            .addOnTriggered([] { dd->onRunUnderCursorTriggered(TestRunMode::Run); });

        ActionBuilder(this, Constants::ACTION_RUN_UCURSOR_NODEPLOY)
            .setText(Tr::tr("Run Test Without Deployment"))
            .setIcon(Utils::Icons::RUN_SMALL.icon())
            .setEnabled(false)
            .addToContainer(menuId)
            .addOnTriggered([] { dd->onRunUnderCursorTriggered(TestRunMode::RunWithoutDeploy); });

        ActionBuilder(this, Constants::ACTION_RUN_DBG_UCURSOR)
            .setText(Tr::tr("&Debug Test"))
            .setIcon(ProjectExplorer::Icons::DEBUG_START_SMALL.icon())
            .setEnabled(false)
            .addToContainer(menuId)
            .addOnTriggered([] { dd->onRunUnderCursorTriggered(TestRunMode::Debug); });

        ActionBuilder(this, Constants::ACTION_RUN_DBG_UCURSOR_NODEPLOY)
            .setText(Tr::tr("Debug Test Without Deployment"))
            .setIcon(ProjectExplorer::Icons::DEBUG_START_SMALL.icon())
            .setEnabled(false)
            .addToContainer(menuId)
            .addOnTriggered([] { dd->onRunUnderCursorTriggered(TestRunMode::DebugWithoutDeploy); });
    }

    ShutdownFlag aboutToShutdown() final
    {
        dd->m_testCodeParser.aboutToShutdown(true);
        dd->m_testTreeModel.disconnect();
        return SynchronousShutdown;
    }
};

} // Autotest::Internal

#include "autotestplugin.moc"
