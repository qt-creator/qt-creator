/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
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

#ifdef LICENSECHECKER
#  include <licensechecker/licensecheckerplugin.h>
#endif

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

bool AutotestPlugin::checkLicense()
{
#ifdef LICENSECHECKER
    LicenseChecker::LicenseCheckerPlugin *licenseChecker
            = ExtensionSystem::PluginManager::getObject<LicenseChecker::LicenseCheckerPlugin>();

    if (!licenseChecker || !licenseChecker->hasValidLicense()) {
        qWarning() << "Invalid license, disabling Qt Creator Enterprise Auto Test Add-on.";
        return false;
    } else if (!licenseChecker->enterpriseFeatures())
        return false;
#endif // LICENSECHECKER
    return true;
}

void AutotestPlugin::initializeMenuEntries()
{
    ActionContainer *menu = ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(tr("Tests"));

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

    if (!checkLicense())
        return true;

    initializeMenuEntries();

    m_settings->fromSettings(ICore::settings());
    addAutoReleasedObject(new TestSettingsPage(m_settings));
    addAutoReleasedObject(new TestNavigationWidgetFactory);
    addAutoReleasedObject(TestResultsPane::instance());

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
