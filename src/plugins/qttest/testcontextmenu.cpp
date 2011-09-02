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

#include "testcontextmenu.h"
#include "testcode.h"
#include "testsettings.h"
#include "qsystem.h"
#include "testsuite.h"
#include "testexecuter.h"
#include "testoutputwindow.h"

#include <coreplugin/icore.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <extensionsystem/iplugin.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QDir>

TestContextMenuPrivate *TestContextMenu::m_instance = 0;
int TestContextMenu::m_refCount = 0;

TestContextMenu::TestContextMenu(QObject *widget)
{
    if (m_refCount++ == 0)
        m_instance = new TestContextMenuPrivate(widget);

    connect(m_instance->m_testToggleCurrentSelectAction, SIGNAL(triggered()),
        this, SIGNAL(toggleSelection()));
    connect(m_instance->m_testSelectAllTestsAction, SIGNAL(triggered()),
         this, SIGNAL(selectAllTests()));
    connect(m_instance->m_testDeselectAllTestsAction, SIGNAL(triggered()),
         this, SIGNAL(deselectAllTests()));
    connect(m_instance->m_testSelectAllManualTestsAction, SIGNAL(triggered()),
         this, SIGNAL(selectAllManualTests()));
}

TestContextMenu::~TestContextMenu()
{
    if (--m_refCount == 0) {
        delete m_instance;
    }
}

void TestContextMenu::init(QMenu *testMenu, int mode, QObject *widget)
{
    if (m_instance)
        m_instance->init(testMenu, mode, widget);
}

void TestContextMenu::languageChange()
{
    if (m_instance)
        m_instance->languageChange();
}

void TestContextMenu::updateActions( bool testVisible, bool testBusy, bool testStopped)
{
    if (m_instance)
        m_instance->updateActions(testVisible, testBusy, testStopped);
}

void TestContextMenu::updateToggleAction(const QString &testName)
{
    if (m_instance)
        m_instance->updateToggleAction(testName);
}

void TestContextMenu::updateSingleTestAction(const QString &testName)
{
    if (m_instance)
        m_instance->updateSingleTestAction(testName);
}

TestContextMenuPrivate::TestContextMenuPrivate(QObject *widget)
{
    m_testInsertUnitOrSystemTestAction = new QAction(widget);
    m_testInsertUnitOrSystemTestAction->setEnabled(false);

    m_editorInsertTestFunctionAction = new QAction(widget);
    m_editorInsertTestFunctionAction->setEnabled(false);

    m_testRunAction = new QAction(widget);
    m_testRunAction->setIcon(QIcon(QPixmap(QLatin1String(":/testrun.png"))));
    m_testRunAsManualAction = new QAction(widget);
    m_testRunAsManualAction->setIcon(QIcon(QPixmap(QLatin1String(":/testrun.png"))));
    m_testDebugAction = new QAction(widget);
    m_testDebugAction->setIcon(QIcon(QPixmap(QLatin1String(":/testlearn.png"))));
    m_editorRunSingleTestAction = new QAction(widget);
    m_editorRunSingleTestAction->setIcon(QIcon(QPixmap(QLatin1String(":/testrun.png"))));
    m_editorRunSingleTestAction->setVisible(false);

    m_testStopTestingAction = new QAction(widget);
    m_testStopTestingAction->setIcon(QIcon(QPixmap(QLatin1String(":/teststop.png"))));
    m_editorStopTestingAction = new QAction(widget);
    m_editorStopTestingAction->setIcon(QIcon(QPixmap(QLatin1String(":/teststop.png"))));

    m_testLearnAction = new QAction(widget);
    m_testLearnAction->setCheckable(true);
    m_testLearnAction->setChecked(m_testSettings.learnMode() == 1);
    QObject::connect(m_testLearnAction, SIGNAL(toggled(bool)), this, SLOT(onLearnChanged()));
    m_testLearnAllAction = new QAction(widget);
    m_testLearnAllAction->setCheckable(true);
    m_testLearnAllAction->setChecked(m_testSettings.learnMode() == 2);
    QObject::connect(m_testLearnAllAction, SIGNAL(toggled(bool)), this, SLOT(onLearnAllChanged()));

    m_testLocalSettingsAction = new QAction(widget);
    m_testToggleCurrentSelectAction = new QAction(widget);
    m_testSelectAllTestsAction = new QAction(widget);
    m_testSelectAllManualTestsAction = new QAction(widget);
    m_testDeselectAllTestsAction = new QAction(widget);
    m_testGroupsAction = new QAction(widget);
    m_testRescanAction = new QAction(widget);
    m_testOpenIncludeFileAction = new QAction(widget);

    languageChange();

    Core::ICore *core = Core::ICore::instance();
    Core::EditorManager* em = core->editorManager();
    QObject::connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)),
         this, SLOT(editorChanged(Core::IEditor*)), Qt::DirectConnection);
}

TestContextMenuPrivate::~TestContextMenuPrivate()
{
}

