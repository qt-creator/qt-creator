// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "client.h"

#include "callandtypehierarchy.h"
#include "diagnosticmanager.h"
#include "documentsymbolcache.h"
#include "foldingrangesupport.h"
#include "languageclientcompletionassist.h"
#include "languageclientformatter.h"
#include "languageclientfunctionhint.h"
#include "languageclienthoverhandler.h"
#include "languageclientinterface.h"
#include "languageclientmanager.h"
#include "languageclientoutline.h"
#include "languageclientquickfix.h"
#include "languageclientsymbolsupport.h"
#include "languageclienttr.h"
#include "languageclientutils.h"
#include "progressmanager.h"
#include "semantichighlightsupport.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <languageserverprotocol/lspmessages.h>
#include <languageserverprotocol/lsputils.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/fontsettings.h>
#include <texteditor/ioutlinewidget.h>
#include <texteditor/semantichighlighter.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/appinfo.h>
#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QDebug>
#include <QUuid>
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
    void sendMessage(const QJsonObject &message)
    {
        QMetaObject::invokeMethod(m_interface, [this, message] { m_interface->sendMessage(message); });
    }
    void resetBuffer()
    {
        QMetaObject::invokeMethod(m_interface, &BaseClientInterface::resetBuffer);
    }

signals:
    void messageReceived(const QJsonObject &message);
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
        , m_clientInterface(new InterfaceController(clientInterface))
        , m_serverDeviceTemplate(clientInterface->serverDeviceTemplate())
    {
        using namespace ProjectExplorer;

        m_clientInfo.name(QGuiApplication::applicationDisplayName());
        m_clientInfo.version(Utils::appInfo().displayVersion);

        m_clientProviders.completionAssistProvider = new LanguageClientCompletionAssistProvider(q);
        m_clientProviders.functionHintProvider = new FunctionHintAssistProvider(q);
        m_clientProviders.quickFixAssistProvider = new LanguageClientQuickFixProvider(q);

        m_documentUpdateTimer.setSingleShot(true);
        m_documentUpdateTimer.setInterval(500);
        connect(&m_documentUpdateTimer, &QTimer::timeout, this,
                [this] { sendPostponedDocumentUpdates(Schedule::Now); });
        connect(ProjectManager::instance(), &ProjectManager::aboutToRemoveBuildConfiguration,
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

        m_tokenSupport.setTokenTypesMap(defaultTokenTypesMap());
        m_tokenSupport.setTokenModifiersMap(defaultTokenModifiersMap());

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

        // deactivateEditor modifies m_activeEditors, so copy it beforehand
        const QSet<Core::IEditor *> activeEditors = m_activeEditors;
        for (auto activeEditor : activeEditors)
            q->deactivateEditor(activeEditor);

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
            if (reg.method() == CompletionRequest::method)
                updateCompletion = true;
            if (reg.method() == SignatureHelpRequest::method)
                updateFunctionHint = true;
            if (reg.method() == "textDocument/semanticTokens") {
                updateSemanticToken = true;
                if constexpr (std::is_same_v<R, Registration>) {
                    const Utils::Result<SemanticTokensOptions> options
                        = fromJson<SemanticTokensOptions>(
                            reg.registerOptions().value_or(QJsonValue()));
                    if (options)
                        m_tokenSupport.setLegend(options->legend());
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

    void sendMessageNow(const QJsonObject &message);
    void handleResponse(const MessageId &id, const QJsonObject &message);
    void handleMethod(const QString &method, const MessageId &id, const QJsonObject &message);

    void initializeCallback(const Utils::Result<InitializeResult> &result);
    void shutDownCallback(const Utils::Result<std::monostate> &result);
    bool sendWorkspceFolderChanges() const;
    void log(int messageType, const QString &message);

    ShowMessageRequestResult showMessageBox(const ShowMessageRequestParams &message);

    void removeDiagnostics(const QString &uri);
    void resetAssistProviders(TextEditor::TextDocument *document);

    void sendPostponedDocumentUpdates(Schedule semanticTokensSchedule);
    int serverTextDocumentSyncKind() const;

    void updateCompletionProvider(TextEditor::TextDocument *document);
    void updateFunctionHintProvider(TextEditor::TextDocument *document);

    void requestDocumentHighlights(TextEditor::TextEditorWidget *widget);
    void requestDocumentHighlightsNow(TextEditor::TextEditorWidget *widget);
    void requestCodeActions(
        const QString &uri, const Range &range, const QList<Diagnostic> &diagnostics);
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
    QHash<MessageId, std::function<void(const QJsonObject &)>> m_responseHandlers;
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
    std::unordered_map<TextEditor::TextDocument *, QList<TextDocumentContentChangePartial>>
        m_documentsToUpdate;
    QHash<TextEditor::TextEditorWidget *, QTimer *> m_documentHighlightsTimer;
    QTimer m_documentUpdateTimer;
    const Utils::Id m_id;
    ClientCapabilities m_clientCapabilities{q->defaultClientCapabilities()};
    QJsonObject m_extraClientCapabilities;
    ServerCapabilities m_serverCapabilities;
    DynamicCapabilities m_dynamicCapabilities;
    struct AssistProviders
    {
        QPointer<TextEditor::CompletionAssistProvider> completionAssistProvider;
        QPointer<TextEditor::CompletionAssistProvider> functionHintProvider;
        QPointer<TextEditor::IAssistProvider> quickFixAssistProvider;
    };

    AssistProviders m_clientProviders;
    QHash<TextEditor::TextDocument *, AssistProviders> m_resetAssistProvider;
    QHash<TextEditor::TextEditorWidget *, MessageId> m_highlightRequests;
    QHash<QString, Client::CustomMethodHandler> m_customHandlers;
    static const int MaxRestarts = 5;
    int m_restartsLeft = MaxRestarts;
    QTimer m_restartCountResetTimer;
    InterfaceController * const m_clientInterface;
    DiagnosticManager *m_diagnosticManager = nullptr;
    DocumentSymbolCache m_documentSymbolCache{q};
    HoverHandler m_hoverHandler{q};
    QSet<Core::IEditor *> m_activeEditors;
    QHash<QString, TextEditor::HighlightingResults> m_highlights;
    QPointer<BuildConfiguration> m_bc;
    QSet<TextEditor::IAssistProcessor *> m_runningAssistProcessors;
    SymbolSupport m_symbolSupport{q};
    MessageId m_runningFindLinkRequest;
    ProgressManager m_progressManager{q};
    SemanticTokenSupport m_tokenSupport{q};
    Internal::FoldingRangeSupport m_foldingSupport{q};
    QString m_serverName;
    QString m_serverVersion;
    Client::LogTarget m_logTarget = Client::LogTarget::Ui;
    bool m_locatorsEnabled = true;
    bool m_autoRequestCodeActions = true;
    QTimer m_shutdownTimer;
    ClientInfo m_clientInfo;
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
            //: for example: MyServer for MyProject (Qt 1.2.3, Release)
            return Tr::tr("%1 for %2 (%3, %4)")
                .arg(
                    d->m_displayName,
                    projectDisplayName,
                    d->m_bc->target()->displayName(),
                    d->m_bc->displayName());
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
    WorkspaceClientCapabilities workspaceCapabilities;
    workspaceCapabilities.workspaceEdit(
        WorkspaceEditClientCapabilities().documentChanges(true).resourceOperations(QList{
            ResourceOperationKind::create,
            ResourceOperationKind::rename,
            ResourceOperationKind::delete_}));
    workspaceCapabilities.workspaceFolders(true);
    workspaceCapabilities.applyEdit(true);
    workspaceCapabilities.didChangeConfiguration(
        DidChangeConfigurationClientCapabilities().dynamicRegistration(true));
    workspaceCapabilities.executeCommand(
        ExecuteCommandClientCapabilities().dynamicRegistration(true));
    workspaceCapabilities.configuration(true);
    workspaceCapabilities.semanticTokens(
        SemanticTokensWorkspaceClientCapabilities().refreshSupport(true));
    workspaceCapabilities.foldingRange(
        FoldingRangeWorkspaceClientCapabilities().refreshSupport(true));

    TextDocumentClientCapabilities documentCapabilities;
    documentCapabilities.synchronization(TextDocumentSyncClientCapabilities()
                                             .dynamicRegistration(true)
                                             .willSave(true)
                                             .willSaveWaitUntil(false)
                                             .didSave(true));

    QList<int> symbolKinds;
    for (int kind = SymbolKind::File; kind <= SymbolKind::TypeParameter; ++kind)
        symbolKinds << kind;
    // Only Deprecated exists in LSP 3.17; the other tags are still proposals,
    // and strict servers refuse to initialize when they are advertised.
    documentCapabilities.documentSymbol(
        DocumentSymbolClientCapabilities()
            .symbolKind(ClientSymbolKindOptions().valueSet(symbolKinds))
            .tagSupport(ClientSymbolTagOptions().valueSet({SymbolTag::Deprecated}))
            .hierarchicalDocumentSymbolSupport(true));

    QList<int> completionItemKinds;
    for (int kind = CompletionItemKind::Text; kind <= CompletionItemKind::TypeParameter; ++kind) {
        completionItemKinds << kind;
    }
    documentCapabilities.completion(
        CompletionClientCapabilities()
            .dynamicRegistration(true)
            .completionItemKind(ClientCompletionItemOptionsKind().valueSet(completionItemKinds))
            .completionItem(
                ClientCompletionItemOptions().snippetSupport(true).commitCharactersSupport(true)));

    documentCapabilities.codeAction(CodeActionClientCapabilities().codeActionLiteralSupport(
        ClientCodeActionLiteralOptions().codeActionKind(
            ClientCodeActionKindOptions().valueSet({"*"}))));

    documentCapabilities.hover(HoverClientCapabilities().dynamicRegistration(true).contentFormat(
        QList{MarkupKind::markdown, MarkupKind::plaintext}));

    documentCapabilities.rename(
        RenameClientCapabilities().dynamicRegistration(true).prepareSupport(true));

    documentCapabilities.foldingRange(
        FoldingRangeClientCapabilities()
            .dynamicRegistration(true)
            .lineFoldingOnly(true)
            .foldingRangeKind(ClientFoldingRangeKindOptions().valueSet(
                QList<FoldingRangeKind>{"comment", "imports", "region"})));

    documentCapabilities.signatureHelp(
        SignatureHelpClientCapabilities().dynamicRegistration(true).signatureInformation(
            ClientSignatureInformationOptions()
                .documentationFormat(QList{MarkupKind::markdown, MarkupKind::plaintext})
                .activeParameterSupport(true)));

    documentCapabilities.references(ReferenceClientCapabilities().dynamicRegistration(true));
    documentCapabilities.documentHighlight(
        DocumentHighlightClientCapabilities().dynamicRegistration(true));
    documentCapabilities.definition(DefinitionClientCapabilities().dynamicRegistration(true));
    documentCapabilities.typeDefinition(
        TypeDefinitionClientCapabilities().dynamicRegistration(true));
    documentCapabilities.implementation(
        ImplementationClientCapabilities().dynamicRegistration(true));
    documentCapabilities.formatting(
        DocumentFormattingClientCapabilities().dynamicRegistration(true));
    documentCapabilities.rangeFormatting(
        DocumentRangeFormattingClientCapabilities().dynamicRegistration(true));
    documentCapabilities.onTypeFormatting(
        DocumentOnTypeFormattingClientCapabilities().dynamicRegistration(true));

    documentCapabilities.semanticTokens(SemanticTokensClientCapabilities()
                                            .dynamicRegistration(true)
                                            .requests(ClientSemanticTokensRequestOptions().full(
                                                ClientSemanticTokensRequestFullDelta().delta(true)))
                                            .tokenTypes(
                                                {"type",
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
                                                 "operator"})
                                            .tokenModifiers({"declaration", "definition"})
                                            .formats({TokenFormat::relative}));

    documentCapabilities.callHierarchy(CallHierarchyClientCapabilities().dynamicRegistration(true));
    documentCapabilities.typeHierarchy(TypeHierarchyClientCapabilities().dynamicRegistration(true));

    ClientCapabilities capabilities;
    capabilities.workspace(workspaceCapabilities);
    capabilities.textDocument(documentCapabilities);
    capabilities.window(WindowClientCapabilities().workDoneProgress(true));
    return capabilities;
}

/// Recursively adds everything in  extra to  object, replacing scalars.
static void mergeJson(QJsonObject &object, const QJsonObject &extra)
{
    for (auto it = extra.constBegin(); it != extra.constEnd(); ++it) {
        const QJsonValue existing = object.value(it.key());
        if (existing.isObject() && it.value().isObject()) {
            QJsonObject merged = existing.toObject();
            mergeJson(merged, it.value().toObject());
            object.insert(it.key(), merged);
        } else {
            object.insert(it.key(), it.value());
        }
    }
}

void Client::initialize()
{
    using namespace ProjectExplorer;
    QTC_ASSERT(d->m_clientInterface, return);
    QTC_ASSERT(d->m_state == Uninitialized, return);
    qCDebug(LOGLSPCLIENT) << "initializing language server " << d->m_displayName;
    InitializeParams params;
    params.clientInfo(d->m_clientInfo);
    params.capabilities(d->m_clientCapabilities);
    params.initializationOptions(d->m_initializationOptions);
    if (d->m_bc && d->m_bc->project())
        params.rootUri(uriFor(d->m_bc->project()->projectDirectory()));

    auto projectFilter = [this](Project *project) { return canOpenProject(project); };
    auto toWorkspaceFolder = [this](Project *pro) {
        return WorkspaceFolder().uri(uriFor(pro->projectDirectory())).name(pro->displayName());
    };
    const QList<WorkspaceFolder> workspaces = Utils::transform(
        Utils::filtered(ProjectManager::projects(), projectFilter), toWorkspaceFolder);
    if (workspaces.isEmpty())
        params.workspaceFolders(InitializeParamsWorkspaceFolders(std::monostate{}));
    else
        params.workspaceFolders(InitializeParamsWorkspaceFolders(workspaces));

    const MessageId id = nextMessageId();
    d->m_responseHandlers[id] = [this](const QJsonObject &response) {
        d->initializeCallback(LanguageServerProtocol::result<InitializeRequest>(response));
    };
    QJsonObject request = requestObject<InitializeRequest>(id, params);
    if (!d->m_extraClientCapabilities.isEmpty()) {
        QJsonObject requestParams = request.value("params").toObject();
        QJsonObject capabilities = requestParams.value("capabilities").toObject();
        mergeJson(capabilities, d->m_extraClientCapabilities);
        requestParams.insert("capabilities", capabilities);
        request.insert("params", requestParams);
    }
    // directly send content now otherwise the state check of sendContent would fail
    d->sendMessageNow(request);
    d->setState(InitializeRequested);
}

void Client::shutdown()
{
    QTC_ASSERT(d->m_state == Initialized, emit finished(); return);
    qCDebug(LOGLSPCLIENT) << "shutdown language server " << d->m_displayName;
    sendRequest<ShutdownRequest>({}, [this](const Utils::Result<std::monostate> &result) {
        d->shutDownCallback(result);
    });
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

void Client::setClientInfo(const ClientInfo &clientInfo)
{
    d->m_clientInfo = clientInfo;
}

ClientCapabilities Client::defaultClientCapabilities()
{
    return generateClientCapabilities();
}

void Client::setExtraClientCapabilities(const QJsonObject &caps)
{
    d->m_extraClientCapabilities = caps;
}

void Client::setClientCapabilities(const ClientCapabilities &caps)
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

    const QString method(DidOpenTextDocumentNotification::method);
    if (std::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        if (!*registered)
            return;
        if (!registrationApplies(d->m_dynamicCapabilities.option(method), filePath,
                                 document->mimeType())) {
            return;
        }
    } else if (
        const std::optional<ServerCapabilitiesTextDocumentSync> &sync
        = d->m_serverCapabilities.textDocumentSync()) {
        if (const auto options = std::get_if<TextDocumentSyncOptions>(&*sync)) {
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

void Client::sendRawMessage(const QJsonObject &message, SendDocUpdates sendUpdates,
                            Schedule semanticTokensSchedule)
{
    QTC_ASSERT(d->m_clientInterface, return);
    if (d->m_state == Shutdown || d->m_state == ShutdownRequested) {
        const QString key = message.contains("method") ? QString("method") : QString("id");
        qCDebug(LOGLSPCLIENT) << "Ignoring message " << message.value(key).toString()
                              << "because client is shutting down";
        return;
    }
    QTC_ASSERT(d->m_state == Initialized, return);

    if (sendUpdates == SendDocUpdates::Send)
        d->sendPostponedDocumentUpdates(semanticTokensSchedule);
    d->sendMessageNow(message);
}

MessageId Client::sendRawRequest(
    const QJsonObject &params,
    const QString &method,
    const std::function<void(const QJsonObject &)> &callback,
    SendDocUpdates sendUpdates,
    Schedule semanticTokensSchedule)
{
    const MessageId id = nextMessageId();
    const QJsonObject message{{"jsonrpc", "2.0"}, {"id", id.toJsonValue()},
                              {"method", method}, {"params", params}};
    sendJsonRpcMessage(message, callback, sendUpdates, semanticTokensSchedule);
    return id;
}

MessageId Client::nextMessageId() const
{
    return MessageId(QUuid::createUuid().toString());
}

void Client::sendJsonRpcMessage(const QJsonObject &message,
                                const std::function<void(const QJsonObject &)> &responseCallback,
                                SendDocUpdates sendUpdates,
                                Schedule semanticTokensSchedule)
{
    if (responseCallback && !reachable()) {
        // Answer right away; no response is going to arrive for this one.
        QJsonObject response{
            {"id", message.value("id")},
            {"error",
             ResponseError{
                 LSPErrorCodes::RequestFailed,
                 "The server is currently in an unreachable state.",
                 {}}
                 .toJsonObject()}};
        QMetaObject::invokeMethod(
            this, [responseCallback, response] { responseCallback(response); },
            Qt::QueuedConnection);
        return;
    }
    if (responseCallback) {
        // The response is dispatched by the id the message carries.
        d->m_responseHandlers[messageId(message)] = responseCallback;
    }
    sendRawMessage(message, sendUpdates, semanticTokensSchedule);
}

static CancelParams cancelParams(const MessageId &id)
{
    const QJsonValue value = id.toJsonValue();
    if (value.isDouble())
        return CancelParams().id(value.toInt());
    return CancelParams().id(value.toString());
}

void Client::cancelRequest(const MessageId &id)
{
    d->m_responseHandlers.remove(id);
    if (reachable())
        sendNotification<CancelNotification>(cancelParams(id), SendDocUpdates::Ignore);
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
    if (m_dynamicCapabilities.isRegistered(CompletionRequest::method).value_or(false)) {
        const QJsonValue &options = m_dynamicCapabilities.option(CompletionRequest::method);
        useLanguageServer = registrationApplies(options, document->filePath(),
                                                document->mimeType());

        const Utils::Result<CompletionOptions> completionOptions = fromJson<CompletionOptions>(
            options);
        if (completionOptions)
            clientCompletionProvider->setTriggerCharacters(completionOptions->triggerCharacters());
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
    if (m_dynamicCapabilities.isRegistered(SignatureHelpRequest::method).value_or(false)) {
        const QJsonValue &options = m_dynamicCapabilities.option(SignatureHelpRequest::method);
        useLanguageServer = registrationApplies(options, document->filePath(),
                                                document->mimeType());

        const Utils::Result<SignatureHelpOptions> signatureOptions = fromJson<SignatureHelpOptions>(
            options);
        if (signatureOptions)
            clientFunctionHintProvider->setTriggerCharacters(signatureOptions->triggerCharacters());
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
    const QString uri = q->uriFor(widget->textDocument()->filePath());
    const QString method(DocumentHighlightRequest::method);
    if (m_dynamicCapabilities.isRegistered(method).value_or(false)) {
        if (!registrationApplies(m_dynamicCapabilities.option(method),
                                 widget->textDocument()->filePath())) {
            return;
        }
    } else {
        const std::optional<ServerCapabilitiesDocumentHighlightProvider> &provider
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
    DocumentHighlightParams params;
    params.textDocument(TextDocumentIdentifier().uri(uri));
    params.position(positionOf(adjustedCursor));
    auto connection = connect(widget, &QObject::destroyed, this, [this, widget]() {
        if (m_highlightRequests.contains(widget))
            q->cancelRequest(m_highlightRequests.take(widget));
    });
    m_highlightRequests[widget] = q->sendRequest<DocumentHighlightRequest>(
        params,
        [widget, this, connection, adjustedCursor](
            const Utils::Result<DocumentHighlightRequestResult> &result) {
            m_highlightRequests.remove(widget);
            disconnect(connection);
            const Id &id = TextEditor::TextEditorWidget::CodeSemanticsSelection;
            QList<QTextEdit::ExtraSelection> selections;
            const auto highlights = result ? std::get_if<QList<DocumentHighlight>>(&*result)
                                           : nullptr;
            if (!highlights) {
                widget->setExtraSelections(id, selections);
                return;
            }

            const QTextCharFormat &format =
                widget->textDocument()->fontSettings().toTextCharFormat(TextEditor::C_OCCURRENCES);
            QTextDocument *document = widget->document();
            for (const auto &highlight : *highlights) {
                QTextEdit::ExtraSelection selection{widget->textCursor(), format};
                const int start = positionInDocument(highlight.range().start(), document);
                const int end = positionInDocument(highlight.range().end(), document);
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
                    selection.cursor = range.toTextCursor(document);
                    if (!selection.cursor.hasSelection())
                        continue;
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
}

void Client::activateDocument(TextEditor::TextDocument *document)
{
    QTC_ASSERT(d->m_activatable, return);
    const FilePath &filePath = document->filePath();
    if (d->m_diagnosticManager)
        d->m_diagnosticManager->showDiagnostics(filePath, d->m_documentVersions.value(filePath));
    d->m_tokenSupport.updateSemanticTokens(document);
    d->m_foldingSupport.requestFoldingRanges(document);
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
    if (TextEditor::TextEditorWidget *widget = TextEditor::TextEditorWidget::fromEditor(editor)) {
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
        if (supportsCallHierarchy(this, widget->textDocument()))
            optionalActions |= TextEditor::OptionalActions::CallHierarchy;
        if (supportsTypeHierarchy(this, widget->textDocument()))
            optionalActions |= TextEditor::OptionalActions::TypeHierarchy;
        widget->setOptionalActions(optionalActions);
        d->m_activeEditors.insert(editor);
        connect(editor, &QObject::destroyed, this, [this, editor]() {
            d->m_activeEditors.remove(editor);
        });
    }
}

void Client::deactivateDocument(TextEditor::TextDocument *document)
{
    if (d->m_diagnosticManager)
        d->m_diagnosticManager->hideDiagnostics(document->filePath());
    d->resetAssistProviders(document);
    document->setFormatter(nullptr);
    d->m_tokenSupport.deactivateDocument(document);
    d->m_foldingSupport.deactivate(document);
    for (Core::IEditor *editor : Core::DocumentModel::editorsForDocument(document))
        deactivateEditor(editor);
}

void Client::deactivateEditor(Core::IEditor *editor)
{
    d->m_activeEditors.remove(editor);
    TextEditor::TextEditorWidget *widget = TextEditor::TextEditorWidget::fromEditor(editor);
    if (widget) {
        widget->removeHoverHandler(&d->m_hoverHandler);
        widget->setExtraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection, {});
        widget->clearRefactorMarkers(id());
    }
    updateEditorToolBar(editor);
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
    item.languageId(mimeTypeToLanguageId(mimeType));
    item.uri(q->uriFor(filePath));
    item.text(content);
    item.version(version);
    q->sendNotification<DidOpenTextDocumentNotification>(
        DidOpenTextDocumentParams().textDocument(item), Client::SendDocUpdates::Ignore);
}

void ClientPrivate::sendCloseNotification(const FilePath &filePath)
{
    q->sendNotification<DidCloseTextDocumentNotification>(
        DidCloseTextDocumentParams().textDocument(TextDocumentIdentifier().uri(q->uriFor(filePath))),
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
            DidChangeTextDocumentParams params;
            params.textDocument(VersionedTextDocumentIdentifier()
                                    .uri(uriFor(filePath))
                                    .version(++d->m_documentVersions[filePath]));
            params.contentChanges({TextDocumentContentChangeWholeDocument().text(content)});
            sendNotification<DidChangeTextDocumentNotification>(params, SendDocUpdates::Ignore);
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
    const QString method(DidSaveTextDocumentNotification::method);
    if (std::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        send = *registered;
        if (send) {
            const QJsonValue &options = d->m_dynamicCapabilities.option(method);
            send = registrationApplies(options, document->filePath(), document->mimeType());
            const Utils::Result<TextDocumentSaveRegistrationOptions> option
                = fromJson<TextDocumentSaveRegistrationOptions>(options);
            if (option)
                includeText = option->includeText().value_or(includeText);
        }
    } else if (
        const std::optional<ServerCapabilitiesTextDocumentSync> &sync
        = d->m_serverCapabilities.textDocumentSync()) {
        if (const auto options = std::get_if<TextDocumentSyncOptions>(&*sync)) {
            if (const std::optional<TextDocumentSyncOptionsSave> &save = options->save()) {
                if (const auto saveOptions = std::get_if<SaveOptions>(&*save))
                    includeText = saveOptions->includeText().value_or(includeText);
            }
        }
    }
    if (!send || !shouldSendDidSave(document))
        return;
    DidSaveTextDocumentParams params;
    params.textDocument(TextDocumentIdentifier().uri(uriFor(document->filePath())));
    d->openRequiredShadowDocuments(document);
    if (includeText)
        params.text(document->plainText());
    sendNotification<DidSaveTextDocumentNotification>(params, SendDocUpdates::Send, Schedule::Now);
}

void Client::documentWillSave(Core::IDocument *document)
{
    const FilePath &filePath = document->filePath();
    auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);
    if (d->m_openedDocument.find(textDocument) == d->m_openedDocument.end())
        return;
    bool send = false;
    const QString method(WillSaveTextDocumentNotification::method);
    if (std::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        send = *registered;
        if (send) {
            send = registrationApplies(d->m_dynamicCapabilities.option(method), filePath,
                                       document->mimeType());
        }
    } else if (
        const std::optional<ServerCapabilitiesTextDocumentSync> &sync
        = d->m_serverCapabilities.textDocumentSync()) {
        if (const auto options = std::get_if<TextDocumentSyncOptions>(&*sync))
            send = options->willSave().value_or(send);
    }
    if (!send)
        return;
    WillSaveTextDocumentParams params;
    params.textDocument(TextDocumentIdentifier().uri(uriFor(filePath)));
    params.reason(TextDocumentSaveReason::Manual);
    sendNotification<WillSaveTextDocumentNotification>(params);
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
    const QString method(DidChangeTextDocumentNotification::method);
    int syncKind = d->serverTextDocumentSyncKind();
    if (std::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        syncKind = *registered ? TextDocumentSyncKind::Full : TextDocumentSyncKind::None;
        if (syncKind != TextDocumentSyncKind::None) {
            const Utils::Result<TextDocumentChangeRegistrationOptions> option
                = fromJson<TextDocumentChangeRegistrationOptions>(
                    d->m_dynamicCapabilities.option(method));
            if (option)
                syncKind = option->syncKind();
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
                TextDocumentContentChangePartial &prev = queue.last();
                const int prevStart = positionInDocument(prev.range().start(), document->document());
                if (prevStart + prev.text().size() == position) {
                    prev.text(prev.text() + text);
                    append = false;
                }
            }
            if (append) {
                queue << TextDocumentContentChangePartial()
                             .range(rangeOf(cursor))
                             .rangeLength(cursor.selectionEnd() - cursor.selectionStart())
                             .text(text);
            }
        } else {
            d->m_documentsToUpdate[document] = {
                TextDocumentContentChangePartial().text(document->plainText())};
        }
    }
    cursor.insertText(text);

    ++d->m_documentVersions[document->filePath()];
    using namespace TextEditor;
    for (TextEditorWidget *widget : TextEditorWidget::textEditorWidgetsForDocument(document)) {
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

void Client::requestCodeActions(const QString &uri, const Diagnostic &diagnostic)
{
    d->requestCodeActions(uri, diagnostic.range(), {diagnostic});
}

void Client::requestCodeActions(const QString &uri, const QList<Diagnostic> &diagnostics)
{
    d->requestCodeActions(uri, {}, diagnostics);
}

void ClientPrivate::requestCodeActions(
    const QString &uri, const Range &range, const QList<Diagnostic> &diagnostics)
{
    const Utils::FilePath fileName = q->filePathFor(uri);
    TextEditor::TextDocument *doc = TextEditor::TextDocument::textDocumentForFilePath(fileName);
    if (!doc)
        return;

    CodeActionParams codeActionParams;
    codeActionParams.context(CodeActionContext().diagnostics(diagnostics));
    codeActionParams.textDocument(TextDocumentIdentifier().uri(uri));
    if (isEmpty(range)) {
        const QTextBlock &lastBlock = doc->document()->lastBlock();
        codeActionParams.range(
            Range()
                .start(Position().line(0).character(0))
                .end(Position().line(lastBlock.blockNumber()).character(lastBlock.length() - 1)));
    } else {
        codeActionParams.range(range);
    }
    q->requestCodeActions(codeActionParams);
}

void Client::requestCodeActions(const CodeActionParams &params)
{
    const QString uri = params.textDocument().uri();
    const Utils::FilePath fileName = filePathFor(uri);

    const QString method(CodeActionRequest::method);
    if (std::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        if (!*registered)
            return;
        if (!registrationApplies(d->m_dynamicCapabilities.option(method), fileName))
            return;
    } else {
        const ServerCapabilitiesCodeActionProvider provider
            = d->m_serverCapabilities.codeActionProvider().value_or(false);
        const auto boolvalue = std::get_if<bool>(&provider);
        if (boolvalue && !*boolvalue)
            return;
    }

    sendRequest<CodeActionRequest>(
        params,
        [uri, self = QPointer<Client>(this)](const Utils::Result<CodeActionRequestResult> &result) {
            if (self)
                self->handleCodeActionResult(result, uri);
        });
}

void Client::handleCodeActionResult(
    const Utils::Result<CodeActionRequestResult> &result, const QString &uri)
{
    if (!result) {
        log(QtMsgType::QtCriticalMsg, result.error());
        return;
    }
    if (const auto list = std::get_if<QList<CommandOrCodeAction>>(&*result)) {
        QList<CodeAction> codeActions;
        for (const CommandOrCodeAction &item : *list) {
            if (const auto action = std::get_if<CodeAction>(&item))
                codeActions << *action;
        }
        updateCodeActionRefactoringMarker(this, codeActions, uri);
    }
}

void Client::executeCommand(const Command &command)
{
    bool serverSupportsExecuteCommand = d->m_serverCapabilities.executeCommandProvider().has_value();
    serverSupportsExecuteCommand = d->m_dynamicCapabilities
                                       .isRegistered(ExecuteCommandRequest::method)
                                       .value_or(serverSupportsExecuteCommand);
    if (serverSupportsExecuteCommand) {
        sendRequest<ExecuteCommandRequest>(
            ExecuteCommandParams().command(command.command()).arguments(command.arguments()),
            [](const Utils::Result<ExecuteCommandRequestResult> &) {});
    }
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
    const WorkspaceFolder folder
        = WorkspaceFolder().uri(uriFor(project->projectDirectory())).name(project->displayName());
    sendNotification<DidChangeWorkspaceFoldersNotification>(
        DidChangeWorkspaceFoldersParams().event(WorkspaceFoldersChangeEvent().added({folder})));
}

void Client::buildConfigurationClosed(BuildConfiguration *bc)
{
    Project *project = bc->project();
    if (d->sendWorkspceFolderChanges() && canOpenProject(project)) {
        const WorkspaceFolder folder = WorkspaceFolder()
                                           .uri(uriFor(project->projectDirectory()))
                                           .name(project->displayName());
        sendNotification<DidChangeWorkspaceFoldersNotification>(
            DidChangeWorkspaceFoldersParams().event(
                WorkspaceFoldersChangeEvent().removed({folder})));
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

    QJsonValue mergedConfig;
    for (ExtensionSystem::PluginSpec *plugin : ExtensionSystem::PluginManager::plugins()) {
        if (!plugin->isEffectivelyEnabled())
            continue;

        QJsonValue lcValue = plugin->metaData().value("languageclient");
        if (lcValue.isUndefined())
            continue;
        QTC_ASSERT(lcValue.isObject(), continue);
        QJsonObject lspConfig = lcValue.toObject();

        QJsonValue workspaceConfigValue = lspConfig.value("WorkspaceConfig");
        if (workspaceConfigValue.isUndefined())
            continue;
        QTC_ASSERT(workspaceConfigValue.isObject(), continue);
        applyJsonPatch(mergedConfig, workspaceConfigValue);
    }
    if (configuration.isObject())
        applyJsonPatch(mergedConfig, configuration);

    qCDebug(LOGLSPCLIENT).noquote()
        << "Merged configuration for" << name() << ":"
        << QJsonDocument(mergedConfig.toObject()).toJson(QJsonDocument::Indented);

    if (reachable() && !mergedConfig.toObject().isEmpty()
        && d->m_dynamicCapabilities.isRegistered(DidChangeConfigurationNotification::method)
               .value_or(true)) {
        sendNotification<DidChangeConfigurationNotification>(
            DidChangeConfigurationParams().settings(mergedConfig));
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

bool Client::isSupportedUri(const QString &uri) const
{
    const FilePath filePath = filePathFor(uri);
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

bool Client::hasDiagnostic(const FilePath &filePath, const Diagnostic &diag) const
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
    if (dc.isRegistered(DocumentSymbolRequest::method).value_or(false)) {
        return registrationApplies(
            dc.option(DocumentSymbolRequest::method), doc->filePath(), doc->mimeType());
    }
    const std::optional<ServerCapabilitiesDocumentSymbolProvider> &provider
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
    log(QtMsgType::QtCriticalMsg, message);
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

void Client::handleMessage(const QJsonObject &message)
{
    LanguageClientManager::logJsonRpcMessage(LspLogMessage::ServerMessage, name(), message);
    const MessageId id = messageId(message);
    const QString method = messageMethod(message);
    if (method.isEmpty())
        d->handleResponse(id, message);
    else
        d->handleMethod(method, id, message);
}

void Client::log(QtMsgType msgType, const QString &message) const
{
    switch (d->m_logTarget) {
    case LogTarget::Ui:
        switch (msgType) {
        case QtMsgType::QtDebugMsg:
        case QtMsgType::QtInfoMsg:
            qCDebug(LOGLSPCLIENT) << message;
            break;
        case QtMsgType::QtWarningMsg:
        case QtMsgType::QtCriticalMsg:
        case QtMsgType::QtFatalMsg:
            Core::MessageManager::writeFlashing(
                QString("LanguageClient %1: %2").arg(name(), message));
        }
        break;
    case LogTarget::Console:
        switch (msgType) {
        case QtMsgType::QtDebugMsg:
            qCDebug(LOGLSPCLIENT) << message;
            break;
        case QtMsgType::QtInfoMsg:
            qCInfo(LOGLSPCLIENT) << message;
            break;
        case QtMsgType::QtWarningMsg:
            qCWarning(LOGLSPCLIENT) << message;
            break;
        case QtMsgType::QtCriticalMsg:
        case QtMsgType::QtFatalMsg:
            qCCritical(LOGLSPCLIENT) << message;
        }
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

void Client::foldOrUnfoldCommentBlocks(TextEditor::TextEditorWidget *widget, bool fold)
{
    d->m_foldingSupport.foldOrUnfoldCommentBlocks(widget, fold);
}

void Client::foldOrUnfoldInactiveRegions(TextEditor::TextEditorWidget *widget, bool fold)
{
    d->m_foldingSupport.foldOrUnfoldInactiveRegions(widget, fold);
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

void ClientPrivate::log(int messageType, const QString &message)
{
    QtMsgType type = QtDebugMsg;
    switch (messageType) {
    case MessageType::Error:
        type = QtCriticalMsg;
        break;
    case MessageType::Warning:
        type = QtWarningMsg;
        break;
    case MessageType::Info:
        type = QtInfoMsg;
        break;
    default:
        break;
    }
    q->log(type, message);
}

ShowMessageRequestResult ClientPrivate::showMessageBox(const ShowMessageRequestParams &message)
{
    QMessageBox box;
    box.setWindowTitle(q->name());
    box.setText(message.message());
    switch (message.type()) {
    case MessageType::Error:
        box.setIcon(QMessageBox::Critical);
        break;
    case MessageType::Warning:
        box.setIcon(QMessageBox::Warning);
        break;
    case MessageType::Info:
        box.setIcon(QMessageBox::Information);
        break;
    default:
        box.setIcon(QMessageBox::NoIcon);
        break;
    }

    QHash<QAbstractButton *, MessageActionItem> itemForButton;
    if (const std::optional<QList<MessageActionItem>> &actions = message.actions()) {
        for (const MessageActionItem &action : *actions) {
            auto button = box.addButton(action.title(), QMessageBox::ActionRole);
            connect(button, &QPushButton::clicked, &box, &QMessageBox::accept);
            itemForButton.insert(button, action);
        }
    }

    if (box.exec() == QDialog::Rejected || itemForButton.isEmpty())
        return std::monostate{};
    return itemForButton.value(box.clickedButton());
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
        DidChangeTextDocumentParams params;
    };
    const auto updates = Utils::transform<QList<DocumentUpdate>>(m_documentsToUpdate,
                                                                 [this](const auto &elem) {
        TextEditor::TextDocument * const document = elem.first;
        const FilePath &filePath = document->filePath();
        DidChangeTextDocumentParams params;
        params.textDocument(VersionedTextDocumentIdentifier()
                                .uri(q->uriFor(filePath))
                                .version(m_documentVersions[filePath]));
        params.contentChanges(
            Utils::transform(elem.second, [](const TextDocumentContentChangePartial &c) {
                return TextDocumentContentChangeEvent(c);
            }));
        return DocumentUpdate{document, params};
    });
    m_documentsToUpdate.clear();

    for (const DocumentUpdate &update : updates) {
        q->sendNotification<DidChangeTextDocumentNotification>(
            update.params, Client::SendDocUpdates::Ignore);
        emit q->documentUpdated(update.document);

        if (currentWidget && currentWidget->textDocument() == update.document)
            requestDocumentHighlights(currentWidget);

        switch (semanticTokensSchedule) {
        case Schedule::Now:
            m_tokenSupport.updateSemanticTokens(update.document);
            m_foldingSupport.requestFoldingRanges(update.document);
            break;
        case Schedule::Delayed:
            QTimer::singleShot(m_documentUpdateTimer.interval(), this,
                               [this, doc = QPointer(update.document)] {
                if (doc && m_documentsToUpdate.find(doc) == m_documentsToUpdate.end()) {
                    m_tokenSupport.updateSemanticTokens(doc);
                    m_foldingSupport.requestFoldingRanges(doc);
                }
            });
            break;
        }
    }
}

void ClientPrivate::handleResponse(const MessageId &id, const QJsonObject &message)
{
    if (auto handler = m_responseHandlers.take(id))
        handler(message);
}

static QJsonObject invalidParamsResponse(const MessageId &id, const QString &message)
{
    return errorResponseObject(id, ResponseError{ErrorCodes::InvalidParams, message, {}});
}

void ClientPrivate::handleMethod(
    const QString &method, const MessageId &id, const QJsonObject &message)
{
    const auto customHandler = m_customHandlers.constFind(method);
    if (customHandler != m_customHandlers.constEnd()) {
        const bool isHandled = (*customHandler)(message);
        if (isHandled)
            return;
    }

    auto createDefaultResponse = [&] {
        QTC_CHECK(id.isValid());
        return responseObject<ShutdownRequest>(id, {});
    };

    const bool isRequest = id.isValid();
    bool responseSend = false;
    auto sendResponse = [&](const QJsonObject &response) {
        responseSend = true;
        if (q->reachable()) {
            q->sendRawMessage(response);
        } else {
            qCDebug(LOGLSPCLIENT)
                << QString("Dropped response to request %1 id %2 for unreachable server %3")
                       .arg(method, id.toString(), q->name());
        }
    };

    // Answers the request with \a result, or with the reason its parameters
    // could not be read.
    const auto respondTo = [&]<typename M>(const Utils::Result<typename M::Params> &params,
                                           auto &&handler) {
        if (!params) {
            q->log(QtMsgType::QtCriticalMsg, params.error());
            if (isRequest)
                sendResponse(invalidParamsResponse(id, params.error()));
            return;
        }
        if constexpr (M::isRequest)
            sendResponse(responseObject<M>(id, handler(*params)));
        else
            handler(*params);
    };

    if (method == PublishDiagnosticsNotification::method) {
        respondTo.template operator()<PublishDiagnosticsNotification>(
            LanguageServerProtocol::params<PublishDiagnosticsNotification>(message),
            [&](const PublishDiagnosticsParams &params) {
                q->handleDiagnostics(params, message.value("params").toObject());
            });
    } else if (method == LogMessageNotification::method) {
        respondTo.template operator()<LogMessageNotification>(
            LanguageServerProtocol::params<LogMessageNotification>(message),
            [&](const LogMessageParams &params) { log(params.type(), params.message()); });
    } else if (method == ShowMessageNotification::method) {
        respondTo.template operator()<ShowMessageNotification>(
            LanguageServerProtocol::params<ShowMessageNotification>(message),
            [&](const ShowMessageParams &params) { log(params.type(), params.message()); });
    } else if (method == ShowMessageRequest::method) {
        respondTo.template operator()<ShowMessageRequest>(
            LanguageServerProtocol::params<ShowMessageRequest>(message),
            [&](const ShowMessageRequestParams &params) { return showMessageBox(params); });
    } else if (method == RegistrationRequest::method) {
        respondTo.template operator()<RegistrationRequest>(
            LanguageServerProtocol::params<RegistrationRequest>(message),
            [&](const RegistrationParams &params) {
                q->registerCapabilities(params.registrations());
                return std::monostate{};
            });
    } else if (method == UnregistrationRequest::method) {
        respondTo.template operator()<UnregistrationRequest>(
            LanguageServerProtocol::params<UnregistrationRequest>(message),
            [&](const UnregistrationParams &params) {
                q->unregisterCapabilities(params.unregisterations());
                return std::monostate{};
            });
    } else if (method == ApplyWorkspaceEditRequest::method) {
        respondTo.template operator()<ApplyWorkspaceEditRequest>(
            LanguageServerProtocol::params<ApplyWorkspaceEditRequest>(message),
            [&](const ApplyWorkspaceEditParams &params) {
                return ApplyWorkspaceEditResult().applied(applyWorkspaceEdit(q, params.edit()));
            });
    } else if (method == WorkspaceFoldersRequest::method) {
        const QList<ProjectExplorer::Project *> projects
            = ProjectExplorer::ProjectManager::projects();
        WorkspaceFoldersRequestResult result{std::monostate{}};
        if (!projects.isEmpty()) {
            result = Utils::transform(projects, [this](ProjectExplorer::Project *project) {
                return WorkspaceFolder()
                    .uri(q->uriFor(project->projectDirectory()))
                    .name(project->displayName());
            });
        }
        sendResponse(responseObject<WorkspaceFoldersRequest>(id, result));
    } else if (method == WorkDoneProgressCreateRequest::method) {
        sendResponse(createDefaultResponse());
    } else if (method == SemanticTokensRefreshRequest::method) {
        m_tokenSupport.refresh();
        sendResponse(createDefaultResponse());
    } else if (method == FoldingRangeRefreshRequest::method) {
        m_foldingSupport.refresh();
        sendResponse(createDefaultResponse());
    } else if (method == ProgressNotification::method) {
        respondTo.template operator()<ProgressNotification>(
            LanguageServerProtocol::params<ProgressNotification>(message),
            [&](const ProgressParams &params) {
                m_progressManager.handleProgress(params);
                if (ProgressManager::isProgressEndMessage(params))
                    emit q->workDone(params.token());
            });
    } else if (method == ConfigurationRequest::method) {
        QJsonArray result;
        const Utils::Result<ConfigurationParams> params
            = LanguageServerProtocol::params<ConfigurationRequest>(message);
        if (params) {
            for (const ConfigurationItem &item : params->items()) {
                if (const std::optional<QString> &section = item.section())
                    result.append(m_configuration[*section]);
                else
                    result.append({});
            }
        }
        sendResponse(responseObject<ConfigurationRequest>(id, result));
    } else if (isRequest) {
        sendResponse(errorResponseObject(
            id,
            ResponseError{ErrorCodes::MethodNotFound,
                          QString("The client cannot handle the method '%1'.").arg(method),
                          {}}));
    }

    // we got a request and handled it somewhere above but we missed to generate a response for it
    QTC_ASSERT(!isRequest || responseSend, sendResponse(createDefaultResponse()));
}

void Client::handleDiagnostics(const PublishDiagnosticsParams &params, const QJsonObject &raw)
{
    Q_UNUSED(raw)
    const QString &uri = params.uri();
    if (!d->m_diagnosticManager)
        d->m_diagnosticManager = createDiagnosticManager();
    const FilePath &path = filePathFor(uri);
    d->m_diagnosticManager->setDiagnostics(path, params.diagnostics(), params.version());
    if (LanguageClientManager::clientForFilePath(path) == this) {
        d->m_diagnosticManager->showDiagnostics(path, d->m_documentVersions.value(path));
        if (d->m_autoRequestCodeActions)
            requestCodeActions(uri, params.diagnostics());
    }
}

int ClientPrivate::serverTextDocumentSyncKind() const
{
    const std::optional<ServerCapabilitiesTextDocumentSync> &sync
        = m_serverCapabilities.textDocumentSync();
    if (!sync)
        return TextDocumentSyncKind::None;
    if (const auto kind = std::get_if<int>(&*sync))
        return *kind;
    if (const auto options = std::get_if<TextDocumentSyncOptions>(&*sync)) {
        if (const std::optional<int> &kind = options->change())
            return *kind;
    }
    return TextDocumentSyncKind::None;
}

void ClientPrivate::sendMessageNow(const QJsonObject &message)
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

int Client::documentVersion(const QString &uri) const
{
    return documentVersion(filePathFor(uri));
}

void Client::setDocumentChangeUpdateThreshold(int msecs)
{
    d->m_documentUpdateTimer.setInterval(msecs);
}

void ClientPrivate::initializeCallback(const Utils::Result<InitializeResult> &result)
{
    if (m_state != Client::InitializeRequested) {
        qCWarning(LOGLSPCLIENT) << "Dropping initialize response in unexpected state " << m_state;
        return;
    }
    if (!result) {
        q->setError(Tr::tr("Initialization error: %1.").arg(result.error()));
        emit q->finished();
        return;
    }

    if (const std::optional<ServerInfo> &serverInfo = result->serverInfo()) {
        m_serverName = serverInfo->name();
        if (const std::optional<QString> &version = serverInfo->version())
            m_serverVersion = *version;
    }
    m_serverCapabilities = result->capabilities();

    if (auto completionProvider = qobject_cast<LanguageClientCompletionAssistProvider *>(
            m_clientProviders.completionAssistProvider)) {
        completionProvider->setTriggerCharacters(m_serverCapabilities.completionProvider()
                                                     .value_or(CompletionOptions())
                                                     .triggerCharacters());
    }
    if (auto functionHintAssistProvider = qobject_cast<FunctionHintAssistProvider *>(
            m_clientProviders.functionHintProvider)) {
        functionHintAssistProvider->setTriggerCharacters(
            m_serverCapabilities.signatureHelpProvider()
                .value_or(SignatureHelpOptions())
                .triggerCharacters());
    }
    if (const std::optional<ServerCapabilitiesSemanticTokensProvider> &tokenProvider
        = m_serverCapabilities.semanticTokensProvider()) {
        if (const auto options = std::get_if<SemanticTokensOptions>(&*tokenProvider))
            m_tokenSupport.setLegend(options->legend());
        else if (const auto options = std::get_if<SemanticTokensRegistrationOptions>(&*tokenProvider))
            m_tokenSupport.setLegend(options->legend());
    }

    qCDebug(LOGLSPCLIENT) << "language server " << m_displayName << " initialized";
    setState(Client::Initialized);
    q->sendNotification<InitializedNotification>({});

    q->updateConfiguration(m_configuration);

    m_tokenSupport.clearTokens(); // clear cached tokens from a pre reset run
    for (TextEditor::TextDocument *doc : std::as_const(m_postponedDocuments))
        q->openDocument(doc);
    m_postponedDocuments.clear();

    emit q->initialized(m_serverCapabilities);
}

void ClientPrivate::shutDownCallback(const Utils::Result<std::monostate> &result)
{
    m_shutdownTimer.stop();
    QTC_ASSERT(m_state == Client::ShutdownRequested, return);
    QTC_ASSERT(m_clientInterface, return);
    if (!result)
        q->log(QtMsgType::QtCriticalMsg, result.error());
    // directly send content now otherwise the state check of sendContent would fail
    sendMessageNow(notificationObject<ExitNotification>());
    qCDebug(LOGLSPCLIENT) << "language server " << m_displayName << " shutdown";
    setState(Client::Shutdown);
    m_shutdownTimer.start();
}

bool ClientPrivate::sendWorkspceFolderChanges() const
{
    if (!q->reachable())
        return false;
    if (m_dynamicCapabilities.isRegistered(DidChangeWorkspaceFoldersNotification::method)
            .value_or(false)) {
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

LanguageClientOutlineItem *Client::createOutlineItem(const DocumentSymbol &symbol)
{
    return new LanguageClientOutlineItem(this, symbol);
}

static FilePath toHostPath(const FilePath serverDeviceTemplate, const FilePath localClientPath)
{
    const FilePath onDevice = serverDeviceTemplate.withNewPath(localClientPath.path());
    return onDevice.localSource().value_or(onDevice);
}

QString Client::uriFor(const Utils::FilePath &path) const
{
    return uriFromPath(d->m_serverDeviceTemplate.withNewPath(path.path()));
}

FilePath Client::filePathFor(const QString &uri) const
{
    const FilePath serverPath = pathFromUri(uri);
    if (serverPath.isEmpty())
        return {};

    // If the server is local, no mapping is needed
    if (d->m_serverDeviceTemplate.isLocal())
        return serverPath;

    // If both server and project are on the same device, map the paths to the remote device.
    if (project() && project()->projectFilePath().isSameDevice(d->m_serverDeviceTemplate))
        return d->m_serverDeviceTemplate.withNewPath(serverPath.path());

    // If the server is remote, but the project is local, map server paths to host paths
    return toHostPath(d->m_serverDeviceTemplate, serverPath);
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
