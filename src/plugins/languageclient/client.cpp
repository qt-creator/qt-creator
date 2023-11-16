// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "client.h"

#include "callhierarchy.h"
#include "diagnosticmanager.h"
#include "documentsymbolcache.h"
#include "languageclientcompletionassist.h"
#include "languageclientformatter.h"
#include "languageclientfunctionhint.h"
#include "languageclienthoverhandler.h"
#include "languageclientinterface.h"
#include "languageclientmanager.h"
#include "languageclientoutline.h"
#include "languageclientquickfix.h"
#include "languageclientsymbolsupport.h"
#include "languageclientutils.h"
#include "languageclienttr.h"
#include "progressmanager.h"
#include "semantichighlightsupport.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <languageserverprotocol/completion.h>
#include <languageserverprotocol/diagnostics.h>
#include <languageserverprotocol/initializemessages.h>
#include <languageserverprotocol/jsonrpcmessages.h>
#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/messages.h>
#include <languageserverprotocol/progresssupport.h>
#include <languageserverprotocol/servercapabilities.h>
#include <languageserverprotocol/shutdownmessages.h>
#include <languageserverprotocol/workspace.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/codeassist/documentcontentcompletion.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/ioutlinewidget.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>

#include <utils/appinfo.h>
#include <utils/mimeutils.h>
#include <utils/process.h>

#include <QDebug>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QThread>
#include <QTimer>

using namespace LanguageServerProtocol;
using namespace Utils;

namespace LanguageClient {

static Q_LOGGING_CATEGORY(LOGLSPCLIENT, "qtc.languageclient.client", QtWarningMsg);

class InterfaceController : public QObject
{
    Q_OBJECT

public:
    InterfaceController(BaseClientInterface *interface)
        : m_interface(interface)
    {
        using Interface = BaseClientInterface;
        interface->moveToThread(&m_thread);
        connect(interface, &Interface::messageReceived, this, &InterfaceController::messageReceived);
        connect(interface, &Interface::error, this, &InterfaceController::error);
        connect(interface, &Interface::finished, this, &InterfaceController::finished);
        connect(interface, &Interface::started, this, &InterfaceController::started);
        m_thread.start();
    }
    ~InterfaceController()
    {
        m_interface->deleteLater();
        m_thread.quit();
        m_thread.wait();
    }

    void start()
    {
        QMetaObject::invokeMethod(m_interface, &BaseClientInterface::start);
    }
    void sendMessage(const JsonRpcMessage &message)
    {
        QMetaObject::invokeMethod(m_interface, [=]() { m_interface->sendMessage(message); });
    }
    void resetBuffer()
    {
        QMetaObject::invokeMethod(m_interface, &BaseClientInterface::resetBuffer);
    }

signals:
    void messageReceived(const JsonRpcMessage &message);
    void started();
    void error(const QString &message);
    void finished();

private:
    BaseClientInterface *m_interface;
    QThread m_thread;
};

static void updateEditorToolBar(QList<TextEditor::TextDocument *> documents)
{
    for (TextEditor::TextDocument *document : documents) {
        for (Core::IEditor *editor : Core::DocumentModel::editorsForDocument(document))
            updateEditorToolBar(editor);
    }
}

class ClientPrivate : public QObject
{
    Q_OBJECT
public:
    ClientPrivate(Client *client, BaseClientInterface *clientInterface, const Utils::Id &id)
        : q(client)
        , m_id(id.isValid() ? id : Utils::Id::fromString(QUuid::createUuid().toString()))
        , m_clientCapabilities(q->defaultClientCapabilities())
        , m_clientInterface(new InterfaceController(clientInterface))
        , m_documentSymbolCache(q)
        , m_hoverHandler(q)
        , m_symbolSupport(q)
        , m_tokenSupport(q)
        , m_serverDeviceTemplate(clientInterface->serverDeviceTemplate())
    {
        using namespace ProjectExplorer;

        m_clientInfo.setName(QGuiApplication::applicationDisplayName());
        m_clientInfo.setVersion(Utils::appInfo().displayVersion);

        m_clientProviders.completionAssistProvider = new LanguageClientCompletionAssistProvider(q);
        m_clientProviders.functionHintProvider = new FunctionHintAssistProvider(q);
        m_clientProviders.quickFixAssistProvider = new LanguageClientQuickFixProvider(q);

        m_documentUpdateTimer.setSingleShot(true);
        m_documentUpdateTimer.setInterval(500);
        connect(&m_documentUpdateTimer, &QTimer::timeout, this,
                [this] { sendPostponedDocumentUpdates(Schedule::Now); });
        connect(ProjectManager::instance(), &ProjectManager::projectRemoved,
                q, &Client::projectClosed);

        QTC_ASSERT(clientInterface, return);
        connect(m_clientInterface, &InterfaceController::messageReceived, q, &Client::handleMessage);
        connect(m_clientInterface, &InterfaceController::error, q, &Client::setError);
        connect(m_clientInterface, &InterfaceController::finished, q, &Client::finished);
        connect(m_clientInterface, &InterfaceController::started, this, [this]() {
            LanguageClientManager::clientStarted(q);
        });
        connect(Core::EditorManager::instance(),
                &Core::EditorManager::documentClosed,
                this,
                &ClientPrivate::documentClosed);

        m_tokenSupport.setTokenTypesMap(SemanticTokens::defaultTokenTypesMap());
        m_tokenSupport.setTokenModifiersMap(SemanticTokens::defaultTokenModifiersMap());

        m_shutdownTimer.setInterval(20 /*seconds*/ * 1000);
        connect(&m_shutdownTimer, &QTimer::timeout, this, [this] {
            LanguageClientManager::deleteClient(q);
        });

        m_restartCountResetTimer.setSingleShot(true);
        m_restartCountResetTimer.setInterval(2 * 60 * 1000);
        connect(&m_restartCountResetTimer, &QTimer::timeout,
                this, [this] { m_restartsLeft = MaxRestarts; });
    }

    ~ClientPrivate()
    {
        using namespace TextEditor;
        // FIXME: instead of replacing the completion provider in the text document store the
        // completion provider as a prioritised list in the text document
        // temporary container needed since m_resetAssistProvider is changed in resetAssistProviders
        for (TextDocument *document : m_resetAssistProvider.keys())
            resetAssistProviders(document);
        if (!ExtensionSystem::PluginManager::isShuttingDown()) {
            // prevent accessing deleted editors on Creator shutdown
            const QList<Core::IEditor *> &editors = Core::DocumentModel::editorsForOpenedDocuments();
            for (Core::IEditor *editor : editors) {
                if (auto textEditor = qobject_cast<BaseTextEditor *>(editor)) {
                    TextEditorWidget *widget = textEditor->editorWidget();
                    widget->clearRefactorMarkers(m_id);
                    widget->removeHoverHandler(&m_hoverHandler);
                }
            }
            updateEditorToolBar(m_openedDocument.keys());
        }
        for (IAssistProcessor *processor : std::as_const(m_runningAssistProcessors))
            processor->setAsyncProposalAvailable(nullptr);
        qDeleteAll(m_documentHighlightsTimer);
        m_documentHighlightsTimer.clear();
        // do not handle messages while shutting down
        disconnect(m_clientInterface, &InterfaceController::messageReceived,
                   q, &Client::handleMessage);
        delete m_diagnosticManager;
        delete m_clientInterface;
    }

    Client *q;

    void sendMessageNow(const JsonRpcMessage &message);
    void handleResponse(const MessageId &id,
                        const JsonRpcMessage &message);
    void handleMethod(const QString &method,
                      const MessageId &id,
                      const JsonRpcMessage &message);

    void initializeCallback(const LanguageServerProtocol::InitializeRequest::Response &initResponse);
    void shutDownCallback(const LanguageServerProtocol::ShutdownRequest::Response &shutdownResponse);
    bool sendWorkspceFolderChanges() const;
    void log(const LanguageServerProtocol::ShowMessageParams &message);

    LanguageServerProtocol::LanguageClientValue<LanguageServerProtocol::MessageActionItem>
    showMessageBox(const LanguageServerProtocol::ShowMessageRequestParams &message);

    void removeDiagnostics(const LanguageServerProtocol::DocumentUri &uri);
    void resetAssistProviders(TextEditor::TextDocument *document);

    void sendPostponedDocumentUpdates(Schedule semanticTokensSchedule);

    void updateCompletionProvider(TextEditor::TextDocument *document);
    void updateFunctionHintProvider(TextEditor::TextDocument *document);

    void requestDocumentHighlights(TextEditor::TextEditorWidget *widget);
    void requestDocumentHighlightsNow(TextEditor::TextEditorWidget *widget);
    LanguageServerProtocol::SemanticRequestTypes supportedSemanticRequests(TextEditor::TextDocument *document) const;
    void handleSemanticTokens(const LanguageServerProtocol::SemanticTokens &tokens);
    void requestCodeActions(const LanguageServerProtocol::DocumentUri &uri,
                            const LanguageServerProtocol::Range &range,
                            const QList<LanguageServerProtocol::Diagnostic> &diagnostics);
    void documentClosed(Core::IDocument *document);
    void sendOpenNotification(const FilePath &filePath, const QString &mimeType,
                              const QString &content, int version);
    void sendCloseNotification(const FilePath &filePath);
    void openRequiredShadowDocuments(const TextEditor::TextDocument *doc);
    void closeRequiredShadowDocuments(const TextEditor::TextDocument *doc);

    using ShadowDocIterator = QMap<FilePath, QPair<QString, QList<const TextEditor::TextDocument *>>>::iterator;
    void openShadowDocument(const TextEditor::TextDocument *requringDoc, ShadowDocIterator shadowIt);
    void closeShadowDocument(ShadowDocIterator docIt);

    bool reset();

    Client::State m_state = Client::Uninitialized;
    QHash<LanguageServerProtocol::MessageId,
          LanguageServerProtocol::ResponseHandler::Callback> m_responseHandlers;
    QString m_displayName;
    LanguageFilter m_languagFilter;
    QJsonObject m_initializationOptions;
    class OpenedDocument
    {
    public:
        ~OpenedDocument()
        {
            QObject::disconnect(contentsChangedConnection);
            QObject::disconnect(filePathChangedConnection);
            QObject::disconnect(aboutToSaveConnection);
            QObject::disconnect(savedConnection);
            delete document;
        }
        QMetaObject::Connection contentsChangedConnection;
        QMetaObject::Connection filePathChangedConnection;
        QMetaObject::Connection aboutToSaveConnection;
        QMetaObject::Connection savedConnection;
        QTextDocument *document = nullptr;
    };
    QMap<TextEditor::TextDocument *, OpenedDocument> m_openedDocument;

    // Used for build system artifacts (e.g. UI headers) that Qt Creator "live-generates" ahead of
    // the build.
    // The Value is the file content + the documents that require the shadow file to be open
    // (empty <=> shadow document is not open).
    QMap<FilePath, QPair<QString, QList<const TextEditor::TextDocument *>>> m_shadowDocuments;

