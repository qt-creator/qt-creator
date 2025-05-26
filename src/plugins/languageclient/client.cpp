// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "client.h"

#include "callandtypehierarchy.h"
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

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/codeassist/documentcontentcompletion.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/ioutlinewidget.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>

#include <utils/appinfo.h>
#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>

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

using namespace ProjectExplorer;
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
        QMetaObject::invokeMethod(m_interface, [this, message] { m_interface->sendMessage(message); });
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

class ClientPrivate : public QObject
{
    Q_OBJECT
public:
    ClientPrivate(Client *client, BaseClientInterface *clientInterface, const Utils::Id &id)
        : q(client)
        , m_id(id.isValid() ? id : Id::generate())
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
        connect(ProjectManager::instance(), &ProjectManager::buildConfigurationRemoved,
                q, &Client::buildConfigurationClosed);

        QTC_ASSERT(clientInterface, return);
        connect(m_clientInterface, &InterfaceController::messageReceived, q, &Client::handleMessage);
        connect(m_clientInterface, &InterfaceController::error, q, &Client::setError);
        connect(m_clientInterface, &InterfaceController::finished, q, &Client::finished);
        connect(m_clientInterface, &InterfaceController::started, this, [this] {
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
            updateOpenedEditorToolBars();
        }
        for (IAssistProcessor *processor : std::as_const(m_runningAssistProcessors))
            processor->setAsyncProposalAvailable(nullptr);
        qDeleteAll(m_documentHighlightsTimer);
        m_documentHighlightsTimer.clear();
        // do not handle messages while shutting down
        disconnect(m_clientInterface, &InterfaceController::messageReceived,
                   q, &Client::handleMessage);
        delete m_clientProviders.completionAssistProvider;
        delete m_clientProviders.functionHintProvider;
        delete m_clientProviders.quickFixAssistProvider;
        delete m_diagnosticManager;
        delete m_clientInterface;
    }

    Client *q;

    void updateOpenedEditorToolBars()
    {
        for (auto it = m_openedDocument.cbegin(); it != m_openedDocument.cend(); ++it) {
            for (Core::IEditor *editor : Core::DocumentModel::editorsForDocument(it->first))
                updateEditorToolBar(editor);
        }
    }

    template <typename R>
    void updateCapabilities(const QList<R> &regs)
    {
        bool updateCompletion = false;
        bool updateFunctionHint = false;
        bool updateSemanticToken = false;
        for (const R &reg : regs) {
            if (reg.method() == CompletionRequest::methodName)
                updateCompletion = true;
            if (reg.method() == SignatureHelpRequest::methodName)
                updateFunctionHint = true;
            if (reg.method() == "textDocument/semanticTokens") {
                updateSemanticToken = true;
                if constexpr (std::is_same_v<R, Registration>) {
                    const SemanticTokensOptions options(reg.registerOptions());
                    if (options.isValid())
                        m_tokenSupport.setLegend(options.legend());
                }
            }
        }
        if (updateCompletion || updateFunctionHint || updateSemanticToken) {
            for (auto it = m_openedDocument.cbegin(); it != m_openedDocument.cend(); ++it) {
                if (updateCompletion)
                    updateCompletionProvider(it->first);
                if (updateFunctionHint)
                    updateFunctionHintProvider(it->first);
                if (updateSemanticToken)
                    m_tokenSupport.updateSemanticTokens(it->first);
            }
        }
        emit q->capabilitiesChanged(m_dynamicCapabilities);
    }

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

    void setState(Client::State state)
    {
        m_state = state;
        emit q->stateChanged(state);
    }

