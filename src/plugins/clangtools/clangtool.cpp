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
#include <utils/fancylineedit.h>
#include <utils/fancymainwindow.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QFileDialog>
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

    for (CppTools::ProjectPart::Ptr projectPart : projectParts) {
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

    Utils::sort(fileInfos, &FileInfo::file);
    fileInfos.erase(std::unique(fileInfos.begin(), fileInfos.end()), fileInfos.end());

    return fileInfos;
}

static RunSettings runSettings()
{
    Project *project = SessionManager::startupProject();
    auto *projectSettings = ClangToolsProjectSettingsManager::getSettings(project);
    if (projectSettings->useGlobalSettings())
        return ClangToolsSettings::instance()->runSettings();
    return projectSettings->runSettings();
}

static ClangDiagnosticConfig diagnosticConfig(const Core::Id &diagConfigId)
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

    m_diagnosticView = new DiagnosticView;
    initDiagnosticView();
    m_diagnosticView->setModel(m_diagnosticFilterModel);
    m_diagnosticView->setSortingEnabled(true);
    m_diagnosticView->sortByColumn(Debugger::DetailedErrorView::DiagnosticColumn,
                                   Qt::AscendingOrder);
    m_diagnosticView->setObjectName(QLatin1String("ClangTidyClazyIssuesView"));
    m_diagnosticView->setWindowTitle(tr("Clang-Tidy and Clazy Diagnostics"));

    foreach (auto * const model,
             QList<QAbstractItemModel *>({m_diagnosticModel, m_diagnosticFilterModel})) {
        connect(model, &QAbstractItemModel::rowsInserted,
                this, &ClangTool::handleStateUpdate);
        connect(model, &QAbstractItemModel::rowsRemoved,
                this, &ClangTool::handleStateUpdate);
        connect(model, &QAbstractItemModel::modelReset,
                this, &ClangTool::handleStateUpdate);
        connect(model, &QAbstractItemModel::layoutChanged, // For QSortFilterProxyModel::invalidate()
                this, &ClangTool::handleStateUpdate);
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
    action->setToolTip(tr("Load Diagnostics from YAML Files exported with \"-export-fixes\"."));
    connect(action, &QAction::triggered, this, &ClangTool::loadDiagnosticsFromFiles);
    m_loadExported = action;

    // Clear data
    action = new QAction(this);
    action->setDisabled(true);
    action->setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
    action->setToolTip(tr("Clear"));
    connect(action, &QAction::triggered, [this](){
        m_clear->setEnabled(false);
        m_diagnosticModel->clear();
        Debugger::showPermanentStatusMessage(QString());
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

    // Filter line edit
    m_filterLineEdit = new Utils::FancyLineEdit();
    m_filterLineEdit->setFiltering(true);
    m_filterLineEdit->setPlaceholderText(tr("Filter Diagnostics"));
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

    m_perspective.addWindow(m_diagnosticView, Perspective::SplitVertical, nullptr);

    action = new QAction(tr("Clang-Tidy and Clazy..."), this);
    action->setToolTip(toolTip);
    menu->addAction(ActionManager::registerAction(action, "ClangTidyClazy.Action"),
                    Debugger::Constants::G_ANALYZER_TOOLS);
    QObject::connect(action, &QAction::triggered, this, [this]() {
        startTool(FileSelection::AskUser);
    });
    QObject::connect(m_startAction, &QAction::triggered, action, &QAction::triggered);
    QObject::connect(m_startAction, &QAction::changed, action, [action, this] {
        action->setEnabled(m_startAction->isEnabled());
    });

    QObject::connect(m_startOnCurrentFileAction, &QAction::triggered, this, [this] {
        startTool(FileSelection::CurrentFile);
    });

    m_perspective.addToolBarAction(m_startAction);
    m_perspective.addToolBarAction(m_startOnCurrentFileAction);
    m_perspective.addToolBarAction(m_stopAction);
    m_perspective.addToolBarAction(m_openProjectSettings);
    m_perspective.addToolBarAction(m_loadExported);
    m_perspective.addToolBarAction(m_clear);
    m_perspective.addToolBarAction(m_goBack);
    m_perspective.addToolBarAction(m_goNext);
    m_perspective.addToolBarAction(m_expandCollapse);
    m_perspective.addToolBarWidget(m_filterLineEdit);
    m_perspective.addToolBarWidget(m_applyFixitsButton);

    updateRunActions();

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::updateRunActions,
            this, &ClangTool::updateRunActions);
}

ClangTool::~ClangTool()
{
    delete m_diagnosticView;
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

void ClangTool::startTool(ClangTool::FileSelection fileSelection,
                          const RunSettings &runSettings,
                          const CppTools::ClangDiagnosticConfig &diagnosticConfig)
{
    Project *project = SessionManager::startupProject();
    QTC_ASSERT(project, return);
    QTC_ASSERT(project->activeTarget(), return);

    auto runControl = new RunControl(Constants::CLANGTIDYCLAZY_RUN_MODE);
    runControl->setDisplayName(tr("Clang-Tidy and Clazy"));
    runControl->setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);
    runControl->setTarget(project->activeTarget());

    const FileInfos fileInfos = collectFileInfos(project, fileSelection);
    if (fileInfos.empty())
        return;

    const bool preventBuild = fileSelection == FileSelection::CurrentFile;
    auto clangTool = new ClangToolRunWorker(runControl,
                                            runSettings,
                                            diagnosticConfig,
                                            fileInfos,
                                            preventBuild);

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

    m_diagnosticFilterModel->setProject(project);
    m_applyFixitsButton->setEnabled(false);
    m_running = true;

    setToolBusy(true);
    handleStateUpdate();
    updateRunActions();

    ProjectExplorerPlugin::startRunControl(runControl);
}

Diagnostics ClangTool::read(OutputFileFormat outputFileFormat,
                            const QString &logFilePath,
                            const QString &mainFilePath,
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
    return readSerializedDiagnostics(Utils::FilePath::fromString(logFilePath),
                                     Utils::FilePath::fromString(mainFilePath),
                                     acceptFromFilePath,
                                     errorMessage);
}

FileInfos ClangTool::collectFileInfos(Project *project, FileSelection fileSelection)
{
    auto projectInfo = CppTools::CppModelManager::instance()->projectInfo(project);
    QTC_ASSERT(projectInfo.isValid(), return FileInfos());

    const FileInfos allFileInfos = sortedFileInfos(projectInfo.projectParts());

    if (fileSelection == FileSelection::AllFiles)
        return allFileInfos;

    if (fileSelection == FileSelection::AskUser) {
        static int initialProviderIndex = 0;
        SelectableFilesDialog dialog(projectInfo,
                                     fileInfoProviders(project, allFileInfos),
                                     initialProviderIndex);
        if (dialog.exec() == QDialog::Rejected)
            return FileInfos();
        initialProviderIndex = dialog.currentProviderIndex();
        return dialog.fileInfos();
    }

    if (fileSelection == FileSelection::CurrentFile) {
        if (const IDocument *document = EditorManager::currentDocument()) {
            const Utils::FilePath filePath = document->filePath();
            if (!filePath.isEmpty()) {
                const FileInfo fileInfo = Utils::findOrDefault(allFileInfos,
                                                               [&](const FileInfo &fi) {
                                                                   return fi.file == filePath;
                                                               });
                if (!fileInfo.file.isEmpty())
                    return {fileInfo};
            }
        }
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
        = QFileDialog::getOpenFileNames(Core::ICore::mainWindow(),
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
    if (!errors.isEmpty())
        AsynchronousMessageBox::critical(tr("Error Loading Diagnostics"), errors);

    // Show imported
    m_diagnosticModel->clear();
    onNewDiagnosticsAvailable(diagnostics);
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
    ClangToolsProjectSettings *s = ClangToolsProjectSettingsManager::getSettings(project);
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
    if (!m_diagnosticFilterModel->filterRegExp().pattern().isEmpty())
        m_diagnosticFilterModel->invalidateFilter();
}

void ClangTool::updateRunActions()
{
    if (m_toolBusy) {
        QString tooltipText = tr("Clang-Tidy and Clazy are still running.");

        m_startAction->setEnabled(false);
        m_startAction->setToolTip(tooltipText);

        m_startOnCurrentFileAction->setEnabled(false);
        m_startOnCurrentFileAction->setToolTip(tooltipText);

        m_stopAction->setEnabled(true);
        m_loadExported->setEnabled(false);
        m_clear->setEnabled(false);
    } else {
        QString toolTipStart = m_startAction->text();
        QString toolTipStartOnCurrentFile = m_startOnCurrentFileAction->text();

        Project *project = SessionManager::startupProject();
        Target *target = project ? project->activeTarget() : nullptr;
        const Core::Id cxx = ProjectExplorer::Constants::CXX_LANGUAGE_ID;
        bool canRun = target && project->projectLanguages().contains(cxx)
                && ToolChainKitAspect::toolChain(target->kit(), cxx);
        if (!canRun)
            toolTipStart = toolTipStartOnCurrentFile = tr("This is not a C/C++ project.");

        m_startAction->setEnabled(canRun);
        m_startAction->setToolTip(toolTipStart);

        m_startOnCurrentFileAction->setEnabled(canRun);
        m_startOnCurrentFileAction->setToolTip(toolTipStartOnCurrentFile);

        m_stopAction->setEnabled(false);
        m_loadExported->setEnabled(true);
        m_clear->setEnabled(m_diagnosticModel->diagnostics().count());
    }
}

void ClangTool::handleStateUpdate()
{
    QTC_ASSERT(m_goBack, return);
    QTC_ASSERT(m_goNext, return);
    QTC_ASSERT(m_diagnosticModel, return);
    QTC_ASSERT(m_diagnosticFilterModel, return);

    const int issuesFound = m_diagnosticModel->diagnostics().count();
    const int issuesVisible = m_diagnosticFilterModel->rowCount();
    m_goBack->setEnabled(issuesVisible > 1);
    m_goNext->setEnabled(issuesVisible > 1);
    m_clear->setEnabled(issuesFound > 0);
    m_expandCollapse->setEnabled(issuesVisible);

    m_loadExported->setEnabled(!m_running);

    QString message;
    if (m_running) {
        if (issuesFound)
            message = tr("Running - %n diagnostics", nullptr, issuesFound);
        else
            message = tr("Running - No diagnostics");
    } else {
        if (issuesFound)
            message = tr("Finished - %n diagnostics", nullptr, issuesFound);
        else
            message = tr("Finished - No diagnostics");
    }

    Debugger::showPermanentStatusMessage(message);
}

void ClangTool::setToolBusy(bool busy)
{
    QTC_ASSERT(m_diagnosticView, return);
    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_diagnosticView->setCursor(cursor);
    m_toolBusy = busy;
}

} // namespace Internal
} // namespace ClangTools