    QSet<TextEditor::TextDocument *> m_postponedDocuments;
    QMap<Utils::FilePath, int> m_documentVersions;
    std::unordered_map<TextEditor::TextDocument *,
                       QList<LanguageServerProtocol::DidChangeTextDocumentParams::TextDocumentContentChangeEvent>>
        m_documentsToUpdate;
    QMap<TextEditor::TextEditorWidget *, QTimer *> m_documentHighlightsTimer;
    QTimer m_documentUpdateTimer;
    Utils::Id m_id;
    LanguageServerProtocol::ClientCapabilities m_clientCapabilities;
    LanguageServerProtocol::ServerCapabilities m_serverCapabilities;
    DynamicCapabilities m_dynamicCapabilities;
    struct AssistProviders
    {
        QPointer<TextEditor::CompletionAssistProvider> completionAssistProvider;
        QPointer<TextEditor::CompletionAssistProvider> functionHintProvider;
        QPointer<TextEditor::IAssistProvider> quickFixAssistProvider;
    };

    AssistProviders m_clientProviders;
    QMap<TextEditor::TextDocument *, AssistProviders> m_resetAssistProvider;
    QHash<TextEditor::TextEditorWidget *, LanguageServerProtocol::MessageId> m_highlightRequests;
    QHash<QString, Client::CustomMethodHandler> m_customHandlers;
    static const int MaxRestarts = 5;
    int m_restartsLeft = MaxRestarts;
    QTimer m_restartCountResetTimer;
    InterfaceController *m_clientInterface = nullptr;
    DiagnosticManager *m_diagnosticManager = nullptr;
    DocumentSymbolCache m_documentSymbolCache;
    HoverHandler m_hoverHandler;
    QHash<LanguageServerProtocol::DocumentUri, TextEditor::HighlightingResults> m_highlights;
    ProjectExplorer::Project *m_project = nullptr;
    QSet<TextEditor::IAssistProcessor *> m_runningAssistProcessors;
    SymbolSupport m_symbolSupport;
    MessageId m_runningFindLinkRequest;
    ProgressManager m_progressManager;
    bool m_activateDocAutomatically = false;
    SemanticTokenSupport m_tokenSupport;
    QString m_serverName;
    QString m_serverVersion;
    Client::LogTarget m_logTarget = Client::LogTarget::Ui;
    bool m_locatorsEnabled = true;
    bool m_autoRequestCodeActions = true;
    QTimer m_shutdownTimer;
    LanguageServerProtocol::ClientInfo m_clientInfo;
    QJsonValue m_configuration;
    int m_completionResultsLimit = -1;
    const Utils::FilePath m_serverDeviceTemplate;
};

Client::Client(BaseClientInterface *clientInterface, const Utils::Id &id)
    : d(new ClientPrivate(this, clientInterface, id))
{}

Id Client::id() const
{
    return d->m_id;
}

void Client::setName(const QString &name)
{
    d->m_displayName = name;
}

QString Client::name() const
{
    if (d->m_project && !d->m_project->displayName().isEmpty())
        //: <language client> for <project>
        return Tr::tr("%1 for %2").arg(d->m_displayName, d->m_project->displayName());
    return d->m_displayName;
}

Client::~Client()
{
    delete d;
}

static ClientCapabilities generateClientCapabilities()
{
    ClientCapabilities capabilities;
    WorkspaceClientCapabilities workspaceCapabilities;
    WorkspaceClientCapabilities::WorkspaceEditCapabilities workspaceEditCapabilities;
    workspaceEditCapabilities.setDocumentChanges(true);
    using ResourceOperationKind
        = WorkspaceClientCapabilities::WorkspaceEditCapabilities::ResourceOperationKind;
    workspaceEditCapabilities.setResourceOperations({ResourceOperationKind::Create,
                                                     ResourceOperationKind::Rename,
                                                     ResourceOperationKind::Delete});
    workspaceCapabilities.setWorkspaceEdit(workspaceEditCapabilities);
    workspaceCapabilities.setWorkspaceFolders(true);
    workspaceCapabilities.setApplyEdit(true);
    DynamicRegistrationCapabilities allowDynamicRegistration;
    allowDynamicRegistration.setDynamicRegistration(true);
    workspaceCapabilities.setDidChangeConfiguration(allowDynamicRegistration);
    workspaceCapabilities.setExecuteCommand(allowDynamicRegistration);
    workspaceCapabilities.setConfiguration(true);
    SemanticTokensWorkspaceClientCapabilities semanticTokensWorkspaceClientCapabilities;
    semanticTokensWorkspaceClientCapabilities.setRefreshSupport(true);
    workspaceCapabilities.setSemanticTokens(semanticTokensWorkspaceClientCapabilities);
    capabilities.setWorkspace(workspaceCapabilities);

    TextDocumentClientCapabilities documentCapabilities;
    TextDocumentClientCapabilities::SynchronizationCapabilities syncCapabilities;
    syncCapabilities.setDynamicRegistration(true);
    syncCapabilities.setWillSave(true);
    syncCapabilities.setWillSaveWaitUntil(false);
    syncCapabilities.setDidSave(true);
    documentCapabilities.setSynchronization(syncCapabilities);

    SymbolCapabilities symbolCapabilities;
    SymbolCapabilities::SymbolKindCapabilities symbolKindCapabilities;
    symbolKindCapabilities.setValueSet(
        {SymbolKind::File,       SymbolKind::Module,       SymbolKind::Namespace,
         SymbolKind::Package,    SymbolKind::Class,        SymbolKind::Method,
         SymbolKind::Property,   SymbolKind::Field,        SymbolKind::Constructor,
         SymbolKind::Enum,       SymbolKind::Interface,    SymbolKind::Function,
         SymbolKind::Variable,   SymbolKind::Constant,     SymbolKind::String,
         SymbolKind::Number,     SymbolKind::Boolean,      SymbolKind::Array,
         SymbolKind::Object,     SymbolKind::Key,          SymbolKind::Null,
         SymbolKind::EnumMember, SymbolKind::Struct,       SymbolKind::Event,
         SymbolKind::Operator,   SymbolKind::TypeParameter});
    symbolCapabilities.setSymbolKind(symbolKindCapabilities);
    symbolCapabilities.setHierarchicalDocumentSymbolSupport(true);
    documentCapabilities.setDocumentSymbol(symbolCapabilities);

    TextDocumentClientCapabilities::CompletionCapabilities completionCapabilities;
    completionCapabilities.setDynamicRegistration(true);
    TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemKindCapabilities
        completionItemKindCapabilities;
    completionItemKindCapabilities.setValueSet(
        {CompletionItemKind::Text,         CompletionItemKind::Method,
         CompletionItemKind::Function,     CompletionItemKind::Constructor,
         CompletionItemKind::Field,        CompletionItemKind::Variable,
         CompletionItemKind::Class,        CompletionItemKind::Interface,
         CompletionItemKind::Module,       CompletionItemKind::Property,
         CompletionItemKind::Unit,         CompletionItemKind::Value,
         CompletionItemKind::Enum,         CompletionItemKind::Keyword,
         CompletionItemKind::Snippet,      CompletionItemKind::Color,
         CompletionItemKind::File,         CompletionItemKind::Reference,
         CompletionItemKind::Folder,       CompletionItemKind::EnumMember,
         CompletionItemKind::Constant,     CompletionItemKind::Struct,
         CompletionItemKind::Event,        CompletionItemKind::Operator,
         CompletionItemKind::TypeParameter});
    completionCapabilities.setCompletionItemKind(completionItemKindCapabilities);
    TextDocumentClientCapabilities::CompletionCapabilities::CompletionItemCapbilities
        completionItemCapbilities;
    completionItemCapbilities.setSnippetSupport(true);
    completionItemCapbilities.setCommitCharacterSupport(true);
    completionCapabilities.setCompletionItem(completionItemCapbilities);
    documentCapabilities.setCompletion(completionCapabilities);

    TextDocumentClientCapabilities::CodeActionCapabilities codeActionCapabilities;
    TextDocumentClientCapabilities::CodeActionCapabilities::CodeActionLiteralSupport literalSupport;
    literalSupport.setCodeActionKind(
        TextDocumentClientCapabilities::CodeActionCapabilities::CodeActionLiteralSupport::
            CodeActionKind(QList<QString>{"*"}));
    codeActionCapabilities.setCodeActionLiteralSupport(literalSupport);
    documentCapabilities.setCodeAction(codeActionCapabilities);

    TextDocumentClientCapabilities::HoverCapabilities hover;
    hover.setContentFormat({MarkupKind::markdown, MarkupKind::plaintext});
    hover.setDynamicRegistration(true);
    documentCapabilities.setHover(hover);

    TextDocumentClientCapabilities::RenameClientCapabilities rename;
    rename.setPrepareSupport(true);
    rename.setDynamicRegistration(true);
    documentCapabilities.setRename(rename);

    TextDocumentClientCapabilities::SignatureHelpCapabilities signatureHelp;
    signatureHelp.setDynamicRegistration(true);
    TextDocumentClientCapabilities::SignatureHelpCapabilities::SignatureInformationCapabilities info;
    info.setDocumentationFormat({MarkupKind::markdown, MarkupKind::plaintext});
    info.setActiveParameterSupport(true);
    signatureHelp.setSignatureInformation(info);
    documentCapabilities.setSignatureHelp(signatureHelp);

    documentCapabilities.setReferences(allowDynamicRegistration);
    documentCapabilities.setDocumentHighlight(allowDynamicRegistration);
    documentCapabilities.setDefinition(allowDynamicRegistration);
    documentCapabilities.setTypeDefinition(allowDynamicRegistration);
    documentCapabilities.setImplementation(allowDynamicRegistration);
    documentCapabilities.setFormatting(allowDynamicRegistration);
    documentCapabilities.setRangeFormatting(allowDynamicRegistration);
    documentCapabilities.setOnTypeFormatting(allowDynamicRegistration);
    SemanticTokensClientCapabilities tokens;
    tokens.setDynamicRegistration(true);
    FullSemanticTokenOptions tokenOptions;
    tokenOptions.setDelta(true);
    SemanticTokensClientCapabilities::Requests tokenRequests;
    tokenRequests.setFull(tokenOptions);
    tokens.setRequests(tokenRequests);
    tokens.setTokenTypes({"type",
                          "class",
                          "enumMember",
                          "typeParameter",
                          "parameter",
                          "variable",
                          "function",
                          "macro",
                          "keyword",
                          "comment",
                          "string",
                          "number",
                          "operator"});
    tokens.setTokenModifiers({"declaration", "definition"});
    tokens.setFormats({"relative"});
    documentCapabilities.setSemanticTokens(tokens);
    documentCapabilities.setCallHierarchy(allowDynamicRegistration);
    capabilities.setTextDocument(documentCapabilities);

    WindowClientClientCapabilities window;
    window.setWorkDoneProgress(true);
    capabilities.setWindow(window);

    return capabilities;
}

void Client::initialize()
{
    using namespace ProjectExplorer;
    QTC_ASSERT(d->m_clientInterface, return);
    QTC_ASSERT(d->m_state == Uninitialized, return);
    qCDebug(LOGLSPCLIENT) << "initializing language server " << d->m_displayName;
    InitializeParams params;
    params.setClientInfo(d->m_clientInfo);
    params.setCapabilities(d->m_clientCapabilities);
    params.setInitializationOptions(d->m_initializationOptions);
    if (d->m_project)
        params.setRootUri(hostPathToServerUri(d->m_project->projectDirectory()));

    auto projectFilter = [this](Project *project) { return canOpenProject(project); };
    auto toWorkSpaceFolder = [this](Project *pro) {
        return WorkSpaceFolder(hostPathToServerUri(pro->projectDirectory()), pro->displayName());
    };
    const QList<WorkSpaceFolder> workspaces
        = Utils::transform(Utils::filtered(ProjectManager::projects(), projectFilter),
                           toWorkSpaceFolder);
    if (workspaces.isEmpty())
        params.setWorkSpaceFolders(nullptr);
    else
        params.setWorkSpaceFolders(workspaces);
    InitializeRequest initRequest(params);
    initRequest.setResponseCallback([this](const InitializeRequest::Response &initResponse) {
        d->initializeCallback(initResponse);
    });
    if (std::optional<ResponseHandler> responseHandler = initRequest.responseHandler())
        d->m_responseHandlers[responseHandler->id] = responseHandler->callback;

    // directly send content now otherwise the state check of sendContent would fail
    d->sendMessageNow(initRequest);
    d->m_state = InitializeRequested;
}

void Client::shutdown()
{
    QTC_ASSERT(d->m_state == Initialized, emit finished(); return);
    qCDebug(LOGLSPCLIENT) << "shutdown language server " << d->m_displayName;
    ShutdownRequest shutdown;
    shutdown.setResponseCallback([this](const ShutdownRequest::Response &shutdownResponse){
        d->shutDownCallback(shutdownResponse);
    });
    sendMessage(shutdown);
    d->m_state = ShutdownRequested;
    d->m_shutdownTimer.start();
}

Client::State Client::state() const
{
    return d->m_state;
}

QString Client::stateString() const
{
    switch (d->m_state){
    //: language client state
    case Uninitialized: return Tr::tr("uninitialized");
    //: language client state
    case InitializeRequested: return Tr::tr("initialize requested");
    //: language client state
    case FailedToInitialize: return Tr::tr("failed to initialize");
    //: language client state
    case Initialized: return Tr::tr("initialized");
    //: language client state
    case ShutdownRequested: return Tr::tr("shutdown requested");
    //: language client state
    case Shutdown: return Tr::tr("shut down");
    //: language client state
    case Error: return Tr::tr("error");
    }
    return {};
}

bool Client::reachable() const
{
    return d->m_state == Initialized;
}

void Client::resetRestartCounter()
{
    d->m_restartsLeft = ClientPrivate::MaxRestarts;
}

void Client::setClientInfo(const LanguageServerProtocol::ClientInfo &clientInfo)
{
    d->m_clientInfo = clientInfo;
}

ClientCapabilities Client::defaultClientCapabilities()
{
    return generateClientCapabilities();
}

void Client::setClientCapabilities(const LanguageServerProtocol::ClientCapabilities &caps)
{
    d->m_clientCapabilities = caps;
}

void Client::openDocument(TextEditor::TextDocument *document)
{
    using namespace TextEditor;
    if (d->m_openedDocument.contains(document) || !isSupportedDocument(document))
        return;

    if (d->m_state != Initialized) {
        d->m_postponedDocuments << document;
        return;
    }

    const FilePath &filePath = document->filePath();
    const auto shadowIt = d->m_shadowDocuments.find(filePath);
    if (shadowIt != d->m_shadowDocuments.end()) {
        d->closeShadowDocument(shadowIt);
        emit shadowDocumentSwitched(filePath);
    }
    d->openRequiredShadowDocuments(document);

    const QString method(DidOpenTextDocumentNotification::methodName);
    if (std::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        if (!*registered)
            return;
        const TextDocumentRegistrationOptions option(
            d->m_dynamicCapabilities.option(method).toObject());
        if (option.isValid()
            && !option.filterApplies(filePath, Utils::mimeTypeForName(document->mimeType()))) {
            return;
        }
    } else if (std::optional<ServerCapabilities::TextDocumentSync> _sync
               = d->m_serverCapabilities.textDocumentSync()) {
        if (auto options = std::get_if<TextDocumentSyncOptions>(&*_sync)) {
            if (!options->openClose().value_or(true))
                return;
        }
    }

    d->m_openedDocument[document].document = new QTextDocument(document->document()->toPlainText());
    d->m_openedDocument[document].contentsChangedConnection
        = connect(document,
                  &TextDocument::contentsChangedWithPosition,
                  this,
                  [this, document](int position, int charsRemoved, int charsAdded) {
                      documentContentsChanged(document, position, charsRemoved, charsAdded);
                  });
    d->m_openedDocument[document].filePathChangedConnection
        = connect(document,
                  &TextDocument::filePathChanged,
                  this,
                  [this, document](const FilePath &oldPath, const FilePath &newPath) {
                      if (oldPath == newPath)
                          return;
                      closeDocument(document, oldPath);
                      if (isSupportedDocument(document))
                          openDocument(document);
                  });
    d->m_openedDocument[document].savedConnection
        = connect(document,
                  &TextDocument::saved,
                  this,
                  [this, document](const FilePath &saveFilePath) {
                      if (saveFilePath == document->filePath())
                          documentContentsSaved(document);
                  });
    d->m_openedDocument[document].aboutToSaveConnection
        = connect(document,
                  &TextDocument::aboutToSave,
                  this,
                  [this, document](const FilePath &saveFilePath) {
                      if (saveFilePath == document->filePath())
                          documentWillSave(document);
                  });
    if (!d->m_documentVersions.contains(filePath))
        d->m_documentVersions[filePath] = 0;
    d->sendOpenNotification(filePath, document->mimeType(), document->plainText(),
                            d->m_documentVersions[filePath]);
    handleDocumentOpened(document);

    const Client *currentClient = LanguageClientManager::clientForDocument(document);
    if (currentClient == this) {
        // this is the active client for the document so directly activate it
        activateDocument(document);
    } else if (d->m_activateDocAutomatically && currentClient == nullptr) {
        // there is no client for this document so assign it to this server
        LanguageClientManager::openDocumentWithClient(document, this);
    }
}

void Client::sendMessage(const JsonRpcMessage &message, SendDocUpdates sendUpdates,
                         Schedule semanticTokensSchedule)
{
    QScopeGuard guard([responseHandler = message.responseHandler()](){
        if (responseHandler) {
            static ResponseError<std::nullptr_t> error;
            if (!error.isValid()) {
                error.setCode(-32803); // RequestFailed
                error.setMessage("The server is currently in an unreachable state.");
            }
            QJsonObject response;
            response[idKey] = responseHandler->id;
            response[errorKey] = QJsonObject(error);
            responseHandler->callback(JsonRpcMessage(response));
        }
    });

    QTC_ASSERT(d->m_clientInterface, return);
    if (d->m_state == Shutdown || d->m_state == ShutdownRequested) {
        auto key = message.toJsonObject().contains(methodKey) ? QString(methodKey) : QString(idKey);
        const QString method = message.toJsonObject()[key].toString();
        qCDebug(LOGLSPCLIENT) << "Ignoring message " << method << " because client is shutting down";
        return;
    }
    QTC_ASSERT(d->m_state == Initialized, return);
    guard.dismiss();

    if (sendUpdates == SendDocUpdates::Send)
        d->sendPostponedDocumentUpdates(semanticTokensSchedule);
    if (std::optional<ResponseHandler> responseHandler = message.responseHandler())
        d->m_responseHandlers[responseHandler->id] = responseHandler->callback;
    QString error;
    if (!QTC_GUARD(message.isValid(&error)))
        Core::MessageManager::writeFlashing(error);
    d->sendMessageNow(message);
}

void Client::cancelRequest(const MessageId &id)
{
    d->m_responseHandlers.remove(id);
    if (reachable())
        sendMessage(CancelRequest(CancelParameter(id)), SendDocUpdates::Ignore);
}

void Client::closeDocument(TextEditor::TextDocument *document,
                           const std::optional<FilePath> &overwriteFilePath)
{
    deactivateDocument(document);
    d->m_postponedDocuments.remove(document);
    d->m_documentsToUpdate.erase(document);
    if (d->m_openedDocument.remove(document) != 0) {
        handleDocumentClosed(document);
        if (d->m_state == Initialized)
            d->sendCloseNotification(overwriteFilePath.value_or(document->filePath()));
    }

    if (d->m_state != Initialized)
        return;
    d->closeRequiredShadowDocuments(document);
    const auto shadowIt = d->m_shadowDocuments.find(document->filePath());
    if (shadowIt == d->m_shadowDocuments.constEnd())
        return;
    QTC_CHECK(shadowIt.value().second.isEmpty());
    bool isReferenced = false;
    for (auto it = d->m_openedDocument.cbegin(); it != d->m_openedDocument.cend(); ++it) {
        if (referencesShadowFile(it.key(), shadowIt.key())) {
            d->openShadowDocument(it.key(), shadowIt);
            isReferenced = true;
        }
    }
    if (isReferenced)
        emit shadowDocumentSwitched(document->filePath());
}

void ClientPrivate::updateCompletionProvider(TextEditor::TextDocument *document)
{
    bool useLanguageServer = m_serverCapabilities.completionProvider().has_value();
    auto clientCompletionProvider = static_cast<LanguageClientCompletionAssistProvider *>(
        m_clientProviders.completionAssistProvider.data());
    if (m_dynamicCapabilities.isRegistered(CompletionRequest::methodName).value_or(false)) {
        const QJsonValue &options = m_dynamicCapabilities.option(CompletionRequest::methodName);
        const TextDocumentRegistrationOptions docOptions(options);
        useLanguageServer = docOptions.filterApplies(document->filePath(),
                                                     Utils::mimeTypeForName(document->mimeType()));

        const ServerCapabilities::CompletionOptions completionOptions(options);
        if (completionOptions.isValid())
            clientCompletionProvider->setTriggerCharacters(completionOptions.triggerCharacters());
    }

    if (document->completionAssistProvider() != clientCompletionProvider) {
        if (useLanguageServer) {
            m_resetAssistProvider[document].completionAssistProvider
                = document->completionAssistProvider();
            document->setCompletionAssistProvider(clientCompletionProvider);
        }
    } else if (!useLanguageServer) {
        document->setCompletionAssistProvider(
            m_resetAssistProvider[document].completionAssistProvider);
    }
}

void ClientPrivate::updateFunctionHintProvider(TextEditor::TextDocument *document)
{
    bool useLanguageServer = m_serverCapabilities.signatureHelpProvider().has_value();
    auto clientFunctionHintProvider = static_cast<FunctionHintAssistProvider *>(
        m_clientProviders.functionHintProvider.data());
    if (m_dynamicCapabilities.isRegistered(SignatureHelpRequest::methodName).value_or(false)) {
        const QJsonValue &options = m_dynamicCapabilities.option(SignatureHelpRequest::methodName);
        const TextDocumentRegistrationOptions docOptions(options);
        useLanguageServer = docOptions.filterApplies(document->filePath(),
                                                     Utils::mimeTypeForName(document->mimeType()));

        const ServerCapabilities::SignatureHelpOptions signatureOptions(options);
        if (signatureOptions.isValid())
            clientFunctionHintProvider->setTriggerCharacters(signatureOptions.triggerCharacters());
    }

    if (document->functionHintAssistProvider() != clientFunctionHintProvider) {
        if (useLanguageServer) {
            m_resetAssistProvider[document].functionHintProvider
                = document->functionHintAssistProvider();
            document->setFunctionHintAssistProvider(clientFunctionHintProvider);
        }
    } else if (!useLanguageServer) {
        document->setFunctionHintAssistProvider(
            m_resetAssistProvider[document].functionHintProvider);
    }
}

void ClientPrivate::requestDocumentHighlights(TextEditor::TextEditorWidget *widget)
{
    QTimer *timer = m_documentHighlightsTimer[widget];
    if (!timer) {
        if (m_highlightRequests.contains(widget))
            q->cancelRequest(m_highlightRequests.take(widget));
        timer = new QTimer;
        timer->setSingleShot(true);
        m_documentHighlightsTimer.insert(widget, timer);
        auto connection = connect(widget, &QWidget::destroyed, this, [widget, this]() {
            delete m_documentHighlightsTimer.take(widget);
        });
        connect(timer, &QTimer::timeout, this, [this, widget, connection]() {
            if (q->reachable()) {
                disconnect(connection);
                requestDocumentHighlightsNow(widget);
                m_documentHighlightsTimer.take(widget)->deleteLater();
            } else {
                m_documentHighlightsTimer[widget]->start(250);
            }
        });
    }
    timer->start(250);
}

void ClientPrivate::requestDocumentHighlightsNow(TextEditor::TextEditorWidget *widget)
{
    QTC_ASSERT(q->reachable(), return);
    const auto uri = q->hostPathToServerUri(widget->textDocument()->filePath());
    if (m_dynamicCapabilities.isRegistered(DocumentHighlightsRequest::methodName).value_or(false)) {
        TextDocumentRegistrationOptions option(
            m_dynamicCapabilities.option(DocumentHighlightsRequest::methodName));
        if (!option.filterApplies(widget->textDocument()->filePath()))
            return;
    } else {
        std::optional<std::variant<bool, WorkDoneProgressOptions>> provider
            = m_serverCapabilities.documentHighlightProvider();
        if (!provider.has_value())
            return;
        if (std::holds_alternative<bool>(*provider) && !std::get<bool>(*provider))
            return;
    }

    if (m_highlightRequests.contains(widget))
        q->cancelRequest(m_highlightRequests.take(widget));

    const QTextCursor adjustedCursor = q->adjustedCursorForHighlighting(widget->textCursor(),
                                                                        widget->textDocument());
    DocumentHighlightsRequest request(
        TextDocumentPositionParams(TextDocumentIdentifier(uri), Position{adjustedCursor}));
    auto connection = connect(widget, &QObject::destroyed, this, [this, widget]() {
        if (m_highlightRequests.contains(widget))
            q->cancelRequest(m_highlightRequests.take(widget));
    });
    request.setResponseCallback(
        [widget, this, uri, connection, adjustedCursor]
        (const DocumentHighlightsRequest::Response &response)
        {
            m_highlightRequests.remove(widget);
            disconnect(connection);
            const Id &id = TextEditor::TextEditorWidget::CodeSemanticsSelection;
            QList<QTextEdit::ExtraSelection> selections;
            const std::optional<DocumentHighlightsResult> &result = response.result();
            if (!result.has_value() || std::holds_alternative<std::nullptr_t>(*result)) {
                widget->setExtraSelections(id, selections);
                return;
            }

            const QTextCharFormat &format =
                widget->textDocument()->fontSettings().toTextCharFormat(TextEditor::C_OCCURRENCES);
            QTextDocument *document = widget->document();
            for (const auto &highlight : std::get<QList<DocumentHighlight>>(*result)) {
                QTextEdit::ExtraSelection selection{widget->textCursor(), format};
                const int &start = highlight.range().start().toPositionInDocument(document);
                const int &end = highlight.range().end().toPositionInDocument(document);
                if (start < 0 || end < 0)
                    continue;
                selection.cursor.setPosition(start);
                selection.cursor.setPosition(end, QTextCursor::KeepAnchor);
                selections << selection;
            }
            if (!selections.isEmpty()) {
                const QList<Text::Range> extraRanges = q->additionalDocumentHighlights(
                    widget, adjustedCursor);
                for (const Text::Range &range : extraRanges) {
                    QTextEdit::ExtraSelection selection{widget->textCursor(), format};
                    const Text::Position &startPos = range.begin;
                    const Text::Position &endPos = range.end;
                    const int start = Text::positionInText(document, startPos.line,
                                                           startPos.column + 1);
                    const int end = Text::positionInText(document, endPos.line,
                                                         endPos.column + 1);
                    if (start < 0 || end < 0 || start >= end)
                        continue;
                    selection.cursor.setPosition(start);
                    selection.cursor.setPosition(end, QTextCursor::KeepAnchor);
                    static const auto cmp = [](const QTextEdit::ExtraSelection &s1,
                                        const QTextEdit::ExtraSelection &s2) {
                        return s1.cursor.position() < s2.cursor.position();
                    };
                    const auto it = std::lower_bound(selections.begin(), selections.end(),
                                                     selection, cmp);
                    selections.insert(it, selection);
                }
            }
            widget->setExtraSelections(id, selections);
        });
    m_highlightRequests[widget] = request.id();
    q->sendMessage(request);
}

void Client::activateDocument(TextEditor::TextDocument *document)
{
    const FilePath &filePath = document->filePath();
    if (d->m_diagnosticManager)
        d->m_diagnosticManager->showDiagnostics(filePath, d->m_documentVersions.value(filePath));
    d->m_tokenSupport.updateSemanticTokens(document);
    // only replace the assist provider if the language server support it
    d->updateCompletionProvider(document);
    d->updateFunctionHintProvider(document);
    if (d->m_serverCapabilities.codeActionProvider()) {
        d->m_resetAssistProvider[document].quickFixAssistProvider = document->quickFixAssistProvider();
        document->setQuickFixAssistProvider(d->m_clientProviders.quickFixAssistProvider);
    }
    document->setFormatter(new LanguageClientFormatter(document, this));
    for (Core::IEditor *editor : Core::DocumentModel::editorsForDocument(document))
        activateEditor(editor);
}

void Client::activateEditor(Core::IEditor *editor)
{
    updateEditorToolBar(editor);
    if (editor == Core::EditorManager::currentEditor())
        TextEditor::IOutlineWidgetFactory::updateOutline();
    if (auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor)) {
        TextEditor::TextEditorWidget *widget = textEditor->editorWidget();
        widget->addHoverHandler(&d->m_hoverHandler);
        d->requestDocumentHighlights(widget);
        uint optionalActions = widget->optionalActions();
        if (symbolSupport().supportsFindUsages(widget->textDocument()))
            optionalActions |= TextEditor::TextEditorActionHandler::FindUsage;
        if (symbolSupport().supportsRename(widget->textDocument()))
            optionalActions |= TextEditor::TextEditorActionHandler::RenameSymbol;
        if (symbolSupport().supportsFindLink(widget->textDocument(), LinkTarget::SymbolDef))
            optionalActions |= TextEditor::TextEditorActionHandler::FollowSymbolUnderCursor;
        if (symbolSupport().supportsFindLink(widget->textDocument(), LinkTarget::SymbolTypeDef))
            optionalActions |= TextEditor::TextEditorActionHandler::FollowTypeUnderCursor;
        if (CallHierarchyFactory::supportsCallHierarchy(this, textEditor->document()))
            optionalActions |= TextEditor::TextEditorActionHandler::CallHierarchy;
        widget->setOptionalActions(optionalActions);
    }
}

