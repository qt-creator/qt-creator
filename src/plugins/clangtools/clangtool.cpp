/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clangtool.h"

#include "clangfixitsrefactoringchanges.h"
#include "clangselectablefilesdialog.h"
#include "clangtoolruncontrol.h"
#include "clangtoolsconstants.h"
#include "clangtoolsdiagnostic.h"
#include "clangtoolsdiagnosticmodel.h"
#include "clangtoolsdiagnosticview.h"
#include "clangtoolslogfilereader.h"
#include "clangtoolsplugin.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolssettings.h"
#include "clangtoolsutils.h"
#include "filterdialog.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <cpptools/clangdiagnosticconfigsmodel.h>
#include <cpptools/cppmodelmanager.h>

#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/fancylineedit.h>
#include <utils/fancymainwindow.h>
#include <utils/infolabel.h>
#include <utils/progressindicator.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QCheckBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QToolButton>

using namespace Core;
using namespace CppTools;
using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangTools {
namespace Internal {

static ClangTool *s_instance;

static QString makeLink(const QString &text)
{
    return QString("<a href=t>%1</a>").arg(text);
}

class InfoBarWidget : public QFrame
{
    Q_OBJECT

public:
    InfoBarWidget()
        : m_progressIndicator(new Utils::ProgressIndicator(ProgressIndicatorSize::Small))
        , m_info(new InfoLabel({}, InfoLabel::Information))
        , m_error(new InfoLabel({}, InfoLabel::Warning))
        , m_diagStats(new QLabel)
    {
        m_info->setElideMode(Qt::ElideNone);
        m_error->setElideMode(Qt::ElideNone);

        m_diagStats->setTextInteractionFlags(Qt::TextBrowserInteraction);

        QHBoxLayout *layout = new QHBoxLayout;
        layout->setContentsMargins(5, 5, 5, 5);
        layout->addWidget(m_progressIndicator);
        layout->addWidget(m_info);
        layout->addWidget(m_error);
        layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
        layout->addWidget(m_diagStats);
        setLayout(layout);

        QPalette pal;
        pal.setColor(QPalette::Window, Utils::creatorTheme()->color(Utils::Theme::InfoBarBackground));
        pal.setColor(QPalette::WindowText, Utils::creatorTheme()->color(Utils::Theme::InfoBarText));
        setPalette(pal);

        setAutoFillBackground(true);
    }

    // Info
    enum InfoIconType { ProgressIcon, InfoIcon };
    void setInfoIcon(InfoIconType type)
    {
        const bool showProgress = type == ProgressIcon;
        m_progressIndicator->setVisible(showProgress);
        m_info->setType(showProgress ? InfoLabel::None : InfoLabel::Information);
    }
    QString infoText() const { return m_info->text(); }
    void setInfoText(const QString &text)
    {
        m_info->setVisible(!text.isEmpty());
        m_info->setText(text);
        evaluateVisibility();
    }

    // Error
    using OnLinkActivated = std::function<void()>;
    enum IssueType { Warning, Error };

    QString errorText() const { return m_error->text(); }
    void setError(IssueType type,
                  const QString &text,
                  const OnLinkActivated &linkAction = OnLinkActivated())
    {
        m_error->setVisible(!text.isEmpty());
        m_error->setText(text);
        m_error->setType(type == Warning ? InfoLabel::Warning : InfoLabel::Error);
        m_error->disconnect();
        if (linkAction)
            connect(m_error, &QLabel::linkActivated, this, linkAction);
        evaluateVisibility();
    }

    // Diag stats
    void setDiagText(const QString &text) { m_diagStats->setText(text); }

    void reset()
    {
        setInfoIcon(InfoIcon);
        setInfoText({});
        setError(Warning, {}, {});
        setDiagText({});
    }

    void evaluateVisibility()
    {
        setVisible(!infoText().isEmpty() || !errorText().isEmpty());
    }

private:
    Utils::ProgressIndicator *m_progressIndicator;
    InfoLabel *m_info;
    InfoLabel *m_error;
    QLabel *m_diagStats;
};

class SelectFixitsCheckBox : public QCheckBox
{
    Q_OBJECT

private:
    void nextCheckState() final override
    {
        setCheckState(checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    }
};

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
        if (!diagnosticItem->hasNewFixIts())
            return;

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

    void apply(ClangToolsDiagnosticModel *model)
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

            if (ops.empty())
                continue;

            // Apply file
            QVector<DiagnosticItem *> itemsApplied;
            QVector<DiagnosticItem *> itemsFailedToApply;
            QVector<DiagnosticItem *> itemsInvalidated;

            fileInfo.file.setReplacements(ops);
            model->removeWatchedPath(ops.first()->fileName);
            if (fileInfo.file.apply()) {
                itemsApplied = itemsScheduled;
            } else {
                itemsFailedToApply = itemsScheduled;
                itemsInvalidated = itemsSchedulable;
            }
            model->addWatchedPath(ops.first()->fileName);

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

static FileInfos sortedFileInfos(const QVector<CppTools::ProjectPart::Ptr> &projectParts)
{
    FileInfos fileInfos;

    for (const CppTools::ProjectPart::Ptr &projectPart : projectParts) {
        QTC_ASSERT(projectPart, continue);
        if (!projectPart->selectedForBuilding)
            continue;

        for (const CppTools::ProjectFile &file : projectPart->files) {
            QTC_ASSERT(file.kind != CppTools::ProjectFile::Unclassified, continue);
            QTC_ASSERT(file.kind != CppTools::ProjectFile::Unsupported, continue);
            if (file.path == CppTools::CppModelManager::configurationFileName())
                continue;

            if (file.active && CppTools::ProjectFile::isSource(file.kind)) {
                fileInfos.emplace_back(Utils::FilePath::fromString(file.path),
                                       file.kind,
                                       projectPart);
            }
        }
    }

    Utils::sort(fileInfos, [](const FileInfo &fi1, const FileInfo &fi2) {
        if (fi1.file == fi2.file) {
            // If the same file appears more than once, prefer contexts where the file is
            // built as part of an application or library to those where it may not be,
            // e.g. because it is just listed as some sort of resource.
            return fi1.projectPart->buildTargetType != BuildTargetType::Unknown
                    && fi2.projectPart->buildTargetType == BuildTargetType::Unknown;
        }
        return fi1.file < fi2.file;
    });
    fileInfos.erase(std::unique(fileInfos.begin(), fileInfos.end()), fileInfos.end());

    return fileInfos;
}

static RunSettings runSettings()
{
    if (Project *project = SessionManager::startupProject()) {
        const auto projectSettings = ClangToolsProjectSettings::getSettings(project);
        if (!projectSettings->useGlobalSettings())
            return projectSettings->runSettings();
    }
    return ClangToolsSettings::instance()->runSettings();
}

static ClangDiagnosticConfig diagnosticConfig(const Utils::Id &diagConfigId)
{
    const ClangDiagnosticConfigsModel configs = diagnosticConfigsModel();
    QTC_ASSERT(configs.hasConfigWithId(diagConfigId), return ClangDiagnosticConfig());
    return configs.configWithId(diagConfigId);
}

ClangTool *ClangTool::instance()
{
    return s_instance;
}

ClangTool::ClangTool()
    : m_name("Clang-Tidy and Clazy")
{
    setObjectName("ClangTidyClazyTool");
    s_instance = this;
    m_diagnosticModel = new ClangToolsDiagnosticModel(this);

    const Utils::Icon RUN_FILE_OVERLAY(
        {{":/utils/images/run_file.png", Utils::Theme::IconsBaseColor}});

    const Utils::Icon RUN_SELECTED_OVERLAY(
        {{":/utils/images/runselected_boxes.png", Utils::Theme::BackgroundColorDark},
         {":/utils/images/runselected_tickmarks.png", Utils::Theme::IconsBaseColor}});

    auto action = new QAction(tr("Analyze Project..."), this);
    Utils::Icon runSelectedIcon = Utils::Icons::RUN_SMALL_TOOLBAR;
    for (const Utils::IconMaskAndColor &maskAndColor : RUN_SELECTED_OVERLAY)
        runSelectedIcon.append(maskAndColor);
    action->setIcon(runSelectedIcon.icon());
    m_startAction = action;

    action = new QAction(tr("Analyze Current File"), this);
    Utils::Icon runFileIcon = Utils::Icons::RUN_SMALL_TOOLBAR;
    for (const Utils::IconMaskAndColor &maskAndColor : RUN_FILE_OVERLAY)
        runFileIcon.append(maskAndColor);
    action->setIcon(runFileIcon.icon());
    m_startOnCurrentFileAction = action;

    m_stopAction = Debugger::createStopAction();

    m_diagnosticFilterModel = new DiagnosticFilterModel(this);
    m_diagnosticFilterModel->setSourceModel(m_diagnosticModel);
    m_diagnosticFilterModel->setDynamicSortFilter(true);

    m_infoBarWidget = new InfoBarWidget;

    m_diagnosticView = new DiagnosticView;
    initDiagnosticView();
    m_diagnosticView->setModel(m_diagnosticFilterModel);
    m_diagnosticView->setSortingEnabled(true);
    m_diagnosticView->sortByColumn(Debugger::DetailedErrorView::DiagnosticColumn,
                                   Qt::AscendingOrder);
    connect(m_diagnosticView, &DiagnosticView::showHelp,
            this, &ClangTool::help);
    connect(m_diagnosticView, &DiagnosticView::showFilter,
            this, &ClangTool::filter);
    connect(m_diagnosticView, &DiagnosticView::clearFilter,
            this, &ClangTool::clearFilter);
    connect(m_diagnosticView, &DiagnosticView::filterForCurrentKind,
            this, &ClangTool::filterForCurrentKind);
    connect(m_diagnosticView, &DiagnosticView::filterOutCurrentKind,
            this, &ClangTool::filterOutCurrentKind);

    foreach (auto * const model,
             QList<QAbstractItemModel *>({m_diagnosticModel, m_diagnosticFilterModel})) {
        connect(model, &QAbstractItemModel::rowsInserted,
                this, &ClangTool::updateForCurrentState);
        connect(model, &QAbstractItemModel::rowsRemoved,
                this, &ClangTool::updateForCurrentState);
        connect(model, &QAbstractItemModel::modelReset,
                this, &ClangTool::updateForCurrentState);
        connect(model, &QAbstractItemModel::layoutChanged, // For QSortFilterProxyModel::invalidate()
                this, &ClangTool::updateForCurrentState);
    }

    // Go to previous diagnostic
    action = new QAction(this);
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

    // Load diagnostics from file
    action = new QAction(this);
    action->setIcon(Utils::Icons::OPENFILE_TOOLBAR.icon());
    action->setToolTip(tr("Load diagnostics from YAML files exported with \"-export-fixes\"."));
    connect(action, &QAction::triggered, this, &ClangTool::loadDiagnosticsFromFiles);
    m_loadExported = action;

    // Clear data
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
    action->setToolTip(tr("Clear"));
    connect(action, &QAction::triggered, this, [this]() {
        reset();
        update();
    });
    m_clear = action;

    // Expand/Collapse
    action = new QAction(this);
    action->setDisabled(true);
    action->setCheckable(true);
    action->setIcon(Utils::Icons::EXPAND_ALL_TOOLBAR.icon());
    action->setToolTip(tr("Expand All"));
    connect(action, &QAction::toggled, [this](bool checked){
        if (checked) {
            m_expandCollapse->setToolTip(tr("Collapse All"));
            m_diagnosticView->expandAll();
        } else {
            m_expandCollapse->setToolTip(tr("Expand All"));
            m_diagnosticView->collapseAll();
        }
    });
    m_expandCollapse = action;

    // Filter button
    action = m_showFilter = new QAction(this);
    action->setIcon(
        Utils::Icon({{":/utils/images/filtericon.png", Utils::Theme::IconsBaseColor}}).icon());
    action->setToolTip(tr("Filter Diagnostics"));
    action->setCheckable(true);
    connect(action, &QAction::triggered, this, &ClangTool::filter);

    // Schedule/Unschedule all fixits
    m_selectFixitsCheckBox = new SelectFixitsCheckBox;
    m_selectFixitsCheckBox->setText("Select Fixits");
    m_selectFixitsCheckBox->setEnabled(false);
    m_selectFixitsCheckBox->setTristate(true);
    connect(m_selectFixitsCheckBox, &QCheckBox::clicked, this, [this]() {
        m_diagnosticView->scheduleAllFixits(m_selectFixitsCheckBox->isChecked());
    });

    // Apply fixits button
    m_applyFixitsButton = new QToolButton;
    m_applyFixitsButton->setText(tr("Apply Fixits"));
    m_applyFixitsButton->setEnabled(false);

    connect(m_diagnosticModel, &ClangToolsDiagnosticModel::fixitStatusChanged,
            m_diagnosticFilterModel, &DiagnosticFilterModel::onFixitStatusChanged);
    connect(m_diagnosticFilterModel, &DiagnosticFilterModel::fixitCountersChanged,
            this,
            [this](int scheduled, int scheduable){
                m_selectFixitsCheckBox->setEnabled(scheduable > 0);
                m_applyFixitsButton->setEnabled(scheduled > 0);

                if (scheduled == 0)
                    m_selectFixitsCheckBox->setCheckState(Qt::Unchecked);
                else if (scheduled == scheduable)
                    m_selectFixitsCheckBox->setCheckState(Qt::Checked);
                else
                    m_selectFixitsCheckBox->setCheckState(Qt::PartiallyChecked);

                updateForCurrentState();
            });
    connect(m_applyFixitsButton, &QToolButton::clicked, [this]() {
        QVector<DiagnosticItem *> diagnosticItems;
        m_diagnosticModel->forItemsAtLevel<2>([&](DiagnosticItem *item){
            diagnosticItems += item;
        });

        ApplyFixIts(diagnosticItems).apply(m_diagnosticModel);
    });

    // Open Project Settings
    action = new QAction(this);
    action->setIcon(Utils::Icons::SETTINGS_TOOLBAR.icon());
    //action->setToolTip(tr("Open Project Settings")); // TODO: Uncomment in master.
    connect(action, &QAction::triggered, []() {
        ProjectExplorerPlugin::activateProjectPanel(Constants::PROJECT_PANEL_ID);
    });
    m_openProjectSettings = action;

    ActionContainer *menu = ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);
    const QString toolTip = tr("Clang-Tidy and Clazy use a customized Clang executable from the "
                               "Clang project to search for diagnostics.");

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(1);
    mainLayout->addWidget(m_infoBarWidget);
    mainLayout->addWidget(m_diagnosticView);
    auto mainWidget = new QWidget;
    mainWidget->setObjectName("ClangTidyClazyIssuesView");
    mainWidget->setWindowTitle(tr("Clang-Tidy and Clazy"));
    mainWidget->setLayout(mainLayout);

    m_perspective.addWindow(mainWidget, Perspective::SplitVertical, nullptr);

    action = new QAction(tr("Clang-Tidy and Clazy..."), this);
    action->setToolTip(toolTip);
    menu->addAction(ActionManager::registerAction(action, "ClangTidyClazy.Action"),
                    Debugger::Constants::G_ANALYZER_TOOLS);
    QObject::connect(action, &QAction::triggered, this, [this]() {
        startTool(FileSelectionType::AskUser);
    });
    QObject::connect(m_startAction, &QAction::triggered, action, &QAction::triggered);
    QObject::connect(m_startAction, &QAction::changed, action, [action, this] {
        action->setEnabled(m_startAction->isEnabled());
    });

    QObject::connect(m_startOnCurrentFileAction, &QAction::triggered, this, [this] {
        startTool(FileSelectionType::CurrentFile);
    });

    m_perspective.addToolBarAction(m_startAction);
    m_perspective.addToolBarAction(m_startOnCurrentFileAction);
    m_perspective.addToolBarAction(m_stopAction);
    m_perspective.addToolBarAction(m_openProjectSettings);
    m_perspective.addToolbarSeparator();
    m_perspective.addToolBarAction(m_loadExported);
    m_perspective.addToolBarAction(m_clear);
    m_perspective.addToolbarSeparator();
    m_perspective.addToolBarAction(m_expandCollapse);
    m_perspective.addToolBarAction(m_goBack);
    m_perspective.addToolBarAction(m_goNext);
    m_perspective.addToolbarSeparator();
    m_perspective.addToolBarAction(m_showFilter);
    m_perspective.addToolBarWidget(m_selectFixitsCheckBox);
    m_perspective.addToolBarWidget(m_applyFixitsButton);

    update();

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runActionsUpdated,
            this, &ClangTool::update);
    connect(CppModelManager::instance(), &CppModelManager::projectPartsUpdated,
            this, &ClangTool::update);
    connect(ClangToolsSettings::instance(), &ClangToolsSettings::changed,
            this, &ClangTool::update);
}

void ClangTool::selectPerspective()
{
    m_perspective.select();
}

void ClangTool::startTool(ClangTool::FileSelection fileSelection)
{
    const RunSettings theRunSettings = runSettings();
    startTool(fileSelection, theRunSettings, diagnosticConfig(theRunSettings.diagnosticConfigId()));
}

static bool continueDespiteReleaseBuild(const QString &toolName)
{
    const QString wrongMode = ClangTool::tr("Release");
    const QString title = ClangTool::tr("Run %1 in %2 Mode?").arg(toolName, wrongMode);
    const QString problem
        = ClangTool::tr(
              "You are trying to run the tool \"%1\" on an application in %2 mode. The tool is "
              "designed to be used in Debug mode since enabled assertions can reduce the number of "
              "false positives.")
              .arg(toolName, wrongMode);
    const QString question = ClangTool::tr(
                                 "Do you want to continue and run the tool in %1 mode?")
                                 .arg(wrongMode);
    const QString message = QString("<html><head/><body>"
                                    "<p>%1</p>"
                                    "<p>%2</p>"
                                    "</body></html>")
                                .arg(problem, question);
    return CheckableMessageBox::doNotAskAgainQuestion(ICore::dialogParent(),
                                                      title,
                                                      message,
                                                      ICore::settings(),
                                                      "ClangToolsCorrectModeWarning")
           == QDialogButtonBox::Yes;
}

void ClangTool::startTool(ClangTool::FileSelection fileSelection,
                          const RunSettings &runSettings,
                          const CppTools::ClangDiagnosticConfig &diagnosticConfig)
{
    Project *project = SessionManager::startupProject();
    QTC_ASSERT(project, return);
    QTC_ASSERT(project->activeTarget(), return);

    // Continue despite release mode?
    if (BuildConfiguration *bc = project->activeTarget()->activeBuildConfiguration()) {
        if (bc->buildType() == BuildConfiguration::Release)
            if (!continueDespiteReleaseBuild(m_name))
                return;
    }

    // Collect files to analyze
    const FileInfos fileInfos = collectFileInfos(project, fileSelection);
    if (fileInfos.empty())
        return;

    // Reset
    reset();

    // Run control
    m_runControl = new RunControl(Constants::CLANGTIDYCLAZY_RUN_MODE);
    m_runControl->setDisplayName(tr("Clang-Tidy and Clazy"));
    m_runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);
    m_runControl->setTarget(project->activeTarget());
    m_stopAction->disconnect();
    connect(m_stopAction, &QAction::triggered, m_runControl, [this] {
        m_runControl->appendMessage(tr("Clang-Tidy and Clazy tool stopped by user."),
                                    NormalMessageFormat);
        m_runControl->initiateStop();
        setState(State::StoppedByUser);
    });
    connect(m_runControl, &RunControl::stopped, this, &ClangTool::onRunControlStopped);

    // Run worker
    const bool preventBuild = holds_alternative<FilePath>(fileSelection)
                              || get<FileSelectionType>(fileSelection)
                                     == FileSelectionType::CurrentFile;
    const bool buildBeforeAnalysis = !preventBuild && runSettings.buildBeforeAnalysis();
    m_runWorker = new ClangToolRunWorker(m_runControl,
                                         runSettings,
                                         diagnosticConfig,
                                         fileInfos,
                                         buildBeforeAnalysis);
    connect(m_runWorker, &ClangToolRunWorker::buildFailed,this, &ClangTool::onBuildFailed);
    connect(m_runWorker, &ClangToolRunWorker::startFailed, this, &ClangTool::onStartFailed);
    connect(m_runWorker, &ClangToolRunWorker::started, this, &ClangTool::onStarted);
    connect(m_runWorker, &ClangToolRunWorker::runnerFinished, this, [this]() {
        m_filesCount = m_runWorker->totalFilesToAnalyze();
        m_filesSucceeded = m_runWorker->filesAnalyzed();
        m_filesFailed = m_runWorker->filesNotAnalyzed();
        updateForCurrentState();
    });

    // More init and UI update
    m_diagnosticFilterModel->setProject(project);
    m_perspective.select();
    if (buildBeforeAnalysis)
        m_infoBarWidget->setInfoText("Waiting for build to finish...");
    setState(State::PreparationStarted);

    // Start
    ProjectExplorerPlugin::startRunControl(m_runControl);
}

