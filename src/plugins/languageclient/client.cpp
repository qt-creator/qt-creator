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

#include "client.h"

#include "diagnosticmanager.h"
#include "documentsymbolcache.h"
#include "languageclientcompletionassist.h"
#include "languageclientformatter.h"
#include "languageclientfunctionhint.h"
#include "languageclienthoverhandler.h"
#include "languageclientinterface.h"
#include "languageclientmanager.h"
#include "languageclientquickfix.h"
#include "languageclientsymbolsupport.h"
#include "languageclientutils.h"
#include "progressmanager.h"
#include "semantichighlightsupport.h"

#include <app/app_version.h>

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

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
#include <projectexplorer/session.h>

#include <texteditor/codeassist/documentcontentcompletion.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/ioutlinewidget.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>

#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>

#include <QDebug>
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
    ClientPrivate(Client *client, BaseClientInterface *clientInterface)
        : q(client)
        , m_id(Utils::Id::fromString(QUuid::createUuid().toString()))
        , m_clientCapabilities(q->defaultClientCapabilities())
        , m_clientInterface(new InterfaceController(clientInterface))
        , m_documentSymbolCache(q)
        , m_hoverHandler(q)
        , m_symbolSupport(q)
        , m_tokenSupport(q)
    {
        using namespace ProjectExplorer;

        m_clientInfo.setName(Core::Constants::IDE_DISPLAY_NAME);
        m_clientInfo.setVersion(Core::Constants::IDE_VERSION_DISPLAY);

        m_clientProviders.completionAssistProvider = new LanguageClientCompletionAssistProvider(q);
        m_clientProviders.functionHintProvider = new FunctionHintAssistProvider(q);
        m_clientProviders.quickFixAssistProvider = new LanguageClientQuickFixProvider(q);

        m_documentUpdateTimer.setSingleShot(true);
        m_documentUpdateTimer.setInterval(500);
        connect(&m_documentUpdateTimer, &QTimer::timeout, this,
                [this] { sendPostponedDocumentUpdates(Schedule::Now); });
        connect(SessionManager::instance(), &SessionManager::projectRemoved,
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
    }

    ~ClientPrivate()
    {
        using namespace TextEditor;
        // FIXME: instead of replacing the completion provider in the text document store the
        // completion provider as a prioritised list in the text document
        // temporary container needed since m_resetAssistProvider is changed in resetAssistProviders
        for (TextDocument *document : m_resetAssistProvider.keys())
            resetAssistProviders(document);
        const QList<Core::IEditor *> &editors = Core::DocumentModel::editorsForOpenedDocuments();
        for (Core::IEditor *editor : editors) {
            if (auto textEditor = qobject_cast<BaseTextEditor *>(editor)) {
                TextEditorWidget *widget = textEditor->editorWidget();
                widget->setRefactorMarkers(RefactorMarker::filterOutType(widget->refactorMarkers(), m_id));
                widget->removeHoverHandler(&m_hoverHandler);
            }
        }
        for (IAssistProcessor *processor : qAsConst(m_runningAssistProcessors))
            processor->setAsyncProposalAvailable(nullptr);
        qDeleteAll(m_documentHighlightsTimer);
        m_documentHighlightsTimer.clear();
        updateEditorToolBar(m_openedDocument.keys());
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

    bool reset();

    Client::State m_state = Client::Uninitialized;
    QHash<LanguageServerProtocol::MessageId,
          LanguageServerProtocol::ResponseHandler::Callback> m_responseHandlers;
    QString m_displayName;
    LanguageFilter m_languagFilter;
    QJsonObject m_initializationOptions;
    QMap<TextEditor::TextDocument *, QString> m_openedDocument;
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
    int m_restartsLeft = 5;
    InterfaceController *m_clientInterface = nullptr;
    DiagnosticManager *m_diagnosticManager = nullptr;
    DocumentSymbolCache m_documentSymbolCache;
    HoverHandler m_hoverHandler;
    QHash<LanguageServerProtocol::DocumentUri, TextEditor::HighlightingResults> m_highlights;
    ProjectExplorer::Project *m_project = nullptr;
    QSet<TextEditor::IAssistProcessor *> m_runningAssistProcessors;
    SymbolSupport m_symbolSupport;
    ProgressManager m_progressManager;
    bool m_activateDocAutomatically = false;
    SemanticTokenSupport m_tokenSupport;
    QString m_serverName;
    QString m_serverVersion;
    LanguageServerProtocol::SymbolStringifier m_symbolStringifier;
    Client::LogTarget m_logTarget = Client::LogTarget::Ui;
    bool m_locatorsEnabled = true;
    bool m_autoRequestCodeActions = true;
    QTimer m_shutdownTimer;
    LanguageServerProtocol::ClientInfo m_clientInfo;
};

Client::Client(BaseClientInterface *clientInterface)
    : d(new ClientPrivate(this, clientInterface))
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
        return tr("%1 for %2").arg(d->m_displayName, d->m_project->displayName());
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
        params.setRootUri(DocumentUri::fromFilePath(d->m_project->projectDirectory()));

    const QList<WorkSpaceFolder> workspaces
        = Utils::transform(SessionManager::projects(), [](Project *pro) {
              return WorkSpaceFolder(DocumentUri::fromFilePath(pro->projectDirectory()),
                                     pro->displayName());
          });
    if (workspaces.isEmpty())
        params.setWorkSpaceFolders(nullptr);
    else
        params.setWorkSpaceFolders(workspaces);
    InitializeRequest initRequest(params);
    initRequest.setResponseCallback([this](const InitializeRequest::Response &initResponse){
        d->initializeCallback(initResponse);
    });
    if (Utils::optional<ResponseHandler> responseHandler = initRequest.responseHandler())
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
    case Uninitialized: return tr("uninitialized");
    case InitializeRequested: return tr("initialize requested");
    case Initialized: return tr("initialized");
    case ShutdownRequested: return tr("shutdown requested");
    case Shutdown: return tr("shutdown");
    case Error: return tr("error");
    }
    return {};
}