void Client::deactivateDocument(TextEditor::TextDocument *document)
{
    if (d->m_diagnosticManager)
        d->m_diagnosticManager->hideDiagnostics(document->filePath());
    d->resetAssistProviders(document);
    document->setFormatter(nullptr);
    d->m_tokenSupport.clearHighlight(document);
    for (Core::IEditor *editor : Core::DocumentModel::editorsForDocument(document)) {
        if (auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor)) {
            TextEditor::TextEditorWidget *widget = textEditor->editorWidget();
            widget->removeHoverHandler(&d->m_hoverHandler);
            widget->setExtraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection, {});
            widget->clearRefactorMarkers(id());
            updateEditorToolBar(editor);
        }
    }
}

void ClientPrivate::documentClosed(Core::IDocument *document)
{
    if (auto textDocument = qobject_cast<TextEditor::TextDocument *>(document))
        q->closeDocument(textDocument);
}

void ClientPrivate::sendOpenNotification(const FilePath &filePath, const QString &mimeType,
                                         const QString &content, int version)
{
    TextDocumentItem item;
    item.setLanguageId(TextDocumentItem::mimeTypeToLanguageId(mimeType));
    item.setUri(q->hostPathToServerUri(filePath));
    item.setText(content);
    item.setVersion(version);
    q->sendMessage(DidOpenTextDocumentNotification(DidOpenTextDocumentParams(item)),
                   Client::SendDocUpdates::Ignore);
}