Diagnostics ClangTool::read(OutputFileFormat outputFileFormat,
                            const QString &logFilePath,
                            const QSet<FilePath> &projectFiles,
                            QString *errorMessage) const
{
    const auto acceptFromFilePath = [projectFiles](const Utils::FilePath &filePath) {
        return projectFiles.contains(filePath);
    };

    if (outputFileFormat == OutputFileFormat::Yaml) {
        return readExportedDiagnostics(Utils::FilePath::fromString(logFilePath),
                                       acceptFromFilePath,
                                       errorMessage);
    }

    return {};
}

FileInfos ClangTool::collectFileInfos(Project *project, FileSelection fileSelection)
{
    FileSelectionType *selectionType = get_if<FileSelectionType>(&fileSelection);
    // early bailout
    if (selectionType && *selectionType == FileSelectionType::CurrentFile
        && !EditorManager::currentDocument())
        return {};

    auto projectInfo = CppTools::CppModelManager::instance()->projectInfo(project);
    QTC_ASSERT(projectInfo.isValid(), return FileInfos());

    const FileInfos allFileInfos = sortedFileInfos(projectInfo.projectParts());

    if (selectionType && *selectionType == FileSelectionType::AllFiles)
        return allFileInfos;

    if (selectionType && *selectionType == FileSelectionType::AskUser) {
        static int initialProviderIndex = 0;
        SelectableFilesDialog dialog(projectInfo,
                                     fileInfoProviders(project, allFileInfos),
                                     initialProviderIndex);
        if (dialog.exec() == QDialog::Rejected)
            return FileInfos();
        initialProviderIndex = dialog.currentProviderIndex();
        return dialog.fileInfos();
    }

    const FilePath filePath = holds_alternative<FilePath>(fileSelection)
                                  ? get<FilePath>(fileSelection)
                                  : EditorManager::currentDocument()->filePath(); // see early bailout
    if (!filePath.isEmpty()) {
        const FileInfo fileInfo = Utils::findOrDefault(allFileInfos, [&](const FileInfo &fi) {
            return fi.file == filePath;
        });
        if (!fileInfo.file.isEmpty())
            return {fileInfo};
    }

    return {};
}