bool Client::reachable() const
{
    return d->m_state == Initialized;
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
    const QString method(DidOpenTextDocumentNotification::methodName);
    if (Utils::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        if (!*registered)
            return;
        const TextDocumentRegistrationOptions option(
            d->m_dynamicCapabilities.option(method).toObject());
        if (option.isValid()
            && !option.filterApplies(filePath, Utils::mimeTypeForName(document->mimeType()))) {
            return;
        }
    } else if (Utils::optional<ServerCapabilities::TextDocumentSync> _sync
               = d->m_serverCapabilities.textDocumentSync()) {
        if (auto options = Utils::get_if<TextDocumentSyncOptions>(&*_sync)) {
            if (!options->openClose().value_or(true))
                return;
        }
    }

    d->m_openedDocument[document] = document->plainText();
    connect(document, &TextDocument::contentsChangedWithPosition, this,
            [this, document](int position, int charsRemoved, int charsAdded) {
        documentContentsChanged(document, position, charsRemoved, charsAdded);
    });
    TextDocumentItem item;
    item.setLanguageId(TextDocumentItem::mimeTypeToLanguageId(document->mimeType()));
    item.setUri(DocumentUri::fromFilePath(filePath));
    item.setText(document->plainText());
    if (!d->m_documentVersions.contains(filePath))
        d->m_documentVersions[filePath] = 0;
    item.setVersion(d->m_documentVersions[filePath]);
    sendMessage(DidOpenTextDocumentNotification(DidOpenTextDocumentParams(item)));
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
    QTC_ASSERT(d->m_clientInterface, return);
    QTC_ASSERT(d->m_state == Initialized, return);
    if (sendUpdates == SendDocUpdates::Send)
        d->sendPostponedDocumentUpdates(semanticTokensSchedule);
    if (Utils::optional<ResponseHandler> responseHandler = message.responseHandler())
        d->m_responseHandlers[responseHandler->id] = responseHandler->callback;
    QString error;
    if (!QTC_GUARD(message.isValid(&error)))
        Core::MessageManager::writeFlashing(error);
    d->sendMessageNow(message);
}

