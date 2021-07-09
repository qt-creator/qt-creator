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

#include "clangmodelmanagersupport.h"

#include "clangconstants.h"
#include "clangcurrentdocumentfilter.h"
#include "clangdclient.h"
#include "clangdquickfixfactory.h"
#include "clangeditordocumentprocessor.h"
#include "clangfollowsymbol.h"
#include "clanggloballocatorfilters.h"
#include "clanghoverhandler.h"
#include "clangoverviewmodel.h"
#include "clangprojectsettings.h"
#include "clangrefactoringengine.h"
#include "clangutils.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cppfollowsymbolundercursor.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/editordocumenthandle.h>
#include <cpptools/projectinfo.h>

#include <languageclient/languageclientmanager.h>

#include <texteditor/quickfix.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <clangsupport/filecontainer.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QApplication>
#include <QMenu>
#include <QTextBlock>
#include <QTimer>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace LanguageClient;

static ClangModelManagerSupport *m_instance = nullptr;

static CppTools::CppModelManager *cppModelManager()
{
    return CppTools::CppModelManager::instance();
}

ClangModelManagerSupport::ClangModelManagerSupport()
    : m_completionAssistProvider(m_communicator, CompletionType::Other)
    , m_functionHintAssistProvider(m_communicator, CompletionType::FunctionHint)
    , m_followSymbol(new ClangFollowSymbol)
    , m_refactoringEngine(new RefactoringEngine)
{
    QTC_CHECK(!m_instance);
    m_instance = this;

    CppTools::CppModelManager::instance()->setCurrentDocumentFilter(
                std::make_unique<ClangCurrentDocumentFilter>());
    cppModelManager()->setLocatorFilter(std::make_unique<ClangGlobalSymbolFilter>());
    cppModelManager()->setClassesFilter(std::make_unique<ClangClassesFilter>());
    cppModelManager()->setFunctionsFilter(std::make_unique<ClangFunctionsFilter>());

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::editorOpened,
            this, &ClangModelManagerSupport::onEditorOpened);
    connect(editorManager, &Core::EditorManager::currentEditorChanged,
            this, &ClangModelManagerSupport::onCurrentEditorChanged);
    connect(editorManager, &Core::EditorManager::editorsClosed,
            this, &ClangModelManagerSupport::onEditorClosed);

    CppTools::CppModelManager *modelManager = cppModelManager();
    connect(modelManager, &CppTools::CppModelManager::abstractEditorSupportContentsUpdated,
            this, &ClangModelManagerSupport::onAbstractEditorSupportContentsUpdated);
    connect(modelManager, &CppTools::CppModelManager::abstractEditorSupportRemoved,
            this, &ClangModelManagerSupport::onAbstractEditorSupportRemoved);
    connect(modelManager, &CppTools::CppModelManager::projectPartsUpdated,
            this, &ClangModelManagerSupport::onProjectPartsUpdated);
    connect(modelManager, &CppTools::CppModelManager::projectPartsRemoved,
            this, &ClangModelManagerSupport::onProjectPartsRemoved);

    auto *sessionManager = ProjectExplorer::SessionManager::instance();
    connect(sessionManager, &ProjectExplorer::SessionManager::projectAdded,
            this, &ClangModelManagerSupport::onProjectAdded);
    connect(sessionManager, &ProjectExplorer::SessionManager::aboutToRemoveProject,
            this, &ClangModelManagerSupport::onAboutToRemoveProject);

    CppTools::ClangdSettings::setDefaultClangdPath(Utils::FilePath::fromString(
            Core::ICore::clangdExecutable(CLANG_BINDIR)));
    connect(&CppTools::ClangdSettings::instance(), &CppTools::ClangdSettings::changed,
            this, &ClangModelManagerSupport::onClangdSettingsChanged);
    CppTools::CppCodeModelSettings *settings = CppTools::codeModelSettings();
    connect(settings, &CppTools::CppCodeModelSettings::clangDiagnosticConfigsInvalidated,
            this, &ClangModelManagerSupport::onDiagnosticConfigsInvalidated);

    // TODO: Enable this once we do document-level stuff with clangd (highlighting etc)
    // createClient(nullptr, {});
    m_generatorSynchronizer.setCancelOnWait(true);
    new ClangdQuickFixFactory(); // memory managed by CppEditor::g_cppQuickFixFactories
}

