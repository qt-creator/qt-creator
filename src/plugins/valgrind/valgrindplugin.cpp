/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "valgrindplugin.h"

#include "callgrindtool.h"
#include "memchecktool.h"
#include "memcheckengine.h"
#include "valgrindruncontrolfactory.h"
#include "valgrindsettings.h"
#include "valgrindconfigwidget.h"

#include <analyzerbase/analyzericons.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/startremotedialog.h>

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

static bool fillParameters(AnalyzerStartParameters *sp)
{
    StartRemoteDialog dlg;
    if (dlg.exec() != QDialog::Accepted)
        return false;

    sp->connParams = dlg.sshParams();
    sp->debuggee = dlg.executable();
    sp->debuggeeArgs = dlg.arguments();
    sp->displayName = dlg.executable();
    sp->workingDirectory = dlg.workingDirectory();
    return false;
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
    AnalyzerAction *action = 0;

    QString callgrindToolTip = tr("Valgrind Function Profile uses the "
        "Callgrind tool to record function calls when a program runs.");

    QString memcheckToolTip = tr("Valgrind Analyze Memory uses the "
         "Memcheck tool to find memory leaks.");

    auto mcTool = new MemcheckTool(this);
    auto mcWidgetCreator = [mcTool] { return mcTool->createWidgets(); };
    auto mcRunControlCreator = [mcTool](const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration) -> AnalyzerRunControl * {
        return mcTool->createRunControl(sp, runConfiguration);
    };

    auto cgTool = new CallgrindTool(this);
    auto cgWidgetCreator = [cgTool] { return cgTool->createWidgets(); };
    auto cgRunControlCreator = [cgTool](const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration) {
        return cgTool->createRunControl(sp, runConfiguration);
    };

    if (!Utils::HostOsInfo::isWindowsHost()) {
        action = new AnalyzerAction(this);
        action->setActionId("Memcheck.Local");
        action->setToolId("Memcheck");
        action->setWidgetCreator(mcWidgetCreator);
        action->setRunControlCreator(mcRunControlCreator);
        action->setToolMode(SymbolsMode);
        action->setRunMode(MEMCHECK_RUN_MODE);
        action->setText(tr("Valgrind Memory Analyzer"));
        action->setToolTip(memcheckToolTip);
        action->setMenuGroup(Analyzer::Constants::G_ANALYZER_TOOLS);
        action->setEnabled(false);
        AnalyzerManager::addAction(action);

        using namespace std::placeholders;
        auto mcgTool = new MemcheckWithGdbTool(this);
        action = new AnalyzerAction(this);
        action->setActionId("MemcheckWithGdb.Local");
        action->setToolId("MemcheckWithGdb");
        action->setWidgetCreator([mcgTool] { return mcgTool->createWidgets(); });
        action->setRunControlCreator(std::bind(&MemcheckWithGdbTool::createRunControl,
                                               mcgTool, _1, _2));
        action->setToolMode(SymbolsMode);
        action->setRunMode(MEMCHECK_WITH_GDB_RUN_MODE);
        action->setText(tr("Valgrind Memory Analyzer with GDB"));
        action->setToolTip(tr("Valgrind Analyze Memory with GDB uses the "
            "Memcheck tool to find memory leaks.\nWhen a problem is detected, "
            "the application is interrupted and can be debugged."));
        action->setMenuGroup(Analyzer::Constants::G_ANALYZER_TOOLS);
        action->setEnabled(false);
        AnalyzerManager::addAction(action);

        action = new AnalyzerAction(this);
        action->setActionId(CallgrindLocalActionId);
        action->setToolId(CallgrindToolId);
        action->setWidgetCreator(cgWidgetCreator);
        action->setRunControlCreator(cgRunControlCreator);
        action->setToolMode(OptimizedMode);
        action->setRunMode(CALLGRIND_RUN_MODE);
        action->setText(tr("Valgrind Function Profiler"));
        action->setToolTip(callgrindToolTip);
        action->setMenuGroup(Analyzer::Constants::G_ANALYZER_TOOLS);
        action->setEnabled(false);
        AnalyzerManager::addAction(action);
    }

    action = new AnalyzerAction(this);
    action->setActionId("Memcheck.Remote");
    action->setToolId("Memcheck");
    action->setWidgetCreator(mcWidgetCreator);
    action->setCustomToolStarter([mcTool] {
        AnalyzerStartParameters sp;
        if (!fillParameters(&sp))
            return;
        ValgrindRunControl *rc = mcTool->createRunControl(sp, 0);
        QTC_ASSERT(rc, return);
        rc->setCustomStart();
        ProjectExplorerPlugin::startRunControl(rc, MEMCHECK_RUN_MODE);
    });
    action->setText(tr("Valgrind Memory Analyzer (External Remote Application)"));
    action->setToolTip(memcheckToolTip);
    action->setMenuGroup(Analyzer::Constants::G_ANALYZER_REMOTE_TOOLS);
    AnalyzerManager::addAction(action);

    action = new AnalyzerAction(this);
    action->setActionId(CallgrindRemoteActionId);
    action->setToolId(CallgrindToolId);
    action->setWidgetCreator(cgWidgetCreator);
    action->setCustomToolStarter([cgTool] {
        AnalyzerStartParameters sp;
        if (!fillParameters(&sp))
            return;
        ValgrindRunControl *rc = cgTool->createRunControl(sp, 0);
        QTC_ASSERT(rc, return);
        rc->setCustomStart();
        ProjectExplorerPlugin::startRunControl(rc, CALLGRIND_RUN_MODE);
    });

    action->setText(tr("Valgrind Function Profiler (External Remote Application)"));
    action->setToolTip(callgrindToolTip);
    action->setMenuGroup(Analyzer::Constants::G_ANALYZER_REMOTE_TOOLS);
    AnalyzerManager::addAction(action);

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
