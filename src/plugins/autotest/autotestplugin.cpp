/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "autotestplugin.h"
#include "autotestconstants.h"
#include "autotesticons.h"
#include "projectsettingswidget.h"
#include "testcodeparser.h"
#include "testframeworkmanager.h"
#include "testprojectsettings.h"
#include "testrunner.h"
#include "testsettings.h"
#include "testsettingspage.h"
#include "testtreeitem.h"
#include "testtreeview.h"
#include "testtreemodel.h"
#include "testresultspane.h"
#include "testnavigationwidget.h"

#include "qtest/qttestframework.h"
#include "quick/quicktestframework.h"
#include "gtest/gtestframework.h"
#include "boost/boosttestframework.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/messagemanager.h>
#include <cppeditor/cppeditorconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <utils/textutils.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QList>
#include <QMap>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>
#include <QTextCursor>
#include <QtPlugin>

#ifdef WITH_TESTS
#include "autotestunittests.h"
#endif

using namespace Core;

namespace Autotest {
namespace Internal {

class AutotestPluginPrivate : public QObject
{
    Q_OBJECT
public:
    explicit AutotestPluginPrivate(AutotestPlugin *parent);
    ~AutotestPluginPrivate() override;

    AutotestPlugin *q = nullptr;
    TestFrameworkManager *m_frameworkManager = nullptr;
    TestSettingsPage *m_testSettingPage = nullptr;
    TestNavigationWidgetFactory *m_navigationWidgetFactory = nullptr;
    TestResultsPane *m_resultsPane = nullptr;
    QMap<QString, ChoicePair> m_runconfigCache;

    void initializeMenuEntries();
    void onRunAllTriggered();
    void onRunSelectedTriggered();
    void onRunFileTriggered();
    void onRunUnderCursorTriggered(TestRunMode mode);
};

static AutotestPlugin *s_instance = nullptr;
static QHash<ProjectExplorer::Project *, TestProjectSettings *> s_projectSettings;

AutotestPlugin::AutotestPlugin()
    : m_settings(new TestSettings)
{
    // needed to be used in QueuedConnection connects
    qRegisterMetaType<TestResult>();
    qRegisterMetaType<TestTreeItem *>();
    qRegisterMetaType<TestCodeLocationAndType>();

    s_instance = this;
}

AutotestPlugin::~AutotestPlugin()
{
    delete d;
}

AutotestPluginPrivate::AutotestPluginPrivate(AutotestPlugin *parent)
    : q(parent)
{
    m_frameworkManager = TestFrameworkManager::instance();
    initializeMenuEntries();
    m_frameworkManager->registerTestFramework(new QtTestFramework);
    m_frameworkManager->registerTestFramework(new QuickTestFramework);
    m_frameworkManager->registerTestFramework(new GTestFramework);
    m_frameworkManager->registerTestFramework(new BoostTestFramework);

    m_frameworkManager->synchronizeSettings(ICore::settings());
    m_testSettingPage = new TestSettingsPage(q->settings());
    m_navigationWidgetFactory = new TestNavigationWidgetFactory;
    m_resultsPane = TestResultsPane::instance();

    auto panelFactory = new ProjectExplorer::ProjectPanelFactory();
    panelFactory->setPriority(666);
//    panelFactory->setIcon();  // TODO ?
    panelFactory->setDisplayName(tr("Testing"));
    panelFactory->setCreateWidgetFunction([](ProjectExplorer::Project *project) {
        return new ProjectTestSettingsWidget(project);
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);

    m_frameworkManager->activateFrameworksFromSettings(q->settings());
    TestTreeModel::instance()->synchronizeTestFrameworks();

    connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::startupProjectChanged, this, [this] {
        m_runconfigCache.clear();
    });

}

AutotestPluginPrivate::~AutotestPluginPrivate()
{
    delete m_navigationWidgetFactory;
    delete m_resultsPane;
    delete m_testSettingPage;
    delete m_frameworkManager;
}

QSharedPointer<TestSettings> AutotestPlugin::settings()
{
    return s_instance->m_settings;
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
    menu->menu()->setTitle(tr("&Tests"));
    menu->setOnAllDisabledBehavior(ActionContainer::Show);

    QAction *action = new QAction(tr("Run &All Tests"), this);
    action->setIcon(Utils::Icons::RUN_SMALL_TOOLBAR.icon());
    action->setToolTip(tr("Run All Tests"));
    Command *command = ActionManager::registerAction(action, Constants::ACTION_RUN_ALL_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? tr("Ctrl+Meta+T, Ctrl+Meta+A") : tr("Alt+Shift+T,Alt+A")));
    connect(action, &QAction::triggered, this, &AutotestPluginPrivate::onRunAllTriggered);
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(tr("&Run Selected Tests"), this);
    Utils::Icon runSelectedIcon = Utils::Icons::RUN_SMALL_TOOLBAR;
    for (const Utils::IconMaskAndColor &maskAndColor : Icons::RUN_SELECTED_OVERLAY)
        runSelectedIcon.append(maskAndColor);
    action->setIcon(runSelectedIcon.icon());
    action->setToolTip(tr("Run Selected Tests"));
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_SELECTED_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? tr("Ctrl+Meta+T, Ctrl+Meta+R") : tr("Alt+Shift+T,Alt+R")));
    connect(action, &QAction::triggered, this, &AutotestPluginPrivate::onRunSelectedTriggered);
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(tr("Run Tests for Current &File"), this);
    Utils::Icon runFileIcon = Utils::Icons::RUN_SMALL_TOOLBAR;
    for (const Utils::IconMaskAndColor &maskAndColor : Icons::RUN_FILE_OVERLAY)
        runFileIcon.append(maskAndColor);
    action->setIcon(runFileIcon.icon());
    action->setToolTip(tr("Run Tests for Current File"));
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_FILE_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? tr("Ctrl+Meta+T, Ctrl+Meta+F") : tr("Alt+Shift+T,Alt+F")));
    connect(action, &QAction::triggered, this, &AutotestPluginPrivate::onRunFileTriggered);
    action->setEnabled(false);
    menu->addAction(command);

    action = new QAction(tr("Re&scan Tests"), this);
    command = ActionManager::registerAction(action, Constants::ACTION_SCAN_ID);
    command->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? tr("Ctrl+Meta+T, Ctrl+Meta+S") : tr("Alt+Shift+T,Alt+S")));
    connect(action, &QAction::triggered, this, []() {
        TestTreeModel::instance()->parser()->updateTestTree();
    });
    menu->addAction(command);

    ActionContainer *toolsMenu = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsMenu->addMenu(menu);
    using namespace ProjectExplorer;
    connect(BuildManager::instance(), &BuildManager::buildStateChanged,
            this, &AutotestPlugin::updateMenuItemsEnabledState);
    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            this, &AutotestPlugin::updateMenuItemsEnabledState);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::updateRunActions,
            this, &AutotestPlugin::updateMenuItemsEnabledState);
    connect(TestTreeModel::instance(), &TestTreeModel::testTreeModelChanged,
            this, &AutotestPlugin::updateMenuItemsEnabledState);
}