void ClientPrivate::sendCloseNotification(const FilePath &filePath)
{
    q->sendMessage(DidCloseTextDocumentNotification(DidCloseTextDocumentParams(
            TextDocumentIdentifier{q->hostPathToServerUri(filePath)})),
                   Client::SendDocUpdates::Ignore);
}

void ClientPrivate::openRequiredShadowDocuments(const TextEditor::TextDocument *doc)
{
    for (auto it = m_shadowDocuments.begin(); it != m_shadowDocuments.end(); ++it) {
        if (!it.value().second.contains(doc) && q->referencesShadowFile(doc, it.key()))
            openShadowDocument(doc, it);
    }
}

void ClientPrivate::closeRequiredShadowDocuments(const TextEditor::TextDocument *doc)
{
    for (auto it = m_shadowDocuments.begin(); it != m_shadowDocuments.end(); ++it) {
        if (it.value().second.removeOne(doc) && it.value().second.isEmpty())
            closeShadowDocument(it);
    }
}

bool Client::documentOpen(const TextEditor::TextDocument *document) const
{
    return d->m_openedDocument.contains(const_cast<TextEditor::TextDocument *>(document));
}

TextEditor::TextDocument *Client::documentForFilePath(const Utils::FilePath &file) const
{
    for (auto it = d->m_openedDocument.cbegin(); it != d->m_openedDocument.cend(); ++it) {
        if (it.key()->filePath() == file)
            return it.key();
    }
    return nullptr;
}

