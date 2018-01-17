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

#include "clangtidyclazytool.h"

#include "clangtoolsconstants.h"
#include "clangtoolsdiagnosticmodel.h"
#include "clangtoolslogfilereader.h"
#include "clangtidyclazyruncontrol.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>

#include <utils/utilsicons.h>

#include <QAction>

using namespace Core;
using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangTools {
namespace Internal {

static ClangTidyClazyTool *s_instance;

ClangTidyClazyTool::ClangTidyClazyTool()
    : ClangTool("Clang-Tidy and Clazy")
{
    setObjectName("ClangTidyClazyTool");
    s_instance = this;

    m_diagnosticView = new Debugger::DetailedErrorView;
    initDiagnosticView();
    m_diagnosticView->setModel(m_diagnosticModel);
    m_diagnosticView->setObjectName(QLatin1String("ClangTidyClazyIssuesView"));
    m_diagnosticView->setWindowTitle(tr("Clang-Tidy and Clazy Issues"));

    ActionContainer *menu = ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);
    const QString toolTip = tr("Clang-Tidy and Clazy use a customized Clang executable from the "
                               "Clang project to search for errors and warnings.");

    Debugger::registerPerspective(ClangTidyClazyPerspectiveId, new Perspective(
        tr("Clang-Tidy and Clazy"),
        {{ClangTidyClazyDockId, m_diagnosticView, {}, Perspective::SplitVertical}}
    ));

    auto *action = new QAction(tr("Clang-Tidy and Clazy"), this);
    action->setToolTip(toolTip);
    menu->addAction(ActionManager::registerAction(action, "ClangTidyClazy.Action"),
                    Debugger::Constants::G_ANALYZER_TOOLS);
    QObject::connect(action, &QAction::triggered, this, &ClangTidyClazyTool::startTool);
    QObject::connect(m_startAction, &QAction::triggered, action, &QAction::triggered);
    QObject::connect(m_startAction, &QAction::changed, action, [action, this] {
        action->setEnabled(m_startAction->isEnabled());
    });

    ToolbarDescription tidyClazyToolbar;
    tidyClazyToolbar.addAction(m_startAction);
    tidyClazyToolbar.addAction(m_stopAction);
    Debugger::registerToolbar(ClangTidyClazyPerspectiveId, tidyClazyToolbar);

    updateRunActions();

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::updateRunActions,
            this, &ClangTidyClazyTool::updateRunActions);

}

ClangTidyClazyTool *ClangTidyClazyTool::instance()
{
    return s_instance;
}

void ClangTidyClazyTool::startTool()
{
    auto runControl = new RunControl(nullptr, Constants::CLANGTIDYCLAZY_RUN_MODE);
    runControl->setDisplayName(tr("Clang-Tidy and Clazy"));
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);

    Project *project = SessionManager::startupProject();
    QTC_ASSERT(project, return);

    auto clangTool = new ClangTidyClazyRunControl(runControl, project->activeTarget());

    m_stopAction->disconnect();
    connect(m_stopAction, &QAction::triggered, runControl, [runControl] {
        runControl->appendMessage(tr("Clang-Tidy and Clazy tool stopped by user."),
                                  NormalMessageFormat);
        runControl->initiateStop();
    });

    connect(runControl, &RunControl::stopped, this, [this, clangTool] {
        bool success = clangTool->success();
        setToolBusy(false);
        m_running = false;
        handleStateUpdate();
        updateRunActions();
        emit finished(success);
    });

    Debugger::selectPerspective(ClangTidyClazyPerspectiveId);

    m_diagnosticModel->clear();
    setToolBusy(true);
    m_running = true;
    handleStateUpdate();
    updateRunActions();

    ProjectExplorerPlugin::startRunControl(runControl);
}

void ClangTidyClazyTool::updateRunActions()
{
    if (m_toolBusy) {
        m_startAction->setEnabled(false);
        QString tooltipText = tr("Clang-Tidy and Clazy are still running.");
        m_startAction->setToolTip(tooltipText);
        m_stopAction->setEnabled(true);
    } else {
        QString toolTip = tr("Start Clang-Tidy and Clazy.");
        Project *project = SessionManager::startupProject();
        Target *target = project ? project->activeTarget() : nullptr;
        const Core::Id cxx = ProjectExplorer::Constants::CXX_LANGUAGE_ID;
        bool canRun = target && project->projectLanguages().contains(cxx)
                && ToolChainKitInformation::toolChain(target->kit(), cxx);
        if (!canRun)
            toolTip = tr("This is not a C++ project.");

        m_startAction->setToolTip(toolTip);
        m_startAction->setEnabled(canRun);
        m_stopAction->setEnabled(false);
    }
}

void ClangTidyClazyTool::handleStateUpdate()
{
    QTC_ASSERT(m_diagnosticModel, return);

    const int issuesFound = m_diagnosticModel->diagnostics().count();
    QString message;
    if (m_running)
        message = tr("Clang-Tidy and Clazy are running.");
    else
        message = tr("Clang-Tidy and Clazy finished.");

    message += QLatin1Char(' ');
    if (issuesFound == 0)
        message += tr("No issues found.");
    else
        message += tr("%n issues found.", nullptr, issuesFound);

    Debugger::showPermanentStatusMessage(message);
}

QList<Diagnostic> ClangTidyClazyTool::read(const QString &filePath,
                                           const QString &logFilePath,
                                           QString *errorMessage) const
{
    return LogFileReader::readSerialized(filePath, logFilePath, errorMessage);
}

} // namespace Internal
} // namespace ClangTools

