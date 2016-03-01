/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindplugin.h"

#include "callgrindtool.h"
#include "memchecktool.h"
#include "memcheckengine.h"
#include "valgrindruncontrolfactory.h"
#include "valgrindsettings.h"
#include "valgrindconfigwidget.h"

#include <debugger/analyzer/analyzericons.h>
#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerruncontrol.h>
#include <debugger/analyzer/analyzerstartparameters.h>
#include <debugger/analyzer/startremotedialog.h>

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <cppeditor/cppeditorconstants.h>

#include <projectexplorer/projectexplorer.h>

#include <utils/hostosinfo.h>

#include <QtPlugin>
#include <QAction>
#include <QCoreApplication>
#include <QPointer>

using namespace Analyzer;
using namespace Core;
using namespace ProjectExplorer;

namespace Valgrind {
namespace Internal {

static ValgrindGlobalSettings *theGlobalSettings = 0;

class ValgrindOptionsPage : public IOptionsPage
{
public:
    explicit ValgrindOptionsPage()
    {
        setId(ANALYZER_VALGRIND_SETTINGS);
        setDisplayName(QCoreApplication::translate("Valgrind::Internal::ValgrindOptionsPage", "Valgrind"));
        setCategory("T.Analyzer");
        setDisplayCategory(QCoreApplication::translate("Analyzer", "Analyzer"));
        setCategoryIcon(QLatin1String(":/images/analyzer_category.png"));
    }

    QWidget *widget()
    {
        if (!m_widget)
            m_widget = new ValgrindConfigWidget(theGlobalSettings, 0, true);
        return m_widget;
    }

    void apply()
    {
        theGlobalSettings->writeSettings();
    }

