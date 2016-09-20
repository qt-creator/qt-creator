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

#include "clangstaticanalyzerplugin.h"

#include "clangstaticanalyzerconfigwidget.h"
#include "clangstaticanalyzerconstants.h"
#include "clangstaticanalyzerprojectsettingswidget.h"
#include "clangstaticanalyzerruncontrolfactory.h"
#include "clangstaticanalyzertool.h"

#ifdef WITH_TESTS
#include "clangstaticanalyzerpreconfiguredsessiontests.h"
#include "clangstaticanalyzerunittests.h"
#endif

#include <debugger/analyzer/analyzermanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <projectexplorer/projectpanelfactory.h>

#include <QAction>
#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QMenu>

#include <QtPlugin>

using namespace Debugger;
using namespace ProjectExplorer;

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
        setCategoryIcon(Utils::Icon(":/images/analyzer_category.png"));
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

    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    auto panelFactory = new ProjectPanelFactory();
    panelFactory->setPriority(100);
    panelFactory->setDisplayName(tr("Clang Static Analyzer"));
    panelFactory->setCreateWidgetFunction([](Project *project) { return new ProjectSettingsWidget(project); });
    ProjectPanelFactory::registerFactory(panelFactory);

    m_analyzerTool = new ClangStaticAnalyzerTool(this);
    addAutoReleasedObject(new ClangStaticAnalyzerRunControlFactory(m_analyzerTool));
    addAutoReleasedObject(new ClangStaticAnalyzerOptionsPage);

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

QList<QObject *> ClangStaticAnalyzerPlugin::createTestObjects() const
{
    QList<QObject *> tests;
#ifdef WITH_TESTS
    tests << new ClangStaticAnalyzerPreconfiguredSessionTests(m_analyzerTool);
    tests << new ClangStaticAnalyzerUnitTests(m_analyzerTool);
#endif
    return tests;
}

} // namespace Internal
} // namespace ClangStaticAnalyzerPlugin
