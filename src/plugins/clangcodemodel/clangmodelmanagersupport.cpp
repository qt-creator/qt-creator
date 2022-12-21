// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangmodelmanagersupport.h"

#include "clangconstants.h"
#include "clangdclient.h"
#include "clangdquickfixes.h"
#include "clangeditordocumentprocessor.h"
#include "clangdlocatorfilters.h"
#include "clangutils.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppeditorwidget.h>
#include <cppeditor/cppfollowsymbolundercursor.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cppprojectfile.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/editordocumenthandle.h>

#include <languageclient/languageclientmanager.h>

#include <texteditor/quickfix.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/infobar.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QApplication>
#include <QLabel>
#include <QMenu>
#include <QTextBlock>
#include <QTimer>
#include <QtDebug>

using namespace CppEditor;
using namespace LanguageClient;
using namespace Utils;

namespace ClangCodeModel {
namespace Internal {

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

static bool sessionModeEnabled()
{
    return ClangdSettings::instance().granularity() == ClangdSettings::Granularity::Session;
}

static const QList<TextEditor::TextDocument *> allCppDocuments()
{
    const auto isCppDocument = Utils::equal(&Core::IDocument::id,
                                            Utils::Id(CppEditor::Constants::CPPEDITOR_ID));
    const QList<Core::IDocument *> documents
        = Utils::filtered(Core::DocumentModel::openedDocuments(), isCppDocument);
    return Utils::qobject_container_cast<TextEditor::TextDocument *>(documents);
}

static const QList<ProjectExplorer::Project *> projectsForClient(const Client *client)
{
    QList<ProjectExplorer::Project *> projects;
    if (sessionModeEnabled()) {
        for (ProjectExplorer::Project * const p : ProjectExplorer::SessionManager::projects()) {
            if (ClangdProjectSettings(p).settings().useClangd)
                projects << p;
        }
    } else if (client->project()) {
        projects << client->project();
    }
    return projects;
}

static bool fileIsProjectBuildArtifact(const Client *client, const Utils::FilePath &filePath)
{
    for (const ProjectExplorer::Project * const p : projectsForClient(client)) {
        if (const auto t = p->activeTarget()) {
            if (const auto bc = t->activeBuildConfiguration()) {
                if (filePath.isChildOf(bc->buildDirectory()))
                    return true;
            }
        }
    }
    return false;
}

static Client *clientForGeneratedFile(const Utils::FilePath &filePath)
{
    for (Client * const client : LanguageClientManager::clients()) {
        if (qobject_cast<ClangdClient *>(client) && client->reachable()
                && fileIsProjectBuildArtifact(client, filePath)) {
            return client;
        }
    }
    return nullptr;
}

static void checkSystemForClangdSuitability()
{
    if (ClangdSettings::haveCheckedHardwareRequirements())
        return;
    if (ClangdSettings::hardwareFulfillsRequirements())
        return;

    ClangdSettings::setUseClangdAndSave(false);
    const QString warnStr = ClangModelManagerSupport::tr("The use of clangd for the C/C++ "
            "code model was disabled, because it is likely that its memory requirements "
            "would be higher than what your system can handle.");
    const Utils::Id clangdWarningSetting("WarnAboutClangd");
    Utils::InfoBarEntry info(clangdWarningSetting, warnStr);
    info.setDetailsWidgetCreator([] {
        const auto label = new QLabel(ClangModelManagerSupport::tr(
            "With clangd enabled, Qt Creator fully supports modern C++ "
            "when highlighting code, completing symbols and so on.<br>"
            "This comes at a higher cost in terms of CPU load and memory usage compared "
            "to the built-in code model, which therefore might be the better choice "
            "on older machines and/or with legacy code.<br>"
            "You can enable/disable and fine-tune clangd <a href=\"dummy\">here</a>."));
        label->setWordWrap(true);
        QObject::connect(label, &QLabel::linkActivated, [] {
            Core::ICore::showOptionsDialog(CppEditor::Constants::CPP_CLANGD_SETTINGS_ID);
        });
        return label;
    });
    info.addCustomButton(ClangModelManagerSupport::tr("Enable Anyway"), [clangdWarningSetting] {
        ClangdSettings::setUseClangdAndSave(true);
        Core::ICore::infoBar()->removeInfo(clangdWarningSetting);
    });
    Core::ICore::infoBar()->addInfo(info);
}

static void updateParserConfig(ClangdClient *client)
{
    if (!client->reachable())
        return;
    if (const auto editor = TextEditor::BaseTextEditor::currentTextEditor()) {
        if (!client->documentOpen(editor->textDocument()))
            return;
        const Utils::FilePath filePath = editor->textDocument()->filePath();
        if (const auto processor = ClangEditorDocumentProcessor::get(filePath))
            client->updateParserConfig(filePath, processor->parserConfig());
    }
}

static bool projectIsParsing(const ClangdClient *client)
{
    for (const ProjectExplorer::Project * const p : projectsForClient(client)) {
        const ProjectExplorer::BuildSystem * const bs = p && p->activeTarget()
                ? p->activeTarget()->buildSystem() : nullptr;
        if (bs && (bs->isParsing() || bs->isWaitingForParse()))
            return true;
    }
    return false;
}


ClangModelManagerSupport::ClangModelManagerSupport()
    : m_clientRestartTimer(new QTimer(this))
{
    m_clientRestartTimer->setInterval(3000);
    connect(m_clientRestartTimer, &QTimer::timeout, this, [this] {
        const auto clients = m_clientsToRestart;
        m_clientsToRestart.clear();
        for (ClangdClient * const client : clients) {
            if (client && client->state() != Client::Shutdown
                    && client->state() != Client::ShutdownRequested
                    && !projectIsParsing(client)) {
                updateLanguageClient(client->project());
            }
        }
    });
    watchForExternalChanges();
    watchForInternalChanges();
    setupClangdConfigFile();
    checkSystemForClangdSuitability();
    cppModelManager()->setCurrentDocumentFilter(std::make_unique<ClangdCurrentDocumentFilter>());
    cppModelManager()->setLocatorFilter(std::make_unique<ClangGlobalSymbolFilter>());
    cppModelManager()->setClassesFilter(std::make_unique<ClangClassesFilter>());
    cppModelManager()->setFunctionsFilter(std::make_unique<ClangFunctionsFilter>());

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::editorOpened,
            this, &ClangModelManagerSupport::onEditorOpened);
    connect(editorManager, &Core::EditorManager::currentEditorChanged,
            this, &ClangModelManagerSupport::onCurrentEditorChanged);

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
        if (sessionModeEnabled())
            return;
        if (ClangdClient * const fallbackClient = clientForProject(nullptr)) {
            LanguageClientManager::shutdownClient(fallbackClient);
            claimNonProjectSources(new ClangdClient(nullptr, {}));
        }
    });

    auto *sessionManager = ProjectExplorer::SessionManager::instance();
    connect(sessionManager, &ProjectExplorer::SessionManager::projectRemoved,
            this, [this] {
        if (!sessionModeEnabled())
            claimNonProjectSources(clientForProject(fallbackProject()));
    });
    connect(sessionManager, &ProjectExplorer::SessionManager::sessionLoaded,
            this, [this] {
        if (sessionModeEnabled())
            onClangdSettingsChanged();
    });

    CppEditor::ClangdSettings::setDefaultClangdPath(Core::ICore::clangdExecutable(CLANG_BINDIR));
    connect(&CppEditor::ClangdSettings::instance(), &CppEditor::ClangdSettings::changed,
            this, &ClangModelManagerSupport::onClangdSettingsChanged);

    if (CppEditor::ClangdSettings::instance().useClangd())
        new ClangdClient(nullptr, {});

    m_generatorSynchronizer.setCancelOnWait(true);
    new ClangdQuickFixFactory(); // memory managed by CppEditor::g_cppQuickFixFactories
}