ClangModelManagerSupport::~ClangModelManagerSupport()
{
    QTC_CHECK(m_projectSettings.isEmpty());
    m_generatorSynchronizer.waitForFinished();
    m_instance = nullptr;
}

CppTools::CppCompletionAssistProvider *ClangModelManagerSupport::completionAssistProvider()
{
    return &m_completionAssistProvider;
}

CppTools::CppCompletionAssistProvider *ClangModelManagerSupport::functionHintAssistProvider()
{
    return &m_functionHintAssistProvider;
}

TextEditor::BaseHoverHandler *ClangModelManagerSupport::createHoverHandler()
{
    return new Internal::ClangHoverHandler;
}

CppTools::FollowSymbolInterface &ClangModelManagerSupport::followSymbolInterface()
{
    return *m_followSymbol;
}

CppTools::RefactoringEngineInterface &ClangModelManagerSupport::refactoringEngineInterface()
{
    return *m_refactoringEngine;
}

std::unique_ptr<CppTools::AbstractOverviewModel> ClangModelManagerSupport::createOverviewModel()
{
    return std::make_unique<OverviewModel>();
}

bool ClangModelManagerSupport::supportsOutline(const TextEditor::TextDocument *document) const
{
    return !clientForFile(document->filePath());
}

CppTools::BaseEditorDocumentProcessor *ClangModelManagerSupport::createEditorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument)
{
    return new ClangEditorDocumentProcessor(m_communicator, baseTextDocument);
}

void ClangModelManagerSupport::onCurrentEditorChanged(Core::IEditor *editor)
{
    m_communicator.documentVisibilityChanged();

    // Update task hub issues for current CppEditorDocument
    ClangEditorDocumentProcessor::clearTaskHubIssues();
    if (!editor || !editor->document() || !cppModelManager()->isCppEditor(editor))
        return;

    const ::Utils::FilePath filePath = editor->document()->filePath();
    if (auto processor = ClangEditorDocumentProcessor::get(filePath.toString())) {
        processor->semanticRehighlight();
        processor->generateTaskHubIssues();
    }
}

void ClangModelManagerSupport::connectTextDocumentToTranslationUnit(TextEditor::TextDocument *textDocument)
{
    // Handle externally changed documents
    connect(textDocument, &Core::IDocument::aboutToReload,
            this, &ClangModelManagerSupport::onCppDocumentAboutToReloadOnTranslationUnit,
            Qt::UniqueConnection);
    connect(textDocument, &Core::IDocument::reloadFinished,
            this, &ClangModelManagerSupport::onCppDocumentReloadFinishedOnTranslationUnit,
            Qt::UniqueConnection);

    // Handle changes from e.g. refactoring actions
    connectToTextDocumentContentsChangedForTranslationUnit(textDocument);
}

void ClangModelManagerSupport::connectTextDocumentToUnsavedFiles(TextEditor::TextDocument *textDocument)
{
    // Handle externally changed documents
    connect(textDocument, &Core::IDocument::aboutToReload,
            this, &ClangModelManagerSupport::onCppDocumentAboutToReloadOnUnsavedFile,
            Qt::UniqueConnection);
    connect(textDocument, &Core::IDocument::reloadFinished,
            this, &ClangModelManagerSupport::onCppDocumentReloadFinishedOnUnsavedFile,
            Qt::UniqueConnection);

    // Handle changes from e.g. refactoring actions
    connectToTextDocumentContentsChangedForUnsavedFile(textDocument);
}

void ClangModelManagerSupport::connectToTextDocumentContentsChangedForTranslationUnit(
        TextEditor::TextDocument *textDocument)
{
    connect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
            this, &ClangModelManagerSupport::onCppDocumentContentsChangedOnTranslationUnit,
            Qt::UniqueConnection);
}

void ClangModelManagerSupport::connectToTextDocumentContentsChangedForUnsavedFile(
        TextEditor::TextDocument *textDocument)
{
    connect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
            this, &ClangModelManagerSupport::onCppDocumentContentsChangedOnUnsavedFile,
            Qt::UniqueConnection);
}