void Client::setShadowDocument(const Utils::FilePath &filePath, const QString &content)
{
    QTC_ASSERT(reachable(), return);
    auto shadowIt = d->m_shadowDocuments.find(filePath);
    if (shadowIt == d->m_shadowDocuments.end()) {
        shadowIt = d->m_shadowDocuments.insert(filePath, {content, {}});
    } else  {
        if (shadowIt.value().first == content)
            return;
        shadowIt.value().first = content;
        if (!shadowIt.value().second.isEmpty()) {
            VersionedTextDocumentIdentifier docId(hostPathToServerUri(filePath));
            docId.setVersion(++d->m_documentVersions[filePath]);
            const DidChangeTextDocumentParams params(docId, content);
            sendMessage(DidChangeTextDocumentNotification(params), SendDocUpdates::Ignore);
            return;
        }
    }
    if (documentForFilePath(filePath))
        return;
    for (auto docIt = d->m_openedDocument.cbegin(); docIt != d->m_openedDocument.cend(); ++docIt) {
        if (referencesShadowFile(docIt.key(), filePath))
            d->openShadowDocument(docIt.key(), shadowIt);
    }
}

void Client::removeShadowDocument(const Utils::FilePath &filePath)
{
    const auto it = d->m_shadowDocuments.find(filePath);
    if (it == d->m_shadowDocuments.end())
        return;
    if (!it.value().second.isEmpty())
        d->closeShadowDocument(it);
    d->m_shadowDocuments.erase(it);
}

void ClientPrivate::openShadowDocument(const TextEditor::TextDocument *requringDoc,
                                       ShadowDocIterator shadowIt)
{
    shadowIt.value().second << requringDoc;
    if (shadowIt.value().second.size() > 1)
        return;
    const QString mimeType = mimeTypeForFile(shadowIt.key(), MimeMatchMode::MatchExtension).name();
    sendOpenNotification(shadowIt.key(), mimeType, shadowIt.value().first,
                         ++m_documentVersions[shadowIt.key()]);
}

void ClientPrivate::closeShadowDocument(ShadowDocIterator shadowIt)
{
    sendCloseNotification(shadowIt.key());
    shadowIt.value().second.clear();
}

void Client::documentContentsSaved(TextEditor::TextDocument *document)
{
    if (!d->m_openedDocument.contains(document))
        return;
    bool send = true;
    bool includeText = false;
    const QString method(DidSaveTextDocumentNotification::methodName);
    if (std::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        send = *registered;
        if (send) {
            const TextDocumentSaveRegistrationOptions option(
                        d->m_dynamicCapabilities.option(method).toObject());
            if (option.isValid()) {
                send = option.filterApplies(document->filePath(),
                                                   Utils::mimeTypeForName(document->mimeType()));
                includeText = option.includeText().value_or(includeText);
            }
        }
    } else if (std::optional<ServerCapabilities::TextDocumentSync> _sync
               = d->m_serverCapabilities.textDocumentSync()) {
        if (auto options = std::get_if<TextDocumentSyncOptions>(&*_sync)) {
            if (std::optional<SaveOptions> saveOptions = options->save())
                includeText = saveOptions->includeText().value_or(includeText);
        }
    }
    if (!send)
        return;
    DidSaveTextDocumentParams params(
                TextDocumentIdentifier(hostPathToServerUri(document->filePath())));
    d->openRequiredShadowDocuments(document);
    if (includeText)
        params.setText(document->plainText());
    sendMessage(DidSaveTextDocumentNotification(params), SendDocUpdates::Send, Schedule::Now);
}

void Client::documentWillSave(Core::IDocument *document)
{
    const FilePath &filePath = document->filePath();
    auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);
    if (!d->m_openedDocument.contains(textDocument))
        return;
    bool send = false;
    const QString method(WillSaveTextDocumentNotification::methodName);
    if (std::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        send = *registered;
        if (send) {
            const TextDocumentRegistrationOptions option(d->m_dynamicCapabilities.option(method));
            if (option.isValid()) {
                send = option.filterApplies(filePath,
                                                   Utils::mimeTypeForName(document->mimeType()));
            }
        }
    } else if (std::optional<ServerCapabilities::TextDocumentSync> _sync
               = d->m_serverCapabilities.textDocumentSync()) {
        if (auto options = std::get_if<TextDocumentSyncOptions>(&*_sync))
            send = options->willSave().value_or(send);
    }
    if (!send)
        return;
    const WillSaveTextDocumentParams params(
        TextDocumentIdentifier(hostPathToServerUri(filePath)));
    sendMessage(WillSaveTextDocumentNotification(params));
}

void Client::documentContentsChanged(TextEditor::TextDocument *document,
                                     int position,
                                     int charsRemoved,
                                     int charsAdded)
{
    if (!d->m_openedDocument.contains(document) || !reachable())
        return;
    if (d->m_runningFindLinkRequest.isValid())
        cancelRequest(d->m_runningFindLinkRequest);
    if (d->m_diagnosticManager)
        d->m_diagnosticManager->disableDiagnostics(document);
    const QString method(DidChangeTextDocumentNotification::methodName);
    TextDocumentSyncKind syncKind = d->m_serverCapabilities.textDocumentSyncKindHelper();
    if (std::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        syncKind = *registered ? TextDocumentSyncKind::Full : TextDocumentSyncKind::None;
        if (syncKind != TextDocumentSyncKind::None) {
            const TextDocumentChangeRegistrationOptions option(
                                    d->m_dynamicCapabilities.option(method).toObject());
            syncKind = option.isValid() ? option.syncKind() : syncKind;
        }
    }

    const QString &text = document->textAt(position, charsAdded);
    QTextCursor cursor(d->m_openedDocument[document].document);
    // Workaround https://bugreports.qt.io/browse/QTBUG-80662
    // The contentsChanged gives a character count that can be wrong for QTextCursor
    // when there are special characters removed/added (like formating characters).
    // Also, characterCount return the number of characters + 1 because of the hidden
    // paragraph separator character.
    // This implementation is based on QWidgetTextControlPrivate::_q_contentsChanged.
    // For charsAdded, textAt handles the case itself.
    cursor.setPosition(qMin(d->m_openedDocument[document].document->characterCount() - 1,
                            position + charsRemoved));
    cursor.setPosition(position, QTextCursor::KeepAnchor);

    if (syncKind != TextDocumentSyncKind::None) {
        if (syncKind == TextDocumentSyncKind::Incremental) {
            // If the new change is a pure insertion and its range is adjacent to the range of the
            // previous change, we can trivially merge the two changes.
            // For the typical case of the user typing a continuous sequence of characters,
            // this will save a lot of TextDocumentContentChangeEvent elements in the data stream,
            // as otherwise we'd send tons of single-character changes.
            auto &queue = d->m_documentsToUpdate[document];
            bool append = true;
            if (!queue.isEmpty() && charsRemoved == 0) {
                auto &prev = queue.last();
                const int prevStart = prev.range()->start()
                        .toPositionInDocument(document->document());
                if (prevStart + prev.text().length() == position) {
                    prev.setText(prev.text() + text);
                    append = false;
                }
            }
            if (append) {
                DidChangeTextDocumentParams::TextDocumentContentChangeEvent change;
                change.setRange(Range(cursor));
                change.setRangeLength(cursor.selectionEnd() - cursor.selectionStart());
                change.setText(text);
                queue << change;
            }
        } else {
            d->m_documentsToUpdate[document] = {
                DidChangeTextDocumentParams::TextDocumentContentChangeEvent(document->plainText())};
        }
    }
    cursor.insertText(text);

    ++d->m_documentVersions[document->filePath()];
    using namespace TextEditor;
    for (BaseTextEditor *editor : BaseTextEditor::textEditorsForDocument(document)) {
        TextEditorWidget *widget = editor->editorWidget();
        QTC_ASSERT(widget, continue);
        delete d->m_documentHighlightsTimer.take(widget);
        widget->clearRefactorMarkers(id());
    }
    d->m_documentUpdateTimer.start();
}

void Client::registerCapabilities(const QList<Registration> &registrations)
{
    d->m_dynamicCapabilities.registerCapability(registrations);
    for (const Registration &registration : registrations) {
        if (registration.method() == CompletionRequest::methodName) {
            for (auto document : d->m_openedDocument.keys())
                d->updateCompletionProvider(document);
        }
        if (registration.method() == SignatureHelpRequest::methodName) {
            for (auto document : d->m_openedDocument.keys())
                d->updateFunctionHintProvider(document);
        }
        if (registration.method() == "textDocument/semanticTokens") {
            SemanticTokensOptions options(registration.registerOptions());
            if (options.isValid())
                d->m_tokenSupport.setLegend(options.legend());
            for (auto document : d->m_openedDocument.keys())
                d->m_tokenSupport.updateSemanticTokens(document);
        }
    }
    emit capabilitiesChanged(d->m_dynamicCapabilities);
}

void Client::unregisterCapabilities(const QList<Unregistration> &unregistrations)
{
    d->m_dynamicCapabilities.unregisterCapability(unregistrations);
    for (const Unregistration &unregistration : unregistrations) {
        if (unregistration.method() == CompletionRequest::methodName) {
            for (auto document : d->m_openedDocument.keys())
                d->updateCompletionProvider(document);
        }
        if (unregistration.method() == SignatureHelpRequest::methodName) {
            for (auto document : d->m_openedDocument.keys())
                d->updateFunctionHintProvider(document);
        }
        if (unregistration.method() == "textDocument/semanticTokens") {
            for (auto document : d->m_openedDocument.keys())
                d->m_tokenSupport.updateSemanticTokens(document);
        }
    }
    emit capabilitiesChanged(d->m_dynamicCapabilities);
}

void Client::setLocatorsEnabled(bool enabled)
{
    d->m_locatorsEnabled = enabled;
}

bool Client::locatorsEnabled() const
{
    return d->m_locatorsEnabled;
}

void Client::setAutoRequestCodeActions(bool enabled)
{
    d->m_autoRequestCodeActions = enabled;
}

TextEditor::HighlightingResult createHighlightingResult(const SymbolInformation &info)
{
    if (!info.isValid())
        return {};
    const Position &start = info.location().range().start();
    return TextEditor::HighlightingResult(start.line() + 1,
                                          start.character() + 1,
                                          info.name().length(),
                                          info.kind());
}

void Client::cursorPositionChanged(TextEditor::TextEditorWidget *widget)
{
    if (d->m_runningFindLinkRequest.isValid())
        cancelRequest(d->m_runningFindLinkRequest);
    TextEditor::TextDocument *document = widget->textDocument();
    if (d->m_documentsToUpdate.find(document) != d->m_documentsToUpdate.end())
        return; // we are currently changing this document so postpone the DocumentHighlightsRequest
    d->requestDocumentHighlights(widget);
    const Id selectionsId(TextEditor::TextEditorWidget::CodeSemanticsSelection);
    const QList semanticSelections = widget->extraSelections(selectionsId);
    if (!semanticSelections.isEmpty()) {
        auto selectionContainsPos =
            [pos = widget->position()](const QTextEdit::ExtraSelection &selection) {
                const QTextCursor cursor = selection.cursor;
                return cursor.selectionStart() <= pos && cursor.selectionEnd() >= pos;
            };
        if (!Utils::anyOf(semanticSelections, selectionContainsPos))
            widget->setExtraSelections(selectionsId, {});
    }
}