ClangModelManagerSupport::~ClangModelManagerSupport()
{
    m_generatorSynchronizer.waitForFinished();
}

void ClangModelManagerSupport::followSymbol(const CppEditor::CursorInEditor &data,
                  const Utils::LinkHandler &processLinkCallback, bool resolveTarget,
                  bool inNextSplit)
{
    if (ClangdClient * const client = clientForFile(data.filePath());
            client && client->isFullyIndexed()) {
        client->followSymbol(data.textDocument(), data.cursor(), data.editorWidget(),
                             processLinkCallback, resolveTarget, FollowTo::SymbolDef, inNextSplit);
        return;
    }

    CppModelManager::followSymbol(data, processLinkCallback, resolveTarget, inNextSplit,
                                  CppModelManager::Backend::Builtin);
}

void ClangModelManagerSupport::followSymbolToType(const CppEditor::CursorInEditor &data,
                                                  const Utils::LinkHandler &processLinkCallback,
                                                  bool inNextSplit)
{
    if (ClangdClient * const client = clientForFile(data.filePath())) {
        client->followSymbol(data.textDocument(), data.cursor(), data.editorWidget(),
                             processLinkCallback, false, FollowTo::SymbolType, inNextSplit);
        return;
    }
    CppModelManager::followSymbolToType(data, processLinkCallback, inNextSplit,
                                        CppModelManager::Backend::Builtin);
}

