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

#include <debugger/analyzer/analyzermanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>
#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/checkablemessagebox.h>
#include <utils/fancymainwindow.h>

#include <QAction>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QToolButton>

using namespace Analyzer;
using namespace ProjectExplorer;

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerTool::ClangStaticAnalyzerTool(QObject *parent)
    : QObject(parent)
    , m_diagnosticModel(0)
    , m_diagnosticFilterModel(0)
    , m_diagnosticView(0)
    , m_goBack(0)
    , m_goNext(0)
    , m_running(false)
{
    setObjectName(QLatin1String("ClangStaticAnalyzerTool"));
}

QWidget *ClangStaticAnalyzerTool::createWidgets()
{
    QTC_ASSERT(!m_diagnosticView, return 0);
    QTC_ASSERT(!m_diagnosticModel, return 0);
    QTC_ASSERT(!m_goBack, return 0);
    QTC_ASSERT(!m_goNext, return 0);

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

    AnalyzerManager::registerDockWidget(ClangStaticAnalyzerDockId, m_diagnosticView);

    //
    // Toolbar widget
    //
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    QAction *action = 0;
    QToolButton *button = 0;

    // Go to previous diagnostic
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Core::Icons::PREV.icon());
    action->setToolTip(tr("Go to previous bug."));
    connect(action, &QAction::triggered, m_diagnosticView, &DetailedErrorView::goBack);
    button = new QToolButton;
    button->setDefaultAction(action);
    layout->addWidget(button);
    m_goBack = action;

    // Go to next diagnostic
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Core::Icons::NEXT.icon());
    action->setToolTip(tr("Go to next bug."));
    connect(action, &QAction::triggered, m_diagnosticView, &DetailedErrorView::goNext);
    button = new QToolButton;
    button->setDefaultAction(action);
    layout->addWidget(button);
    m_goNext = action;

    layout->addStretch();

    QWidget *toolbarWidget = new QWidget;
    toolbarWidget->setObjectName(QLatin1String("ClangStaticAnalyzerToolBarWidget"));
    toolbarWidget->setLayout(layout);
    return toolbarWidget;
}

AnalyzerRunControl *ClangStaticAnalyzerTool::createRunControl(RunConfiguration *runConfiguration,
                                                              Core::Id runMode)
{
    QTC_ASSERT(runConfiguration, return 0);
    QTC_ASSERT(m_projectInfoBeforeBuild.isValid(), return 0);

    // Some projects provides CompilerCallData once a build is finished,
    // so pass on the updated Project Info unless no configuration change
    // (defines/includes/files) happened.
    Project *project = runConfiguration->target()->project();
    QTC_ASSERT(project, return 0);
    const CppTools::ProjectInfo projectInfoAfterBuild
            = CppTools::CppModelManager::instance()->projectInfo(project);
    QTC_ASSERT(!projectInfoAfterBuild.configurationOrFilesChanged(m_projectInfoBeforeBuild),
               return 0);
    m_projectInfoBeforeBuild = CppTools::ProjectInfo();

    auto runControl = new ClangStaticAnalyzerRunControl(runConfiguration, runMode,
                                                        projectInfoAfterBuild);
    connect(runControl, &ClangStaticAnalyzerRunControl::starting,
            this, &ClangStaticAnalyzerTool::onEngineIsStarting);
    connect(runControl, &ClangStaticAnalyzerRunControl::newDiagnosticsAvailable,
            this, &ClangStaticAnalyzerTool::onNewDiagnosticsAvailable);
    connect(runControl, &ClangStaticAnalyzerRunControl::finished,
            this, &ClangStaticAnalyzerTool::onEngineFinished);
    return runControl;
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

void ClangStaticAnalyzerTool::startTool(ProjectExplorer::RunConfiguration *runConfiguration)
{
    AnalyzerManager::showMode();

    Project *project = SessionManager::startupProject();
    QTC_ASSERT(project, emit finished(false); return);

    if (dontStartAfterHintForDebugMode(project))
        return;

    m_diagnosticModel->clear();
    setBusyCursor(true);
    m_diagnosticFilterModel->setProject(project);
    m_projectInfoBeforeBuild = CppTools::CppModelManager::instance()->projectInfo(project);
    QTC_ASSERT(m_projectInfoBeforeBuild.isValid(), emit finished(false); return);
    m_running = true;
    handleStateUpdate();

    ProjectExplorerPlugin::runRunConfiguration(runConfiguration,
                                               Constants::CLANGSTATICANALYZER_RUN_MODE);
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

void ClangStaticAnalyzerTool::onEngineFinished()
{
    resetCursorAndProjectInfoBeforeBuild();
    m_running = false;
    handleStateUpdate();
    emit finished(static_cast<ClangStaticAnalyzerRunControl *>(sender())->success());
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

    QString message = m_running ? tr("Clang Static Analyzer running.")
                                : tr("Clang Static Analyzer finished.");
    message += QLatin1Char(' ');
    if (issuesFound == 0) {
        message += tr("No issues found.");
    } else {
        message += tr("%n issues found (%1 suppressed).", 0, issuesFound)
                .arg(issuesFound - issuesVisible);
    }
    AnalyzerManager::showPermanentStatusMessage(message);
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
