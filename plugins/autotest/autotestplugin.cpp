/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
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

#include <licensechecker/licensecheckerplugin.h>

#include <QAction>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

#include <QtPlugin>

#ifdef WITH_TESTS
#include "autotestunittests.h"
#endif

using namespace Autotest::Internal;

static AutotestPlugin *m_instance = 0;

AutotestPlugin::AutotestPlugin()
    : m_settings(new TestSettings)
{
    // needed to be used in QueuedConnection connects
    qRegisterMetaType<TestResult>();
    qRegisterMetaType<TestTreeItem>();
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
    LicenseChecker::LicenseCheckerPlugin *licenseChecker
            = ExtensionSystem::PluginManager::getObject<LicenseChecker::LicenseCheckerPlugin>();

    if (!licenseChecker || !licenseChecker->hasValidLicense()) {
        qWarning() << "Invalid license, disabling Qt Creator Enterprise Auto Test Add-on.";
        return false;
    } else if (!licenseChecker->enterpriseFeatures())
        return false;
    return true;
}

void AutotestPlugin::initializeMenuEntries()
{
    QAction *action = new QAction(tr("Re&scan Tests"), this);
    Core::Command *command = Core::ActionManager::registerAction(action, Constants::ACTION_ID,
                                                             Core::Context(Core::Constants::C_GLOBAL));
    command->setDefaultKeySequence(QKeySequence(tr("Alt+Shift+T,Alt+S")));
    connect(action, &QAction::triggered,
            TestTreeModel::instance()->parser(), &TestCodeParser::updateTestTree);

    Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(tr("Tests"));
    menu->addAction(command);
    Core::ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);
}

bool AutotestPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    if (!checkLicense())
        return true;

    initializeMenuEntries();

    m_settings->fromSettings(Core::ICore::settings());
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

QList<QObject *> AutotestPlugin::createTestObjects() const
{
    QList<QObject *> tests;
#ifdef WITH_TESTS
    tests << new AutoTestUnitTests(TestTreeModel::instance());
#endif
    return tests;
}
