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

#include "clangfixitsrefactoringchanges.h"
#include "clangselectablefilesdialog.h"
#include "clangtoolsconstants.h"
#include "clangtoolsdiagnosticmodel.h"
#include "clangtoolslogfilereader.h"
#include "clangtidyclazyruncontrol.h"
#include "clangtoolsdiagnosticview.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolssettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <cpptools/clangdiagnosticconfigsmodel.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>

#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>

#include <utils/fancylineedit.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QToolButton>

using namespace Core;
using namespace CppTools;
using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangTools {
namespace Internal {

static ClangTidyClazyTool *s_instance;

class ApplyFixIts
{
public:
    class RefactoringFileInfo
    {
    public:
        bool isValid() const { return file.isValid(); }

        FixitsRefactoringFile file;
        QVector<DiagnosticItem *> diagnosticItems;
        bool hasScheduledFixits = false;
    };

    ApplyFixIts(const QVector<DiagnosticItem *> &diagnosticItems)
    {
        for (DiagnosticItem *diagnosticItem : diagnosticItems) {
            const QString &filePath = diagnosticItem->diagnostic().location.filePath;
            QTC_ASSERT(!filePath.isEmpty(), continue);

            // Get or create refactoring file
            RefactoringFileInfo &fileInfo = m_refactoringFileInfos[filePath];
            if (!fileInfo.isValid())
                fileInfo.file = FixitsRefactoringFile(filePath);

            // Append item
            fileInfo.diagnosticItems += diagnosticItem;
            if (diagnosticItem->fixItStatus() == FixitStatus::Scheduled)
                fileInfo.hasScheduledFixits = true;
        }
    }

    static void addFixitOperations(DiagnosticItem *diagnosticItem,
                                   const FixitsRefactoringFile &file, bool apply)
    {
        // Did we already created the fixit operations?
        ReplacementOperations currentOps = diagnosticItem->fixitOperations();
        if (!currentOps.isEmpty()) {
            for (ReplacementOperation *op : currentOps)
                op->apply = apply;
            return;
        }

        // Collect/construct the fixit operations
        ReplacementOperations replacements;

        for (const ExplainingStep &step : diagnosticItem->diagnostic().explainingSteps) {
            if (!step.isFixIt)
                continue;

            const Debugger::DiagnosticLocation start = step.ranges.first();
            const Debugger::DiagnosticLocation end = step.ranges.last();
            const int startPos = file.position(start.filePath, start.line, start.column);
            const int endPos = file.position(start.filePath, end.line, end.column);

            auto op = new ReplacementOperation;
            op->pos = startPos;
            op->length = endPos - startPos;
            op->text = step.message;
            op->fileName = start.filePath;
            op->apply = apply;

            replacements += op;
        }

        diagnosticItem->setFixitOperations(replacements);
    }

    void apply()
    {
        for (auto it = m_refactoringFileInfos.begin(); it != m_refactoringFileInfos.end(); ++it) {
            RefactoringFileInfo &fileInfo = it.value();

            QVector<DiagnosticItem *> itemsScheduledOrSchedulable;
            QVector<DiagnosticItem *> itemsScheduled;
            QVector<DiagnosticItem *> itemsSchedulable;

            // Construct refactoring operations
            for (DiagnosticItem *diagnosticItem : fileInfo.diagnosticItems) {
                const FixitStatus fixItStatus = diagnosticItem->fixItStatus();

                const bool isScheduled = fixItStatus == FixitStatus::Scheduled;
                const bool isSchedulable = fileInfo.hasScheduledFixits
                                           && fixItStatus == FixitStatus::NotScheduled;

                if (isScheduled || isSchedulable) {
                    addFixitOperations(diagnosticItem, fileInfo.file, isScheduled);
                    itemsScheduledOrSchedulable += diagnosticItem;
                    if (isScheduled)
                        itemsScheduled += diagnosticItem;
                    else
                        itemsSchedulable += diagnosticItem;
                }
            }

            // Collect replacements
            ReplacementOperations ops;
            for (DiagnosticItem *item : itemsScheduledOrSchedulable)
                ops += item->fixitOperations();

            // Apply file
            QVector<DiagnosticItem *> itemsApplied;
            QVector<DiagnosticItem *> itemsFailedToApply;
            QVector<DiagnosticItem *> itemsInvalidated;

            fileInfo.file.setReplacements(ops);
            if (fileInfo.file.apply()) {
                itemsApplied = itemsScheduled;
            } else {
                itemsFailedToApply = itemsScheduled;
                itemsInvalidated = itemsSchedulable;
            }

            // Update DiagnosticItem state
            for (DiagnosticItem *diagnosticItem : itemsScheduled)
                diagnosticItem->setFixItStatus(FixitStatus::Applied);
            for (DiagnosticItem *diagnosticItem : itemsFailedToApply)
                diagnosticItem->setFixItStatus(FixitStatus::FailedToApply);
            for (DiagnosticItem *diagnosticItem : itemsInvalidated)
                diagnosticItem->setFixItStatus(FixitStatus::Invalidated);
        }
    }

private:
    QMap<QString, RefactoringFileInfo> m_refactoringFileInfos;
};

ClangTidyClazyTool::ClangTidyClazyTool()
    : ClangTool("Clang-Tidy and Clazy")
{
    setObjectName("ClangTidyClazyTool");
    s_instance = this;

    m_diagnosticFilterModel = new DiagnosticFilterModel(this);
    m_diagnosticFilterModel->setSourceModel(m_diagnosticModel);

    m_diagnosticView = new DiagnosticView;
    initDiagnosticView();
    m_diagnosticView->setModel(m_diagnosticFilterModel);
    m_diagnosticView->setObjectName(QLatin1String("ClangTidyClazyIssuesView"));
    m_diagnosticView->setWindowTitle(tr("Clang-Tidy and Clazy Issues"));

    foreach (auto * const model,
             QList<QAbstractItemModel *>() << m_diagnosticModel << m_diagnosticFilterModel) {
        connect(model, &QAbstractItemModel::rowsInserted,
                this, &ClangTidyClazyTool::handleStateUpdate);
        connect(model, &QAbstractItemModel::rowsRemoved,
                this, &ClangTidyClazyTool::handleStateUpdate);
        connect(model, &QAbstractItemModel::modelReset,
                this, &ClangTidyClazyTool::handleStateUpdate);
        connect(model, &QAbstractItemModel::layoutChanged, // For QSortFilterProxyModel::invalidate()
                this, &ClangTidyClazyTool::handleStateUpdate);
    }

    // Go to previous diagnostic
    auto action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::PREV_TOOLBAR.icon());
    action->setToolTip(tr("Go to previous diagnostic."));
    connect(action, &QAction::triggered, m_diagnosticView, &DetailedErrorView::goBack);
    m_goBack = action;

    // Go to next diagnostic
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::NEXT_TOOLBAR.icon());
    action->setToolTip(tr("Go to next diagnostic."));
    connect(action, &QAction::triggered, m_diagnosticView, &DetailedErrorView::goNext);
    m_goNext = action;

    // Filter line edit
    m_filterLineEdit = new Utils::FancyLineEdit();
    m_filterLineEdit->setFiltering(true);
    m_filterLineEdit->setHistoryCompleter("CppTools.ClangTidyClazyIssueFilter", true);
    connect(m_filterLineEdit, &Utils::FancyLineEdit::filterChanged, [this](const QString &filter) {
        m_diagnosticFilterModel->setFilterRegExp(
            QRegExp(filter, Qt::CaseSensitive, QRegExp::WildcardUnix));
    });

    // Apply fixits button
    m_applyFixitsButton = new QToolButton;
    m_applyFixitsButton->setText(tr("Apply Fixits"));
    m_applyFixitsButton->setEnabled(false);
    connect(m_diagnosticModel,
            &ClangToolsDiagnosticModel::fixItsToApplyCountChanged,
            [this](int c) {
        m_applyFixitsButton->setEnabled(c);
        static_cast<DiagnosticView *>(m_diagnosticView.data())->setSelectedFixItsCount(c);
    });
    connect(m_applyFixitsButton, &QToolButton::clicked, [this]() {
        QVector<DiagnosticItem *> diagnosticItems;
        m_diagnosticModel->rootItem()->forChildrenAtLevel(1, [&](TreeItem *item){
            diagnosticItems += static_cast<DiagnosticItem *>(item);
        });

        ApplyFixIts(diagnosticItems).apply();
    });

    ActionContainer *menu = ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);
    const QString toolTip = tr("Clang-Tidy and Clazy use a customized Clang executable from the "
                               "Clang project to search for errors and warnings.");

    m_perspective.addWindow(m_diagnosticView, Perspective::SplitVertical, nullptr);

    action = new QAction(tr("Clang-Tidy and Clazy..."), this);
    action->setToolTip(toolTip);
    menu->addAction(ActionManager::registerAction(action, "ClangTidyClazy.Action"),
                    Debugger::Constants::G_ANALYZER_TOOLS);
    QObject::connect(action, &QAction::triggered, this, [this]() { startTool(true); });
    QObject::connect(m_startAction, &QAction::triggered, action, &QAction::triggered);
    QObject::connect(m_startAction, &QAction::changed, action, [action, this] {
        action->setEnabled(m_startAction->isEnabled());
    });

    m_perspective.addToolBarAction(m_startAction);
    m_perspective.addToolBarAction(m_stopAction);
    m_perspective.addToolBarAction(m_goBack);
    m_perspective.addToolBarAction(m_goNext);
    m_perspective.addToolBarWidget(m_filterLineEdit);
    m_perspective.addToolBarWidget(m_applyFixitsButton);

    updateRunActions();

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::updateRunActions,
            this, &ClangTidyClazyTool::updateRunActions);

}