const QString &ClangTool::name() const
{
    return m_name;
}

void ClangTool::initDiagnosticView()
{
    m_diagnosticView->setFrameStyle(QFrame::NoFrame);
    m_diagnosticView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_diagnosticView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_diagnosticView->setAutoScroll(false);
}

void ClangTool::loadDiagnosticsFromFiles()
{
    // Ask user for files
    const QStringList filePaths
        = QFileDialog::getOpenFileNames(Core::ICore::dialogParent(),
                                        tr("Select YAML Files with Diagnostics"),
                                        QDir::homePath(),
                                        tr("YAML Files (*.yml *.yaml);;All Files (*)"));
    if (filePaths.isEmpty())
        return;

    // Load files
    Diagnostics diagnostics;
    QString errors;
    for (const QString &filePath : filePaths) {
        QString currentError;
        diagnostics << readExportedDiagnostics(Utils::FilePath::fromString(filePath),
                                               {},
                                               &currentError);

        if (!currentError.isEmpty()) {
            if (!errors.isEmpty())
                errors.append("\n");
            errors.append(currentError);
        }
    }

    // Show errors
    if (!errors.isEmpty()) {
        AsynchronousMessageBox::critical(tr("Error Loading Diagnostics"), errors);
        return;
    }

    // Show imported
    reset();
    onNewDiagnosticsAvailable(diagnostics);
    setState(State::ImportFinished);
}