void Client::cancelRequest(const MessageId &id)
{
    d->m_responseHandlers.remove(id);
    sendMessage(CancelRequest(CancelParameter(id)), SendDocUpdates::Ignore);
}

void Client::closeDocument(TextEditor::TextDocument *document)
{
    deactivateDocument(document);
    const DocumentUri &uri = DocumentUri::fromFilePath(document->filePath());
    d->m_postponedDocuments.remove(document);
    if (d->m_openedDocument.remove(document) != 0) {
        handleDocumentClosed(document);
        if (d->m_state == Initialized) {
            DidCloseTextDocumentParams params(TextDocumentIdentifier{uri});
            sendMessage(DidCloseTextDocumentNotification(params));
        }
    }
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
        const auto uri = DocumentUri::fromFilePath(widget->textDocument()->filePath());
        if (m_highlightRequests.contains(widget))
            q->cancelRequest(m_highlightRequests.take(widget));
        timer = new QTimer;
        timer->setSingleShot(true);
        m_documentHighlightsTimer.insert(widget, timer);
        auto connection = connect(widget, &QWidget::destroyed, this, [widget, this]() {
            delete m_documentHighlightsTimer.take(widget);
        });
        connect(timer, &QTimer::timeout, this, [this, widget, connection]() {
            disconnect(connection);
            requestDocumentHighlightsNow(widget);
            m_documentHighlightsTimer.take(widget)->deleteLater();
        });
    }
    timer->start(250);
}

void ClientPrivate::requestDocumentHighlightsNow(TextEditor::TextEditorWidget *widget)
{
    const auto uri = DocumentUri::fromFilePath(widget->textDocument()->filePath());
    if (m_dynamicCapabilities.isRegistered(DocumentHighlightsRequest::methodName).value_or(false)) {
        TextDocumentRegistrationOptions option(
            m_dynamicCapabilities.option(DocumentHighlightsRequest::methodName));
        if (!option.filterApplies(widget->textDocument()->filePath()))
            return;
    } else {
        Utils::optional<Utils::variant<bool, WorkDoneProgressOptions>> provider
            = m_serverCapabilities.documentHighlightProvider();
        if (!provider.has_value())
            return;
        if (Utils::holds_alternative<bool>(*provider) && !Utils::get<bool>(*provider))
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
        [widget, this, uri, connection]
        (const DocumentHighlightsRequest::Response &response)
        {
            m_highlightRequests.remove(widget);
            disconnect(connection);
            const Id &id = TextEditor::TextEditorWidget::CodeSemanticsSelection;
            QList<QTextEdit::ExtraSelection> selections;
            const Utils::optional<DocumentHighlightsResult> &result = response.result();
            if (!result.has_value() || holds_alternative<std::nullptr_t>(*result)) {
                widget->setExtraSelections(id, selections);
                return;
            }

            const QTextCharFormat &format =
                widget->textDocument()->fontSettings().toTextCharFormat(TextEditor::C_OCCURRENCES);
            QTextDocument *document = widget->document();
            for (const auto &highlight : get<QList<DocumentHighlight>>(*result)) {
                QTextEdit::ExtraSelection selection{widget->textCursor(), format};
                const int &start = highlight.range().start().toPositionInDocument(document);
                const int &end = highlight.range().end().toPositionInDocument(document);
                if (start < 0 || end < 0)
                    continue;
                selection.cursor.setPosition(start);
                selection.cursor.setPosition(end, QTextCursor::KeepAnchor);
                selections << selection;
            }
            widget->setExtraSelections(id, selections);
        });
    m_highlightRequests[widget] = request.id();
    q->sendMessage(request);
}