void ClangModelManagerSupport::connectToWidgetsMarkContextMenuRequested(QWidget *editorWidget)
{
    const auto widget = qobject_cast<TextEditor::TextEditorWidget *>(editorWidget);
    if (widget) {
        connect(widget, &TextEditor::TextEditorWidget::markContextMenuRequested,
                this, &ClangModelManagerSupport::onTextMarkContextMenuRequested);
    }
}

void ClangModelManagerSupport::updateLanguageClient(ProjectExplorer::Project *project,
                                                    const CppTools::ProjectInfo &projectInfo)
{
    if (!CppTools::ClangdProjectSettings(project).settings().useClangd)
        return;
    const auto getJsonDbDir = [project] {
        if (const ProjectExplorer::Target * const target = project->activeTarget()) {
            if (const ProjectExplorer::BuildConfiguration * const bc
                    = target->activeBuildConfiguration()) {
                return bc->buildDirectory();
            }
        }
        return Utils::FilePath();
    };

    const Utils::FilePath jsonDbDir = getJsonDbDir();
    if (jsonDbDir.isEmpty())
        return;
    const auto generatorWatcher = new QFutureWatcher<GenerateCompilationDbResult>;
    connect(generatorWatcher, &QFutureWatcher<GenerateCompilationDbResult>::finished,
            [this, project, projectInfo, getJsonDbDir, jsonDbDir, generatorWatcher] {
        generatorWatcher->deleteLater();
        if (!ProjectExplorer::SessionManager::hasProject(project))
            return;
        if (!CppTools::ClangdProjectSettings(project).settings().useClangd)
            return;
        if (cppModelManager()->projectInfo(project) != projectInfo)
            return;
        if (getJsonDbDir() != jsonDbDir)
            return;
        const GenerateCompilationDbResult result = generatorWatcher->result();
        if (!result.error.isEmpty()) {
            Core::MessageManager::writeDisrupting(
                        tr("Cannot use clangd: Failed to generate compilation database:\n%1")
                        .arg(result.error));
            return;
        }
        if (Client * const oldClient = clientForProject(project))
            LanguageClientManager::shutdownClient(oldClient);
        ClangdClient * const client = createClient(project, jsonDbDir);
        connect(client, &Client::initialized, this, [client, project, projectInfo, jsonDbDir] {
            using namespace ProjectExplorer;
            if (!SessionManager::hasProject(project))
                return;
            if (!CppTools::ClangdProjectSettings(project).settings().useClangd)
                return;
            if (cppModelManager()->projectInfo(project) != projectInfo)
                return;

            // Acquaint the client with all open C++ documents for this project.
            bool hasDocuments = false;
            for (const Core::DocumentModel::Entry * const entry : Core::DocumentModel::entries()) {
                const auto textDocument = qobject_cast<TextEditor::TextDocument *>(entry->document);
                if (!textDocument)
                    continue;
                const bool isCppDocument = Utils::contains(
                            Core::DocumentModel::editorsForDocument(textDocument),
                            [](Core::IEditor *editor) {
                                return CppTools::CppModelManager::isCppEditor(editor);
                            });
                if (!isCppDocument)
                    continue;
                if (!project->isKnownFile(entry->fileName()))
                    continue;
                client->openDocument(textDocument);
                ClangEditorDocumentProcessor::clearTextMarks(textDocument->filePath());
                hasDocuments = true;
            }

            if (hasDocuments)
                return;

            // clangd oddity: Background indexing only starts after opening a random file.
            // TODO: changes to the compilation db do not seem to trigger re-indexing.
            //       How to force it?
            ProjectNode * const rootNode = project->rootProjectNode();
            if (!rootNode)
                return;
            const Node * const cxxNode = rootNode->findNode([](Node *n) {
                const FileNode * const fileNode = n->asFileNode();
                return fileNode && (fileNode->fileType() == FileType::Source
                                    || fileNode->fileType() == FileType::Header)
                    && fileNode->filePath().exists();
            });
            if (!cxxNode)
                return;

            client->openExtraFile(cxxNode->filePath());
            client->closeExtraFile(cxxNode->filePath());
        });

    });
    auto future = Utils::runAsync(&Internal::generateCompilationDB, projectInfo,
                                  CompilationDbPurpose::CodeModel);
    generatorWatcher->setFuture(future);
    m_generatorSynchronizer.addFuture(future);
}