DiagnosticItem *ClangTool::diagnosticItem(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    TreeItem *item = m_diagnosticModel->itemForIndex(m_diagnosticFilterModel->mapToSource(index));
    if (item->level() == 3)
        item = item->parent();
    if (item->level() == 2)
        return static_cast<DiagnosticItem *>(item);

    return {};
}

void ClangTool::showOutputPane()
{
    ProjectExplorerPlugin::showOutputPaneForRunControl(m_runControl);
}

void ClangTool::reset()
{
    m_clear->setEnabled(false);
    m_showFilter->setEnabled(false);
    m_showFilter->setChecked(false);
    m_selectFixitsCheckBox->setEnabled(false);
    m_applyFixitsButton->setEnabled(false);

    m_diagnosticModel->clear();
    m_diagnosticFilterModel->reset();

    m_infoBarWidget->reset();

    m_state = State::Initial;
    m_runControl = nullptr;
    m_runWorker = nullptr;

    m_filesCount = 0;
    m_filesSucceeded = 0;
    m_filesFailed = 0;
}

static bool canAnalyzeProject(Project *project)
{
    if (const Target *target = project->activeTarget()) {
        const Utils::Id c = ProjectExplorer::Constants::C_LANGUAGE_ID;
        const Utils::Id cxx = ProjectExplorer::Constants::CXX_LANGUAGE_ID;
        const bool projectSupportsLanguage = project->projectLanguages().contains(c)
                                             || project->projectLanguages().contains(cxx);
        return projectSupportsLanguage
               && CppModelManager::instance()->projectInfo(project).isValid()
               && ToolChainKitAspect::cxxToolChain(target->kit());
    }
    return false;
}