    Client::State m_state = Client::Uninitialized;
    QHash<LanguageServerProtocol::MessageId,
          LanguageServerProtocol::ResponseHandler::Callback> m_responseHandlers;
    QString m_displayName;
    LanguageFilter m_languagFilter;
    QJsonObject m_initializationOptions;
    using TextDocumentDeleter = std::function<void(QTextDocument *)>;
    using TextDocumentWithDeleter = std::unique_ptr<QTextDocument, TextDocumentDeleter>;
    std::unordered_map<TextEditor::TextDocument *, TextDocumentWithDeleter> m_openedDocument;

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
    QHash<TextEditor::TextEditorWidget *, QTimer *> m_documentHighlightsTimer;
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
    QHash<TextEditor::TextDocument *, AssistProviders> m_resetAssistProvider;
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
    QPointer<BuildConfiguration> m_bc;
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
    bool m_activatable = true;
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
    if (d->m_bc) {
        const QString projectDisplayName = d->m_bc->project()->displayName();
        if (!projectDisplayName.isEmpty()) {
            //: <language client> for <project>
            return Tr::tr("%1 for %2").arg(d->m_displayName, projectDisplayName);
        }
    }
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
    SymbolCapabilities::SymbolTagCapabilities symbolTagCapabilities;
    symbolTagCapabilities.setValueSet({SymbolTag::Deprecated});
    symbolCapabilities.setSymbolTag(symbolTagCapabilities);
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
    documentCapabilities.setTypeHierarchy(allowDynamicRegistration);
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
    if (d->m_bc && d->m_bc->project())
        params.setRootUri(hostPathToServerUri(d->m_bc->project()->projectDirectory()));

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
    d->setState(InitializeRequested);
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
    d->setState(ShutdownRequested);
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
    //: language client state
    case FailedToShutdown: return Tr::tr("failed to shutdown");
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
    if ((d->m_openedDocument.find(document) != d->m_openedDocument.end())
        || !isSupportedDocument(document)) {
        return;
    }

    connect(document, &TextDocument::destroyed, this, [this, document] {
        d->m_postponedDocuments.remove(document);
        const auto it = d->m_openedDocument.find(document);
        if (it != d->m_openedDocument.end())
            d->m_openedDocument.erase(it);
        d->m_documentsToUpdate.erase(document);
        d->m_resetAssistProvider.remove(document);
    });

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

    const QList<QMetaObject::Connection> connections {
    connect(document, &TextDocument::contentsChangedWithPosition, this,
            [this, document](int position, int charsRemoved, int charsAdded) {
                documentContentsChanged(document, position, charsRemoved, charsAdded);
            }),
    connect(document, &TextDocument::filePathChanged, this,
            [this, document](const FilePath &oldPath, const FilePath &newPath) {
                if (oldPath == newPath)
                    return;
                closeDocument(document, oldPath);
                if (isSupportedDocument(document))
                    openDocument(document);
            }),
    connect(document, &TextDocument::saved, this,
            [this, document](const FilePath &saveFilePath) {
                if (saveFilePath == document->filePath())
                    documentContentsSaved(document);
            }),
    connect(document, &TextDocument::aboutToSave, this,
            [this, document](const FilePath &saveFilePath) {
                if (saveFilePath == document->filePath())
                    documentWillSave(document);
            })
    };
    const auto deleter = [connections](QTextDocument *document) {
        for (const QMetaObject::Connection &connection : connections)
            QObject::disconnect(connection);
        delete document;
    };

    d->m_openedDocument.emplace(document, ClientPrivate::TextDocumentWithDeleter(
        new QTextDocument(document->document()->toPlainText()), deleter));
    if (!d->m_documentVersions.contains(filePath))
        d->m_documentVersions[filePath] = 0;
    d->sendOpenNotification(filePath, document->mimeType(), document->plainText(),
                            d->m_documentVersions[filePath]);
    handleDocumentOpened(document);

    if (d->m_activatable) {
        const Client *currentClient = LanguageClientManager::clientForDocument(document);
        if (currentClient == this) {
            // this is the active client for the document so directly activate it
            activateDocument(document);
        } else if (currentClient == nullptr) {
            // there is no client for this document so assign it to this server
            LanguageClientManager::openDocumentWithClient(document, this);
        }
    }
}

bool Client::activatable() const
{
    return d->m_activatable;
}

void Client::setActivatable(bool activatable)
{
    d->m_activatable = activatable;
}

