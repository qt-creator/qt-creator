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
#include "testcodeparser.h"
#include "testrunner.h"
#include "testsettings.h"
#include "testsettingspage.h"
#include "testtreeitem.h"
#include "testtreeview.h"
#include "testtreemodel.h"
#include "testresultspane.h"
#include "testnavigationwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

#include <QtPlugin>

#ifdef WITH_TESTS
#include "autotestunittests.h"
#endif

using namespace Autotest::Internal;
using namespace Core;

static AutotestPlugin *m_instance = 0;

AutotestPlugin::AutotestPlugin()
    : m_settings(new TestSettings)
{
    // needed to be used in QueuedConnection connects
    qRegisterMetaType<TestResult>();
    qRegisterMetaType<TestTreeItem *>();
    qRegisterMetaType<TestCodeLocationAndType>();
    qRegisterMetaType<TestTreeModel::Type>();

    m_instance = this;
}

AutotestPlugin::~AutotestPlugin()
{
    // Delete members
    TestTreeModel *model = TestTreeModel::instance();
    delete model;
    TestRunner *runner = TestRunner::instance();
    delete runner;
}

AutotestPlugin *AutotestPlugin::instance()
{
    return m_instance;
}

QSharedPointer<TestSettings> AutotestPlugin::settings() const
{
    return m_settings;
}

void AutotestPlugin::initializeMenuEntries()
{
    ActionContainer *menu = ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(tr("&Tests"));

    QAction *action = new QAction(tr("Run &All Tests"), this);
    Command *command = ActionManager::registerAction(action, Constants::ACTION_RUN_ALL_ID);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+Shift+T,Alt+A")));
    connect(action, &QAction::triggered,
            this, &AutotestPlugin::onRunAllTriggered);
    menu->addAction(command);

    action = new QAction(tr("&Run Selected Tests"), this);
    command = ActionManager::registerAction(action, Constants::ACTION_RUN_SELECTED_ID);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+Shift+T,Alt+R")));
    connect(action, &QAction::triggered,
            this, &AutotestPlugin::onRunSelectedTriggered);
    menu->addAction(command);

    action = new QAction(tr("Re&scan Tests"), this);
    command = ActionManager::registerAction(action, Constants::ACTION_SCAN_ID);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+Shift+T,Alt+S")));
    connect(action, &QAction::triggered,
            TestTreeModel::instance()->parser(), &TestCodeParser::updateTestTree);
    menu->addAction(command);

    ActionContainer *toolsMenu = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsMenu->addMenu(menu);
    connect(toolsMenu->menu(), &QMenu::aboutToShow,
            this, &AutotestPlugin::updateMenuItemsEnabledState);
}

bool AutotestPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    initializeMenuEntries();

    m_settings->fromSettings(ICore::settings());
    addAutoReleasedObject(new TestSettingsPage(m_settings));
    addAutoReleasedObject(new TestNavigationWidgetFactory);
    addAutoReleasedObject(TestResultsPane::instance());

    if (m_settings->alwaysParse)
        TestTreeModel::instance()->enableParsingFromSettings();

    return true;
}

void AutotestPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag AutotestPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

void AutotestPlugin::onRunAllTriggered()
{
    TestRunner *runner = TestRunner::instance();
    TestTreeModel *model = TestTreeModel::instance();
    runner->setSelectedTests(model->getAllTestCases());
    runner->prepareToRunTests();
}

void AutotestPlugin::onRunSelectedTriggered()
{
    TestRunner *runner = TestRunner::instance();
    TestTreeModel *model = TestTreeModel::instance();
    runner->setSelectedTests(model->getSelectedTests());
    runner->prepareToRunTests();
}

void AutotestPlugin::updateMenuItemsEnabledState()
{
    const bool enabled = !TestRunner::instance()->isTestRunning()
            && TestTreeModel::instance()->parser()->state() == TestCodeParser::Idle;
    const bool hasTests = TestTreeModel::instance()->hasTests();

    ActionManager::command(Constants::ACTION_RUN_ALL_ID)->action()->setEnabled(enabled && hasTests);
    ActionManager::command(Constants::ACTION_RUN_SELECTED_ID)->action()->setEnabled(enabled && hasTests);
    ActionManager::command(Constants::ACTION_SCAN_ID)->action()->setEnabled(enabled);
}

QList<QObject *> AutotestPlugin::createTestObjects() const
{
    QList<QObject *> tests;
#ifdef WITH_TESTS
    tests << new AutoTestUnitTests(TestTreeModel::instance());
#endif
    return tests;
}