SymbolSupport &Client::symbolSupport()
{
    return d->m_symbolSupport;
}

void Client::findLinkAt(TextEditor::TextDocument *document,
                        const QTextCursor &cursor,
                        Utils::LinkHandler callback,
                        const bool resolveTarget,
                        LinkTarget target)
{
    if (d->m_runningFindLinkRequest.isValid())
        cancelRequest(d->m_runningFindLinkRequest);
    d->m_runningFindLinkRequest = symbolSupport().findLinkAt(
        document,
        cursor,
        [this, callback](const Link &link) {
            d->m_runningFindLinkRequest = {};
            callback(link);
        },
        resolveTarget,
        target);
}

void Client::requestCodeActions(const LanguageServerProtocol::DocumentUri &uri,
                                const LanguageServerProtocol::Diagnostic &diagnostic)
{
    d->requestCodeActions(uri, diagnostic.range(), {diagnostic});
}

void Client::requestCodeActions(const DocumentUri &uri, const QList<Diagnostic> &diagnostics)
{
    d->requestCodeActions(uri, {}, diagnostics);
}

void ClientPrivate::requestCodeActions(const DocumentUri &uri,
                                       const Range &range,
                                       const QList<Diagnostic> &diagnostics)
{
    const Utils::FilePath fileName = q->serverUriToHostPath(uri);
    TextEditor::TextDocument *doc = TextEditor::TextDocument::textDocumentForFilePath(fileName);
    if (!doc)
        return;

    CodeActionParams codeActionParams;
    CodeActionParams::CodeActionContext context;
    context.setDiagnostics(diagnostics);
    codeActionParams.setContext(context);
    codeActionParams.setTextDocument(TextDocumentIdentifier(uri));
    if (range.isEmpty()) {
        Position start(0, 0);
        const QTextBlock &lastBlock = doc->document()->lastBlock();
        Position end(lastBlock.blockNumber(), lastBlock.length() - 1);
        codeActionParams.setRange(Range(start, end));
    } else {
        codeActionParams.setRange(range);
    }
    CodeActionRequest request(codeActionParams);
    request.setResponseCallback(
        [uri, q = QPointer<Client>(q)](const CodeActionRequest::Response &response) {
        if (q)
            q->handleCodeActionResponse(response, uri);
    });
    q->requestCodeActions(request);
}

void Client::requestCodeActions(const CodeActionRequest &request)
{
    if (!request.isValid(nullptr))
        return;

    const Utils::FilePath fileName = request.params()
                                         .value_or(CodeActionParams())
                                         .textDocument()
                                         .uri()
                                         .toFilePath(hostPathMapper());

    const QString method(CodeActionRequest::methodName);
    if (std::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        if (!*registered)
            return;
        const TextDocumentRegistrationOptions option(
            d->m_dynamicCapabilities.option(method).toObject());
        if (option.isValid() && !option.filterApplies(fileName))
            return;
    } else {
        std::variant<bool, CodeActionOptions> provider
            = d->m_serverCapabilities.codeActionProvider().value_or(false);
        if (!(std::holds_alternative<CodeActionOptions>(provider) || std::get<bool>(provider)))
            return;
    }

    sendMessage(request);
}

void Client::handleCodeActionResponse(const CodeActionRequest::Response &response,
                                          const DocumentUri &uri)
{
    if (const std::optional<CodeActionRequest::Response::Error> &error = response.error())
        log(*error);
    if (const std::optional<CodeActionResult> &result = response.result()) {
        if (auto list = std::get_if<QList<std::variant<Command, CodeAction>>>(&*result)) {
            QList<CodeAction> codeActions;
            for (const std::variant<Command, CodeAction> &item : *list) {
                if (auto action = std::get_if<CodeAction>(&item))
                    codeActions << *action;
                else if (auto command = std::get_if<Command>(&item))
                    Q_UNUSED(command) // todo
            }
            updateCodeActionRefactoringMarker(this, codeActions, uri);
        }
    }
}

void Client::executeCommand(const Command &command)
{
    bool serverSupportsExecuteCommand = d->m_serverCapabilities.executeCommandProvider().has_value();
    serverSupportsExecuteCommand = d->m_dynamicCapabilities
                                       .isRegistered(ExecuteCommandRequest::methodName)
                                       .value_or(serverSupportsExecuteCommand);
    if (serverSupportsExecuteCommand)
        sendMessage(ExecuteCommandRequest(ExecuteCommandParams(command)));
}

ProjectExplorer::Project *Client::project() const
{
    return d->m_project;
}

void Client::setCurrentProject(ProjectExplorer::Project *project)
{
    QTC_ASSERT(canOpenProject(project), return);
    if (d->m_project == project)
        return;
    if (d->m_project)
        d->m_project->disconnect(this);
    d->m_project = project;
    if (d->m_project) {
        connect(d->m_project, &ProjectExplorer::Project::destroyed, this, [this]() {
            // the project of the client should already be null since we expect the session and
            // the language client manager to reset it before it gets deleted.
            QTC_ASSERT(d->m_project == nullptr, projectClosed(d->m_project));
        });
    }
}

void Client::projectOpened(ProjectExplorer::Project *project)
{
    if (!d->sendWorkspceFolderChanges() || !canOpenProject(project))
        return;
    WorkspaceFoldersChangeEvent event;
    event.setAdded({WorkSpaceFolder(hostPathToServerUri(project->projectDirectory()),
                                    project->displayName())});
    DidChangeWorkspaceFoldersParams params;
    params.setEvent(event);
    DidChangeWorkspaceFoldersNotification change(params);
    sendMessage(change);
}

void Client::projectClosed(ProjectExplorer::Project *project)
{
    if (d->sendWorkspceFolderChanges() && canOpenProject(project)) {
        WorkspaceFoldersChangeEvent event;
        event.setRemoved({WorkSpaceFolder(hostPathToServerUri(project->projectDirectory()),
                                          project->displayName())});
        DidChangeWorkspaceFoldersParams params;
        params.setEvent(event);
        DidChangeWorkspaceFoldersNotification change(params);
        sendMessage(change);
    }
    if (project == d->m_project) {
        if (d->m_state == Initialized) {
            shutdown();
        } else {
            d->m_state = Shutdown; // otherwise the manager would try to restart this server
            emit finished();
        }
        d->m_project = nullptr;
    }
}

bool Client::canOpenProject(ProjectExplorer::Project *project)
{
    Q_UNUSED(project);
    return true;
}

void Client::updateConfiguration(const QJsonValue &configuration)
{
    d->m_configuration = configuration;
    if (reachable() && !configuration.isNull()
        && d->m_dynamicCapabilities.isRegistered(DidChangeConfigurationNotification::methodName)
               .value_or(true)) {
        DidChangeConfigurationParams params;
        params.setSettings(configuration);
        DidChangeConfigurationNotification notification(params);
        sendMessage(notification);
    }
}

void Client::setSupportedLanguage(const LanguageFilter &filter)
{
    d->m_languagFilter = filter;
}

void Client::setActivateDocumentAutomatically(bool enabled)
{
    d->m_activateDocAutomatically = enabled;
}

void Client::setInitializationOptions(const QJsonObject &initializationOptions)
{
    d->m_initializationOptions = initializationOptions;
}

bool Client::isSupportedDocument(const TextEditor::TextDocument *document) const
{
    QTC_ASSERT(document, return false);
    return d->m_languagFilter.isSupported(document);
}

bool Client::isSupportedFile(const Utils::FilePath &filePath, const QString &mimeType) const
{
    return d->m_languagFilter.isSupported(filePath, mimeType);
}

bool Client::isSupportedUri(const DocumentUri &uri) const
{
    const FilePath &filePath = serverUriToHostPath(uri);
    return d->m_languagFilter.isSupported(filePath, Utils::mimeTypeForFile(filePath).name());
}

void Client::addAssistProcessor(TextEditor::IAssistProcessor *processor)
{
    d->m_runningAssistProcessors.insert(processor);
}

void Client::removeAssistProcessor(TextEditor::IAssistProcessor *processor)
{
    d->m_runningAssistProcessors.remove(processor);
}

QList<Diagnostic> Client::diagnosticsAt(const FilePath &filePath, const QTextCursor &cursor) const
{
    if (d->m_diagnosticManager)
        return d->m_diagnosticManager->diagnosticsAt(filePath, cursor);
    return {};
}

bool Client::hasDiagnostic(const FilePath &filePath,
                           const LanguageServerProtocol::Diagnostic &diag) const
{
    if (d->m_diagnosticManager)
        return d->m_diagnosticManager->hasDiagnostic(filePath, documentForFilePath(filePath), diag);
    return false;
}

bool Client::hasDiagnostics(const TextEditor::TextDocument *document) const
{
    if (d->m_diagnosticManager)
        return d->m_diagnosticManager->hasDiagnostics(document);
    return false;
}

DiagnosticManager *Client::createDiagnosticManager()
{
    return new DiagnosticManager(this);
}

void Client::setSemanticTokensHandler(const SemanticTokensHandler &handler)
{
    d->m_tokenSupport.setTokensHandler(handler);
}

void Client::setSnippetsGroup(const QString &group)
{
    if (const auto provider = qobject_cast<LanguageClientCompletionAssistProvider *>(
                d->m_clientProviders.completionAssistProvider)) {
        provider->setSnippetsGroup(group);
    }
}

void Client::setCompletionAssistProvider(LanguageClientCompletionAssistProvider *provider)
{
    delete d->m_clientProviders.completionAssistProvider;
    d->m_clientProviders.completionAssistProvider = provider;
}

void Client::setQuickFixAssistProvider(LanguageClientQuickFixProvider *provider)
{
    delete d->m_clientProviders.quickFixAssistProvider;
    d->m_clientProviders.quickFixAssistProvider = provider;
}

bool Client::supportsDocumentSymbols(const TextEditor::TextDocument *doc) const
{
    if (!doc || !reachable())
        return false;
    DynamicCapabilities dc = dynamicCapabilities();
    if (dc.isRegistered(DocumentSymbolsRequest::methodName).value_or(false)) {
        TextDocumentRegistrationOptions options(dc.option(DocumentSymbolsRequest::methodName));
        return !options.isValid()
               || options.filterApplies(doc->filePath(), Utils::mimeTypeForName(doc->mimeType()));
    }
    const std::optional<std::variant<bool, WorkDoneProgressOptions>> &provider
        = capabilities().documentSymbolProvider();
    if (!provider.has_value())
        return false;
    if (std::holds_alternative<bool>(*provider))
        return std::get<bool>(*provider);
    return true;
}

void Client::setLogTarget(LogTarget target)
{
    d->m_logTarget = target;
}

void Client::start()
{
    d->m_shutdownTimer.stop();
    LanguageClientManager::addClient(this);
    d->m_clientInterface->start();
}

bool Client::reset()
{
    return d->reset();
}