    void finish()
    {
        delete m_widget;
    }

private:
    QPointer<QWidget> m_widget;
};

ValgrindPlugin::~ValgrindPlugin()
{
    delete theGlobalSettings;
    theGlobalSettings = 0;
}

bool ValgrindPlugin::initialize(const QStringList &, QString *)
{
    theGlobalSettings = new ValgrindGlobalSettings();
    theGlobalSettings->readSettings();

    addAutoReleasedObject(new ValgrindOptionsPage());
    addAutoReleasedObject(new ValgrindRunControlFactory());

    return true;
}

void ValgrindPlugin::extensionsInitialized()
{
    using namespace std::placeholders;

    QString callgrindToolTip = tr("Valgrind Function Profile uses the "
        "Callgrind tool to record function calls when a program runs.");

    QString memcheckToolTip = tr("Valgrind Analyze Memory uses the "
         "Memcheck tool to find memory leaks.");

    auto mcTool = new MemcheckTool(this);
    auto cgTool = new CallgrindTool(this);
    auto cgRunControlCreator = [cgTool](RunConfiguration *runConfiguration, Id) {
        return cgTool->createRunControl(runConfiguration);
    };

    AnalyzerManager::registerToolbar(MemcheckPerspectiveId, mcTool->createWidgets());
    AnalyzerManager::registerToolbar(CallgrindPerspectiveId, cgTool->createWidgets());

    ActionDescription desc;

    if (!Utils::HostOsInfo::isWindowsHost()) {
        desc.setText(tr("Valgrind Memory Analyzer"));
        desc.setToolTip(memcheckToolTip);
        desc.setPerspectiveId(MemcheckPerspectiveId);
        desc.setRunControlCreator([mcTool](RunConfiguration *runConfig, Id runMode) {
            return mcTool->createRunControl(runConfig, runMode);
        });
        desc.setToolMode(DebugMode);
        desc.setRunMode(MEMCHECK_RUN_MODE);
        desc.setMenuGroup(Analyzer::Constants::G_ANALYZER_TOOLS);
        AnalyzerManager::registerAction("Memcheck.Local", desc);

        desc.setText(tr("Valgrind Memory Analyzer with GDB"));
        desc.setToolTip(tr("Valgrind Analyze Memory with GDB uses the "
            "Memcheck tool to find memory leaks.\nWhen a problem is detected, "
            "the application is interrupted and can be debugged."));
        desc.setPerspectiveId(MemcheckPerspectiveId);
        desc.setRunControlCreator([mcTool](RunConfiguration *runConfig, Id runMode) {
            return mcTool->createRunControl(runConfig, runMode);
        });
        desc.setToolMode(DebugMode);
        desc.setRunMode(MEMCHECK_WITH_GDB_RUN_MODE);
        desc.setMenuGroup(Analyzer::Constants::G_ANALYZER_TOOLS);
        AnalyzerManager::registerAction("MemcheckWithGdb.Local", desc);

        desc.setText(tr("Valgrind Function Profiler"));
        desc.setToolTip(callgrindToolTip);
        desc.setPerspectiveId(CallgrindPerspectiveId);
        desc.setRunControlCreator(cgRunControlCreator);
        desc.setToolMode(OptimizedMode);
        desc.setRunMode(CALLGRIND_RUN_MODE);
        desc.setMenuGroup(Analyzer::Constants::G_ANALYZER_TOOLS);
        AnalyzerManager::registerAction(CallgrindLocalActionId, desc);
    }

    desc.setText(tr("Valgrind Memory Analyzer (External Remote Application)"));
    desc.setToolTip(memcheckToolTip);
    desc.setPerspectiveId(MemcheckPerspectiveId);
    desc.setCustomToolStarter([mcTool](ProjectExplorer::RunConfiguration *runConfig) {
        StartRemoteDialog dlg;
        if (dlg.exec() != QDialog::Accepted)
            return;
        ValgrindRunControl *rc = mcTool->createRunControl(runConfig, MEMCHECK_RUN_MODE);
        QTC_ASSERT(rc, return);
        const auto runnable = dlg.runnable();
        rc->setRunnable(runnable);
        AnalyzerConnection connection;
        connection.connParams = dlg.sshParams();
        rc->setConnection(connection);
        rc->setDisplayName(runnable.executable);
        ProjectExplorerPlugin::startRunControl(rc, MEMCHECK_RUN_MODE);
    });
    desc.setMenuGroup(Analyzer::Constants::G_ANALYZER_REMOTE_TOOLS);
    AnalyzerManager::registerAction("Memcheck.Remote", desc);

    desc.setText(tr("Valgrind Function Profiler (External Remote Application)"));
    desc.setToolTip(callgrindToolTip);
    desc.setPerspectiveId(CallgrindPerspectiveId);
    desc.setCustomToolStarter([cgTool](ProjectExplorer::RunConfiguration *runConfig) {
        StartRemoteDialog dlg;
        if (dlg.exec() != QDialog::Accepted)
            return;
        ValgrindRunControl *rc = cgTool->createRunControl(runConfig);
        QTC_ASSERT(rc, return);
        const auto runnable = dlg.runnable();
        rc->setRunnable(runnable);
        AnalyzerConnection connection;
        connection.connParams = dlg.sshParams();
        rc->setConnection(connection);
        rc->setDisplayName(runnable.executable);
        ProjectExplorerPlugin::startRunControl(rc, CALLGRIND_RUN_MODE);
    });
    desc.setMenuGroup(Analyzer::Constants::G_ANALYZER_REMOTE_TOOLS);
    AnalyzerManager::registerAction(CallgrindRemoteActionId, desc);

    // If there is a CppEditor context menu add our own context menu actions.
    if (ActionContainer *editorContextMenu =
            ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT)) {
        Context analyzerContext = Context(Analyzer::Constants::C_ANALYZEMODE);
        editorContextMenu->addSeparator(analyzerContext);

        QAction *action = new QAction(tr("Profile Costs of This Function and Its Callees"), this);
        action->setIcon(Analyzer::Icons::ANALYZER_CONTROL_START.icon());
        connect(action, &QAction::triggered, cgTool,
                &CallgrindTool::handleShowCostsOfFunction);
        Command *cmd = ActionManager::registerAction(action, "Analyzer.Callgrind.ShowCostsOfFunction",
            analyzerContext);
        editorContextMenu->addAction(cmd);
        cmd->setAttribute(Command::CA_Hide);
        cmd->setAttribute(Command::CA_NonConfigurable);
    }
}

ValgrindGlobalSettings *ValgrindPlugin::globalSettings()
{
   return theGlobalSettings;
}

} // namespace Internal
} // namespace Valgrind