struct CheckResult {
    enum {
        InvalidTidyExecutable,
        InvalidClazyExecutable,
        ProjectNotOpen,
        ProjectNotSuitable,
        ReadyToAnalyze,
    } kind;
    QString errorText;
};

static CheckResult canAnalyze()
{
    const ClangDiagnosticConfig config = diagnosticConfig(runSettings().diagnosticConfigId());

    if (config.isClangTidyEnabled() && !isFileExecutable(clangTidyExecutable())) {
        return {CheckResult::InvalidTidyExecutable,
                ClangTool::tr("Set a valid Clang-Tidy executable.")};
    }

    if (config.isClazyEnabled() && !isFileExecutable(clazyStandaloneExecutable())) {
        return {CheckResult::InvalidClazyExecutable,
                ClangTool::tr("Set a valid Clazy-Standalone executable.")};
    }

    if (Project *project = SessionManager::startupProject()) {
        if (!canAnalyzeProject(project)) {
            return {CheckResult::ProjectNotSuitable,
                    ClangTool::tr("Project \"%1\" is not a C/C++ project.")
                        .arg(project->displayName())};
        }
    } else {
        return {CheckResult::ProjectNotOpen,
                ClangTool::tr("Open a C/C++ project to start analyzing.")};
    }

    return {CheckResult::ReadyToAnalyze, {}};
}