void Client::sendMessage(const JsonRpcMessage &message, SendDocUpdates sendUpdates,
                         Schedule semanticTokensSchedule)
{
    QScopeGuard guard([this, responseHandler = message.responseHandler()](){
        if (responseHandler) {
            static ResponseError<std::nullptr_t> error;
            if (!error.isValid()) {
                error.setCode(-32803); // RequestFailed
                error.setMessage("The server is currently in an unreachable state.");
            }
            QJsonObject response;
            response[idKey] = responseHandler->id;
            response[errorKey] = QJsonObject(error);
            QMetaObject::invokeMethod(this, [callback = responseHandler->callback, response](){
                callback(JsonRpcMessage(response));
            }, Qt::QueuedConnection);
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
    d->m_postponedDocuments.remove(document);
    d->m_documentsToUpdate.erase(document);
    const auto it = d->m_openedDocument.find(document);
    if (it != d->m_openedDocument.end()) {
        d->m_openedDocument.erase(it);
        deactivateDocument(document);
        handleDocumentClosed(document);
        if (d->m_state == Initialized)
            d->sendCloseNotification(overwriteFilePath.value_or(document->filePath()));
    }
    d->m_tokenSupport.clearCache(document);

    if (d->m_state != Initialized)
        return;
    d->closeRequiredShadowDocuments(document);
    const auto shadowIt = d->m_shadowDocuments.find(document->filePath());
    if (shadowIt == d->m_shadowDocuments.constEnd())
        return;
    QTC_CHECK(shadowIt.value().second.isEmpty());
    bool isReferenced = false;
    for (auto it = d->m_openedDocument.cbegin(); it != d->m_openedDocument.cend(); ++it) {
        if (referencesShadowFile(it->first, shadowIt.key())) {
            d->openShadowDocument(it->first, shadowIt);
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
        const auto boolvalue = std::get_if<bool>(&*provider);
        if (boolvalue && !*boolvalue)
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
            const auto highlights = std::get_if<QList<DocumentHighlight>>(&*result);
            for (const auto &highlight : *highlights) {
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
    QTC_ASSERT(d->m_activatable, return);
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
            optionalActions |= TextEditor::OptionalActions::FindUsage;
        if (symbolSupport().supportsRename(widget->textDocument()))
            optionalActions |= TextEditor::OptionalActions::RenameSymbol;
        if (symbolSupport().supportsFindLink(widget->textDocument(), LinkTarget::SymbolDef))
            optionalActions |= TextEditor::OptionalActions::FollowSymbolUnderCursor;
        if (symbolSupport().supportsFindLink(widget->textDocument(), LinkTarget::SymbolTypeDef))
            optionalActions |= TextEditor::OptionalActions::FollowTypeUnderCursor;
        if (supportsCallHierarchy(this, textEditor->document()))
            optionalActions |= TextEditor::OptionalActions::CallHierarchy;
        if (supportsTypeHierarchy(this, textEditor->document()))
            optionalActions |= TextEditor::OptionalActions::TypeHierarchy;
        widget->setOptionalActions(optionalActions);
    }
}

void Client::deactivateDocument(TextEditor::TextDocument *document)
{
    if (d->m_diagnosticManager)
        d->m_diagnosticManager->hideDiagnostics(document->filePath());
    d->resetAssistProviders(document);
    document->setFormatter(nullptr);
    d->m_tokenSupport.deactivateDocument(document);
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
    return d->m_openedDocument.find(const_cast<TextEditor::TextDocument *>(document))
           != d->m_openedDocument.end();
}

TextEditor::TextDocument *Client::documentForFilePath(const Utils::FilePath &file) const
{
    for (auto it = d->m_openedDocument.cbegin(); it != d->m_openedDocument.cend(); ++it) {
        if (it->first->filePath() == file)
            return it->first;
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
        if (referencesShadowFile(docIt->first, filePath))
            d->openShadowDocument(docIt->first, shadowIt);
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
    if (d->m_openedDocument.find(document) == d->m_openedDocument.end())
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
    if (!send || !shouldSendDidSave(document))
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
    if (d->m_openedDocument.find(textDocument) == d->m_openedDocument.end())
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
    const auto it = d->m_openedDocument.find(document);
    if (it == d->m_openedDocument.end() || !reachable())
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
    QTextCursor cursor(it->second.get());
    // Workaround https://bugreports.qt.io/browse/QTBUG-80662
    // The contentsChanged gives a character count that can be wrong for QTextCursor
    // when there are special characters removed/added (like formating characters).
    // Also, characterCount return the number of characters + 1 because of the hidden
    // paragraph separator character.
    // This implementation is based on QWidgetTextControlPrivate::_q_contentsChanged.
    // For charsAdded, textAt handles the case itself.
    cursor.setPosition(qMin(it->second->characterCount() - 1, position + charsRemoved));
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
    d->updateCapabilities(registrations);
}

void Client::unregisterCapabilities(const QList<Unregistration> &unregistrations)
{
    d->m_dynamicCapabilities.unregisterCapability(unregistrations);
    d->updateCapabilities(unregistrations);
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
        const auto boolvalue = std::get_if<bool>(&provider);
        if (boolvalue && !*boolvalue)
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

Project *Client::project() const
{
    return d->m_bc ? d->m_bc->project() : nullptr;
}

BuildConfiguration *Client::buildConfiguration() const
{
    return d->m_bc;
}

void Client::setCurrentBuildConfiguration(BuildConfiguration *bc)
{
    QTC_ASSERT(!bc ||canOpenProject(bc->project()), return);
    if (d->m_bc == bc)
        return;
    if (d->m_bc)
        d->m_bc->disconnect(this);
    d->m_bc = bc;
}

void Client::buildConfigurationOpened(BuildConfiguration *bc)
{
    Project *project = bc->project();
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

void Client::buildConfigurationClosed(BuildConfiguration *bc)
{
    Project *project = bc->project();
    if (d->sendWorkspceFolderChanges() && canOpenProject(project)) {
        WorkspaceFoldersChangeEvent event;
        event.setRemoved({WorkSpaceFolder(hostPathToServerUri(project->projectDirectory()),
                                          project->displayName())});
        DidChangeWorkspaceFoldersParams params;
        params.setEvent(event);
        DidChangeWorkspaceFoldersNotification change(params);
        sendMessage(change);
    }
    if (bc == d->m_bc) {
        if (d->m_state == Initialized) {
            LanguageClientManager::shutdownClient(this);
        } else {
            d->setState(Shutdown); // otherwise the manager would try to restart this server
            emit finished();
        }
        d->m_bc = nullptr;
    }
}

bool Client::canOpenProject(Project *project)
{
    Q_UNUSED(project)
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

void Client::hideDiagnostics(const Utils::FilePath &documentPath)
{
    if (d->m_diagnosticManager)
        d->m_diagnosticManager->hideDiagnostics(documentPath);
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

void Client::setFunctionHintAssistProvider(FunctionHintAssistProvider *provider)
{
    delete d->m_clientProviders.functionHintProvider;
    d->m_clientProviders.functionHintProvider = provider;
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
    if (const auto boolvalue = std::get_if<bool>(&*provider))
        return *boolvalue;
    return true;
}

void Client::setLogTarget(LogTarget target)
{
    d->m_logTarget = target;
}

void Client::start()
{
    startImpl();
}

void Client::startImpl()
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
    setState(Client::Uninitialized);
    m_responseHandlers.clear();
    m_clientInterface->resetBuffer();
    updateOpenedEditorToolBars();
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
    switch (d->m_state) {
    case Uninitialized:
    case InitializeRequested:
    case FailedToInitialize:
        d->setState(FailedToInitialize);
        return;
    case Initialized:
    case Error:
        d->setState(Error);
        return;
    case ShutdownRequested:
    case FailedToShutdown:
    case Shutdown:
        d->setState(FailedToShutdown);
        return;
    }
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

TextEditor::RefactoringFilePtr Client::createRefactoringFile(const FilePath &filePath) const
{
    return TextEditor::PlainRefactoringFileFactory().file(filePath);
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

DynamicCapabilities &Client::dynamicCapabilities()
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

SemanticTokenSupport *Client::semanticTokenSupport()
{
    return &d->m_tokenSupport;
}

void ClientPrivate::log(const ShowMessageParams &message)
{
    q->log(message.toString());
}

LanguageClientValue<MessageActionItem> ClientPrivate::showMessageBox(
    const ShowMessageRequestParams &message)
{
    QMessageBox box;
    box.setWindowTitle(q->name());
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
    const auto customHandler = m_customHandlers.constFind(method);
    if (customHandler != m_customHandlers.constEnd()) {
        const bool isHandled = (*customHandler)(message);
        if (isHandled)
            return;
    }

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
                    setState(Client::Uninitialized);
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
    setState(Client::Initialized);
    q->sendMessage(InitializeNotification(InitializedParams()));

    q->updateConfiguration(m_configuration);

    m_tokenSupport.clearTokens(); // clear cached tokens from a pre reset run
    for (TextEditor::TextDocument *doc : std::as_const(m_postponedDocuments))
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
    setState(Client::Shutdown);
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
                const std::variant<QString, bool> notification
                    = folder->changeNotifications().value_or(false);
                const auto boolvalue = std::get_if<bool>(&notification);
                return !boolvalue || *boolvalue;
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
    if (d->m_customHandlers.contains(method))
        qCWarning(LOGLSPCLIENT) << "Overwriting custom method handler for:" << method;
    d->m_customHandlers.insert(method, handler);
}

} // namespace LanguageClient

#include <client.moc>
