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
#include "clangdclient.h"
#include "clangdquickfixfactory.h"
#include "clangeditordocumentprocessor.h"
#include "clangdlocatorfilters.h"
#include "clangprojectsettings.h"
#include "clangrefactoringengine.h"
#include "clangutils.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <cppeditor/abstractoverviewmodel.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppeditorwidget.h>
#include <cppeditor/cppfollowsymbolundercursor.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cppprojectfile.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/editordocumenthandle.h>
#include <cppeditor/symbolfinder.h>

#include <languageclient/languageclientmanager.h>

#include <texteditor/quickfix.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
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
#include <QtDebug>

using namespace CppEditor;
using namespace LanguageClient;

namespace ClangCodeModel {
namespace Internal {

static ClangModelManagerSupport *m_instance = nullptr;

static CppEditor::CppModelManager *cppModelManager()
{
    return CppEditor::CppModelManager::instance();
}

static ProjectExplorer::Project *fallbackProject()
{
    if (ProjectExplorer::Project * const p = ProjectExplorer::ProjectTree::currentProject())
        return p;
    return ProjectExplorer::SessionManager::startupProject();
}

static const QList<TextEditor::TextDocument *> allCppDocuments()
{
    const auto isCppDocument = Utils::equal(&Core::IDocument::id,
                                            Utils::Id(CppEditor::Constants::CPPEDITOR_ID));
    const QList<Core::IDocument *> documents
        = Utils::filtered(Core::DocumentModel::openedDocuments(), isCppDocument);
    return Utils::qobject_container_cast<TextEditor::TextDocument *>(documents);
}

ClangModelManagerSupport::ClangModelManagerSupport()
    : m_completionAssistProvider(m_communicator, CompletionType::Other)
    , m_functionHintAssistProvider(m_communicator, CompletionType::FunctionHint)
    , m_refactoringEngine(new RefactoringEngine)
{
    QTC_CHECK(!m_instance);
    m_instance = this;

    watchForExternalChanges();
    watchForInternalChanges();
    setupClangdConfigFile();
    cppModelManager()->setCurrentDocumentFilter(std::make_unique<ClangdCurrentDocumentFilter>());
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

    CppEditor::CppModelManager *modelManager = cppModelManager();
    connect(modelManager, &CppEditor::CppModelManager::abstractEditorSupportContentsUpdated,
            this, &ClangModelManagerSupport::onAbstractEditorSupportContentsUpdated);
    connect(modelManager, &CppEditor::CppModelManager::abstractEditorSupportRemoved,
            this, &ClangModelManagerSupport::onAbstractEditorSupportRemoved);
    connect(modelManager, &CppEditor::CppModelManager::projectPartsUpdated,
            this, &ClangModelManagerSupport::onProjectPartsUpdated);
    connect(modelManager, &CppEditor::CppModelManager::projectPartsRemoved,
            this, &ClangModelManagerSupport::onProjectPartsRemoved);
    connect(modelManager, &CppModelManager::fallbackProjectPartUpdated, this, [this] {
        if (ClangdClient * const fallbackClient = clientForProject(nullptr)) {
            LanguageClientManager::shutdownClient(fallbackClient);
            claimNonProjectSources(createClient(nullptr, {}));
        }
    });

    auto *sessionManager = ProjectExplorer::SessionManager::instance();
    connect(sessionManager, &ProjectExplorer::SessionManager::projectAdded,
            this, &ClangModelManagerSupport::onProjectAdded);
    connect(sessionManager, &ProjectExplorer::SessionManager::aboutToRemoveProject,
            this, &ClangModelManagerSupport::onAboutToRemoveProject);
    connect(sessionManager, &ProjectExplorer::SessionManager::projectRemoved,
            this, [this] { claimNonProjectSources(clientForProject(fallbackProject())); });

    CppEditor::ClangdSettings::setDefaultClangdPath(Core::ICore::clangdExecutable(CLANG_BINDIR));
    connect(&CppEditor::ClangdSettings::instance(), &CppEditor::ClangdSettings::changed,
            this, &ClangModelManagerSupport::onClangdSettingsChanged);
    CppEditor::CppCodeModelSettings *settings = CppEditor::codeModelSettings();
    connect(settings, &CppEditor::CppCodeModelSettings::clangDiagnosticConfigsInvalidated,
            this, &ClangModelManagerSupport::onDiagnosticConfigsInvalidated);

    if (CppEditor::ClangdSettings::instance().useClangd())
        createClient(nullptr, {});

    m_generatorSynchronizer.setCancelOnWait(true);
    new ClangdQuickFixFactory(); // memory managed by CppEditor::g_cppQuickFixFactories
}

ClangModelManagerSupport::~ClangModelManagerSupport()
{
    QTC_CHECK(m_projectSettings.isEmpty());
    m_generatorSynchronizer.waitForFinished();
    m_instance = nullptr;
}

CppEditor::CppCompletionAssistProvider *ClangModelManagerSupport::completionAssistProvider()
{
    return &m_completionAssistProvider;
}

CppEditor::CppCompletionAssistProvider *ClangModelManagerSupport::functionHintAssistProvider()
{
    return &m_functionHintAssistProvider;
}

void ClangModelManagerSupport::followSymbol(const CppEditor::CursorInEditor &data,
                  Utils::ProcessLinkCallback &&processLinkCallback, bool resolveTarget,
                  bool inNextSplit)
{
    if (ClangdClient * const client = clientForFile(data.filePath());
            client && client->isFullyIndexed()) {
        client->followSymbol(data.textDocument(), data.cursor(), data.editorWidget(),
                             std::move(processLinkCallback), resolveTarget, inNextSplit);
        return;
    }

    SymbolFinder finder;
    CppModelManager::builtinFollowSymbol().findLink(data, std::move(processLinkCallback),
            resolveTarget, CppModelManager::instance()->snapshot(),
            data.editorWidget()->semanticInfo().doc, &finder, inNextSplit);
}

void ClangModelManagerSupport::switchDeclDef(const CppEditor::CursorInEditor &data,
                   Utils::ProcessLinkCallback &&processLinkCallback)
{
    if (ClangdClient * const client = clientForFile(data.filePath());
            client && client->isFullyIndexed()) {
        client->switchDeclDef(data.textDocument(), data.cursor(), data.editorWidget(),
                              std::move(processLinkCallback));
        return;
    }

    SymbolFinder finder;
    CppModelManager::builtinFollowSymbol().switchDeclDef(data, std::move(processLinkCallback),
            CppModelManager::instance()->snapshot(), data.editorWidget()->semanticInfo().doc,
            &finder);
}

CppEditor::RefactoringEngineInterface &ClangModelManagerSupport::refactoringEngineInterface()
{
    return *m_refactoringEngine;
}

std::unique_ptr<CppEditor::AbstractOverviewModel> ClangModelManagerSupport::createOverviewModel()
{
    return {};
}

bool ClangModelManagerSupport::supportsOutline(const TextEditor::TextDocument *document) const
{
    return !clientForFile(document->filePath());
}

bool ClangModelManagerSupport::supportsLocalUses(const TextEditor::TextDocument *document) const
{
    return !clientForFile(document->filePath());
}

bool ClangModelManagerSupport::hasSpecialHoverHandler(
        const TextEditor::TextDocument *document) const
{
    return clientForFile(document->filePath());
}

CppEditor::BaseEditorDocumentProcessor *ClangModelManagerSupport::createEditorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument)
{
    const auto processor = new ClangEditorDocumentProcessor(m_communicator, baseTextDocument);
    const auto handleConfigChange = [this](const Utils::FilePath &fp,
            const BaseEditorDocumentParser::Configuration &config) {
        if (const auto client = clientForFile(fp))
            client->updateParserConfig(fp, config);
    };
    connect(processor, &ClangEditorDocumentProcessor::parserConfigChanged,
            this, handleConfigChange);
    return processor;
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
        if (const auto client = clientForFile(filePath))
            client->updateParserConfig(filePath, processor->parserConfig());
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

void ClangModelManagerSupport::updateLanguageClient(
        ProjectExplorer::Project *project, const CppEditor::ProjectInfo::ConstPtr &projectInfo)
{
    if (!CppEditor::ClangdProjectSettings(project).settings().useClangd)
        return;
    const auto getJsonDbDir = [project] {
        if (const ProjectExplorer::Target * const target = project->activeTarget()) {
            if (const ProjectExplorer::BuildConfiguration * const bc
                    = target->activeBuildConfiguration()) {
                return bc->buildDirectory() / ".qtc_clangd";
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
        if (!CppEditor::ClangdProjectSettings(project).settings().useClangd)
            return;
        const CppEditor::ProjectInfo::ConstPtr newProjectInfo
                = cppModelManager()->projectInfo(project);
        if (!newProjectInfo || *newProjectInfo != *projectInfo)
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
            if (!CppEditor::ClangdProjectSettings(project).settings().useClangd)
                return;
            const CppEditor::ProjectInfo::ConstPtr newProjectInfo
                = cppModelManager()->projectInfo(project);
            if (!newProjectInfo || *newProjectInfo != *projectInfo)
                return;

            const auto updateParserConfig = [client] {
                if (const auto editor = TextEditor::BaseTextEditor::currentTextEditor()) {
                    if (!client->documentOpen(editor->textDocument()))
                        return;
                    const Utils::FilePath filePath = editor->textDocument()->filePath();
                    if (const auto processor = ClangEditorDocumentProcessor::get(
                                filePath.toString())) {
                        const CppEditor::BaseEditorDocumentParser::Configuration config
                                = processor->parserConfig();
                        client->updateParserConfig(filePath, config);
                    }
                }
            };

            // Acquaint the client with all open C++ documents for this project.
            bool hasDocuments = false;
            const ClangdSettings settings(ClangdProjectSettings(project).settings());
            for (TextEditor::TextDocument * const doc : allCppDocuments()) {
                const Client * const currentClient = LanguageClientManager::clientForDocument(doc);
                if (!settings.sizeIsOkay(doc->filePath()))
                    continue;
                if (!currentClient || !currentClient->project()
                        || currentClient->state() != Client::Initialized
                        || project->isKnownFile(doc->filePath())) {
                    LanguageClientManager::openDocumentWithClient(doc, client);
                    ClangEditorDocumentProcessor::clearTextMarks(doc->filePath());
                    hasDocuments = true;
                }
            }

            if (client->state() == Client::Initialized)
                updateParserConfig();
            else
                connect(client, &Client::initialized, client, updateParserConfig);
            connect(CppModelManager::instance(), &CppModelManager::projectPartsUpdated,
                    client, updateParserConfig);

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
    auto future = Utils::runAsync(&Internal::generateCompilationDB, projectInfo, jsonDbDir,
                                  CompilationDbPurpose::CodeModel,
                                  warningsConfigForProject(project),
                                  optionsForProject(project));
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
    QTC_ASSERT(clients.size() <= 1, qDebug() << project << clients.size());
    if (clients.size() > 1) {
        Client *activeClient = nullptr;
        for (Client * const c : clients) {
            if (!activeClient && (c->state() == Client::Initialized
                                  || c->state() == Client::InitializeRequested)) {
                activeClient = c;
            } else {
                LanguageClientManager::shutdownClient(c);
            }
        }
        return qobject_cast<ClangdClient *>(activeClient);
    }
    return clients.empty() ? nullptr : qobject_cast<ClangdClient *>(clients.first());
}

ClangdClient *ClangModelManagerSupport::clientForFile(const Utils::FilePath &file) const
{
    return qobject_cast<ClangdClient *>(LanguageClientManager::clientForFilePath(file));
}

ClangdClient *ClangModelManagerSupport::createClient(ProjectExplorer::Project *project,
                                                     const Utils::FilePath &jsonDbDir)
{
    const auto client = new ClangdClient(project, jsonDbDir);
    emit createdClient(client);
    return client;
}

void ClangModelManagerSupport::claimNonProjectSources(ClangdClient *client)
{
    if (!client)
        return;
    for (TextEditor::TextDocument * const doc : allCppDocuments()) {
        if (Client * const currentClient = LanguageClientManager::clientForDocument(doc);
                currentClient && currentClient->state() == Client::Initialized
                && (currentClient == client || currentClient->project())) {
            continue;
        }
        ClangEditorDocumentProcessor::clearTextMarks(doc->filePath());
        if (!ClangdSettings::instance().sizeIsOkay(doc->filePath()))
            continue;
        client->openDocument(doc);
    }
}

// If any open C/C++ source file is changed from outside Qt Creator, we restart the client
// for the respective project to force re-parsing of open documents and re-indexing.
// While this is not 100% bullet-proof, chances are good that in a typical session-based
// workflow, e.g. a git branch switch will hit at least one open file.
void ClangModelManagerSupport::watchForExternalChanges()
{
    const auto projectIsParsing = [](const ProjectExplorer::Project *project) {
        const ProjectExplorer::BuildSystem * const bs = project && project->activeTarget()
                ? project->activeTarget()->buildSystem() : nullptr;
        return bs && (bs->isParsing() || bs->isWaitingForParse());
    };

    const auto timer = new QTimer(this);
    timer->setInterval(3000);
    connect(timer, &QTimer::timeout, this, [this, projectIsParsing] {
        const auto clients = m_clientsToRestart;
        m_clientsToRestart.clear();
        for (ClangdClient * const client : clients) {
            if (client && client->state() != Client::Shutdown
                    && client->state() != Client::ShutdownRequested
                    && !projectIsParsing(client->project())) {
                ProjectExplorer::Project * const project = client->project();
                updateLanguageClient(project, CppModelManager::instance()->projectInfo(project));
            }
        }
    });

    connect(Core::DocumentManager::instance(), &Core::DocumentManager::filesChangedExternally,
            this, [this, timer, projectIsParsing](const QSet<Utils::FilePath> &files) {
        if (!LanguageClientManager::hasClients<ClangdClient>())
            return;
        for (const Utils::FilePath &file : files) {
            const ProjectFile::Kind kind = ProjectFile::classify(file.toString());
            if (!ProjectFile::isSource(kind) && !ProjectFile::isHeader(kind))
                continue;
            const ProjectExplorer::Project * const project
                    = ProjectExplorer::SessionManager::projectForFile(file);
            if (!project)
                continue;

            // If a project file was changed, it is very likely that we will have to generate
            // a new compilation database, in which case the client will be restarted via
            // a different code path.
            if (projectIsParsing(project))
                return;

            ClangdClient * const client = clientForProject(project);
            if (client && !m_clientsToRestart.contains(client)) {
                m_clientsToRestart.append(client);
                timer->start();
            }

            // It's unlikely that the same signal carries files from different projects,
            // so we exit the loop as soon as we have dealt with one project, as the
            // project look-up is not free.
            return;
        }
    });
}

void ClangModelManagerSupport::watchForInternalChanges()
{
    connect(Core::DocumentManager::instance(), &Core::DocumentManager::filesChangedInternally,
            this, [this](const Utils::FilePaths &filePaths) {
        for (const Utils::FilePath &fp : filePaths) {
            ClangdClient * const client = clientForFile(fp);
            if (!client || client->documentForFilePath(fp))
                continue;
            client->openExtraFile(fp);

            // We need to give clangd some time to start re-parsing the file.
            // Closing right away does not work, and neither does doing it queued.
            // If it turns out that this delay is not always enough, we'll need to come up
            // with something more clever.
            // Ideally, clangd would implement workspace/didChangeWatchedFiles; let's keep
            // any eye on that.
            QTimer::singleShot(5000, client, [client, fp] {
                if (!client->documentForFilePath(fp))
                    client->closeExtraFile(fp); });
        }
    });
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

        ProjectExplorer::Project * project
                = ProjectExplorer::SessionManager::projectForFile(document->filePath());
        const ClangdSettings settings(ClangdProjectSettings(project).settings());
        if (!settings.sizeIsOkay(textDocument->filePath()))
            return;
        if (!project)
            project = fallbackProject();
        if (ClangdClient * const client = clientForProject(project))
            LanguageClientManager::openDocumentWithClient(textDocument, client);
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
    ClangdClient::handleUiHeaderChange(Utils::FilePath::fromString(filePath).fileName());
}

void ClangModelManagerSupport::onAbstractEditorSupportRemoved(const QString &filePath)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    if (!cppModelManager()->cppEditorDocument(filePath)) {
        const QString mappedPath = m_uiHeaderOnDiskManager.remove(filePath);
        const QString projectPartId = projectPartIdForFile(filePath);
        m_communicator.unsavedFilesRemoved({{mappedPath, projectPartId}});
        ClangdClient::handleUiHeaderChange(Utils::FilePath::fromString(filePath).fileName());
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
        return p->hasProjectPart() && p->projectPart()->belongsToProject(project);
    });
}

static void updateProcessors(const ClangEditorDocumentProcessors &processors)
{
    CppEditor::CppModelManager *modelManager = cppModelManager();
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
    const CppEditor::ProjectInfo::ConstPtr projectInfo = cppModelManager()->projectInfo(project);
    QTC_ASSERT(projectInfo, return);

    updateLanguageClient(project, projectInfo);

    QStringList projectPartIds;
    for (const CppEditor::ProjectPart::ConstPtr &projectPart : projectInfo->projectParts())
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
    for (ProjectExplorer::Project * const project : ProjectExplorer::SessionManager::projects()) {
        const CppEditor::ClangdSettings settings(
                    CppEditor::ClangdProjectSettings(project).settings());
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

    ClangdClient * const fallbackClient = clientForProject(nullptr);
    const ClangdSettings &settings = ClangdSettings::instance();
    const auto startNewFallbackClient = [this] {
        claimNonProjectSources(createClient(nullptr, {}));
    };
    if (!fallbackClient) {
        if (settings.useClangd())
            startNewFallbackClient();
        return;
    }
    if (!settings.useClangd()) {
        LanguageClientManager::shutdownClient(fallbackClient);
        return;
    }
    if (fallbackClient->settingsData() != settings.data()) {
        LanguageClientManager::shutdownClient(fallbackClient);
        startNewFallbackClient();
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

CppEditor::ModelManagerSupport::Ptr ClangModelManagerSupportProvider::createModelManagerSupport()
{
    return CppEditor::ModelManagerSupport::Ptr(new ClangModelManagerSupport);
}

} // Internal
} // ClangCodeModel