void ClangTool::updateForInitialState()
{
    if (m_state != State::Initial)
        return;

    m_infoBarWidget->reset();

    const CheckResult result = canAnalyze();
    switch (result.kind)
    case CheckResult::InvalidTidyExecutable: {
    case CheckResult::InvalidClazyExecutable:
        m_infoBarWidget->setError(InfoBarWidget::Warning,
                                  makeLink(result.errorText),
                                  [](){ ICore::showOptionsDialog(Constants::SETTINGS_PAGE_ID); });
        break;
    case CheckResult::ProjectNotSuitable:
    case CheckResult::ProjectNotOpen:
    case CheckResult::ReadyToAnalyze:
        break;
        }
}

void ClangTool::help()
{
    if (DiagnosticItem *item = diagnosticItem(m_diagnosticView->currentIndex())) {
        const QString url = documentationUrl(item->diagnostic().name);
        if (!url.isEmpty())
            QDesktopServices::openUrl(url);
    }
}

void ClangTool::setFilterOptions(const OptionalFilterOptions &filterOptions)
{
    m_diagnosticFilterModel->setFilterOptions(filterOptions);
    const bool isFilterActive = filterOptions
                                    ? (filterOptions->checks != m_diagnosticModel->allChecks())
                                    : false;
    m_showFilter->setChecked(isFilterActive);
}