ClangdClient *ClangModelManagerSupport::clientForProject(
        const ProjectExplorer::Project *project) const
{
    const QList<Client *> clients = Utils::filtered(
                LanguageClientManager::clientsForProject(project),
                    [](const LanguageClient::Client *c) {
        return qobject_cast<const ClangdClient *>(c)
                && c->state() != Client::ShutdownRequested
                && c->state() != Client::Shutdown;
    });
    QTC_CHECK(clients.size() <= 1);
    return clients.empty() ? nullptr : qobject_cast<ClangdClient *>(clients.first());
}

ClangdClient *ClangModelManagerSupport::clientForFile(const Utils::FilePath &file) const
{
    return clientForProject(ProjectExplorer::SessionManager::projectForFile(file));
}

ClangdClient *ClangModelManagerSupport::createClient(ProjectExplorer::Project *project,
                                                     const Utils::FilePath &jsonDbDir)
{
    const auto client = new ClangdClient(project, jsonDbDir);
    emit createdClient(client);
    return client;
}

void ClangModelManagerSupport::onEditorOpened(Core::IEditor *editor)
{
    QTC_ASSERT(editor, return);
    Core::IDocument *document = editor->document();
    QTC_ASSERT(document, return);
    auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);

    if (textDocument && cppModelManager()->isCppEditor(editor)) {
        connectTextDocumentToTranslationUnit(textDocument);
        connectToWidgetsMarkContextMenuRequested(editor->widget());

        // TODO: Ensure that not fully loaded documents are updated?

        ProjectExplorer::Project * const project
                = ProjectExplorer::SessionManager::projectForFile(document->filePath());
        if (Client * const client = clientForProject(project))
            client->openDocument(textDocument);
    }
}

void ClangModelManagerSupport::onEditorClosed(const QList<Core::IEditor *> &)
{
    m_communicator.documentVisibilityChanged();
}

void ClangModelManagerSupport::onCppDocumentAboutToReloadOnTranslationUnit()
{
    auto textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
    disconnect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
               this, &ClangModelManagerSupport::onCppDocumentContentsChangedOnTranslationUnit);
}

void ClangModelManagerSupport::onCppDocumentReloadFinishedOnTranslationUnit(bool success)
{
    if (success) {
        auto textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
        connectToTextDocumentContentsChangedForTranslationUnit(textDocument);
        m_communicator.documentsChangedWithRevisionCheck(textDocument);
    }
}

namespace {
void clearDiagnosticFixIts(const QString &filePath)
{
    auto processor = ClangEditorDocumentProcessor::get(filePath);
    if (processor)
        processor->clearDiagnosticsWithFixIts();
}
}

void ClangModelManagerSupport::onCppDocumentContentsChangedOnTranslationUnit(int position,
                                                                             int /*charsRemoved*/,
                                                                             int /*charsAdded*/)
{
    auto document = qobject_cast<Core::IDocument *>(sender());

    m_communicator.updateChangeContentStartPosition(document->filePath().toString(),
                                                       position);
    m_communicator.documentsChangedIfNotCurrentDocument(document);

    clearDiagnosticFixIts(document->filePath().toString());
}

void ClangModelManagerSupport::onCppDocumentAboutToReloadOnUnsavedFile()
{
    auto textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
    disconnect(textDocument, &TextEditor::TextDocument::contentsChangedWithPosition,
               this, &ClangModelManagerSupport::onCppDocumentContentsChangedOnUnsavedFile);
}

void ClangModelManagerSupport::onCppDocumentReloadFinishedOnUnsavedFile(bool success)
{
    if (success) {
        auto textDocument = qobject_cast<TextEditor::TextDocument *>(sender());
        connectToTextDocumentContentsChangedForUnsavedFile(textDocument);
        m_communicator.unsavedFilesUpdated(textDocument);
    }
}

void ClangModelManagerSupport::onCppDocumentContentsChangedOnUnsavedFile()
{
    auto document = qobject_cast<Core::IDocument *>(sender());
    m_communicator.unsavedFilesUpdated(document);
}