bool AutotestPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    d = new AutotestPluginPrivate(this);
    return true;
}

void AutotestPlugin::extensionsInitialized()
{
    ActionContainer *contextMenu = ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT);
    if (!contextMenu) // if QC is started without CppEditor plugin
        return;

    QAction *action = new QAction(tr("&Run Test Under Cursor"), this);
    action->setEnabled(false);
    action->setIcon(Utils::Icons::RUN_SMALL_TOOLBAR.icon());

    Command *command = ActionManager::registerAction(action, Constants::ACTION_RUN_UCURSOR);
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunUnderCursorTriggered, d, TestRunMode::Run));
    contextMenu->addSeparator();
    contextMenu->addAction(command);

    action = new QAction(tr("&Debug Test Under Cursor"), this);
    action->setEnabled(false);
    action->setIcon(ProjectExplorer::Icons::DEBUG_START_SMALL.icon());

    command = ActionManager::registerAction(action, Constants::ACTION_RUN_DBG_UCURSOR);
    connect(action, &QAction::triggered,
            std::bind(&AutotestPluginPrivate::onRunUnderCursorTriggered, d, TestRunMode::Debug));
    contextMenu->addAction(command);
    contextMenu->addSeparator();
}

ExtensionSystem::IPlugin::ShutdownFlag AutotestPlugin::aboutToShutdown()
{
    TestTreeModel::instance()->parser()->aboutToShutdown();
    return SynchronousShutdown;
}

void AutotestPluginPrivate::onRunAllTriggered()
{
    TestRunner *runner = TestRunner::instance();
    TestTreeModel *model = TestTreeModel::instance();
    runner->setSelectedTests(model->getAllTestCases());
    runner->prepareToRunTests(TestRunMode::Run);
}

void AutotestPluginPrivate::onRunSelectedTriggered()
{
    TestRunner *runner = TestRunner::instance();
    TestTreeModel *model = TestTreeModel::instance();
    runner->setSelectedTests(model->getSelectedTests());
    runner->prepareToRunTests(TestRunMode::Run);
}