void ClangTool::filter()
{
    const OptionalFilterOptions filterOptions = m_diagnosticFilterModel->filterOptions();

    // Collect available and currently shown checks
    QHash<QString, Check> checks;
    m_diagnosticModel->forItemsAtLevel<2>([&](DiagnosticItem *item) {
        const QString checkName = item->diagnostic().name;
        Check &check = checks[checkName];
        if (check.name.isEmpty()) {
            check.name = checkName;
            check.displayName = checkName;
            const QString clangDiagPrefix = "clang-diagnostic-";
            if (check.displayName.startsWith(clangDiagPrefix))
                check.displayName = QString("-W%1").arg(check.name.mid(clangDiagPrefix.size()));
            check.count = 1;
            check.isShown = filterOptions ? filterOptions->checks.contains(checkName) : true;
            check.hasFixit = check.hasFixit || item->diagnostic().hasFixits;
            checks.insert(check.name, check);
        } else {
            ++check.count;
        }
    });

    // Show dialog
    FilterDialog dialog(checks.values());
    if (dialog.exec() == QDialog::Rejected)
        return;

    // Apply filter
    setFilterOptions(FilterOptions{dialog.selectedChecks()});
}

void ClangTool::clearFilter()
{
    m_diagnosticFilterModel->setFilterOptions({});
    m_showFilter->setChecked(false);
}

void ClangTool::filterForCurrentKind()
{
    if (DiagnosticItem *item = diagnosticItem(m_diagnosticView->currentIndex()))
        setFilterOptions(FilterOptions{{item->diagnostic().name}});
}

void ClangTool::filterOutCurrentKind()
{
    if (DiagnosticItem *item = diagnosticItem(m_diagnosticView->currentIndex())) {
        const OptionalFilterOptions filterOpts = m_diagnosticFilterModel->filterOptions();
        QSet<QString> checks = filterOpts ? filterOpts->checks : m_diagnosticModel->allChecks();
        checks.remove(item->diagnostic().name);

        setFilterOptions(FilterOptions{checks});
    }
}

void ClangTool::onBuildFailed()
{
    m_infoBarWidget->setError(InfoBarWidget::Error,
                              tr("Failed to build the project."),
                              [this]() { showOutputPane(); });
    setState(State::PreparationFailed);
}

void ClangTool::onStartFailed()
{
    m_infoBarWidget->setError(InfoBarWidget::Error,
                              makeLink(tr("Failed to start the analyzer.")),
                              [this]() { showOutputPane(); });
    setState(State::PreparationFailed);
}

void ClangTool::onStarted()
{
    setState(State::AnalyzerRunning);
}

void ClangTool::onRunControlStopped()
{
    if (m_state != State::StoppedByUser && m_state != State::PreparationFailed)
        setState(State::AnalyzerFinished);
    emit finished(m_infoBarWidget->errorText());
}

void ClangTool::update()
{
    updateForInitialState();
    updateForCurrentState();
}

using DocumentPredicate = std::function<bool(Core::IDocument *)>;

static FileInfos fileInfosMatchingDocuments(const FileInfos &fileInfos,
                                            const DocumentPredicate &predicate)
{
    QSet<Utils::FilePath> documentPaths;
    for (const Core::DocumentModel::Entry *e : Core::DocumentModel::entries()) {
        if (predicate(e->document))
            documentPaths.insert(e->fileName());
    }

    return Utils::filtered(fileInfos, [documentPaths](const FileInfo &fileInfo) {
        return documentPaths.contains(fileInfo.file);
    });
}

static FileInfos fileInfosMatchingOpenedDocuments(const FileInfos &fileInfos)
{
    // Note that (initially) suspended text documents are still IDocuments, not yet TextDocuments.
    return fileInfosMatchingDocuments(fileInfos, [](Core::IDocument *) { return true; });
}

static FileInfos fileInfosMatchingEditedDocuments(const FileInfos &fileInfos)
{
    return fileInfosMatchingDocuments(fileInfos, [](Core::IDocument *document) {
        if (auto textDocument = qobject_cast<TextEditor::TextDocument*>(document))
            return textDocument->document()->revision() > 1;
        return false;
    });
}

FileInfoProviders ClangTool::fileInfoProviders(ProjectExplorer::Project *project,
                                               const FileInfos &allFileInfos)
{
    const QSharedPointer<ClangToolsProjectSettings> s = ClangToolsProjectSettings::getSettings(project);
    static FileInfoSelection openedFilesSelection;
    static FileInfoSelection editeddFilesSelection;

    return {
        {ClangTool::tr("All Files"),
         allFileInfos,
         FileInfoSelection{s->selectedDirs(), s->selectedFiles()},
         FileInfoProvider::Limited,
         [s](const FileInfoSelection &selection) {
             s->setSelectedDirs(selection.dirs);
             s->setSelectedFiles(selection.files);
         }},

        {ClangTool::tr("Opened Files"),
         fileInfosMatchingOpenedDocuments(allFileInfos),
         openedFilesSelection,
         FileInfoProvider::All,
         [](const FileInfoSelection &selection) { openedFilesSelection = selection; }},

        {ClangTool::tr("Edited Files"),
         fileInfosMatchingEditedDocuments(allFileInfos),
         editeddFilesSelection,
         FileInfoProvider::All,
         [](const FileInfoSelection &selection) { editeddFilesSelection = selection; }},
    };
}