void ClangModelManagerSupport::onAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                                      const QString &,
                                                                      const QByteArray &content)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    if (content.size() == 0)
        return; // Generation not yet finished.

    const QString mappedPath = m_uiHeaderOnDiskManager.write(filePath, content);
    m_communicator.unsavedFilesUpdated(mappedPath, content, 0);
}

void ClangModelManagerSupport::onAbstractEditorSupportRemoved(const QString &filePath)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    if (!cppModelManager()->cppEditorDocument(filePath)) {
        const QString mappedPath = m_uiHeaderOnDiskManager.remove(filePath);
        const QString projectPartId = projectPartIdForFile(filePath);
        m_communicator.unsavedFilesRemoved({{mappedPath, projectPartId}});
    }
}

void addFixItsActionsToMenu(QMenu *menu, const TextEditor::QuickFixOperations &fixItOperations)
{
    foreach (const auto &fixItOperation, fixItOperations) {
        QAction *action = menu->addAction(fixItOperation->description());
        QObject::connect(action, &QAction::triggered, [fixItOperation]() {
            fixItOperation->perform();
        });
    }
}

static int lineToPosition(const QTextDocument *textDocument, int lineNumber)
{
    QTC_ASSERT(textDocument, return 0);
    const QTextBlock textBlock = textDocument->findBlockByLineNumber(lineNumber);
    return textBlock.isValid() ? textBlock.position() - 1 : 0;
}

static TextEditor::AssistInterface createAssistInterface(TextEditor::TextEditorWidget *widget,
                                                         int lineNumber)
{
    return TextEditor::AssistInterface(widget->document(),
                                       lineToPosition(widget->document(), lineNumber),
                                       widget->textDocument()->filePath(),
                                       TextEditor::IdleEditor);
}

void ClangModelManagerSupport::onTextMarkContextMenuRequested(TextEditor::TextEditorWidget *widget,
                                                              int lineNumber,
                                                              QMenu *menu)
{
    QTC_ASSERT(widget, return);
    QTC_ASSERT(lineNumber >= 1, return);
    QTC_ASSERT(menu, return);

    const auto filePath = widget->textDocument()->filePath().toString();
    ClangEditorDocumentProcessor *processor = ClangEditorDocumentProcessor::get(filePath);
    if (processor) {
        const auto assistInterface = createAssistInterface(widget, lineNumber);
        const auto fixItOperations = processor->extraRefactoringOperations(assistInterface);

        addFixItsActionsToMenu(menu, fixItOperations);
    }
}

using ClangEditorDocumentProcessors = QVector<ClangEditorDocumentProcessor *>;
static ClangEditorDocumentProcessors clangProcessors()
{
    ClangEditorDocumentProcessors result;
    foreach (auto *editorDocument, cppModelManager()->cppEditorDocuments())
        result.append(qobject_cast<ClangEditorDocumentProcessor *>(editorDocument->processor()));

    return result;
}

static ClangEditorDocumentProcessors
clangProcessorsWithProject(const ProjectExplorer::Project *project)
{
    return ::Utils::filtered(clangProcessors(), [project](ClangEditorDocumentProcessor *p) {
        return p->hasProjectPart() && p->projectPart()->project == project;
    });
}

static void updateProcessors(const ClangEditorDocumentProcessors &processors)
{
    CppTools::CppModelManager *modelManager = cppModelManager();
    for (ClangEditorDocumentProcessor *processor : processors)
        modelManager->cppEditorDocument(processor->filePath())->resetProcessor();
    modelManager->updateCppEditorDocuments(/*projectsUpdated=*/ false);
}

void ClangModelManagerSupport::onProjectAdded(ProjectExplorer::Project *project)
{
    QTC_ASSERT(!m_projectSettings.value(project), return);

    auto *settings = new Internal::ClangProjectSettings(project);
    connect(settings, &Internal::ClangProjectSettings::changed, [project]() {
        updateProcessors(clangProcessorsWithProject(project));
    });

    m_projectSettings.insert(project, settings);
}