void ClangModelManagerSupport::switchDeclDef(const CppEditor::CursorInEditor &data,
                   const Utils::LinkHandler &processLinkCallback)
{
    if (ClangdClient * const client = clientForFile(data.filePath());
            client && client->isFullyIndexed()) {
        client->switchDeclDef(data.textDocument(), data.cursor(), data.editorWidget(),
                              processLinkCallback);
        return;
    }

    CppModelManager::switchDeclDef(data, processLinkCallback, CppModelManager::Backend::Builtin);
}

void ClangModelManagerSupport::startLocalRenaming(const CppEditor::CursorInEditor &data,
                                           const CppEditor::ProjectPart *projectPart,
                                           RenameCallback &&renameSymbolsCallback)
{
    if (ClangdClient * const client = clientForFile(data.filePath());
            client && client->reachable()) {
        client->findLocalUsages(data.textDocument(), data.cursor(),
                                std::move(renameSymbolsCallback));
        return;
    }

    CppModelManager::startLocalRenaming(data, projectPart,
            std::move(renameSymbolsCallback), CppModelManager::Backend::Builtin);
}

void ClangModelManagerSupport::globalRename(const CppEditor::CursorInEditor &cursor,
                                            const QString &replacement)
{
    if (ClangdClient * const client = clientForFile(cursor.filePath());
            client && client->isFullyIndexed()) {
        QTC_ASSERT(client->documentOpen(cursor.textDocument()),
                   client->openDocument(cursor.textDocument()));
        client->findUsages(cursor.textDocument(), cursor.cursor(), replacement);
        return;
    }
    CppModelManager::globalRename(cursor, replacement, CppModelManager::Backend::Builtin);
}

void ClangModelManagerSupport::findUsages(const CppEditor::CursorInEditor &cursor) const
{
    if (ClangdClient * const client = clientForFile(cursor.filePath());
            client && client->isFullyIndexed()) {
        QTC_ASSERT(client->documentOpen(cursor.textDocument()),
                   client->openDocument(cursor.textDocument()));
        client->findUsages(cursor.textDocument(), cursor.cursor(), {});

        return;
    }
    CppModelManager::findUsages(cursor, CppModelManager::Backend::Builtin);
}

void ClangModelManagerSupport::switchHeaderSource(const FilePath &filePath, bool inNextSplit)
{
    if (ClangdClient * const client = clientForFile(filePath)) {
        // The fast, synchronous approach works most of the time, so let's try that one first.
        const FilePath otherFile = correspondingHeaderOrSource(filePath);
        if (!otherFile.isEmpty())
            openEditor(otherFile, inNextSplit);
        else
            client->switchHeaderSource(filePath, inNextSplit);
        return;
    }
    CppModelManager::switchHeaderSource(inNextSplit, CppModelManager::Backend::Builtin);
}

void ClangModelManagerSupport::checkUnused(const Utils::Link &link, Core::SearchResult *search,
                                           const Utils::LinkHandler &callback)
{
    if (const ProjectExplorer::Project * const project
            = ProjectExplorer::SessionManager::projectForFile(link.targetFilePath)) {
        if (ClangdClient * const client = clientWithProject(project);
                client && client->isFullyIndexed()) {
            client->checkUnused(link, search, callback);
            return;
        }
    }

    CppModelManager::instance()->modelManagerSupport(
                CppModelManager::Backend::Builtin)->checkUnused(link, search, callback);
}