void ClangTool::setState(ClangTool::State state)
{
    m_state = state;
    updateForCurrentState();
}

QSet<Diagnostic> ClangTool::diagnostics() const
{
    return Utils::filtered(m_diagnosticModel->diagnostics(), [](const Diagnostic &diagnostic) {
        using CppTools::ProjectFile;
        return ProjectFile::isSource(ProjectFile::classify(diagnostic.location.filePath));
    });
}

void ClangTool::onNewDiagnosticsAvailable(const Diagnostics &diagnostics)
{
    QTC_ASSERT(m_diagnosticModel, return);
    m_diagnosticModel->addDiagnostics(diagnostics);
}

void ClangTool::updateForCurrentState()
{
    // Actions
    bool canStart = false;
    const bool isPreparing = m_state == State::PreparationStarted;
    const bool isRunning = m_state == State::AnalyzerRunning;
    QString startActionToolTip = m_startAction->text();
    QString startOnCurrentToolTip = m_startOnCurrentFileAction->text();
    if (!isRunning) {
        const CheckResult result = canAnalyze();
        canStart = result.kind == CheckResult::ReadyToAnalyze;
        if (!canStart) {
            startActionToolTip = result.errorText;
            startOnCurrentToolTip = result.errorText;
        }
    }
    m_startAction->setEnabled(canStart);
    m_startAction->setToolTip(startActionToolTip);
    m_startOnCurrentFileAction->setEnabled(canStart);
    m_startOnCurrentFileAction->setToolTip(startOnCurrentToolTip);
    m_stopAction->setEnabled(isPreparing || isRunning);

    const int issuesFound = m_diagnosticModel->diagnostics().count();
    const int issuesVisible = m_diagnosticFilterModel->diagnostics();
    m_goBack->setEnabled(issuesVisible > 1);
    m_goNext->setEnabled(issuesVisible > 1);
    m_clear->setEnabled(!isRunning);
    m_expandCollapse->setEnabled(issuesVisible);
    m_loadExported->setEnabled(!isRunning);
    m_showFilter->setEnabled(issuesFound > 1);

    // Diagnostic view
    m_diagnosticView->setCursor(isRunning ? Qt::BusyCursor : Qt::ArrowCursor);

    // Info bar: errors
    const bool hasErrorText = !m_infoBarWidget->errorText().isEmpty();
    const bool hasErrors = m_filesFailed > 0;
    if (hasErrors && !hasErrorText) {
        const QString text = makeLink( tr("Failed to analyze %1 files.").arg(m_filesFailed));
        m_infoBarWidget->setError(InfoBarWidget::Warning, text, [this]() { showOutputPane(); });
    }

    // Info bar: info
    bool showProgressIcon = false;
    QString infoText;
    switch (m_state) {
    case State::Initial:
        infoText = m_infoBarWidget->infoText();
        break;
    case State::AnalyzerRunning:
        showProgressIcon = true;
        if (m_filesCount == 0) {
            infoText = tr("Analyzing..."); // Not yet fully started/initialized
        } else {
            infoText = tr("Analyzing... %1 of %2 files processed.")
                           .arg(m_filesSucceeded + m_filesFailed)
                           .arg(m_filesCount);
        }
        break;
    case State::PreparationStarted:
        showProgressIcon = true;
        infoText = m_infoBarWidget->infoText();
        break;
    case State::PreparationFailed:
        break; // OK, we just show an error.
    case State::StoppedByUser:
        infoText = tr("Analysis stopped by user.");
        break;
    case State::AnalyzerFinished:
        infoText = tr("Finished processing %1 files.").arg(m_filesCount);
        break;
    case State::ImportFinished:
        infoText = tr("Diagnostics imported.");
        break;
    }
    m_infoBarWidget->setInfoText(infoText);
    m_infoBarWidget->setInfoIcon(showProgressIcon ? InfoBarWidget::ProgressIcon
                                                  : InfoBarWidget::InfoIcon);

    // Info bar: diagnostic stats
    QString diagText;
    if (issuesFound) {
        diagText = tr("%1 diagnostics. %2 fixits, %3 selected.")
                   .arg(issuesVisible)
                   .arg(m_diagnosticFilterModel->fixitsScheduable())
                   .arg(m_diagnosticFilterModel->fixitsScheduled());
    } else if (m_state != State::AnalyzerRunning
               && m_state != State::Initial
               && m_state != State::PreparationStarted
               && m_state != State::PreparationFailed) {
        diagText = tr("No diagnostics.");
    }
    m_infoBarWidget->setDiagText(diagText);
}

} // namespace Internal
} // namespace ClangTools

#include "clangtool.moc"
