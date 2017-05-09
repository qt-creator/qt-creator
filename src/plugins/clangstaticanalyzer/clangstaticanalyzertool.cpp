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

#include "clangstaticanalyzerconstants.h"
#include "clangstaticanalyzerdiagnostic.h"
#include "clangstaticanalyzerdiagnosticmodel.h"
#include "clangstaticanalyzerdiagnosticview.h"
#include "clangstaticanalyzerruncontrol.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <cpptools/cppmodelmanager.h>

#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/checkablemessagebox.h>
#include <utils/fancymainwindow.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QToolButton>

using namespace Core;
using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangStaticAnalyzer {
namespace Internal {

class DummyRunConfiguration : public RunConfiguration
{
    Q_OBJECT

public:
    DummyRunConfiguration(Target *parent)
        : RunConfiguration(parent, "ClangStaticAnalyzer.DummyRunConfig")
    {
        setDefaultDisplayName(tr("Clang Static Analyzer"));
    }

private:
    QWidget *createConfigurationWidget() override { return 0; }
};

static ClangStaticAnalyzerTool *s_instance;

ClangStaticAnalyzerTool::ClangStaticAnalyzerTool()
{
    setObjectName("ClangStaticAnalyzerTool");
    s_instance = this;

    //
    // Diagnostic View
    //
    m_diagnosticView = new ClangStaticAnalyzerDiagnosticView;
    m_diagnosticView->setFrameStyle(QFrame::NoFrame);
    m_diagnosticView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_diagnosticModel = new ClangStaticAnalyzerDiagnosticModel(this);
    m_diagnosticFilterModel = new ClangStaticAnalyzerDiagnosticFilterModel(this);
    m_diagnosticFilterModel->setSourceModel(m_diagnosticModel);
    m_diagnosticView->setModel(m_diagnosticFilterModel);
    m_diagnosticView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_diagnosticView->setAutoScroll(false);
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

    //
    // Toolbar widget
    //

    m_startAction = Debugger::createStartAction();
    m_stopAction = Debugger::createStopAction();

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

    //Debugger::registerAction(Constants::CLANGSTATICANALYZER_RUN_MODE, {});
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

ClangStaticAnalyzerTool::~ClangStaticAnalyzerTool()
{

}

ClangStaticAnalyzerTool *ClangStaticAnalyzerTool::instance()
{
    return s_instance;
}

static bool dontStartAfterHintForDebugMode(Project *project)
{
    BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;
    if (project) {
        if (const Target *target = project->activeTarget()) {
            if (const BuildConfiguration *buildConfig = target->activeBuildConfiguration())
                buildType = buildConfig->buildType();
        }
    }

    if (buildType == BuildConfiguration::Release) {
        const QString wrongMode = ClangStaticAnalyzerTool::tr("Release");
        const QString toolName = ClangStaticAnalyzerTool::tr("Clang Static Analyzer");
        const QString title = ClangStaticAnalyzerTool::tr("Run %1 in %2 Mode?").arg(toolName)
                .arg(wrongMode);
        const QString message = ClangStaticAnalyzerTool::tr(
            "<html><head/><body>"
            "<p>You are trying to run the tool \"%1\" on an application in %2 mode. The tool is "
            "designed to be used in Debug mode since enabled assertions can reduce the number of "
            "false positives.</p>"
            "<p>Do you want to continue and run the tool in %2 mode?</p>"
            "</body></html>")
                .arg(toolName).arg(wrongMode);
        if (Utils::CheckableMessageBox::doNotAskAgainQuestion(Core::ICore::mainWindow(),
                title, message, Core::ICore::settings(),
                QLatin1String("ClangStaticAnalyzerCorrectModeWarning")) != QDialogButtonBox::Yes)
            return true;
    }

    return false;
}

void ClangStaticAnalyzerTool::startTool()
{
    Project *project = SessionManager::startupProject();
    QTC_ASSERT(project, emit finished(false); return);

    if (dontStartAfterHintForDebugMode(project))
        return;

    Debugger::selectPerspective(ClangStaticAnalyzerPerspectiveId);
    m_diagnosticModel->clear();
    setBusyCursor(true);
    m_diagnosticFilterModel->setProject(project);
    m_projectInfoBeforeBuild = CppTools::CppModelManager::instance()->projectInfo(project);
    QTC_ASSERT(m_projectInfoBeforeBuild.isValid(), emit finished(false); return);
    m_running = true;
    handleStateUpdate();

    m_toolBusy = true;
    updateRunActions();

    Target * const target = project->activeTarget();
    QTC_ASSERT(target, return);
    DummyRunConfiguration *& rc = m_runConfigs[target];
    if (!rc) {
        rc = new DummyRunConfiguration(target);
        connect(project, &Project::aboutToRemoveTarget, this,
                [this](Target *t) { m_runConfigs.remove(t); });
        const auto onProjectRemoved = [this](Project *p) {
            foreach (Target * const t, p->targets())
                m_runConfigs.remove(t);
        };
        connect(SessionManager::instance(), &SessionManager::aboutToRemoveProject, this,
                onProjectRemoved, Qt::UniqueConnection);
    }
    ProjectExplorerPlugin::runRunConfiguration(rc, Constants::CLANGSTATICANALYZER_RUN_MODE);
}

CppTools::ProjectInfo ClangStaticAnalyzerTool::projectInfoBeforeBuild() const
{
    return m_projectInfoBeforeBuild;
}

void ClangStaticAnalyzerTool::resetCursorAndProjectInfoBeforeBuild()
{
    setBusyCursor(false);
    m_projectInfoBeforeBuild = CppTools::ProjectInfo();
}

QList<Diagnostic> ClangStaticAnalyzerTool::diagnostics() const
{
    return m_diagnosticModel->diagnostics();
}

void ClangStaticAnalyzerTool::onEngineIsStarting()
{
    QTC_ASSERT(m_diagnosticModel, return);
}

void ClangStaticAnalyzerTool::onNewDiagnosticsAvailable(const QList<Diagnostic> &diagnostics)
{
    QTC_ASSERT(m_diagnosticModel, return);
    m_diagnosticModel->addDiagnostics(diagnostics);
}

void ClangStaticAnalyzerTool::onEngineFinished(bool success)
{
    resetCursorAndProjectInfoBeforeBuild();
    m_running = false;
    handleStateUpdate();
    emit finished(success);
    m_toolBusy = false;
    updateRunActions();
}

void ClangStaticAnalyzerTool::updateRunActions()
{
    if (m_toolBusy) {
        m_startAction->setEnabled(false);
        m_startAction->setToolTip(tr("Clang Static Analyzer is still running."));
        m_stopAction->setEnabled(true);
    } else {
        QString whyNot = tr("Start Clang Static Analyzer.");
        bool canRun = ProjectExplorerPlugin::canRunStartupProject(
            Constants::CLANGSTATICANALYZER_RUN_MODE, &whyNot);
        m_startAction->setToolTip(whyNot);
        m_startAction->setEnabled(canRun);
        m_stopAction->setEnabled(false);
    }
}
void ClangStaticAnalyzerTool::setBusyCursor(bool busy)
{
    QTC_ASSERT(m_diagnosticView, return);
    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_diagnosticView->setCursor(cursor);
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

    QString message = m_running ? tr("Clang Static Analyzer is running.")
                                : tr("Clang Static Analyzer finished.");
    message += QLatin1Char(' ');
    if (issuesFound == 0) {
        message += tr("No issues found.");
    } else {
        message += tr("%n issues found (%1 suppressed).", 0, issuesFound)
                .arg(issuesFound - issuesVisible);
    }
    Debugger::showPermanentStatusMessage(message);
}

} // namespace Internal
} // namespace ClangStaticAnalyzer

#include "clangstaticanalyzertool.moc"
