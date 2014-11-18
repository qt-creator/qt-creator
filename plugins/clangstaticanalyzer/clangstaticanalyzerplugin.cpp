/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "clangstaticanalyzerplugin.h"

#include "clangstaticanalyzerconfigwidget.h"
#include "clangstaticanalyzerruncontrolfactory.h"
#include "clangstaticanalyzertool.h"

#include <analyzerbase/analyzermanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <licensechecker/licensecheckerplugin.h>

#include <extensionsystem/pluginmanager.h>

#include <QAction>
#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QMenu>

#include <QtPlugin>

using namespace Analyzer;

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerOptionsPage : public Core::IOptionsPage
{
public:
    explicit ClangStaticAnalyzerOptionsPage()
    {
        setId("Analyzer.ClangStaticAnalyzer.Settings"); // TODO: Get it from "clangstaticanalyzersettings.h"
        setDisplayName(QCoreApplication::translate(
                           "ClangStaticAnalyzer::Internal::ClangStaticAnalyzerOptionsPage",
                           "Clang Static Analyzer"));
        setCategory("T.Analyzer");
        setDisplayCategory(QCoreApplication::translate("Analyzer", "Analyzer"));
        setCategoryIcon(QLatin1String(":/images/analyzer_category.png"));
    }

    QWidget *widget()
    {
        if (!m_widget)
            m_widget = new ClangStaticAnalyzerConfigWidget(ClangStaticAnalyzerSettings::instance());
        return m_widget;
    }

    void apply()
    {
        ClangStaticAnalyzerSettings::instance()->writeSettings();
    }

    void finish()
    {
        delete m_widget;
    }

private:
    QPointer<QWidget> m_widget;
};

ClangStaticAnalyzerPlugin::ClangStaticAnalyzerPlugin()
{
    // Create your members
}

ClangStaticAnalyzerPlugin::~ClangStaticAnalyzerPlugin()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members
}

bool ClangStaticAnalyzerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    // Register objects in the plugin manager's object pool
    // Load settings
    // Add actions to menus
    // Connect to other plugins' signals
    // In the initialize method, a plugin can be sure that the plugins it
    // depends on have initialized their members.

    LicenseChecker::LicenseCheckerPlugin *licenseChecker
            = ExtensionSystem::PluginManager::getObject<LicenseChecker::LicenseCheckerPlugin>();

    if (licenseChecker && licenseChecker->hasValidLicense()) {
        if (licenseChecker->enterpriseFeatures())
            return initializeEnterpriseFeatures(arguments, errorString);
    } else {
        qWarning() << "Invalid license, disabling Clang Static Analyzer";
    }

    return true;
}

bool ClangStaticAnalyzerPlugin::initializeEnterpriseFeatures(const QStringList &arguments,
                                                             QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    m_analyzerTool = new ClangStaticAnalyzerTool(this);
    addAutoReleasedObject(new ClangStaticAnalyzerRunControlFactory(m_analyzerTool));
    addAutoReleasedObject(new ClangStaticAnalyzerOptionsPage);

    const QString toolTip = tr("Clang Static Analyzer uses the analyzer from the clang project "
                               "to find bugs.");

    AnalyzerAction *action = new AnalyzerAction(this);
    action->setId("ClangStaticAnalyzer");
    action->setTool(m_analyzerTool);
    action->setText(tr("Clang Static Analyzer"));
    action->setToolTip(toolTip);
    action->setMenuGroup(Constants::G_ANALYZER_TOOLS);
    action->setStartMode(StartLocal);
    action->setEnabled(false);
    AnalyzerManager::addAction(action);

    return true;
}

void ClangStaticAnalyzerPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized method, a plugin can be sure that all
    // plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag ClangStaticAnalyzerPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

} // namespace Internal
} // namespace ClangStaticAnalyzerPlugin
