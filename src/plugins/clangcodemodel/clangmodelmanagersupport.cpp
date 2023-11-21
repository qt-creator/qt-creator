// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangmodelmanagersupport.h"

#include "clangcodemodeltr.h"
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
#include <coreplugin/session.h>
#include <coreplugin/vcsmanager.h>

#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppeditorwidget.h>
#include <cppeditor/cppfollowsymbolundercursor.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cppprojectfile.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/editordocumenthandle.h>

#include <languageclient/languageclientmanager.h>
#include <languageclient/locatorfilter.h>

#include <texteditor/quickfix.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/infobar.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QLabel>
#include <QMenu>
#include <QTextBlock>
#include <QTimer>
#include <QtDebug>

using namespace Core;
using namespace CppEditor;
using namespace LanguageClient;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangCodeModel::Internal {

static Project *fallbackProject()
{
    if (Project * const p = ProjectTree::currentProject())
        return p;
    return ProjectManager::startupProject();
}

static bool sessionModeEnabled()
{
    return ClangdSettings::instance().granularity() == ClangdSettings::Granularity::Session;
}

static const QList<TextEditor::TextDocument *> allCppDocuments()
{
    const auto isCppDocument = Utils::equal(&IDocument::id, Id(CppEditor::Constants::CPPEDITOR_ID));
    const QList<IDocument *> documents = Utils::filtered(DocumentModel::openedDocuments(),
                                                         isCppDocument);
    return Utils::qobject_container_cast<TextEditor::TextDocument *>(documents);
}

static const QList<Project *> projectsForClient(const Client *client)
{
    QList<Project *> projects;
    if (sessionModeEnabled()) {
        for (Project * const p : ProjectManager::projects()) {
            if (ClangdProjectSettings(p).settings().useClangd)
                projects << p;
        }
    } else if (client->project()) {
        projects << client->project();
    }
    return projects;
}

static bool fileIsProjectBuildArtifact(const Client *client, const FilePath &filePath)
{
    for (const Project * const p : projectsForClient(client)) {
        if (const auto t = p->activeTarget()) {
            if (const auto bc = t->activeBuildConfiguration()) {
                if (filePath.isChildOf(bc->buildDirectory()))
                    return true;
            }
        }
    }
    return false;
}

static Client *clientForGeneratedFile(const FilePath &filePath)
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
    const QString warnStr = Tr::tr("The use of clangd for the C/C++ "
            "code model was disabled, because it is likely that its memory requirements "
            "would be higher than what your system can handle.");
    const Id clangdWarningSetting("WarnAboutClangd");
    InfoBarEntry info(clangdWarningSetting, warnStr);
    info.setDetailsWidgetCreator([] {
        const auto label = new QLabel(Tr::tr(
            "With clangd enabled, Qt Creator fully supports modern C++ "
            "when highlighting code, completing symbols and so on.<br>"
            "This comes at a higher cost in terms of CPU load and memory usage compared "
            "to the built-in code model, which therefore might be the better choice "
            "on older machines and/or with legacy code.<br>"
            "You can enable/disable and fine-tune clangd <a href=\"dummy\">here</a>."));
        label->setWordWrap(true);
        QObject::connect(label, &QLabel::linkActivated, [] {
            ICore::showOptionsDialog(CppEditor::Constants::CPP_CLANGD_SETTINGS_ID);
        });
        return label;
    });
    info.addCustomButton(Tr::tr("Enable Anyway"), [clangdWarningSetting] {
        ClangdSettings::setUseClangdAndSave(true);
        ICore::infoBar()->removeInfo(clangdWarningSetting);
    });
    ICore::infoBar()->addInfo(info);
}

static void updateParserConfig(ClangdClient *client)
{
    if (!client->reachable())
        return;
    if (const auto editor = TextEditor::BaseTextEditor::currentTextEditor()) {
        if (!client->documentOpen(editor->textDocument()))
            return;
        const FilePath filePath = editor->textDocument()->filePath();
        if (const auto processor = ClangEditorDocumentProcessor::get(filePath))
            client->updateParserConfig(filePath, processor->parserConfig());
    }
}