void Client::activateDocument(TextEditor::TextDocument *document)
{
    const FilePath &filePath = document->filePath();
    auto uri = DocumentUri::fromFilePath(filePath);
    if (d->m_diagnosticManager)
        d->m_diagnosticManager->showDiagnostics(uri, d->m_documentVersions.value(filePath));
    d->m_tokenSupport.updateSemanticTokens(document);
    // only replace the assist provider if the language server support it
    d->updateCompletionProvider(document);
    d->updateFunctionHintProvider(document);
    if (d->m_serverCapabilities.codeActionProvider()) {
        d->m_resetAssistProvider[document].quickFixAssistProvider = document->quickFixAssistProvider();
        document->setQuickFixAssistProvider(d->m_clientProviders.quickFixAssistProvider);
    }
    document->setFormatter(new LanguageClientFormatter(document, this));
    for (Core::IEditor *editor : Core::DocumentModel::editorsForDocument(document)) {
        updateEditorToolBar(editor);
        if (auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor)) {
            TextEditor::TextEditorWidget *widget = textEditor->editorWidget();
            widget->addHoverHandler(&d->m_hoverHandler);
            d->requestDocumentHighlights(widget);
            if (symbolSupport().supportsRename(document))
                widget->addOptionalActions(TextEditor::TextEditorActionHandler::RenameSymbol);
        }
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
        }
    }
}