bool ClientPrivate::reset()
{
    if (!m_restartsLeft) {
        m_restartCountResetTimer.stop();
        return false;
    }
    m_restartCountResetTimer.start();
    --m_restartsLeft;
    m_state = Client::Uninitialized;
    m_responseHandlers.clear();
    m_clientInterface->resetBuffer();
    updateEditorToolBar(m_openedDocument.keys());
    m_serverCapabilities = ServerCapabilities();
    m_dynamicCapabilities.reset();
    if (m_diagnosticManager)
        m_diagnosticManager->clearDiagnostics();
    m_openedDocument.clear();
    // temporary container needed since m_resetAssistProvider is changed in resetAssistProviders
    for (TextEditor::TextDocument *document : m_resetAssistProvider.keys())
        resetAssistProviders(document);
    for (TextEditor::IAssistProcessor *processor : std::as_const(m_runningAssistProcessors))
        processor->setAsyncProposalAvailable(nullptr);
    m_runningAssistProcessors.clear();
    qDeleteAll(m_documentHighlightsTimer);
    m_documentHighlightsTimer.clear();
    m_progressManager.reset();
    for (auto &doc : m_shadowDocuments)
        doc.second.clear();
    m_documentVersions.clear();
    return true;
}

void Client::setError(const QString &message)
{
    log(message);
    d->m_state = d->m_state < Initialized ? FailedToInitialize : Error;
}

ProgressManager *Client::progressManager()
{
    return &d->m_progressManager;
}

void Client::handleMessage(const LanguageServerProtocol::JsonRpcMessage &message)
{
    LanguageClientManager::logJsonRpcMessage(LspLogMessage::ServerMessage, name(), message);
    const MessageId id(message.toJsonObject().value(idKey));
    const QString method = message.toJsonObject().value(methodKey).toString();
    if (method.isEmpty())
        d->handleResponse(id, message);
    else
        d->handleMethod(method, id, message);
}

void Client::log(const QString &message) const
{
    switch (d->m_logTarget) {
    case LogTarget::Ui:
        Core::MessageManager::writeFlashing(QString("LanguageClient %1: %2").arg(name(), message));
        break;
    case LogTarget::Console:
        qCDebug(LOGLSPCLIENT) << message;
        break;
    }
}

TextEditor::RefactoringChangesData *Client::createRefactoringChangesBackend() const
{
    return new TextEditor::RefactoringChangesData;
}

void Client::setCompletionResultsLimit(int limit)
{
    d->m_completionResultsLimit = limit;
}

int Client::completionResultsLimit() const
{
    return d->m_completionResultsLimit;
}

const ServerCapabilities &Client::capabilities() const
{
    return d->m_serverCapabilities;
}

QString Client::serverName() const
{
    return d->m_serverName;
}

QString Client::serverVersion() const
{
    return d->m_serverVersion;
}

const DynamicCapabilities &Client::dynamicCapabilities() const
{
    return d->m_dynamicCapabilities;
}

DocumentSymbolCache *Client::documentSymbolCache()
{
    return &d->m_documentSymbolCache;
}

HoverHandler *Client::hoverHandler()
{
    return &d->m_hoverHandler;
}

void ClientPrivate::log(const ShowMessageParams &message)
{
    q->log(message.toString());
}

LanguageClientValue<MessageActionItem> ClientPrivate::showMessageBox(
    const ShowMessageRequestParams &message)
{
    QMessageBox box;
    box.setText(message.toString());
    switch (message.type()) {
    case Error:
        box.setIcon(QMessageBox::Critical);
        break;
    case Warning:
        box.setIcon(QMessageBox::Warning);
        break;
    case Info:
        box.setIcon(QMessageBox::Information);
        break;
    case Log:
        box.setIcon(QMessageBox::NoIcon);
        break;
    }

    QHash<QAbstractButton *, MessageActionItem> itemForButton;
    if (const std::optional<QList<MessageActionItem>> actions = message.actions()) {
        for (const MessageActionItem &action : *actions) {
            auto button = box.addButton(action.title(), QMessageBox::ActionRole);
            connect(button, &QPushButton::clicked, &box, &QMessageBox::accept);
            itemForButton.insert(button, action);
        }
    }

    if (box.exec() == QDialog::Rejected || itemForButton.isEmpty())
        return {};
    const MessageActionItem &item = itemForButton.value(box.clickedButton());
    return item.isValid() ? LanguageClientValue<MessageActionItem>(item)
                          : LanguageClientValue<MessageActionItem>();
}

void ClientPrivate::resetAssistProviders(TextEditor::TextDocument *document)
{
    const AssistProviders providers = m_resetAssistProvider.take(document);

    if (document->completionAssistProvider() == m_clientProviders.completionAssistProvider)
        document->setCompletionAssistProvider(providers.completionAssistProvider);

    if (document->functionHintAssistProvider() == m_clientProviders.functionHintProvider)
        document->setFunctionHintAssistProvider(providers.functionHintProvider);

    if (document->quickFixAssistProvider() == m_clientProviders.quickFixAssistProvider)
        document->setQuickFixAssistProvider(providers.quickFixAssistProvider);
}

void ClientPrivate::sendPostponedDocumentUpdates(Schedule semanticTokensSchedule)
{
    m_documentUpdateTimer.stop();
    if (m_documentsToUpdate.empty())
        return;
    TextEditor::TextEditorWidget *currentWidget
        = TextEditor::TextEditorWidget::currentTextEditorWidget();

    struct DocumentUpdate
    {
        TextEditor::TextDocument *document;
        DidChangeTextDocumentNotification notification;
    };
    const auto updates = Utils::transform<QList<DocumentUpdate>>(m_documentsToUpdate,
                                                                 [this](const auto &elem) {
        TextEditor::TextDocument * const document = elem.first;
        const FilePath &filePath = document->filePath();
        const LanguageServerProtocol::DocumentUri uri = q->hostPathToServerUri(filePath);
        VersionedTextDocumentIdentifier docId(uri);
        docId.setVersion(m_documentVersions[filePath]);
        DidChangeTextDocumentParams params;
        params.setTextDocument(docId);
        params.setContentChanges(elem.second);
        return DocumentUpdate{document, DidChangeTextDocumentNotification(params)};
    });
    m_documentsToUpdate.clear();

    for (const DocumentUpdate &update : updates) {
        q->sendMessage(update.notification, Client::SendDocUpdates::Ignore);
        emit q->documentUpdated(update.document);

        if (currentWidget && currentWidget->textDocument() == update.document)
            requestDocumentHighlights(currentWidget);

        switch (semanticTokensSchedule) {
        case Schedule::Now:
            m_tokenSupport.updateSemanticTokens(update.document);
            break;
        case Schedule::Delayed:
            QTimer::singleShot(m_documentUpdateTimer.interval(), this,
                               [this, doc = QPointer(update.document)] {
                if (doc && m_documentsToUpdate.find(doc) == m_documentsToUpdate.end())
                    m_tokenSupport.updateSemanticTokens(doc);
            });
            break;
        }
    }
}

void ClientPrivate::handleResponse(const MessageId &id, const JsonRpcMessage &message)
{
    if (auto handler = m_responseHandlers.take(id))
        handler(message);
}

template<typename T>
static ResponseError<T> createInvalidParamsError(const QString &message)
{
    ResponseError<T> error;
    error.setMessage(message);
    error.setCode(ResponseError<T>::InvalidParams);
    return error;
}

void ClientPrivate::handleMethod(const QString &method, const MessageId &id, const JsonRpcMessage &message)
{
    auto invalidParamsErrorMessage = [&](const JsonObject &params) {
        return Tr::tr("Invalid parameter in \"%1\":\n%2")
            .arg(method, QString::fromUtf8(QJsonDocument(params).toJson(QJsonDocument::Indented)));
    };

    auto createDefaultResponse = [&]() {
        Response<std::nullptr_t, JsonObject> response;
        if (QTC_GUARD(id.isValid()))
            response.setId(id);
        response.setResult(nullptr);
        return response;
    };

    const bool isRequest = id.isValid();

    bool responseSend = false;
    auto sendResponse =
        [&](const JsonRpcMessage &response) {
            responseSend = true;
            if (q->reachable()) {
                q->sendMessage(response);
            } else {
                qCDebug(LOGLSPCLIENT)
                    << QString("Dropped response to request %1 id %2 for unreachable server %3")
                           .arg(method, id.toString(), q->name());
            }
        };

    if (method == PublishDiagnosticsNotification::methodName) {
        auto params = PublishDiagnosticsNotification(message.toJsonObject()).params().value_or(
            PublishDiagnosticsParams());
        if (params.isValid())
            q->handleDiagnostics(params);
        else
            q->log(invalidParamsErrorMessage(params));
    } else if (method == LogMessageNotification::methodName) {
        auto params = LogMessageNotification(message.toJsonObject()).params().value_or(
            LogMessageParams());
        if (params.isValid())
            log(params);
        else
            q->log(invalidParamsErrorMessage(params));
    } else if (method == ShowMessageNotification::methodName) {
        auto params = ShowMessageNotification(message.toJsonObject()).params().value_or(
            ShowMessageParams());
        if (params.isValid())
            log(params);
        else
            q->log(invalidParamsErrorMessage(params));
    } else if (method == ShowMessageRequest::methodName) {
        auto request = ShowMessageRequest(message.toJsonObject());
        ShowMessageRequest::Response response(id);
        auto params = request.params().value_or(ShowMessageRequestParams());
        if (params.isValid()) {
            response.setResult(showMessageBox(params));
        } else {
            const QString errorMessage = invalidParamsErrorMessage(params);
            q->log(errorMessage);
            response.setError(createInvalidParamsError<std::nullptr_t>(errorMessage));
        }
        sendResponse(response);
    } else if (method == RegisterCapabilityRequest::methodName) {
        auto params = RegisterCapabilityRequest(message.toJsonObject()).params().value_or(
            RegistrationParams());
        if (params.isValid()) {
            q->registerCapabilities(params.registrations());
            sendResponse(createDefaultResponse());
        } else {
            const QString errorMessage = invalidParamsErrorMessage(params);
            q->log(invalidParamsErrorMessage(params));
            RegisterCapabilityRequest::Response response(id);
            response.setError(createInvalidParamsError<std::nullptr_t>(errorMessage));
            sendResponse(response);
        }
    } else if (method == UnregisterCapabilityRequest::methodName) {
        auto params = UnregisterCapabilityRequest(message.toJsonObject()).params().value_or(
            UnregistrationParams());
        if (params.isValid()) {
            q->unregisterCapabilities(params.unregistrations());
            sendResponse(createDefaultResponse());
        } else {
            const QString errorMessage = invalidParamsErrorMessage(params);
            q->log(invalidParamsErrorMessage(params));
            UnregisterCapabilityRequest::Response response(id);
            response.setError(createInvalidParamsError<std::nullptr_t>(errorMessage));
            sendResponse(response);
        }
    } else if (method == ApplyWorkspaceEditRequest::methodName) {
        ApplyWorkspaceEditRequest::Response response(id);
        auto params = ApplyWorkspaceEditRequest(message.toJsonObject()).params().value_or(
            ApplyWorkspaceEditParams());
        if (params.isValid()) {
            ApplyWorkspaceEditResult result;
            result.setApplied(applyWorkspaceEdit(q, params.edit()));
            response.setResult(result);
        } else {
            const QString errorMessage = invalidParamsErrorMessage(params);
            q->log(errorMessage);
            response.setError(createInvalidParamsError<std::nullptr_t>(errorMessage));
        }
        sendResponse(response);
    } else if (method == WorkSpaceFolderRequest::methodName) {
        WorkSpaceFolderRequest::Response response(id);
        const QList<ProjectExplorer::Project *> projects
            = ProjectExplorer::ProjectManager::projects();
        if (projects.isEmpty()) {
            response.setResult(nullptr);
        } else {
            response.setResult(Utils::transform(
                projects,
                [this](ProjectExplorer::Project *project) {
                    return WorkSpaceFolder(q->hostPathToServerUri(project->projectDirectory()),
                                           project->displayName());
                }));
        }
        sendResponse(response);
    } else if (method == WorkDoneProgressCreateRequest::methodName) {
        sendResponse(createDefaultResponse());
    } else if (method == SemanticTokensRefreshRequest::methodName) {
        m_tokenSupport.refresh();
        sendResponse(createDefaultResponse());
    } else if (method == ProgressNotification::methodName) {
        if (std::optional<ProgressParams> params
            = ProgressNotification(message.toJsonObject()).params()) {
            if (!params->isValid())
                q->log(invalidParamsErrorMessage(*params));
            m_progressManager.handleProgress(*params);
            if (ProgressManager::isProgressEndMessage(*params))
                emit q->workDone(params->token());
        }
    } else if (method == ConfigurationRequest::methodName) {
        ConfigurationRequest::Response response;
        QJsonArray result;
        if (QTC_GUARD(id.isValid()))
            response.setId(id);
        ConfigurationRequest configurationRequest(message.toJsonObject());
        if (std::optional<ConfigurationParams> params = configurationRequest.params()) {
            const QList<ConfigurationParams::ConfigurationItem> items = params->items();
            for (const ConfigurationParams::ConfigurationItem &item : items) {
                if (const std::optional<QString> section = item.section())
                    result.append(m_configuration[*section]);
                else
                    result.append({});
            }
        }
        response.setResult(result);
        sendResponse(response);
    } else if (isRequest) {
        Response<JsonObject, JsonObject> response(id);
        ResponseError<JsonObject> error;
        error.setCode(ResponseError<JsonObject>::MethodNotFound);
        error.setMessage(QString("The client cannot handle the method '%1'.").arg(method));
        response.setError(error);
        sendResponse(response);
    } else {
        const auto customHandler = m_customHandlers.constFind(method);
        if (customHandler != m_customHandlers.constEnd()) {
            (*customHandler)(message);
            return;
        }
    }

    // we got a request and handled it somewhere above but we missed to generate a response for it
    QTC_ASSERT(!isRequest || responseSend, sendResponse(createDefaultResponse()));
}