ClangTidyClazyTool *ClangTidyClazyTool::instance()
{
    return s_instance;
}

static ClangDiagnosticConfig getDiagnosticConfig(Project *project)
{
    ClangToolsProjectSettings *projectSettings = ClangToolsProjectSettingsManager::getSettings(
        project);

    Core::Id diagnosticConfigId;
    if (projectSettings->useGlobalSettings())
        diagnosticConfigId = ClangToolsSettings::instance()->savedDiagnosticConfigId();
    else
        diagnosticConfigId = projectSettings->diagnosticConfig();

    const ClangDiagnosticConfigsModel configsModel(
                CppTools::codeModelSettings()->clangCustomDiagnosticConfigs());

    QTC_ASSERT(configsModel.hasConfigWithId(diagnosticConfigId), return ClangDiagnosticConfig());
    return configsModel.configWithId(diagnosticConfigId);
}

void ClangTidyClazyTool::startTool(bool askUserForFileSelection)
{
    auto runControl = new RunControl(nullptr, Constants::CLANGTIDYCLAZY_RUN_MODE);
    runControl->setDisplayName(tr("Clang-Tidy and Clazy"));
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);

    Project *project = SessionManager::startupProject();
    QTC_ASSERT(project, return);

    const FileInfos fileInfos = collectFileInfos(project, askUserForFileSelection);
    if (fileInfos.empty())
        return;

    auto clangTool = new ClangTidyClazyRunControl(runControl,
                                                  project->activeTarget(),
                                                  getDiagnosticConfig(project),
                                                  fileInfos);

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

    m_perspective.select();

    m_diagnosticModel->clear();
    setToolBusy(true);
    m_diagnosticFilterModel->setProject(project);
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
    QTC_ASSERT(m_goBack, return);
    QTC_ASSERT(m_goNext, return);
    QTC_ASSERT(m_diagnosticModel, return);
    QTC_ASSERT(m_diagnosticFilterModel, return);

    const int issuesFound = m_diagnosticModel->diagnosticsCount();
    const int issuesVisible = m_diagnosticFilterModel->rowCount();
    m_goBack->setEnabled(issuesVisible > 1);
    m_goNext->setEnabled(issuesVisible > 1);

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