void ClientPrivate::documentClosed(Core::IDocument *document)
{
    if (auto textDocument = qobject_cast<TextEditor::TextDocument *>(document))
        q->closeDocument(textDocument);
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

void Client::documentContentsSaved(TextEditor::TextDocument *document)
{
    if (!d->m_openedDocument.contains(document))
        return;
    bool send = true;
    bool includeText = false;
    const QString method(DidSaveTextDocumentNotification::methodName);
    if (Utils::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
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
    } else if (Utils::optional<ServerCapabilities::TextDocumentSync> _sync
               = d->m_serverCapabilities.textDocumentSync()) {
        if (auto options = Utils::get_if<TextDocumentSyncOptions>(&*_sync)) {
            if (Utils::optional<SaveOptions> saveOptions = options->save())
                includeText = saveOptions->includeText().value_or(includeText);
        }
    }
    if (!send)
        return;
    DidSaveTextDocumentParams params(
                TextDocumentIdentifier(DocumentUri::fromFilePath(document->filePath())));
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
    if (Utils::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        send = *registered;
        if (send) {
            const TextDocumentRegistrationOptions option(d->m_dynamicCapabilities.option(method));
            if (option.isValid()) {
                send = option.filterApplies(filePath,
                                                   Utils::mimeTypeForName(document->mimeType()));
            }
        }
    } else if (Utils::optional<ServerCapabilities::TextDocumentSync> _sync
               = d->m_serverCapabilities.textDocumentSync()) {
        if (auto options = Utils::get_if<TextDocumentSyncOptions>(&*_sync))
            send = options->willSave().value_or(send);
    }
    if (!send)
        return;
    const WillSaveTextDocumentParams params(
        TextDocumentIdentifier(DocumentUri::fromFilePath(filePath)));
    sendMessage(WillSaveTextDocumentNotification(params));
}

void Client::documentContentsChanged(TextEditor::TextDocument *document,
                                     int position,
                                     int charsRemoved,
                                     int charsAdded)
{
    if (!d->m_openedDocument.contains(document) || !reachable())
        return;
    const QString method(DidChangeTextDocumentNotification::methodName);
    TextDocumentSyncKind syncKind = d->m_serverCapabilities.textDocumentSyncKindHelper();
    if (Utils::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        syncKind = *registered ? TextDocumentSyncKind::Full : TextDocumentSyncKind::None;
        if (syncKind != TextDocumentSyncKind::None) {
            const TextDocumentChangeRegistrationOptions option(
                                    d->m_dynamicCapabilities.option(method).toObject());
            syncKind = option.isValid() ? option.syncKind() : syncKind;
        }
    }

    if (syncKind != TextDocumentSyncKind::None) {
        if (syncKind == TextDocumentSyncKind::Incremental) {
            // If the new change is a pure insertion and its range is adjacent to the range of the
            // previous change, we can trivially merge the two changes.
            // For the typical case of the user typing a continuous sequence of characters,
            // this will save a lot of TextDocumentContentChangeEvent elements in the data stream,
            // as otherwise we'd send tons of single-character changes.
            const QString &text = document->textAt(position, charsAdded);
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
                QTextDocument oldDoc(d->m_openedDocument[document]);
                QTextCursor cursor(&oldDoc);
                // Workaround https://bugreports.qt.io/browse/QTBUG-80662
                // The contentsChanged gives a character count that can be wrong for QTextCursor
                // when there are special characters removed/added (like formating characters).
                // Also, characterCount return the number of characters + 1 because of the hidden
                // paragraph separator character.
                // This implementation is based on QWidgetTextControlPrivate::_q_contentsChanged.
                // For charsAdded, textAt handles the case itself.
                cursor.setPosition(qMin(oldDoc.characterCount() - 1, position + charsRemoved));
                cursor.setPosition(position, QTextCursor::KeepAnchor);
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
        d->m_openedDocument[document] = document->plainText();
    }

    ++d->m_documentVersions[document->filePath()];
    using namespace TextEditor;
    for (BaseTextEditor *editor : BaseTextEditor::textEditorsForDocument(document)) {
        TextEditorWidget *widget = editor->editorWidget();
        QTC_ASSERT(widget, continue);
        delete d->m_documentHighlightsTimer.take(widget);
        widget->setRefactorMarkers(RefactorMarker::filterOutType(widget->refactorMarkers(), id()));
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
    const Utils::FilePath fileName = uri.toFilePath();
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

    const Utils::FilePath fileName
        = request.params().value_or(CodeActionParams()).textDocument().uri().toFilePath();

    const QString method(CodeActionRequest::methodName);
    if (Utils::optional<bool> registered = d->m_dynamicCapabilities.isRegistered(method)) {
        if (!*registered)
            return;
        const TextDocumentRegistrationOptions option(
            d->m_dynamicCapabilities.option(method).toObject());
        if (option.isValid() && !option.filterApplies(fileName))
            return;
    } else {
        Utils::variant<bool, CodeActionOptions> provider
            = d->m_serverCapabilities.codeActionProvider().value_or(false);
        if (!(Utils::holds_alternative<CodeActionOptions>(provider) || Utils::get<bool>(provider)))
            return;
    }

    sendMessage(request);
}

void Client::handleCodeActionResponse(const CodeActionRequest::Response &response,
                                          const DocumentUri &uri)
{
    if (const Utils::optional<CodeActionRequest::Response::Error> &error = response.error())
        log(*error);
    if (const Utils::optional<CodeActionResult> &result = response.result()) {
        if (auto list = Utils::get_if<QList<Utils::variant<Command, CodeAction>>>(&*result)) {
            QList<CodeAction> codeActions;
            for (const Utils::variant<Command, CodeAction> &item : *list) {
                if (auto action = Utils::get_if<CodeAction>(&item))
                    codeActions << *action;
                else if (auto command = Utils::get_if<Command>(&item))
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
    if (!d->sendWorkspceFolderChanges())
        return;
    WorkspaceFoldersChangeEvent event;
    event.setAdded({WorkSpaceFolder(DocumentUri::fromFilePath(project->projectDirectory()),
                                    project->displayName())});
    DidChangeWorkspaceFoldersParams params;
    params.setEvent(event);
    DidChangeWorkspaceFoldersNotification change(params);
    sendMessage(change);
}

void Client::projectClosed(ProjectExplorer::Project *project)
{
    if (d->sendWorkspceFolderChanges()) {
        WorkspaceFoldersChangeEvent event;
        event.setRemoved({WorkSpaceFolder(DocumentUri::fromFilePath(project->projectDirectory()),
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

void Client::updateConfiguration(const QJsonValue &configuration)
{
    if (d->m_dynamicCapabilities.isRegistered(DidChangeConfigurationNotification::methodName)
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
    const FilePath &filePath = uri.toFilePath();
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

QList<Diagnostic> Client::diagnosticsAt(const DocumentUri &uri, const QTextCursor &cursor) const
{
    if (d->m_diagnosticManager)
        return d->m_diagnosticManager->diagnosticsAt(uri, cursor);
    return {};
}

bool Client::hasDiagnostic(const LanguageServerProtocol::DocumentUri &uri,
                           const LanguageServerProtocol::Diagnostic &diag) const
{
    if (d->m_diagnosticManager)
        return d->m_diagnosticManager->hasDiagnostic(uri, documentForFilePath(uri.toFilePath()), diag);
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

void Client::setSymbolStringifier(const LanguageServerProtocol::SymbolStringifier &stringifier)
{
    d->m_symbolStringifier = stringifier;
}

SymbolStringifier Client::symbolStringifier() const
{
    return d->m_symbolStringifier;
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
    if (!doc)
        return false;
    DynamicCapabilities dc = dynamicCapabilities();
    if (dc.isRegistered(DocumentSymbolsRequest::methodName).value_or(false)) {
        TextDocumentRegistrationOptions options(dc.option(DocumentSymbolsRequest::methodName));
        return !options.isValid()
               || options.filterApplies(doc->filePath(), Utils::mimeTypeForName(doc->mimeType()));
    }
    const Utils::optional<Utils::variant<bool, WorkDoneProgressOptions>> &provider
        = capabilities().documentSymbolProvider();
    if (!provider.has_value())
        return false;
    if (Utils::holds_alternative<bool>(*provider))
        return Utils::get<bool>(*provider);
    return true;
}

void Client::setLogTarget(LogTarget target)
{
    d->m_logTarget = target;
}

void Client::start()
{
    LanguageClientManager::addClient(this);
    d->m_clientInterface->start();
}

bool Client::reset()
{
    return d->reset();
}

bool ClientPrivate::reset()
{
    if (!m_restartsLeft)
        return false;
    --m_restartsLeft;
    m_state = Client::Uninitialized;
    m_responseHandlers.clear();
    m_clientInterface->resetBuffer();
    updateEditorToolBar(m_openedDocument.keys());
    m_serverCapabilities = ServerCapabilities();
    m_dynamicCapabilities.reset();
    if (m_diagnosticManager)
        m_diagnosticManager->clearDiagnostics();
    for (auto it = m_openedDocument.cbegin(); it != m_openedDocument.cend(); ++it)
        it.key()->disconnect(this);
    m_openedDocument.clear();
    // temporary container needed since m_resetAssistProvider is changed in resetAssistProviders
    for (TextEditor::TextDocument *document : m_resetAssistProvider.keys())
        resetAssistProviders(document);
    for (TextEditor::IAssistProcessor *processor : qAsConst(m_runningAssistProcessors))
        processor->setAsyncProposalAvailable(nullptr);
    m_runningAssistProcessors.clear();
    qDeleteAll(m_documentHighlightsTimer);
    m_documentHighlightsTimer.clear();
    m_progressManager.reset();
    m_documentVersions.clear();
    return true;
}

void Client::setError(const QString &message)
{
    log(message);
    d->m_state = Error;
}

void Client::setProgressTitleForToken(const LanguageServerProtocol::ProgressToken &token,
                                      const QString &message)
{
    d->m_progressManager.setTitleForToken(token, message);
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
    auto box = new QMessageBox();
    box->setText(message.toString());
    box->setAttribute(Qt::WA_DeleteOnClose);
    switch (message.type()) {
    case Error: box->setIcon(QMessageBox::Critical); break;
    case Warning: box->setIcon(QMessageBox::Warning); break;
    case Info: box->setIcon(QMessageBox::Information); break;
    case Log: box->setIcon(QMessageBox::NoIcon); break;
    }
    QHash<QAbstractButton *, MessageActionItem> itemForButton;
    if (const Utils::optional<QList<MessageActionItem>> actions = message.actions()) {
        for (const MessageActionItem &action : *actions)
            itemForButton.insert(box->addButton(action.title(), QMessageBox::InvalidRole), action);
    }
    box->exec();
    const MessageActionItem &item = itemForButton.value(box->clickedButton());
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
        const auto uri = DocumentUri::fromFilePath(filePath);
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
    if (auto handler = m_responseHandlers[id])
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
        return tr("Invalid parameter in \"%1\":\n%2")
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
            = ProjectExplorer::SessionManager::projects();
        if (projects.isEmpty()) {
            response.setResult(nullptr);
        } else {
            response.setResult(Utils::transform(
                projects,
                [](ProjectExplorer::Project *project) {
                    return WorkSpaceFolder(DocumentUri::fromFilePath(project->projectDirectory()),
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
        if (Utils::optional<ProgressParams> params
            = ProgressNotification(message.toJsonObject()).params()) {
            if (!params->isValid())
                q->log(invalidParamsErrorMessage(*params));
            m_progressManager.handleProgress(*params);
            if (ProgressManager::isProgressEndMessage(*params))
                emit q->workDone(params->token());
        }
    } else if (isRequest) {
        Response<JsonObject, JsonObject> response(id);
        ResponseError<JsonObject> error;
        error.setCode(ResponseError<JsonObject>::MethodNotFound);
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
    d->m_diagnosticManager->setDiagnostics(uri, diagnostics, params.version());
    if (LanguageClientManager::clientForUri(uri) == this) {
        d->m_diagnosticManager->showDiagnostics(uri, d->m_documentVersions.value(uri.toFilePath()));
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

void Client::setDocumentChangeUpdateThreshold(int msecs)
{
    d->m_documentUpdateTimer.setInterval(msecs);
}

void ClientPrivate::initializeCallback(const InitializeRequest::Response &initResponse)
{
    QTC_ASSERT(m_state == Client::InitializeRequested, return);
    if (optional<ResponseError<InitializeError>> error = initResponse.error()) {
        if (Utils::optional<InitializeError> data = error->data()) {
            if (data->retry()) {
                const QString title(tr("Language Server \"%1\" Initialize Error").arg(m_displayName));
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
        q->setError(tr("Initialize error: ") + error->message());
        emit q->finished();
        return;
    }
    if (const optional<InitializeResult> &result = initResponse.result()) {
        if (!result->isValid()) { // continue on ill formed result
            q->log(QJsonDocument(*result).toJson(QJsonDocument::Indented) + '\n'
                + tr("Initialize result is not valid"));
        }
        const Utils::optional<ServerInfo> serverInfo = result->serverInfo();
        if (serverInfo) {
            if (!serverInfo->isValid()) {
                q->log(QJsonDocument(*result).toJson(QJsonDocument::Indented) + '\n'
                    + tr("Server Info is not valid"));
            } else {
                m_serverName = serverInfo->name();
                if (const Utils::optional<QString> version = serverInfo->version())
                    m_serverVersion = *version;
            }
        }

        m_serverCapabilities = result->capabilities();
    } else {
        q->log(tr("No initialize result."));
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
    Utils::optional<Utils::variant<bool, WorkDoneProgressOptions>> documentSymbolProvider
        = q->capabilities().documentSymbolProvider();
    if (documentSymbolProvider.has_value()) {
        if (!Utils::holds_alternative<bool>(*documentSymbolProvider)
            || Utils::get<bool>(*documentSymbolProvider)) {
            TextEditor::IOutlineWidgetFactory::updateOutline();
        }
    }

    if (const BaseSettings *settings = LanguageClientManager::settingForClient(q)) {
        const QJsonValue configuration = settings->configuration();
        if (!configuration.isNull())
            q->updateConfiguration(configuration);
    }

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
    if (optional<ShutdownRequest::Response::Error> error = shutdownResponse.error())
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
                return holds_alternative<QString>(notification)
                        || (holds_alternative<bool>(notification) && get<bool>(notification));
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

} // namespace LanguageClient

#include <client.moc>