static bool projectIsParsing(const ClangdClient *client)
{
    for (const Project * const p : projectsForClient(client)) {
        const BuildSystem * const bs = p && p->activeTarget()
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
    CppModelManager::setCurrentDocumentFilter(std::make_unique<ClangdCurrentDocumentFilter>());
    CppModelManager::setLocatorFilter(std::make_unique<ClangdAllSymbolsFilter>());
    CppModelManager::setClassesFilter(std::make_unique<ClangdClassesFilter>());
    CppModelManager::setFunctionsFilter(std::make_unique<ClangdFunctionsFilter>());
    // Setup matchers
    LocatorMatcher::addMatcherCreator(MatcherType::AllSymbols, [] {
        return LanguageClient::languageClientMatchers(
            MatcherType::AllSymbols, clientsForOpenProjects(), 10000);
    });
    LocatorMatcher::addMatcherCreator(MatcherType::Classes, [] {
        return LanguageClient::languageClientMatchers(
            MatcherType::Classes, clientsForOpenProjects(), 10000);
    });
    LocatorMatcher::addMatcherCreator(MatcherType::Functions, [] {
        return LanguageClient::languageClientMatchers(
            MatcherType::Functions, clientsForOpenProjects(), 10000);
    });

    EditorManager *editorManager = EditorManager::instance();
    connect(editorManager, &EditorManager::editorOpened,
            this, &ClangModelManagerSupport::onEditorOpened);
    connect(editorManager, &EditorManager::currentEditorChanged,
            this, &ClangModelManagerSupport::onCurrentEditorChanged);

    CppModelManager *modelManager = CppModelManager::instance();
    connect(modelManager, &CppModelManager::abstractEditorSupportContentsUpdated,
            this, &ClangModelManagerSupport::onAbstractEditorSupportContentsUpdated);
    connect(modelManager, &CppModelManager::abstractEditorSupportRemoved,
            this, &ClangModelManagerSupport::onAbstractEditorSupportRemoved);
    connect(modelManager, &CppModelManager::projectPartsUpdated,
            this, &ClangModelManagerSupport::updateLanguageClient);
    connect(modelManager, &CppModelManager::fallbackProjectPartUpdated, this, [this] {
        if (sessionModeEnabled())
            return;
        if (ClangdClient * const fallbackClient = clientForProject(nullptr))
            LanguageClientManager::shutdownClient(fallbackClient);
        if (ClangdSettings::instance().useClangd())
            claimNonProjectSources(new ClangdClient(nullptr, {}));
    });

    auto projectManager = ProjectManager::instance();
    connect(projectManager, &ProjectManager::projectRemoved, this, [this] {
        if (!sessionModeEnabled())
            claimNonProjectSources(clientForProject(fallbackProject()));
    });
    connect(SessionManager::instance(), &SessionManager::sessionLoaded, this, [this] {
        if (sessionModeEnabled())
            onClangdSettingsChanged();
    });

    ClangdSettings::setDefaultClangdPath(ICore::clangdExecutable(CLANG_BINDIR));
    connect(&ClangdSettings::instance(), &ClangdSettings::changed,
            this, &ClangModelManagerSupport::onClangdSettingsChanged);

    new ClangdQuickFixFactory(); // memory managed by CppEditor::g_cppQuickFixFactories
}

ClangModelManagerSupport::~ClangModelManagerSupport()
{
    m_generatorSynchronizer.waitForFinished();
}

void ClangModelManagerSupport::followSymbol(const CursorInEditor &data,
                                            const LinkHandler &processLinkCallback,
                                            bool resolveTarget, bool inNextSplit)
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

void ClangModelManagerSupport::followSymbolToType(const CursorInEditor &data,
                                                  const LinkHandler &processLinkCallback,
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

void ClangModelManagerSupport::switchDeclDef(const CursorInEditor &data,
                                             const LinkHandler &processLinkCallback)
{
    if (ClangdClient * const client = clientForFile(data.filePath());
            client && client->isFullyIndexed()) {
        client->switchDeclDef(data.textDocument(), data.cursor(), data.editorWidget(),
                              processLinkCallback);
        return;
    }

    CppModelManager::switchDeclDef(data, processLinkCallback, CppModelManager::Backend::Builtin);
}

void ClangModelManagerSupport::startLocalRenaming(const CursorInEditor &data,
                                                  const ProjectPart *projectPart,
                                                  RenameCallback &&renameSymbolsCallback)
{
    if (ClangdClient * const client = clientForFile(data.filePath());
            client && client->reachable()) {
        client->findLocalUsages(data.editorWidget(), data.cursor(),
                                std::move(renameSymbolsCallback));
        return;
    }

    CppModelManager::startLocalRenaming(data, projectPart,
            std::move(renameSymbolsCallback), CppModelManager::Backend::Builtin);
}

void ClangModelManagerSupport::globalRename(const CursorInEditor &cursor,
                                            const QString &replacement,
                                            const std::function<void()> &callback)
{
    if (ClangdClient * const client = clientForFile(cursor.filePath());
            client && client->isFullyIndexed()) {
        QTC_ASSERT(client->documentOpen(cursor.textDocument()),
                   client->openDocument(cursor.textDocument()));
        client->findUsages(cursor, replacement, callback);
        return;
    }
    CppModelManager::globalRename(cursor, replacement, callback, CppModelManager::Backend::Builtin);
}

void ClangModelManagerSupport::findUsages(const CursorInEditor &cursor) const
{
    if (ClangdClient * const client = clientForFile(cursor.filePath());
            client && client->isFullyIndexed()) {
        QTC_ASSERT(client->documentOpen(cursor.textDocument()),
                   client->openDocument(cursor.textDocument()));
        client->findUsages(cursor, {}, {});
        return;
    }
    CppModelManager::findUsages(cursor, CppModelManager::Backend::Builtin);
}

void ClangModelManagerSupport::switchHeaderSource(const FilePath &filePath, bool inNextSplit)
{
    if (ClangdClient * const client = clientForFile(filePath)) {
        switch (ClangdProjectSettings(client->project()).settings().headerSourceSwitchMode) {
        case ClangdSettings::HeaderSourceSwitchMode::BuiltinOnly:
            CppModelManager::switchHeaderSource(inNextSplit, CppModelManager::Backend::Builtin);
            return;
        case ClangdSettings::HeaderSourceSwitchMode::ClangdOnly:
            client->switchHeaderSource(filePath, inNextSplit);
            return;
        case ClangdSettings::HeaderSourceSwitchMode::Both:
            const FilePath otherFile = correspondingHeaderOrSource(filePath);
            if (!otherFile.isEmpty())
                openEditor(otherFile, inNextSplit);
            else
                client->switchHeaderSource(filePath, inNextSplit);
            return;
        }
    }
    CppModelManager::switchHeaderSource(inNextSplit, CppModelManager::Backend::Builtin);
}

void ClangModelManagerSupport::checkUnused(const Link &link, SearchResult *search,
                                           const LinkHandler &callback)
{
    if (const Project * const project = ProjectManager::projectForFile(link.targetFilePath)) {
        if (ClangdClient * const client = clientWithProject(project);
                client && client->isFullyIndexed()) {
            client->checkUnused(link, search, callback);
            return;
        }
    }

    CppModelManager::modelManagerSupport(
                CppModelManager::Backend::Builtin)->checkUnused(link, search, callback);
}

bool ClangModelManagerSupport::usesClangd(const TextEditor::TextDocument *document) const
{
    return clientForFile(document->filePath());
}

BaseEditorDocumentProcessor *ClangModelManagerSupport::createEditorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument)
{
    const auto processor = new ClangEditorDocumentProcessor(baseTextDocument);
    const auto handleConfigChange = [](const FilePath &fp,
            const BaseEditorDocumentParser::Configuration &config) {
        if (const auto client = clientForFile(fp))
            client->updateParserConfig(fp, config);
    };
    connect(processor, &ClangEditorDocumentProcessor::parserConfigChanged,
            this, handleConfigChange);
    return processor;
}

void ClangModelManagerSupport::onCurrentEditorChanged(IEditor *editor)
{
    // Update task hub issues for current CppEditorDocument
    TaskHub::clearTasks(Constants::TASK_CATEGORY_DIAGNOSTICS);
    if (!editor || !editor->document() || !CppModelManager::isCppEditor(editor))
        return;

    const FilePath filePath = editor->document()->filePath();
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

static FilePath getJsonDbDir(const Project *project)
{
    static const QString dirName(".qtc_clangd");
    if (!project) {
        const QString sessionDirName = FileUtils::fileSystemFriendlyName(
                    SessionManager::activeSession());
        return ICore::userResourcePath() / dirName / sessionDirName; // TODO: Make configurable?
    }
    if (const Target * const target = project->activeTarget()) {
        if (const BuildConfiguration * const bc = target->activeBuildConfiguration())
            return bc->buildDirectory() / dirName;
    }
    return {};
}

static bool isProjectDataUpToDate(Project *project, ProjectInfoList projectInfo,
                                  const FilePath &jsonDbDir)
{
    if (project && !ProjectManager::hasProject(project))
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
        if (const ProjectInfo::ConstPtr pi = CppModelManager::projectInfo(project))
            newProjectInfo.append(pi);
        else
            return false;
    } else {
        newProjectInfo = CppModelManager::projectInfos();
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

void ClangModelManagerSupport::updateLanguageClient(Project *project)
{
    const ClangdSettings settings(ClangdProjectSettings(project).settings());
    if (!settings.useClangd())
        return;
    ProjectInfoList projectInfo;
    if (sessionModeEnabled()) {
        project = nullptr;
        projectInfo = CppModelManager::projectInfos();
    } else if (const ProjectInfo::ConstPtr pi = CppModelManager::projectInfo(project)) {
        projectInfo.append(pi);
    } else {
        return;
    }

    const FilePath jsonDbDir = getJsonDbDir(project);
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
            MessageManager::writeDisrupting(
                        Tr::tr("Cannot use clangd: Failed to generate compilation database:\n%1")
                        .arg(result.error));
            return;
        }
        Id previousId;
        if (Client * const oldClient = clientForProject(project)) {
            previousId = oldClient->id();
            LanguageClientManager::shutdownClient(oldClient);
        }
        ClangdClient * const client = new ClangdClient(project, jsonDbDir, previousId);
        connect(client, &Client::shadowDocumentSwitched, this, [](const FilePath &fp) {
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
                const Project * const docProject = ProjectManager::projectForFile(doc->filePath());
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

            for (auto it = m_potentialShadowDocuments.begin();
                 it != m_potentialShadowDocuments.end(); ++it) {
                if (!fileIsProjectBuildArtifact(client, it.key()))
                    continue;
                if (it.value().isEmpty())
                    client->removeShadowDocument(it.key());
                else
                    client->setShadowDocument(it.key(), it.value());
                ClangdClient::handleUiHeaderChange(it.key().fileName());
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
            else if (ProjectManager::startupProject())
                rootNode = ProjectManager::startupProject()->rootProjectNode();
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
    const FilePath includeDir = settings.clangdIncludePath();
    auto future = Utils::asyncRun(&Internal::generateCompilationDB, projectInfo,
                                  jsonDbDir, CompilationDbPurpose::CodeModel,
                                  warningsConfigForProject(project),
                                  globalClangOptions(), includeDir);
    generatorWatcher->setFuture(future);
    m_generatorSynchronizer.addFuture(future);
}

QList<Client *> ClangModelManagerSupport::clientsForOpenProjects()
{
    QSet<Client *> clients;
    const QList<Project *> projects = ProjectManager::projects();
    for (Project *project : projects) {
        if (Client *client = ClangModelManagerSupport::clientForProject(project))
            clients << client;
    }
    return clients.values();
}

ClangdClient *ClangModelManagerSupport::clientForProject(const Project *project)
{
    if (sessionModeEnabled())
        project = nullptr;
    return clientWithProject(project);
}

ClangdClient *ClangModelManagerSupport::clientWithProject(const Project *project)
{
    const QList<Client *> clients = Utils::filtered(
                LanguageClientManager::clientsForProject(project), [](const Client *c) {
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

ClangdClient *ClangModelManagerSupport::clientForFile(const FilePath &file)
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
        if (ProjectManager::projectForFile(doc->filePath()))
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
// We also look for repository changes explicitly.
void ClangModelManagerSupport::watchForExternalChanges()
{
    connect(DocumentManager::instance(), &DocumentManager::filesChangedExternally,
            this, [this](const QSet<FilePath> &files) {
        if (!LanguageClientManager::hasClients<ClangdClient>())
            return;
        for (const FilePath &file : files) {
            if (TextEditor::TextDocument::textDocumentForFilePath(file)) {
                // if we have a document for that file we should receive the content
                // change via the document signals
                continue;
            }
            const ProjectFile::Kind kind = ProjectFile::classify(file.toString());
            if (!ProjectFile::isSource(kind) && !ProjectFile::isHeader(kind))
                continue;
            Project * const project = ProjectManager::projectForFile(file);
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

    connect(VcsManager::instance(), &VcsManager::repositoryChanged,
            this, [this](const FilePath &repoDir) {
        if (sessionModeEnabled()) {
            if (ClangdClient * const client = clientForProject(nullptr))
                scheduleClientRestart(client);
            return;
        }
        for (const Project * const project : ProjectManager::projects()) {
            const FilePath &projectDir = project->projectDirectory();
            if (repoDir == projectDir || repoDir.isChildOf(projectDir)
                || projectDir.isChildOf(repoDir)) {
                if (ClangdClient * const client = clientForProject(project))
                    scheduleClientRestart(client);
            }
        }
    });
}

// If Qt Creator changes a file that is not open (e.g. as part of a quickfix), we have to
// restart clangd for reliable re-parsing and re-indexing.
void ClangModelManagerSupport::watchForInternalChanges()
{
    connect(DocumentManager::instance(), &DocumentManager::filesChangedInternally,
            this, [this](const FilePaths &filePaths) {
        for (const FilePath &fp : filePaths) {
            const ProjectFile::Kind kind = ProjectFile::classify(fp.toString());
            if (!ProjectFile::isSource(kind) && !ProjectFile::isHeader(kind))
                continue;
            Project * const project = ProjectManager::projectForFile(fp);
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

void ClangModelManagerSupport::onEditorOpened(IEditor *editor)
{
    QTC_ASSERT(editor, return);
    IDocument *document = editor->document();
    QTC_ASSERT(document, return);
    auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);

    if (textDocument && CppModelManager::isCppEditor(editor)) {
        connectToWidgetsMarkContextMenuRequested(editor->widget());

        Project * project = ProjectManager::projectForFile(document->filePath());
        const ClangdSettings settings(ClangdProjectSettings(project).settings());
        if (!settings.useClangd())
            return;
        if (!settings.sizeIsOkay(textDocument->filePath()))
            return;
        if (sessionModeEnabled())
            project = nullptr;
        else if (!project && ProjectFile::isHeader(document->filePath()))
            project = fallbackProject();
        ClangdClient *client = clientForProject(project);
        if (!client) {
            if (project)
                return;
            client = new ClangdClient(nullptr, {});
        }
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
    const auto fp = FilePath::fromString(filePath);
    const QString stringContent = QString::fromUtf8(content);
    if (Client * const client = clientForGeneratedFile(fp)) {
        client->setShadowDocument(fp, stringContent);
        ClangdClient::handleUiHeaderChange(fp.fileName());
    }
    m_potentialShadowDocuments.insert(fp, stringContent);
}

void ClangModelManagerSupport::onAbstractEditorSupportRemoved(const QString &filePath)
{
    QTC_ASSERT(!filePath.isEmpty(), return);

    const auto fp = FilePath::fromString(filePath);
    if (Client * const client = clientForGeneratedFile(fp)) {
        client->removeShadowDocument(fp);
        ClangdClient::handleUiHeaderChange(fp.fileName());
    }
    m_potentialShadowDocuments.remove(fp);
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

    const FilePath filePath = widget->textDocument()->filePath();
    ClangEditorDocumentProcessor *processor = ClangEditorDocumentProcessor::get(filePath);
    if (processor) {
        const auto assistInterface = createAssistInterface(widget, lineNumber);
        const auto fixItOperations = processor->extraRefactoringOperations(assistInterface);

        addFixItsActionsToMenu(menu, fixItOperations);
    }
}

void ClangModelManagerSupport::onClangdSettingsChanged()
{
    const bool sessionMode = sessionModeEnabled();

    for (Project * const project : ProjectManager::projects()) {
        const ClangdSettings settings(ClangdProjectSettings(project).settings());
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

} // ClangCodeModel::Internal