void ClangModelManagerSupport::onAboutToRemoveProject(ProjectExplorer::Project *project)
{
    ClangProjectSettings * const settings = m_projectSettings.value(project);
    QTC_ASSERT(settings, return);
    m_projectSettings.remove(project);
    delete settings;
}

void ClangModelManagerSupport::onProjectPartsUpdated(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return);
    const CppTools::ProjectInfo projectInfo = cppModelManager()->projectInfo(project);
    QTC_ASSERT(projectInfo.isValid(), return);

    updateLanguageClient(project, projectInfo);

    QStringList projectPartIds;
    for (const CppTools::ProjectPart::Ptr &projectPart : projectInfo.projectParts())
        projectPartIds.append(projectPart->id());
    onProjectPartsRemoved(projectPartIds);
}

void ClangModelManagerSupport::onProjectPartsRemoved(const QStringList &projectPartIds)
{
    if (!projectPartIds.isEmpty())
        reinitializeBackendDocuments(projectPartIds);
}

void ClangModelManagerSupport::onClangdSettingsChanged()
{
    // TODO: Handle also project-less client
    for (ProjectExplorer::Project * const project : ProjectExplorer::SessionManager::projects()) {
        const CppTools::ClangdSettings settings(
                    CppTools::ClangdProjectSettings(project).settings());
        ClangdClient * const client = clientForProject(project);
        if (!client) {
            if (settings.useClangd())
                updateLanguageClient(project, cppModelManager()->projectInfo(project));
            continue;
        }
        if (!settings.useClangd()) {
            LanguageClientManager::shutdownClient(client);
            continue;
        }
        if (client->settingsData() != settings.data())
            updateLanguageClient(project, cppModelManager()->projectInfo(project));
    }
}

static ClangEditorDocumentProcessors clangProcessorsWithDiagnosticConfig(
    const QVector<::Utils::Id> &configIds)
{
    return ::Utils::filtered(clangProcessors(), [configIds](ClangEditorDocumentProcessor *p) {
        return configIds.contains(p->diagnosticConfigId());
    });
}

void ClangModelManagerSupport::onDiagnosticConfigsInvalidated(const QVector<::Utils::Id> &configIds)
{
    updateProcessors(clangProcessorsWithDiagnosticConfig(configIds));
}

static ClangEditorDocumentProcessors
clangProcessorsWithProjectParts(const QStringList &projectPartIds)
{
    return ::Utils::filtered(clangProcessors(), [projectPartIds](ClangEditorDocumentProcessor *p) {
        return p->hasProjectPart() && projectPartIds.contains(p->projectPart()->id());
    });
}

void ClangModelManagerSupport::reinitializeBackendDocuments(const QStringList &projectPartIds)
{
    const auto processors = clangProcessorsWithProjectParts(projectPartIds);
    foreach (ClangEditorDocumentProcessor *processor, processors) {
        processor->closeBackendDocument();
        processor->clearProjectPart();
        processor->run();
    }
}

ClangModelManagerSupport *ClangModelManagerSupport::instance()
{
    return m_instance;
}

BackendCommunicator &ClangModelManagerSupport::communicator()
{
    return m_communicator;
}

QString ClangModelManagerSupport::dummyUiHeaderOnDiskPath(const QString &filePath) const
{
    return m_uiHeaderOnDiskManager.mapPath(filePath);
}

ClangProjectSettings &ClangModelManagerSupport::projectSettings(
    ProjectExplorer::Project *project) const
{
    return *m_projectSettings.value(project);
}

QString ClangModelManagerSupport::dummyUiHeaderOnDiskDirPath() const
{
    return m_uiHeaderOnDiskManager.directoryPath();
}

QString ClangModelManagerSupportProvider::id() const
{
    return QLatin1String(Constants::CLANG_MODELMANAGERSUPPORT_ID);
}

QString ClangModelManagerSupportProvider::displayName() const
{
    //: Display name
    return QCoreApplication::translate("ClangCodeModel::Internal::ModelManagerSupport",
                                       "Clang");
}

CppTools::ModelManagerSupport::Ptr ClangModelManagerSupportProvider::createModelManagerSupport()
{
    return CppTools::ModelManagerSupport::Ptr(new ClangModelManagerSupport);
}