bool ClangModelManagerSupport::usesClangd(const TextEditor::TextDocument *document) const
{
    return clientForFile(document->filePath());
}

CppEditor::BaseEditorDocumentProcessor *ClangModelManagerSupport::createEditorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument)
{
    const auto processor = new ClangEditorDocumentProcessor(baseTextDocument);
    const auto handleConfigChange = [](const Utils::FilePath &fp,
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
    // Update task hub issues for current CppEditorDocument
    ProjectExplorer::TaskHub::clearTasks(Constants::TASK_CATEGORY_DIAGNOSTICS);
    if (!editor || !editor->document() || !cppModelManager()->isCppEditor(editor))
        return;

    const ::Utils::FilePath filePath = editor->document()->filePath();
    if (auto processor = ClangEditorDocumentProcessor::get(filePath)) {
        processor->semanticRehighlight();
        if (const auto client = clientForFile(filePath)) {
            client->updateParserConfig(filePath, processor->parserConfig());
            client->switchIssuePaneEntries(filePath);
        }
    }
}

void ClangModelManagerSupport::connectToWidgetsMarkContextMenuRequested(QWidget *editorWidget)
{
    const auto widget = qobject_cast<TextEditor::TextEditorWidget *>(editorWidget);
    if (widget) {
        connect(widget, &TextEditor::TextEditorWidget::markContextMenuRequested,
                this, &ClangModelManagerSupport::onTextMarkContextMenuRequested);
    }
}

static Utils::FilePath getJsonDbDir(const ProjectExplorer::Project *project)
{
    static const QString dirName(".qtc_clangd");
    if (!project) {
        const QString sessionDirName = Utils::FileUtils::fileSystemFriendlyName(
                    ProjectExplorer::SessionManager::activeSession());
        return Core::ICore::userResourcePath() / dirName / sessionDirName; // TODO: Make configurable?
    }
    if (const ProjectExplorer::Target * const target = project->activeTarget()) {
        if (const ProjectExplorer::BuildConfiguration * const bc
                = target->activeBuildConfiguration()) {
            return bc->buildDirectory() / dirName;
        }
    }
    return Utils::FilePath();
}

static bool isProjectDataUpToDate(
        ProjectExplorer::Project *project, ProjectInfoList projectInfo,
        const Utils::FilePath &jsonDbDir)
{
    if (project && !ProjectExplorer::SessionManager::hasProject(project))
        return false;
    const ClangdSettings settings(ClangdProjectSettings(project).settings());
    if (!settings.useClangd())
        return false;
    if (!sessionModeEnabled() && !project)
        return false;
    if (sessionModeEnabled() && project)
        return false;
    ProjectInfoList newProjectInfo;
    if (project) {
        if (const ProjectInfo::ConstPtr pi = CppModelManager::instance()->projectInfo(project))
            newProjectInfo.append(pi);
        else
            return false;
    } else {
        newProjectInfo = CppModelManager::instance()->projectInfos();
    }
    if (newProjectInfo.size() != projectInfo.size())
        return false;
    for (int i = 0; i < projectInfo.size(); ++i) {
        if (*projectInfo[i] != *newProjectInfo[i])
            return false;
    }
    if (getJsonDbDir(project) != jsonDbDir)
        return false;
    return true;
}

void ClangModelManagerSupport::updateLanguageClient(ProjectExplorer::Project *project)
{
    const ClangdSettings settings(ClangdProjectSettings(project).settings());
    if (!settings.useClangd())
        return;
    ProjectInfoList projectInfo;
    if (sessionModeEnabled()) {
        project = nullptr;
        projectInfo = CppModelManager::instance()->projectInfos();
    } else if (const ProjectInfo::ConstPtr pi = CppModelManager::instance()->projectInfo(project)) {
        projectInfo.append(pi);
    } else {
        return;
    }

    const Utils::FilePath jsonDbDir = getJsonDbDir(project);
    if (jsonDbDir.isEmpty())
        return;
    const auto generatorWatcher = new QFutureWatcher<GenerateCompilationDbResult>;
    connect(generatorWatcher, &QFutureWatcher<GenerateCompilationDbResult>::finished,
            this, [this, project, projectInfo, jsonDbDir, generatorWatcher] {
        generatorWatcher->deleteLater();
        if (!isProjectDataUpToDate(project, projectInfo, jsonDbDir))
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
        ClangdClient * const client = new ClangdClient(project, jsonDbDir);
        connect(client, &Client::shadowDocumentSwitched, this, [](const Utils::FilePath &fp) {
            ClangdClient::handleUiHeaderChange(fp.fileName());
        });
        connect(CppModelManager::instance(),
                &CppModelManager::projectPartsUpdated,
                client,
                [client] { updateParserConfig(client); });
        connect(client, &Client::initialized, this, [this, client, project, projectInfo, jsonDbDir] {
            if (!isProjectDataUpToDate(project, projectInfo, jsonDbDir))
                return;
            using namespace ProjectExplorer;

            // Acquaint the client with all open C++ documents for this project or session.
            const ClangdSettings settings(ClangdProjectSettings(project).settings());
            bool hasDocuments = false;
            for (TextEditor::TextDocument * const doc : allCppDocuments()) {
                Client * const currentClient = LanguageClientManager::clientForDocument(doc);
                if (currentClient == client) {
                    hasDocuments = true;
                    continue;
                }
                if (!settings.sizeIsOkay(doc->filePath()))
                    continue;
                if (!project) {
                    if (currentClient)
                        currentClient->closeDocument(doc);
                    LanguageClientManager::openDocumentWithClient(doc, client);
                    hasDocuments = true;
                    continue;
                }
                const Project * const docProject = SessionManager::projectForFile(doc->filePath());
                if (currentClient && currentClient->project()
                        && currentClient->project() != project
                        && currentClient->project() == docProject) {
                    continue;
                }
                if (docProject != project
                        && (docProject || !ProjectFile::isHeader(doc->filePath()))) {
                    continue;
                }
                if (currentClient)
                    currentClient->closeDocument(doc);
                LanguageClientManager::openDocumentWithClient(doc, client);
                hasDocuments = true;
            }

            for (auto it = m_queuedShadowDocuments.begin(); it != m_queuedShadowDocuments.end();) {
                if (fileIsProjectBuildArtifact(client, it.key())) {
                    if (it.value().isEmpty())
                        client->removeShadowDocument(it.key());
                    else
                        client->setShadowDocument(it.key(), it.value());
                    ClangdClient::handleUiHeaderChange(it.key().fileName());
                    it = m_queuedShadowDocuments.erase(it);
                } else {
                    ++it;
                }
            }

            updateParserConfig(client);

            if (hasDocuments)
                return;

            // clangd oddity: Background indexing only starts after opening a random file.
            // TODO: changes to the compilation db do not seem to trigger re-indexing.
            //       How to force it?
            ProjectNode *rootNode = nullptr;
            if (project)
                rootNode = project->rootProjectNode();
            else if (SessionManager::startupProject())
                rootNode = SessionManager::startupProject()->rootProjectNode();
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
    const Utils::FilePath includeDir = settings.clangdIncludePath();
    auto future = Utils::runAsync(&Internal::generateCompilationDB, projectInfo,
                                  jsonDbDir, CompilationDbPurpose::CodeModel,
                                  warningsConfigForProject(project),
                                  globalClangOptions(), includeDir);
    generatorWatcher->setFuture(future);
    m_generatorSynchronizer.addFuture(future);
}

ClangdClient *ClangModelManagerSupport::clientForProject(const ProjectExplorer::Project *project)
{
    if (sessionModeEnabled())
        project = nullptr;
    return clientWithProject(project);
}

ClangdClient *ClangModelManagerSupport::clientWithProject(const ProjectExplorer::Project *project)
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

ClangdClient *ClangModelManagerSupport::clientForFile(const Utils::FilePath &file)
{
    return qobject_cast<ClangdClient *>(LanguageClientManager::clientForFilePath(file));
}

void ClangModelManagerSupport::claimNonProjectSources(ClangdClient *client)
{
    if (!client)
        return;
    for (TextEditor::TextDocument * const doc : allCppDocuments()) {
        Client * const currentClient = LanguageClientManager::clientForDocument(doc);
        if (currentClient && currentClient->state() == Client::Initialized
                && (currentClient == client || currentClient->project())) {
            continue;
        }
        if (!ClangdSettings::instance().sizeIsOkay(doc->filePath()))
            continue;
        if (ProjectExplorer::SessionManager::projectForFile(doc->filePath()))
            continue;
        if (client->project() && !ProjectFile::isHeader(doc->filePath()))
            continue;
        if (currentClient)
            currentClient->closeDocument(doc);
        LanguageClientManager::openDocumentWithClient(doc, client);
    }
}

// If any open C/C++ source file is changed from outside Qt Creator, we restart the client
// for the respective project to force re-parsing of open documents and re-indexing.
// While this is not 100% bullet-proof, chances are good that in a typical session-based
// workflow, e.g. a git branch switch will hit at least one open file.
void ClangModelManagerSupport::watchForExternalChanges()
{
    connect(Core::DocumentManager::instance(), &Core::DocumentManager::filesChangedExternally,
            this, [this](const QSet<Utils::FilePath> &files) {
        if (!LanguageClientManager::hasClients<ClangdClient>())
            return;
        for (const Utils::FilePath &file : files) {
            const ProjectFile::Kind kind = ProjectFile::classify(file.toString());
            if (!ProjectFile::isSource(kind) && !ProjectFile::isHeader(kind))
                continue;
            ProjectExplorer::Project * const project
                    = ProjectExplorer::SessionManager::projectForFile(file);
            if (!project)
                continue;

            if (ClangdClient * const client = clientForProject(project))
                scheduleClientRestart(client);

            // It's unlikely that the same signal carries files from different projects,
            // so we exit the loop as soon as we have dealt with one project, as the
            // project look-up is not free.
            return;
        }
    });
}

// If Qt Creator changes a file that is not open (e.g. as part of a quickfix), we have to
// restart clangd for reliable re-parsing and re-indexing.
void ClangModelManagerSupport::watchForInternalChanges()
{
    connect(Core::DocumentManager::instance(), &Core::DocumentManager::filesChangedInternally,
            this, [this](const Utils::FilePaths &filePaths) {
        for (const Utils::FilePath &fp : filePaths) {
            const ProjectFile::Kind kind = ProjectFile::classify(fp.toString());
            if (!ProjectFile::isSource(kind) && !ProjectFile::isHeader(kind))
                continue;
            ProjectExplorer::Project * const project
                    = ProjectExplorer::SessionManager::projectForFile(fp);
            if (!project)
                continue;
            if (ClangdClient * const client = clientForProject(project);
                    client && !client->documentForFilePath(fp)) {
               scheduleClientRestart(client);
            }
        }
    });
}

void ClangModelManagerSupport::scheduleClientRestart(ClangdClient *client)
{
    if (m_clientsToRestart.contains(client))
        return;

    // If a project file was changed, it is very likely that we will have to generate
    // a new compilation database, in which case the client will be restarted via
    // a different code path.
    if (projectIsParsing(client))
        return;

    m_clientsToRestart.append(client);
    m_clientRestartTimer->start();
}

void ClangModelManagerSupport::onEditorOpened(Core::IEditor *editor)
{
    QTC_ASSERT(editor, return);
    Core::IDocument *document = editor->document();
    QTC_ASSERT(document, return);
    auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);

    if (textDocument && cppModelManager()->isCppEditor(editor)) {
        connectToWidgetsMarkContextMenuRequested(editor->widget());

        ProjectExplorer::Project * project
                = ProjectExplorer::SessionManager::projectForFile(document->filePath());
        const ClangdSettings settings(ClangdProjectSettings(project).settings());
        if (!settings.sizeIsOkay(textDocument->filePath()))
            return;
        if (sessionModeEnabled())
            project = nullptr;
        else if (!project && ProjectFile::isHeader(document->filePath()))
            project = fallbackProject();
        if (ClangdClient * const client = clientForProject(project))
            LanguageClientManager::openDocumentWithClient(textDocument, client);
    }
}

void ClangModelManagerSupport::onAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                                      const QString &,
                                                                      const QByteArray &content)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    if (content.size() == 0)
        return; // Generation not yet finished.
    const auto fp = Utils::FilePath::fromString(filePath);
    const QString stringContent = QString::fromUtf8(content);
    if (Client * const client = clientForGeneratedFile(fp)) {
        client->setShadowDocument(fp, stringContent);
        ClangdClient::handleUiHeaderChange(fp.fileName());
        QTC_CHECK(m_queuedShadowDocuments.remove(fp) == 0);
    } else  {
        m_queuedShadowDocuments.insert(fp, stringContent);
    }
}

void ClangModelManagerSupport::onAbstractEditorSupportRemoved(const QString &filePath)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    const auto fp = Utils::FilePath::fromString(filePath);
    if (Client * const client = clientForGeneratedFile(fp)) {
        client->removeShadowDocument(fp);
        ClangdClient::handleUiHeaderChange(fp.fileName());
        QTC_CHECK(m_queuedShadowDocuments.remove(fp) == 0);
    } else {
        m_queuedShadowDocuments.insert(fp, {});
    }
}

void addFixItsActionsToMenu(QMenu *menu, const TextEditor::QuickFixOperations &fixItOperations)
{
    for (const TextEditor::QuickFixOperation::Ptr &fixItOperation : fixItOperations) {
        QAction *action = menu->addAction(fixItOperation->description());
        QObject::connect(action, &QAction::triggered, [fixItOperation] {
            fixItOperation->perform();
        });
    }
}

static TextEditor::AssistInterface createAssistInterface(TextEditor::TextEditorWidget *widget,
                                                         int lineNumber)
{
    QTextCursor cursor(widget->document()->findBlockByLineNumber(lineNumber));
    if (!cursor.atStart())
        cursor.movePosition(QTextCursor::PreviousCharacter);
    return TextEditor::AssistInterface(cursor,
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

    const Utils::FilePath filePath = widget->textDocument()->filePath();
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
    for (const CppEditorDocumentHandle *editorDocument : cppModelManager()->cppEditorDocuments())
        result.append(qobject_cast<ClangEditorDocumentProcessor *>(editorDocument->processor()));

    return result;
}

void ClangModelManagerSupport::onProjectPartsUpdated(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return);

    updateLanguageClient(project);

    QStringList projectPartIds;
    const CppEditor::ProjectInfo::ConstPtr projectInfo = cppModelManager()->projectInfo(project);
    QTC_ASSERT(projectInfo, return);

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
    const bool sessionMode = sessionModeEnabled();

    for (ProjectExplorer::Project * const project : ProjectExplorer::SessionManager::projects()) {
        const CppEditor::ClangdSettings settings(
                    CppEditor::ClangdProjectSettings(project).settings());
        ClangdClient * const client = clientWithProject(project);
        if (sessionMode) {
            if (client && client->project())
                LanguageClientManager::shutdownClient(client);
            continue;
        }
        if (!client) {
            if (settings.useClangd())
                updateLanguageClient(project);
            continue;
        }
        if (!settings.useClangd()) {
            LanguageClientManager::shutdownClient(client);
            continue;
        }
        if (client->settingsData() != settings.data())
            updateLanguageClient(project);
    }

    ClangdClient * const fallbackOrSessionClient = clientForProject(nullptr);
    const auto startNewFallbackOrSessionClient = [this, sessionMode] {
        if (sessionMode)
            updateLanguageClient(nullptr);
        else
            claimNonProjectSources(new ClangdClient(nullptr, {}));
    };
    const ClangdSettings &settings = ClangdSettings::instance();
    if (!fallbackOrSessionClient) {
        if (settings.useClangd())
            startNewFallbackOrSessionClient();
        return;
    }
    if (!settings.useClangd()) {
        LanguageClientManager::shutdownClient(fallbackOrSessionClient);
        return;
    }
    if (fallbackOrSessionClient->settingsData() != settings.data()) {
        LanguageClientManager::shutdownClient(fallbackOrSessionClient);
        startNewFallbackOrSessionClient();
    }
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
    const ClangEditorDocumentProcessors processors = clangProcessorsWithProjectParts(projectPartIds);
    for (ClangEditorDocumentProcessor *processor : processors) {
        processor->clearProjectPart();
        processor->run();
    }
}

} // Internal
} // ClangCodeModel