void AutotestPluginPrivate::onRunFileTriggered()
{
    const IDocument *document = EditorManager::currentDocument();
    if (!document)
        return;

    const Utils::FilePath &fileName = document->filePath();
    if (fileName.isEmpty())
        return;

    TestTreeModel *model = TestTreeModel::instance();
    const QList<TestConfiguration *> tests = model->getTestsForFile(fileName);
    if (tests.isEmpty())
        return;

    TestRunner *runner = TestRunner::instance();
    runner->setSelectedTests(tests);
    runner->prepareToRunTests(TestRunMode::Run);
}

static QList<TestConfiguration *> testItemsToTestConfigurations(const QList<TestTreeItem *> &items,
                                                                TestRunMode mode)
{
    QList<TestConfiguration *> configs;
    for (const TestTreeItem * item : items) {
        if (TestConfiguration *currentConfig = item->asConfiguration(mode))
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

    const QList<TestTreeItem *> testsItems = TestTreeModel::instance()->testItemsByName(text);
    if (testsItems.isEmpty())
        return; // Wrong location triggered

    // check whether we have been triggered on a test function definition
    const int line = currentEditor->currentLine();
    const QString &filePath = currentEditor->textDocument()->filePath().toString();
    const QList<TestTreeItem *> filteredItems = Utils::filtered(testsItems, [&](TestTreeItem *it){
        return it->line() == line && it->filePath() == filePath;
    });

    const QList<TestConfiguration *> testsToRun = testItemsToTestConfigurations(
                filteredItems.size() == 1 ? filteredItems : testsItems, mode);

    if (testsToRun.isEmpty()) {
        MessageManager::write(tr("Selected test was not found (%1).").arg(text), MessageManager::Flash);
        return;
    }

    auto runner = TestRunner::instance();
    runner->setSelectedTests(testsToRun);
    runner->prepareToRunTests(mode);
}

void AutotestPlugin::updateMenuItemsEnabledState()
{
    const ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    const ProjectExplorer::Target *target = project ? project->activeTarget() : nullptr;
    const bool canScan = !TestRunner::instance()->isTestRunning()
            && TestTreeModel::instance()->parser()->state() == TestCodeParser::Idle;
    const bool hasTests = TestTreeModel::instance()->hasTests();
    // avoid expensive call to PE::canRunStartupProject() - limit to minimum necessary checks
    const bool canRun = hasTests && canScan
            && project && !project->needsConfiguration()
            && target && target->activeRunConfiguration()
            && !ProjectExplorer::BuildManager::isBuilding();

    ActionManager::command(Constants::ACTION_RUN_ALL_ID)->action()->setEnabled(canRun);
    ActionManager::command(Constants::ACTION_RUN_SELECTED_ID)->action()->setEnabled(canRun);
    ActionManager::command(Constants::ACTION_RUN_FILE_ID)->action()->setEnabled(canRun);
    ActionManager::command(Constants::ACTION_SCAN_ID)->action()->setEnabled(canScan);

    ActionContainer *contextMenu = ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT);
    if (!contextMenu)
        return; // When no context menu, actions do not exists

    ActionManager::command(Constants::ACTION_RUN_UCURSOR)->action()->setEnabled(canRun);
    ActionManager::command(Constants::ACTION_RUN_DBG_UCURSOR)->action()->setEnabled(canRun);
}

void AutotestPlugin::cacheRunConfigChoice(const QString &buildTargetKey, const ChoicePair &choice)
{
    if (s_instance)
        s_instance->d->m_runconfigCache.insert(buildTargetKey, choice);
}

ChoicePair AutotestPlugin::cachedChoiceFor(const QString &buildTargetKey)
{
    return s_instance ? s_instance->d->m_runconfigCache.value(buildTargetKey) : ChoicePair();
}

void AutotestPlugin::clearChoiceCache()
{
    if (s_instance)
        s_instance->d->m_runconfigCache.clear();
}

void AutotestPlugin::popupResultsPane()
{
    if (s_instance)
        s_instance->d->m_resultsPane->popup(Core::IOutputPane::NoModeSwitch);
}

QVector<QObject *> AutotestPlugin::createTestObjects() const
{
    QVector<QObject *> tests;
#ifdef WITH_TESTS
    tests << new AutoTestUnitTests(TestTreeModel::instance());
#endif
    return tests;
}

bool ChoicePair::matches(const ProjectExplorer::RunConfiguration *rc) const
{
    return rc && rc->displayName() == displayName && rc->runnable().executable.toString() == executable;
}

} // Internal
} // Autotest

#include "autotestplugin.moc"