void TestContextMenuPrivate::init(QMenu *testMenu, int mode, QObject *widget)
{
    if (mode == 0) {
        // menu bar at the top
        testMenu->addAction(m_testInsertUnitOrSystemTestAction);
        testMenu->addAction(m_editorInsertTestFunctionAction);
        testMenu->addSeparator();
        testMenu->addAction(m_testRunAction);
        testMenu->addAction(m_testRunAsManualAction);
        testMenu->addAction(m_testDebugAction);
        testMenu->addAction(m_testStopTestingAction);
        testMenu->addSeparator();
        testMenu->addAction(m_testRescanAction);
        testMenu->addAction(m_testLearnAction);
        testMenu->addAction(m_testLearnAllAction);
    } else if (mode == 1) {
        // context menu in test selection navigator
        testMenu->addAction(m_testToggleCurrentSelectAction);
        testMenu->addAction(m_testSelectAllTestsAction);
        testMenu->addAction(m_testSelectAllManualTestsAction);
        testMenu->addAction(m_testGroupsAction);
        testMenu->addAction(m_testDeselectAllTestsAction);
        testMenu->addSeparator();
        testMenu->addAction(m_testInsertUnitOrSystemTestAction);
        testMenu->addSeparator();
        testMenu->addAction(m_testRunAction);
        testMenu->addAction(m_testRunAsManualAction);
        testMenu->addAction(m_testDebugAction);
        testMenu->addAction(m_testStopTestingAction);
        testMenu->addSeparator();
        testMenu->addAction(m_testRescanAction);
        if (widget) {
            QObject::connect(m_testGroupsAction, SIGNAL(triggered(bool)),
                widget, SLOT(selectGroup()), Qt::DirectConnection);
            QObject::connect(m_testInsertUnitOrSystemTestAction, SIGNAL(triggered(bool)),
                widget, SLOT(testInsertUnitOrSystemTest()), Qt::DirectConnection);
            QObject::connect(m_testRescanAction, SIGNAL(triggered(bool)),
                widget, SLOT(rescan()), Qt::DirectConnection);
        }
    } else if (mode == 2) {
        // context menu in CPP or JS editor
        testMenu->addSeparator();
        testMenu->addAction(m_editorInsertTestFunctionAction);
        testMenu->addSeparator();
        testMenu->addAction(m_editorRunSingleTestAction);
        testMenu->addAction(m_editorStopTestingAction);
        testMenu->addSeparator();
    } else if (mode == 3) {
        // Acttons handled by QtTestPlugin
        QObject::connect(m_testRunAction, SIGNAL(triggered(bool)),
            widget, SLOT(testRun()), Qt::DirectConnection);
        QObject::connect(m_testRunAsManualAction, SIGNAL(triggered(bool)),
            widget, SLOT(testRunAsManual()), Qt::DirectConnection);
        QObject::connect(m_testDebugAction, SIGNAL(triggered(bool)),
            widget, SLOT(testDebug()), Qt::DirectConnection);
        QObject::connect(m_testStopTestingAction, SIGNAL(triggered(bool)),
            widget, SLOT(stopTesting()), Qt::DirectConnection);
        QObject::connect(m_editorRunSingleTestAction, SIGNAL(triggered(bool)),
            widget, SLOT(testRunSingle()), Qt::DirectConnection);
        QObject::connect(m_editorStopTestingAction, SIGNAL(triggered(bool)),
            widget, SLOT(stopTesting()), Qt::DirectConnection);
        QObject::connect(m_editorInsertTestFunctionAction, SIGNAL(triggered(bool)),
            widget, SLOT(insertTestFunction()), Qt::DirectConnection);
    }
}

void TestContextMenuPrivate::onOpenIncludeFile()
{
    emit openIncludeFile(m_includeFile);
}

void TestContextMenuPrivate::updateToggleAction(const QString &testName)
{
    m_testToggleCurrentSelectAction->setVisible(!testName.isEmpty());
    m_testToggleCurrentSelectAction->setText(testName);
}

void TestContextMenuPrivate::updateSingleTestAction(const QString &testName)
{
    m_editorRunSingleTestAction->setVisible(!testName.isEmpty());
    m_editorRunSingleTestAction->setText(tr("Run: '%1'").arg(testName));
}

void TestContextMenuPrivate::updateActions(bool testVisible, bool testBusy, bool testStopped)
{
    m_testInsertUnitOrSystemTestAction->setEnabled(testVisible);
    m_editorInsertTestFunctionAction->setEnabled(testVisible);
    m_testRunAction->setEnabled(!testBusy);
    m_testRunAsManualAction->setEnabled(!testBusy);
    m_testDebugAction->setEnabled(!testBusy);
    m_editorRunSingleTestAction->setEnabled(!testBusy);
    m_testStopTestingAction->setEnabled(testBusy && !testStopped);
    m_editorStopTestingAction->setEnabled(testBusy && !testStopped);
}

void TestContextMenuPrivate::enableIncludeFile(const QString &fileName)
{
    m_includeFile = fileName;
    m_testOpenIncludeFileAction->setText(tr("Open: '%1'").arg(fileName));
    m_testOpenIncludeFileAction->setVisible(!fileName.isEmpty());
}