void Client::handleDiagnostics(const PublishDiagnosticsParams &params)
{
    const DocumentUri &uri = params.uri();

    const QList<Diagnostic> &diagnostics = params.diagnostics();
    if (!d->m_diagnosticManager)
        d->m_diagnosticManager = createDiagnosticManager();
    const FilePath &path = serverUriToHostPath(uri);
    d->m_diagnosticManager->setDiagnostics(path, diagnostics, params.version());
    if (LanguageClientManager::clientForFilePath(path) == this) {
        d->m_diagnosticManager->showDiagnostics(path, d->m_documentVersions.value(path));
        if (d->m_autoRequestCodeActions)
            requestCodeActions(uri, diagnostics);
    }
}

void ClientPrivate::sendMessageNow(const JsonRpcMessage &message)
{
    LanguageClientManager::logJsonRpcMessage(LspLogMessage::ClientMessage, q->name(), message);
    m_clientInterface->sendMessage(message);
}

bool Client::documentUpdatePostponed(const Utils::FilePath &fileName) const
{
    return Utils::contains(d->m_documentsToUpdate, [fileName](const auto &elem) {
        return elem.first->filePath() == fileName;
    });
}

int Client::documentVersion(const Utils::FilePath &filePath) const
{
    return d->m_documentVersions.value(filePath);
}

int Client::documentVersion(const LanguageServerProtocol::DocumentUri &uri) const
{
    return documentVersion(serverUriToHostPath(uri));
}

void Client::setDocumentChangeUpdateThreshold(int msecs)
{
    d->m_documentUpdateTimer.setInterval(msecs);
}

void ClientPrivate::initializeCallback(const InitializeRequest::Response &initResponse)
{
    if (m_state != Client::InitializeRequested) {
        qCWarning(LOGLSPCLIENT) << "Dropping initialize response in unexpected state " << m_state;
        qCDebug(LOGLSPCLIENT) << initResponse.toJsonObject();
        return;
    }
    if (std::optional<ResponseError<InitializeError>> error = initResponse.error()) {
        if (std::optional<InitializeError> data = error->data()) {
            if (data->retry()) {
                const QString title(Tr::tr("Language Server \"%1\" Initialization Error").arg(m_displayName));
                auto result = QMessageBox::warning(Core::ICore::dialogParent(),
                                                   title,
                                                   error->message(),
                                                   QMessageBox::Retry | QMessageBox::Cancel,
                                                   QMessageBox::Retry);
                if (result == QMessageBox::Retry) {
                    m_state = Client::Uninitialized;
                    q->initialize();
                    return;
                }
            }
        }
        q->setError(Tr::tr("Initialization error: %1.").arg(error->message()));
        emit q->finished();
        return;
    }
    if (const std::optional<InitializeResult> &result = initResponse.result()) {
        if (!result->isValid()) { // continue on ill formed result
            q->log(QJsonDocument(*result).toJson(QJsonDocument::Indented) + '\n'
                + Tr::tr("Initialize result is invalid."));
        }
        const std::optional<ServerInfo> serverInfo = result->serverInfo();
        if (serverInfo) {
            if (!serverInfo->isValid()) {
                q->log(QJsonDocument(*result).toJson(QJsonDocument::Indented) + '\n'
                    + Tr::tr("Server Info is invalid."));
            } else {
                m_serverName = serverInfo->name();
                if (const std::optional<QString> version = serverInfo->version())
                    m_serverVersion = *version;
            }
        }

        m_serverCapabilities = result->capabilities();
    } else {
        q->log(Tr::tr("No initialize result."));
    }

    if (auto completionProvider = qobject_cast<LanguageClientCompletionAssistProvider *>(
            m_clientProviders.completionAssistProvider)) {
        completionProvider->setTriggerCharacters(
            m_serverCapabilities.completionProvider()
                .value_or(ServerCapabilities::CompletionOptions())
                .triggerCharacters());
    }
    if (auto functionHintAssistProvider = qobject_cast<FunctionHintAssistProvider *>(
            m_clientProviders.functionHintProvider)) {
        functionHintAssistProvider->setTriggerCharacters(
            m_serverCapabilities.signatureHelpProvider()
                .value_or(ServerCapabilities::SignatureHelpOptions())
                .triggerCharacters());
    }
    auto tokenProvider = m_serverCapabilities.semanticTokensProvider().value_or(
        SemanticTokensOptions());
    if (tokenProvider.isValid())
        m_tokenSupport.setLegend(tokenProvider.legend());

    qCDebug(LOGLSPCLIENT) << "language server " << m_displayName << " initialized";
    m_state = Client::Initialized;
    q->sendMessage(InitializeNotification(InitializedParams()));

    q->updateConfiguration(m_configuration);

    m_tokenSupport.clearTokens(); // clear cached tokens from a pre reset run
    for (TextEditor::TextDocument *doc : m_postponedDocuments)
        q->openDocument(doc);
    m_postponedDocuments.clear();

    emit q->initialized(m_serverCapabilities);
}

void ClientPrivate::shutDownCallback(const ShutdownRequest::Response &shutdownResponse)
{
    m_shutdownTimer.stop();
    QTC_ASSERT(m_state == Client::ShutdownRequested, return);
    QTC_ASSERT(m_clientInterface, return);
    if (std::optional<ShutdownRequest::Response::Error> error = shutdownResponse.error())
        q->log(*error);
    // directly send content now otherwise the state check of sendContent would fail
    sendMessageNow(ExitNotification());
    qCDebug(LOGLSPCLIENT) << "language server " << m_displayName << " shutdown";
    m_state = Client::Shutdown;
    m_shutdownTimer.start();
}

bool ClientPrivate::sendWorkspceFolderChanges() const
{
    if (!q->reachable())
        return false;
    if (m_dynamicCapabilities.isRegistered(
                DidChangeWorkspaceFoldersNotification::methodName).value_or(false)) {
        return true;
    }
    if (auto workspace = m_serverCapabilities.workspace()) {
        if (auto folder = workspace->workspaceFolders()) {
            if (folder->supported().value_or(false)) {
                // holds either the Id for deregistration or whether it is registered
                auto notification = folder->changeNotifications().value_or(false);
                return std::holds_alternative<QString>(notification)
                       || (std::holds_alternative<bool>(notification)
                           && std::get<bool>(notification));
            }
        }
    }
    return false;
}

QTextCursor Client::adjustedCursorForHighlighting(const QTextCursor &cursor,
                                                  TextEditor::TextDocument *doc)
{
    Q_UNUSED(doc)
    return cursor;
}

bool Client::referencesShadowFile(const TextEditor::TextDocument *doc,
                                  const Utils::FilePath &candidate)
{
    Q_UNUSED(doc)
    Q_UNUSED(candidate)
    return false;
}

bool Client::fileBelongsToProject(const Utils::FilePath &filePath) const
{
    return project() && project()->isKnownFile(filePath);
}

LanguageClientOutlineItem *Client::createOutlineItem(
    const LanguageServerProtocol::DocumentSymbol &symbol)
{
    return new LanguageClientOutlineItem(this, symbol);
}

FilePath toHostPath(const FilePath serverDeviceTemplate, const FilePath localClientPath)
{
    const FilePath onDevice = serverDeviceTemplate.withNewPath(localClientPath.path());
    return onDevice.localSource().value_or(onDevice);
}

DocumentUri::PathMapper Client::hostPathMapper() const
{
    return [serverDeviceTemplate = d->m_serverDeviceTemplate](const Utils::FilePath &serverPath) {
        return toHostPath(serverDeviceTemplate, serverPath);
    };
}

FilePath Client::serverUriToHostPath(const LanguageServerProtocol::DocumentUri &uri) const
{
    return uri.toFilePath(hostPathMapper());
}

DocumentUri Client::hostPathToServerUri(const Utils::FilePath &path) const
{
    return DocumentUri::fromFilePath(path, [&](const Utils::FilePath &clientPath) {
        return d->m_serverDeviceTemplate.withNewPath(clientPath.path());
    });
}

OsType Client::osType() const
{
    return d->m_serverDeviceTemplate.osType();
}

void Client::registerCustomMethod(const QString &method, const CustomMethodHandler &handler)
{
    d->m_customHandlers.insert(method, handler);
}

} // namespace LanguageClient

#include <client.moc>
