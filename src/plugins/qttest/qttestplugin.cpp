/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qttestplugin.h"
#include "testselector.h"
#include "dialogs.h"
#include "qsystem.h"
#include "testsettingspropertiespage.h"
#include "resultsview.h"
#include "testexecuter.h"
#include "testcontextmenu.h"
#include "testsuite.h"
#include "testoutputwindow.h"
#include "testconfigurations.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/id.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <qmljseditor/qmljseditorconstants.h>
#include <texteditor/basetexteditor.h>
#include <cppeditor/cppeditorconstants.h>
#include <cpptools/cpptoolsconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <find/ifindsupport.h>
#include <utils/linecolumnlabel.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <debugger/debuggerplugin.h>
#include <debugger/debuggerrunner.h>
#include <debugger/debuggerengine.h>
#include <debugger/debuggerruncontrolfactory.h>
#include <debugger/debuggerstartparameters.h>
#ifdef QTTEST_DEBUGGER_SUPPORT
# include <debugger/qtuitest/qtuitestengine.h>
#endif

#include <QtCore/QtPlugin>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QMenuBar>
#include <QDebug>
#include <QToolButton>

using namespace QtTest::Internal;

enum { debug = 0 };

Core::NavigationView TestNavigationWidgetFactory::createWidget()
{
    Core::NavigationView view;
    TestSelector *ptw = new TestSelector();
    ptw->rescan();
    view.widget = ptw;

    QToolButton *filter = new QToolButton;
    filter->setIcon(QIcon(Core::Constants::ICON_FILTER));
    filter->setToolTip(tr("Filter tree"));
    filter->setPopupMode(QToolButton::InstantPopup);

    QMenu *filterMenu = new QMenu(filter);
    filterMenu->addAction(ptw->m_componentViewMode);
    filterMenu->addAction(ptw->m_showUnitTests);
    filterMenu->addAction(ptw->m_showIntegrationTests);
    filterMenu->addAction(ptw->m_showPerformanceTests);
    filterMenu->addAction(ptw->m_showSystemTests);
    filter->setMenu(filterMenu);

    QToolButton *newTest = new QToolButton;
    newTest->setIcon(QIcon(":/core/images/filenew.png"));
    newTest->setToolTip(tr("New test"));
    newTest->setPopupMode(QToolButton::InstantPopup);
    QObject::connect(newTest, SIGNAL(clicked()), ptw,
        SLOT(testInsertUnitOrSystemTest()), Qt::DirectConnection);

    view.dockToolBarWidgets << filter << newTest;
    return view;
}

QString TestNavigationWidgetFactory::displayName() const
{
    return QtTestPlugin::tr("Tests");
}

//******************************************

QtTestPlugin::QtTestPlugin() :
    m_messageOutputWindow(0), m_testResultsWindow(0),
    m_contextMenu(new TestContextMenu(this))
{
}

QtTestPlugin::~QtTestPlugin()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();

    pm->removeObject(m_messageOutputWindow);
    delete m_messageOutputWindow;

    pm->removeObject(m_testResultsWindow);
    delete m_testResultsWindow;

    delete TestExecuter::instance();
    delete m_contextMenu;
}

bool QtTestPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    addAutoReleasedObject(new TestNavigationWidgetFactory);
    addAutoReleasedObject(new TestSettingsPanelFactory);
    addAutoReleasedObject(new TestConfigurations);

    return true;
}

void QtTestPlugin::extensionsInitialized()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    m_messageOutputWindow = new TestOutputWindow();
    pm->addObject(m_messageOutputWindow);

    m_testResultsWindow = TestResultsWindow::instance();
    connect(m_testResultsWindow, SIGNAL(stopTest()), this, SLOT(stopTesting()));
    connect(m_testResultsWindow, SIGNAL(retryFailedTests(QStringList)),
        this, SLOT(retryTests(QStringList)));
    connect(TestExecuter::instance(), SIGNAL(testStarted()),
        m_testResultsWindow, SLOT(onTestStarted()));
    connect(TestExecuter::instance(), SIGNAL(testStop()),
        m_testResultsWindow, SLOT(onTestStopped()));
    connect(TestExecuter::instance(), SIGNAL(testFinished()),
        m_testResultsWindow, SLOT(onTestFinished()));
    pm->addObject(m_testResultsWindow);
    connect(testResultsPane(), SIGNAL(defectSelected(TestCaseRec)),
        this, SLOT(onDefectSelected(TestCaseRec)));

    // Add context menu to CPP editor
    Core::ActionContainer *mcontext = am->actionContainer(CppEditor::Constants::M_CONTEXT);
    m_contextMenu->init(mcontext->menu(), 2, this);

    // Add context menu to JS editor
    mcontext = am->actionContainer(QmlJSEditor::Constants::M_CONTEXT);
    m_contextMenu->init(mcontext->menu(), 2, this);

    // Add a Test menu to the menu bar
    Core::ActionContainer* ac = am->createMenu("QtTestPlugin.TestMenu");
    ac->menu()->setTitle(tr("&Test"));
    m_contextMenu->init(ac->menu(), 0, 0);

    // Insert the "Test" menu between "Window" and "Help".
    QMenu *windowMenu = am->actionContainer(Core::Constants::M_TOOLS)->menu();
    QMenuBar *menuBar = am->actionContainer(Core::Constants::MENU_BAR)->menuBar();
    menuBar->insertMenu(windowMenu->menuAction(), ac->menu());

    ProjectExplorer::ProjectExplorerPlugin *explorer =
        ProjectExplorer::ProjectExplorerPlugin::instance();

    connect(explorer->session(), SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        this, SLOT(onStartupProjectChanged(ProjectExplorer::Project *)));

    connect(core->progressManager(), SIGNAL(allTasksFinished(QString)),
        this, SLOT(onAllTasksFinished(QString)));

    connect(explorer->session(), SIGNAL(aboutToRemoveProject(ProjectExplorer::Project *)),
        this, SLOT(onProjectRemoved(ProjectExplorer::Project *)));

    m_contextMenu->init(0, 3, this);
}