void TestContextMenuPrivate::languageChange()
{
    m_testInsertUnitOrSystemTestAction->setText(tr("New Test..."));
    m_testInsertUnitOrSystemTestAction->setStatusTip(
        tr("Add a new unit/integration/performance/system test to the currently selected application, library or plugin."));

    m_editorInsertTestFunctionAction->setText(tr("&New Test Function..."));
    m_editorInsertTestFunctionAction->setStatusTip(
        tr("Add a new test function to the currently selected test."));

    m_testRunAction->setText(tr("&Run All Selected Tests"));
    m_testRunAction->setStatusTip(
        tr("Run all currently selected tests. For manual tests a dialog will pop up with a documented set of manual test steps."));
    m_testRunAsManualAction->setText(tr("&Run All Selected Tests as Manual"));
    m_testRunAsManualAction->setStatusTip(
        tr("Run all currently checked tests as manual tests, i.e. automated tests will be converted into a documented set of manual steps and then shown in a dialog."));
    m_testDebugAction->setText(tr("&Debug All Selected Tests"));
    m_testDebugAction->setStatusTip(tr("Debug all currently selected tests."));
    m_editorRunSingleTestAction->setText(tr("&Run"));
    m_editorRunSingleTestAction->setStatusTip(tr("Run currently edited test."));

    m_testStopTestingAction->setText(tr("Stop Testing"));
    m_testStopTestingAction->setStatusTip(tr("Stop the execution of the current test."));
    m_editorStopTestingAction->setText(tr("Stop Testing"));
    m_editorStopTestingAction->setStatusTip(tr("Stop the execution of the current test."));

    m_testLearnAction->setText(tr("Learn New"));
    m_testLearnAction->setStatusTip(tr("Learn test data (snapshots and so on) for new tests."));

    m_testLearnAllAction->setText(tr("Learn All"));
    m_testLearnAllAction->setStatusTip(
        tr("Learn test data (snapshots and so on) for new tests and re-learn data for existing tests."));

    m_testGroupsAction->setText(tr("Select by Groups"));
    m_testGroupsAction->setStatusTip(tr("Select tests based on a group."));

    m_testRescanAction->setText(tr("Rescan All Tests"));
    m_testRescanAction->setStatusTip(tr("Rescan all tests."));

    m_testToggleCurrentSelectAction->setText(tr("Toggle Selection"));
    m_testToggleCurrentSelectAction->setStatusTip(tr("Toggle selection of currently selected test(s)."));

    m_testSelectAllTestsAction->setText(tr("Select All Tests"));
    m_testSelectAllTestsAction->setStatusTip(tr("Select all available tests for testing."));

    m_testSelectAllManualTestsAction->setText(tr("Select All Manual Tests"));
    m_testSelectAllManualTestsAction->setStatusTip(tr("Select all manual tests for testing."));

    m_testDeselectAllTestsAction->setText(tr("Deselect All Tests"));
    m_testDeselectAllTestsAction->setStatusTip(tr("Exclude all tests from testing."));

    m_testOpenIncludeFileAction->setText(tr("Open:"));
    m_testOpenIncludeFileAction->setStatusTip(tr("Open the specified include file."));
}

void TestContextMenuPrivate::editorChanged(Core::IEditor *iface)
{
    bool isTestcase = false;

    if (iface) {
        QString fname = iface->displayName();
        isTestcase = ((fname.endsWith(QLatin1String(".qtt")) || fname.endsWith(QLatin1String(".cpp")))
            && (fname.startsWith(QLatin1String("tst_")) || fname.startsWith(QLatin1String("sys_"))
            || fname.startsWith(QLatin1String("int_")) || fname.startsWith(QLatin1String("prf_"))));
    }

    if (isTestcase) {
        TestCollection tc;
        TestCode *code =
            tc.findCode(QDir::toNativeSeparators(iface->file()->fileName()), QString(), QString());
        isTestcase = (code != 0);
        // Only show "run single test" in context menu if the testcase file is
        // visible (i.e. has focus) in the editor
        m_editorRunSingleTestAction->setVisible(isTestcase
            && m_editorRunSingleTestAction->text().contains(code->testCase()));
    } else {
        m_editorRunSingleTestAction->setVisible(false);
    }

    m_editorInsertTestFunctionAction->setVisible(isTestcase);
    m_editorStopTestingAction->setVisible(isTestcase);
}


void TestContextMenuPrivate::onLearnChanged()
{
    if (m_testLearnAction->isChecked()) {
        if (m_testLearnAllAction->isChecked())
            m_testLearnAllAction->setChecked(false);
        m_testSettings.setLearnMode(1);
    } else {
        m_testSettings.setLearnMode(0);
    }
}

void TestContextMenuPrivate::onLearnAllChanged()
{
    if (m_testLearnAllAction->isChecked()) {
        if (m_testLearnAction->isChecked())
            m_testLearnAction->setChecked(false);
        m_testSettings.setLearnMode(2);
    } else {
        m_testSettings.setLearnMode(0);
    }
}
