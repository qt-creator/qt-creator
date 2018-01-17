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

#include "clangstaticanalyzertool.h"

#include "clangtoolsconstants.h"
#include "clangtoolsdiagnosticmodel.h"
#include "clangtoolslogfilereader.h"
#include "clangstaticanalyzerdiagnosticview.h"
#include "clangstaticanalyzerruncontrol.h"

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

static ClangStaticAnalyzerTool *s_instance;

ClangStaticAnalyzerTool::ClangStaticAnalyzerTool()
    : ClangTool(tr("Clang Static Analyzer"))
{
    setObjectName("ClangStaticAnalyzerTool");
    s_instance = this;

    m_diagnosticFilterModel = new ClangStaticAnalyzerDiagnosticFilterModel(this);
    m_diagnosticFilterModel->setSourceModel(m_diagnosticModel);

    m_diagnosticView = new ClangStaticAnalyzerDiagnosticView;
    initDiagnosticView();
    m_diagnosticView->setModel(m_diagnosticFilterModel);
    m_diagnosticView->setObjectName(QLatin1String("ClangStaticAnalyzerIssuesView"));
    m_diagnosticView->setWindowTitle(tr("Clang Static Analyzer Issues"));

    foreach (auto * const model,
             QList<QAbstractItemModel *>() << m_diagnosticModel << m_diagnosticFilterModel) {
        connect(model, &QAbstractItemModel::rowsInserted,
                this, &ClangStaticAnalyzerTool::handleStateUpdate);
        connect(model, &QAbstractItemModel::rowsRemoved,
                this, &ClangStaticAnalyzerTool::handleStateUpdate);
        connect(model, &QAbstractItemModel::modelReset,
                this, &ClangStaticAnalyzerTool::handleStateUpdate);
        connect(model, &QAbstractItemModel::layoutChanged, // For QSortFilterProxyModel::invalidate()
                this, &ClangStaticAnalyzerTool::handleStateUpdate);
    }

    // Go to previous diagnostic
    auto action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::PREV_TOOLBAR.icon());
    action->setToolTip(tr("Go to previous bug."));
    connect(action, &QAction::triggered, m_diagnosticView, &DetailedErrorView::goBack);
    m_goBack = action;

    // Go to next diagnostic
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::NEXT_TOOLBAR.icon());
    action->setToolTip(tr("Go to next bug."));
    connect(action, &QAction::triggered, m_diagnosticView, &DetailedErrorView::goNext);
    m_goNext = action;

    ActionContainer *menu = ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);
    const QString toolTip = tr("Clang Static Analyzer uses the analyzer from the Clang project "
                               "to find bugs.");

    Debugger::registerPerspective(ClangStaticAnalyzerPerspectiveId, new Perspective(
        tr("Clang Static Analyzer"),
        {{ClangStaticAnalyzerDockId, m_diagnosticView, {}, Perspective::SplitVertical}}
    ));

    action = new QAction(tr("Clang Static Analyzer"), this);
    action->setToolTip(toolTip);
    menu->addAction(ActionManager::registerAction(action, "ClangStaticAnalyzer.Action"),
                    Debugger::Constants::G_ANALYZER_TOOLS);
    QObject::connect(action, &QAction::triggered, this, &ClangStaticAnalyzerTool::startTool);
    QObject::connect(m_startAction, &QAction::triggered, action, &QAction::triggered);
    QObject::connect(m_startAction, &QAction::changed, action, [action, this] {
        action->setEnabled(m_startAction->isEnabled());
    });

    ToolbarDescription toolbar;
    toolbar.addAction(m_startAction);
    toolbar.addAction(m_stopAction);
    toolbar.addAction(m_goBack);
    toolbar.addAction(m_goNext);
    Debugger::registerToolbar(ClangStaticAnalyzerPerspectiveId, toolbar);

    updateRunActions();

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::updateRunActions,
            this, &ClangStaticAnalyzerTool::updateRunActions);
}

ClangStaticAnalyzerTool *ClangStaticAnalyzerTool::instance()
{
    return s_instance;
}

void ClangStaticAnalyzerTool::startTool()
{
    auto runControl = new RunControl(nullptr, Constants::CLANGSTATICANALYZER_RUN_MODE);
    runControl->setDisplayName(tr("Clang Static Analyzer"));
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);

    Project *project = SessionManager::startupProject();
    QTC_ASSERT(project, return);
    QTC_ASSERT(project->activeTarget(), return);

    auto clangTool = new ClangStaticAnalyzerRunControl(runControl, project->activeTarget());

    m_stopAction->disconnect();
    connect(m_stopAction, &QAction::triggered, runControl, [runControl] {
        runControl->appendMessage(tr("Clang Static Analyzer stopped by user."),
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

    Debugger::selectPerspective(ClangStaticAnalyzerPerspectiveId);

    m_diagnosticModel->clear();
    setToolBusy(true);
    m_diagnosticFilterModel->setProject(project);
    m_running = true;
    handleStateUpdate();
    updateRunActions();

    ProjectExplorerPlugin::startRunControl(runControl);
}

void ClangStaticAnalyzerTool::updateRunActions()
{
    if (m_toolBusy) {
        m_startAction->setEnabled(false);
        m_startAction->setToolTip(tr("Clang Static Analyzer is still running."));
        m_stopAction->setEnabled(true);
    } else {
        QString toolTip = tr("Start Clang Static Analyzer.");
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

void ClangStaticAnalyzerTool::handleStateUpdate()
{
    QTC_ASSERT(m_goBack, return);
    QTC_ASSERT(m_goNext, return);
    QTC_ASSERT(m_diagnosticModel, return);
    QTC_ASSERT(m_diagnosticFilterModel, return);

    const int issuesFound = m_diagnosticModel->diagnostics().count();
    const int issuesVisible = m_diagnosticFilterModel->rowCount();
    m_goBack->setEnabled(issuesVisible > 1);
    m_goNext->setEnabled(issuesVisible > 1);

    QString message;
    if (m_running)
        message = tr("Clang Static Analyzer is running.");
    else
        message = tr("Clang Static Analyzer finished.");

    message += QLatin1Char(' ');
    if (issuesFound == 0) {
        message += tr("No issues found.");
    } else {
        message += tr("%n issues found (%1 suppressed).", 0, issuesFound)
                .arg(issuesFound - issuesVisible);
    }
    Debugger::showPermanentStatusMessage(message);
}

QList<Diagnostic> ClangStaticAnalyzerTool::read(const QString &filePath,
                                                QString *errorMessage) const
{
    return LogFileReader::readPlist(filePath, errorMessage);
}

} // namespace Internal
} // namespace ClangTools