void QtTestPlugin::onDefectSelected(TestCaseRec rec)
{
    if (rec.m_code) {
        int line = (rec.m_line > 0 ? rec.m_line : 0);
        TextEditor::BaseTextEditorWidget::openEditorAt(rec.m_code->actualFileName(), line);
    }
}

void QtTestPlugin::onStartupProjectChanged(ProjectExplorer::Project *project)
{
    TestConfigurations::instance().setActiveConfiguration(project);
}

void QtTestPlugin::onProjectRemoved(ProjectExplorer::Project *project)
{
    if (project == startupProject())
        TestConfigurations::instance().setActiveConfiguration(0);
}

void QtTestPlugin::onAllTasksFinished(const QString &t)
{
    if ((t == CppTools::Constants::TASK_INDEX) && startupProject())
        TestConfigurations::instance().setActiveConfiguration(startupProject());
}

void QtTestPlugin::testRun()
{
    runSelectedTests(false, false);
}

void QtTestPlugin::testRunAsManual()
{
    runSelectedTests(false, true);
}

void QtTestPlugin::testDebug()
{
    Debugger::DebuggerRunControl *runControl = 0;
    Debugger::DebuggerStartParameters params;
    params.startMode = Debugger::NoStartMode; // we'll start the test runner here
    params.executable = QLatin1String(".qtt");
    runControl = Debugger::DebuggerPlugin::createDebugger(params);
    if (debug)
        qDebug() << "Debugger run control" << runControl;
    runControl->start();

#ifdef QTTEST_DEBUGGER_SUPPORT
    Debugger::Internal::QtUiTestEngine *engine =
        qobject_cast<Debugger::Internal::QtUiTestEngine*>(runControl->engine());
    TestExecuter::instance()->setDebugEngine(engine);
#endif

    runSelectedTests(false, false);

#ifdef QTTEST_DEBUGGER_SUPPORT
    TestExecuter::instance()->setDebugEngine(0);
    runControl->debuggingFinished();
#endif
}

void QtTestPlugin::testRunSingle()
{
    runSelectedTests(true, false);
}

void QtTestPlugin::runSelectedTests(bool singleTest, bool forceManual)
{
    m_forceManual = forceManual;
    TestExecuter::instance()->runTests(singleTest, forceManual);
}

void QtTestPlugin::stopTesting()
{
    TestExecuter::instance()->manualStop();
}

void QtTestPlugin::retryTests(const QStringList &tests)
{
    QStringList currentSelection(TestConfigurations::instance().selectedTests());
    QStringList newSelection;

    foreach (const QString &test, currentSelection) {
        QString testName = test.mid(test.lastIndexOf(QLatin1Char('/')) + 1);
        if (tests.contains(testName))
            newSelection.append(test);
    }
    TestExecuter::instance()->setSelectedTests(newSelection);
    TestExecuter::instance()->runSelectedTests(m_forceManual);
}

void QtTestPlugin::insertTestFunction()
{
    TestCode *currentTest = m_testCollection.currentEditedTest();
    if (currentTest) {
        QString prompt = QLatin1String("<b>") + currentTest->testTypeString()
            +  QLatin1String(" Test: </b>") + currentTest->testCase();
        QPointer<NewTestFunctionDlg> dlg = new NewTestFunctionDlg(prompt);
        if (dlg->exec() == QDialog::Accepted) {
            QString testFunc = dlg->testFunctionName->text();
            // check for duplicate
            if (TestFunctionInfo *functionInfo = currentTest->findFunction(testFunc)) {
                QMessageBox::critical(0, tr("Error"),
                    tr("Test function \"%1\" already exists.").arg(testFunc));
                currentTest->gotoLine(functionInfo->testStartLine());
                return;
            }
            currentTest->addTestFunction(testFunc, QString(), dlg->insertAtCursor->isChecked());
        }
        delete dlg;
    }
}

ProjectExplorer::Project *QtTestPlugin::startupProject()
{
    return ProjectExplorer::ProjectExplorerPlugin::instance()->session()->startupProject();
}


Q_EXPORT_PLUGIN(QtTestPlugin)
