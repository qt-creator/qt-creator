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

#include "languageclientinterface.h"
#include "languageclientmanager.h"
#include "languageclientutils.h"
#include "semantichighlightsupport.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>
#include <languageserverprotocol/diagnostics.h>
#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/messages.h>
#include <languageserverprotocol/workspace.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <texteditor/codeassist/documentcontentcompletion.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textmark.h>
#include <texteditor/ioutlinewidget.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>
#include <utils/utilsicons.h>

#include <QDebug>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>

using namespace LanguageServerProtocol;
using namespace Utils;

namespace LanguageClient {

static Q_LOGGING_CATEGORY(LOGLSPCLIENT, "qtc.languageclient.client", QtWarningMsg);

class TextMark : public TextEditor::TextMark
{
public:
    TextMark(const Utils::FilePath &fileName, const Diagnostic &diag, const Core::Id &clientId)
        : TextEditor::TextMark(fileName, diag.range().start().line() + 1, clientId)
        , m_diagnostic(diag)
    {
        using namespace Utils;
        setLineAnnotation(diag.message());
        setToolTip(diag.message());
        const bool isError
            = diag.severity().value_or(DiagnosticSeverity::Hint) == DiagnosticSeverity::Error;
        setColor(isError ? Theme::CodeModel_Error_TextMarkColor
                         : Theme::CodeModel_Warning_TextMarkColor);

        setIcon(isError ? Icons::CODEMODEL_ERROR.icon()
                        : Icons::CODEMODEL_WARNING.icon());
    }

    const Diagnostic &diagnostic() const { return m_diagnostic; }

private:
    const Diagnostic m_diagnostic;
};

Client::Client(BaseClientInterface *clientInterface)
    : m_id(Core::Id::fromString(QUuid::createUuid().toString()))
    , m_clientInterface(clientInterface)
    , m_documentSymbolCache(this)
    , m_hoverHandler(this)
{
    m_clientProviders.completionAssistProvider = new LanguageClientCompletionAssistProvider(this);
    m_clientProviders.functionHintProvider = new FunctionHintAssistProvider(this);
    m_clientProviders.quickFixAssistProvider = new LanguageClientQuickFixProvider(this);

    m_contentHandler.insert(JsonRpcMessageHandler::jsonRpcMimeType(),
                            &JsonRpcMessageHandler::parseContent);
    QTC_ASSERT(clientInterface, return);
    connect(clientInterface, &BaseClientInterface::messageReceived, this, &Client::handleMessage);
    connect(clientInterface, &BaseClientInterface::error, this, &Client::setError);
    connect(clientInterface, &BaseClientInterface::finished, this, &Client::finished);
    connect(TextEditor::TextEditorSettings::instance(),
            &TextEditor::TextEditorSettings::fontSettingsChanged,
            this,
            &Client::rehighlight);
}

static void updateEditorToolBar(QList<TextEditor::TextDocument *> documents)
{
    for (TextEditor::TextDocument *document : documents) {
        for (Core::IEditor *editor : Core::DocumentModel::editorsForDocument(document))
            updateEditorToolBar(editor);
    }
}

Client::~Client()
{
    using namespace TextEditor;
    // FIXME: instead of replacing the completion provider in the text document store the
    // completion provider as a prioritised list in the text document
    for (TextDocument *document : m_resetAssistProvider.keys())
        resetAssistProviders(document);
    for (Core::IEditor * editor : Core::DocumentModel::editorsForOpenedDocuments()) {
        if (auto textEditor = qobject_cast<BaseTextEditor *>(editor)) {
            TextEditorWidget *widget = textEditor->editorWidget();
            widget->setRefactorMarkers(RefactorMarker::filterOutType(widget->refactorMarkers(), id()));
            widget->removeHoverHandler(&m_hoverHandler);
        }
    }
    for (const DocumentUri &uri : m_diagnostics.keys())
        removeDiagnostics(uri);
    for (const DocumentUri &uri : m_highlights.keys()) {
        if (TextDocument *doc = TextDocument::textDocumentForFilePath(uri.toFilePath())) {
            if (TextEditor::SyntaxHighlighter *highlighter = doc->syntaxHighlighter())
                highlighter->clearAllExtraFormats();
        }
    }
    updateEditorToolBar(m_openedDocument.keys());
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
    documentCapabilities.setDocumentSymbol(symbolCapabilities);

    TextDocumentClientCapabilities::SemanticHighlightingCapabilities semanticHighlight;
    semanticHighlight.setSemanticHighlighting(true);
    documentCapabilities.setSemanticHighlightingCapabilities(semanticHighlight);

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
    completionItemCapbilities.setSnippetSupport(false);
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
    hover.setContentFormat({MarkupKind::plaintext});
    hover.setDynamicRegistration(true);
    documentCapabilities.setHover(hover);

    documentCapabilities.setReferences(allowDynamicRegistration);
    documentCapabilities.setDocumentHighlight(allowDynamicRegistration);
    documentCapabilities.setDefinition(allowDynamicRegistration);
    documentCapabilities.setTypeDefinition(allowDynamicRegistration);
    documentCapabilities.setImplementation(allowDynamicRegistration);
    capabilities.setTextDocument(documentCapabilities);

    return capabilities;
}

void Client::initialize()
{
    using namespace ProjectExplorer;
    QTC_ASSERT(m_clientInterface, return);
    QTC_ASSERT(m_state == Uninitialized, return);
    qCDebug(LOGLSPCLIENT) << "initializing language server " << m_displayName;
    auto initRequest = new InitializeRequest();
    auto params = initRequest->params().value_or(InitializeParams());
    params.setCapabilities(generateClientCapabilities());
    if (m_project) {
        params.setRootUri(DocumentUri::fromFilePath(m_project->projectDirectory()));
        params.setWorkSpaceFolders(Utils::transform(SessionManager::projects(), [](Project *pro){
            return WorkSpaceFolder(pro->projectDirectory().toString(), pro->displayName());
        }));
    }
    initRequest->setParams(params);
    initRequest->setResponseCallback([this](const InitializeRequest::Response &initResponse){
        initializeCallback(initResponse);
    });
    // directly send data otherwise the state check would fail;
    initRequest->registerResponseHandler(&m_responseHandlers);
    m_clientInterface->sendMessage(initRequest->toBaseMessage());
    m_state = InitializeRequested;
}

void Client::shutdown()
{
    QTC_ASSERT(m_state == Initialized, emit finished(); return);
    qCDebug(LOGLSPCLIENT) << "shutdown language server " << m_displayName;
    ShutdownRequest shutdown;
    shutdown.setResponseCallback([this](const ShutdownRequest::Response &shutdownResponse){
        shutDownCallback(shutdownResponse);
    });
    sendContent(shutdown);
    m_state = ShutdownRequested;
}

Client::State Client::state() const
{
    return m_state;
}

void Client::openDocument(TextEditor::TextDocument *document)
{
    using namespace TextEditor;
    if (!isSupportedDocument(document))
        return;

    m_openedDocument[document] = document->plainText();
    if (m_state != Initialized)
        return;

    const FilePath &filePath = document->filePath();
    const QString method(DidOpenTextDocumentNotification::methodName);
    if (Utils::optional<bool> registered = m_dynamicCapabilities.isRegistered(method)) {
        if (!registered.value())
            return;
        const TextDocumentRegistrationOptions option(
            m_dynamicCapabilities.option(method).toObject());
        if (option.isValid(nullptr)
            && !option.filterApplies(filePath, Utils::mimeTypeForName(document->mimeType()))) {
            return;
        }
    } else if (Utils::optional<ServerCapabilities::TextDocumentSync> _sync
               = m_serverCapabilities.textDocumentSync()) {
        if (auto options = Utils::get_if<TextDocumentSyncOptions>(&_sync.value())) {
            if (!options->openClose().value_or(true))
                return;
        }
    }
    connect(document, &TextDocument::contentsChangedWithPosition, this,
            [this, document](int position, int charsRemoved, int charsAdded) {
        documentContentsChanged(document, position, charsRemoved, charsAdded);
    });
    TextDocumentItem item;
    item.setLanguageId(TextDocumentItem::mimeTypeToLanguageId(document->mimeType()));
    item.setUri(DocumentUri::fromFilePath(filePath));
    item.setText(document->plainText());
    item.setVersion(document->document()->revision());
    sendContent(DidOpenTextDocumentNotification(DidOpenTextDocumentParams(item)));

    if (LanguageClientManager::clientForDocument(document) == this)
        activateDocument(document);
}

void Client::sendContent(const IContent &content)
{
    QTC_ASSERT(m_clientInterface, return);
    QTC_ASSERT(m_state == Initialized, return);
    content.registerResponseHandler(&m_responseHandlers);
    QString error;
    if (!QTC_GUARD(content.isValid(&error)))
        Core::MessageManager::write(error);
    m_clientInterface->sendMessage(content.toBaseMessage());
}

void Client::sendContent(const DocumentUri &uri, const IContent &content)
{
    if (!Utils::anyOf(m_openedDocument.keys(), [uri](TextEditor::TextDocument *documnent) {
            return uri.toFilePath() == documnent->filePath();
        })) {
        sendContent(content);
    }
}

void Client::cancelRequest(const MessageId &id)
{
    m_responseHandlers.remove(id);
    sendContent(CancelRequest(CancelParameter(id)));
}

void Client::closeDocument(TextEditor::TextDocument *document)
{
    if (m_openedDocument.remove(document) == 0)
        return;
    const DocumentUri &uri = DocumentUri::fromFilePath(document->filePath());
    const DidCloseTextDocumentParams params(TextDocumentIdentifier{uri});
    m_highlights[uri].clear();
    sendContent(uri, DidCloseTextDocumentNotification(params));
    deactivateDocument(document);
}

void Client::activateDocument(TextEditor::TextDocument *document)
{
    auto uri = DocumentUri::fromFilePath(document->filePath());
    showDiagnostics(uri);
    SemanticHighligtingSupport::applyHighlight(document, m_highlights.value(uri), capabilities());
    // only replace the assist provider if the completion provider is the default one or null
    if (!document->completionAssistProvider()
        || qobject_cast<TextEditor::DocumentContentCompletionProvider *>(
            document->completionAssistProvider())) {
        m_resetAssistProvider[document] = {document->completionAssistProvider(),
                                           document->functionHintAssistProvider(),
                                           document->quickFixAssistProvider()};
        document->setCompletionAssistProvider(m_clientProviders.completionAssistProvider);
        document->setFunctionHintAssistProvider(m_clientProviders.functionHintProvider);
        document->setQuickFixAssistProvider(m_clientProviders.quickFixAssistProvider);
    }
    for (Core::IEditor *editor : Core::DocumentModel::editorsForDocument(document)) {
        updateEditorToolBar(editor);
        if (auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor))
            textEditor->editorWidget()->addHoverHandler(&m_hoverHandler);
    }
}

void Client::deactivateDocument(TextEditor::TextDocument *document)
{
    hideDiagnostics(document);
    resetAssistProviders(document);
    if (TextEditor::SyntaxHighlighter *highlighter = document->syntaxHighlighter())
        highlighter->clearAllExtraFormats();
    for (Core::IEditor *editor : Core::DocumentModel::editorsForDocument(document)) {
        if (auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor))
            textEditor->editorWidget()->removeHoverHandler(&m_hoverHandler);
    }
}

bool Client::documentOpen(TextEditor::TextDocument *document) const
{
    return m_openedDocument.contains(document);
}

void Client::documentContentsSaved(TextEditor::TextDocument *document)
{
    if (!m_openedDocument.contains(document))
        return;
    bool sendMessage = true;
    bool includeText = false;
    const QString method(DidSaveTextDocumentNotification::methodName);
    if (Utils::optional<bool> registered = m_dynamicCapabilities.isRegistered(method)) {
        sendMessage = registered.value();
        if (sendMessage) {
            const TextDocumentSaveRegistrationOptions option(
                        m_dynamicCapabilities.option(method).toObject());
            if (option.isValid(nullptr)) {
                sendMessage = option.filterApplies(document->filePath(),
                                                   Utils::mimeTypeForName(document->mimeType()));
                includeText = option.includeText().value_or(includeText);
            }
        }
    } else if (Utils::optional<ServerCapabilities::TextDocumentSync> _sync
               = m_serverCapabilities.textDocumentSync()) {
        if (auto options = Utils::get_if<TextDocumentSyncOptions>(&_sync.value())) {
            if (Utils::optional<SaveOptions> saveOptions = options->save())
                includeText = saveOptions.value().includeText().value_or(includeText);
        }
    }
    if (!sendMessage)
        return;
    DidSaveTextDocumentParams params(
                TextDocumentIdentifier(DocumentUri::fromFilePath(document->filePath())));
    if (includeText)
        params.setText(document->plainText());
    sendContent(DidSaveTextDocumentNotification(params));
}

void Client::documentWillSave(Core::IDocument *document)
{
    const FilePath &filePath = document->filePath();
    auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);
    if (!m_openedDocument.contains(textDocument))
        return;
    bool sendMessage = true;
    const QString method(WillSaveTextDocumentNotification::methodName);
    if (Utils::optional<bool> registered = m_dynamicCapabilities.isRegistered(method)) {
        sendMessage = registered.value();
        if (sendMessage) {
            const TextDocumentRegistrationOptions option(m_dynamicCapabilities.option(method));
            if (option.isValid(nullptr)) {
                sendMessage = option.filterApplies(filePath,
                                                   Utils::mimeTypeForName(document->mimeType()));
            }
        }
    } else if (Utils::optional<ServerCapabilities::TextDocumentSync> _sync
               = m_serverCapabilities.textDocumentSync()) {
        if (auto options = Utils::get_if<TextDocumentSyncOptions>(&_sync.value()))
            sendMessage = options->willSave().value_or(sendMessage);
    }
    if (!sendMessage)
        return;
    const WillSaveTextDocumentParams params(
        TextDocumentIdentifier(DocumentUri::fromFilePath(filePath)));
    sendContent(WillSaveTextDocumentNotification(params));
}

void Client::documentContentsChanged(TextEditor::TextDocument *document,
                                     int position,
                                     int charsRemoved,
                                     int charsAdded)
{
    if (!m_openedDocument.contains(document) || !reachable())
        return;
    const QString method(DidChangeTextDocumentNotification::methodName);
    TextDocumentSyncKind syncKind = m_serverCapabilities.textDocumentSyncKindHelper();
    if (Utils::optional<bool> registered = m_dynamicCapabilities.isRegistered(method)) {
        syncKind = registered.value() ? TextDocumentSyncKind::None : TextDocumentSyncKind::Full;
        if (syncKind != TextDocumentSyncKind::None) {
            const TextDocumentChangeRegistrationOptions option(
                                    m_dynamicCapabilities.option(method).toObject());
            syncKind = option.isValid(nullptr) ? option.syncKind() : syncKind;
        }
    }
    auto textDocument = qobject_cast<TextEditor::TextDocument *>(document);

    const auto uri = DocumentUri::fromFilePath(document->filePath());
    m_highlights[uri].clear();
    if (syncKind != TextDocumentSyncKind::None) {
        VersionedTextDocumentIdentifier docId(uri);
        docId.setVersion(textDocument ? textDocument->document()->revision() : 0);
        DidChangeTextDocumentParams params;
        params.setTextDocument(docId);
        if (syncKind == TextDocumentSyncKind::Incremental) {
            DidChangeTextDocumentParams::TextDocumentContentChangeEvent change;
            QTextDocument oldDoc(m_openedDocument[document]);
            QTextCursor cursor(&oldDoc);
            cursor.setPosition(position + charsRemoved);
            cursor.setPosition(position, QTextCursor::KeepAnchor);
            change.setRange(Range(cursor));
            change.setRangeLength(cursor.selectionEnd() - cursor.selectionStart());
            change.setText(document->textAt(position, charsAdded));
            params.setContentChanges({change});
        } else {
            params.setContentChanges({document->plainText()});
        }
        m_openedDocument[document] = document->plainText();
        sendContent(DidChangeTextDocumentNotification(params));
    }

    if (textDocument) {
        using namespace TextEditor;
        for (BaseTextEditor *editor : BaseTextEditor::textEditorsForDocument(textDocument))
            if (TextEditorWidget *widget = editor->editorWidget())
                widget->setRefactorMarkers(RefactorMarker::filterOutType(widget->refactorMarkers(), id()));
    }
}

void Client::registerCapabilities(const QList<Registration> &registrations)
{
    m_dynamicCapabilities.registerCapability(registrations);
}

void Client::unregisterCapabilities(const QList<Unregistration> &unregistrations)
{
    m_dynamicCapabilities.unregisterCapability(unregistrations);
}

template <typename Request>
static bool sendTextDocumentPositionParamsRequest(Client *client,
                                                  const Request &request,
                                                  const DynamicCapabilities &dynamicCapabilities,
                                                  const optional<bool> &serverCapability)
{
    if (!request.isValid(nullptr))
        return false;
    const DocumentUri uri = request.params().value().textDocument().uri();
    const bool supportedFile = client->isSupportedUri(uri);
    bool sendMessage = dynamicCapabilities.isRegistered(Request::methodName).value_or(false);
    if (sendMessage) {
        const TextDocumentRegistrationOptions option(dynamicCapabilities.option(Request::methodName));
        if (option.isValid(nullptr))
            sendMessage = option.filterApplies(FilePath::fromString(QUrl(uri).adjusted(QUrl::PreferLocalFile).toString()));
        else
            sendMessage = supportedFile;
    } else {
        sendMessage = serverCapability.value_or(sendMessage) && supportedFile;
    }
    if (sendMessage)
        client->sendContent(request);
    return sendMessage;
}

bool Client::findLinkAt(GotoDefinitionRequest &request)
{
    return LanguageClient::sendTextDocumentPositionParamsRequest(
                this, request, m_dynamicCapabilities, m_serverCapabilities.definitionProvider());
}

bool Client::findUsages(FindReferencesRequest &request)
{
    return LanguageClient::sendTextDocumentPositionParamsRequest(
                this, request, m_dynamicCapabilities, m_serverCapabilities.referencesProvider());
}

TextEditor::HighlightingResult createHighlightingResult(const SymbolInformation &info)
{
    if (!info.isValid(nullptr))
        return {};
    const Position &start = info.location().range().start();
    return TextEditor::HighlightingResult(start.line() + 1,
                                          start.character() + 1,
                                          info.name().length(),
                                          info.kind());
}

void Client::cursorPositionChanged(TextEditor::TextEditorWidget *widget)
{
    const auto uri = DocumentUri::fromFilePath(widget->textDocument()->filePath());
    if (m_dynamicCapabilities.isRegistered(DocumentHighlightsRequest::methodName).value_or(false)) {
        TextDocumentRegistrationOptions option(
                    m_dynamicCapabilities.option(DocumentHighlightsRequest::methodName));
        if (!option.filterApplies(widget->textDocument()->filePath()))
            return;
    } else if (!m_serverCapabilities.documentHighlightProvider().value_or(false)) {
        return;
    }

    auto runningRequest = m_highlightRequests.find(uri);
    if (runningRequest != m_highlightRequests.end())
        cancelRequest(runningRequest.value());

    DocumentHighlightsRequest request(TextDocumentPositionParams(uri, widget->textCursor()));
    request.setResponseCallback(
                [widget = QPointer<TextEditor::TextEditorWidget>(widget), this, uri]
                (DocumentHighlightsRequest::Response response)
    {
        m_highlightRequests.remove(uri);
        if (!widget)
            return;

        QList<QTextEdit::ExtraSelection> selections;
        const DocumentHighlightsResult result = response.result().value_or(DocumentHighlightsResult());
        if (!holds_alternative<QList<DocumentHighlight>>(result)) {
            widget->setExtraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection, selections);
            return;
        }

        const QTextCharFormat &format =
                widget->textDocument()->fontSettings().toTextCharFormat(TextEditor::C_OCCURRENCES);
        QTextDocument *document = widget->document();
        for (const auto &highlight : get<QList<DocumentHighlight>>(result)) {
            QTextEdit::ExtraSelection selection{widget->textCursor(), format};
            const int &start = highlight.range().start().toPositionInDocument(document);
            const int &end = highlight.range().end().toPositionInDocument(document);
            if (start < 0 || end < 0)
                continue;
            selection.cursor.setPosition(start);
            selection.cursor.setPosition(end, QTextCursor::KeepAnchor);
            selections << selection;
        }
        widget->setExtraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection, selections);
    });
    m_highlightRequests[uri] = request.id();
    sendContent(request);
}

void Client::requestCodeActions(const DocumentUri &uri, const QList<Diagnostic> &diagnostics)
{
    const Utils::FilePath fileName = uri.toFilePath();
    TextEditor::TextDocument *doc = TextEditor::TextDocument::textDocumentForFilePath(fileName);
    if (!doc)
        return;

    CodeActionParams codeActionParams;
    CodeActionParams::CodeActionContext context;
    context.setDiagnostics(diagnostics);
    codeActionParams.setContext(context);
    codeActionParams.setTextDocument(uri);
    Position start(0, 0);
    const QTextBlock &lastBlock = doc->document()->lastBlock();
    Position end(lastBlock.blockNumber(), lastBlock.length() - 1);
    codeActionParams.setRange(Range(start, end));
    CodeActionRequest request(codeActionParams);
    request.setResponseCallback(
        [uri, self = QPointer<Client>(this)](const CodeActionRequest::Response &response) {
        if (self)
            self->handleCodeActionResponse(response, uri);
    });
    requestCodeActions(request);
}

void Client::requestCodeActions(const CodeActionRequest &request)
{
    if (!request.isValid(nullptr))
        return;

    const Utils::FilePath fileName
        = request.params().value_or(CodeActionParams()).textDocument().uri().toFilePath();

    const QString method(CodeActionRequest::methodName);
    if (Utils::optional<bool> registered = m_dynamicCapabilities.isRegistered(method)) {
        if (!registered.value())
            return;
        const TextDocumentRegistrationOptions option(
            m_dynamicCapabilities.option(method).toObject());
        if (option.isValid(nullptr) && !option.filterApplies(fileName))
            return;
    } else {
        Utils::variant<bool, CodeActionOptions> provider
            = m_serverCapabilities.codeActionProvider().value_or(false);
        if (!(Utils::holds_alternative<CodeActionOptions>(provider) || Utils::get<bool>(provider)))
            return;
    }

    sendContent(request);
}

void Client::handleCodeActionResponse(const CodeActionRequest::Response &response,
                                          const DocumentUri &uri)
{
    if (const Utils::optional<CodeActionRequest::Response::Error> &error = response.error())
        log(*error);
    if (const Utils::optional<CodeActionResult> &_result = response.result()) {
        const CodeActionResult &result = _result.value();
        if (auto list = Utils::get_if<QList<Utils::variant<Command, CodeAction>>>(&result)) {
            for (const Utils::variant<Command, CodeAction> &item : *list) {
                if (auto action = Utils::get_if<CodeAction>(&item))
                    updateCodeActionRefactoringMarker(this, *action, uri);
                else if (auto command = Utils::get_if<Command>(&item)) {
                    Q_UNUSED(command) // todo
                }
            }
        }
    }
}

void Client::executeCommand(const Command &command)
{
    using CommandOptions = LanguageServerProtocol::ServerCapabilities::ExecuteCommandOptions;
    const QString method(ExecuteCommandRequest::methodName);
    if (Utils::optional<bool> registered = m_dynamicCapabilities.isRegistered(method)) {
        if (!registered.value())
            return;
        const CommandOptions option(m_dynamicCapabilities.option(method).toObject());
        if (option.isValid(nullptr) && !option.commands().isEmpty() && !option.commands().contains(command.command()))
            return;
    } else if (Utils::optional<CommandOptions> option = m_serverCapabilities.executeCommandProvider()) {
        if (option->isValid(nullptr) && !option->commands().isEmpty() && !option->commands().contains(command.command()))
            return;
    } else {
        return;
    }

    const ExecuteCommandRequest request((ExecuteCommandParams(command)));
    sendContent(request);
}

const ProjectExplorer::Project *Client::project() const
{
    return m_project;
}

void Client::setCurrentProject(ProjectExplorer::Project *project)
{
    m_project = project;
}

void Client::projectOpened(ProjectExplorer::Project *project)
{
    if (!sendWorkspceFolderChanges())
        return;
    WorkspaceFoldersChangeEvent event;
    event.setAdded({WorkSpaceFolder(project->projectDirectory().toString(), project->displayName())});
    DidChangeWorkspaceFoldersParams params;
    params.setEvent(event);
    DidChangeWorkspaceFoldersNotification change(params);
    sendContent(change);
}

void Client::projectClosed(ProjectExplorer::Project *project)
{
    if (project == m_project) {
        if (m_state == Initialized) {
            shutdown();
        } else {
            m_state = Shutdown; // otherwise the manager would try to restart this server
            emit finished();
        }
    }
    if (!sendWorkspceFolderChanges())
        return;
    WorkspaceFoldersChangeEvent event;
    event.setRemoved(
        {WorkSpaceFolder(project->projectDirectory().toString(), project->displayName())});
    DidChangeWorkspaceFoldersParams params;
    params.setEvent(event);
    DidChangeWorkspaceFoldersNotification change(params);
    sendContent(change);
}

void Client::setSupportedLanguage(const LanguageFilter &filter)
{
    m_languagFilter = filter;
}

bool Client::isSupportedDocument(const TextEditor::TextDocument *document) const
{
    QTC_ASSERT(document, return false);
    return m_languagFilter.isSupported(document);
}

bool Client::isSupportedFile(const Utils::FilePath &filePath, const QString &mimeType) const
{
    return m_languagFilter.isSupported(filePath, mimeType);
}

bool Client::isSupportedUri(const DocumentUri &uri) const
{
    return m_languagFilter.isSupported(uri.toFilePath(),
                                       Utils::mimeTypeForFile(uri.toFilePath().fileName()).name());
}

bool Client::needsRestart(const BaseSettings *settings) const
{
    QTC_ASSERT(settings, return false);
    return m_languagFilter.mimeTypes != settings->m_languageFilter.mimeTypes
            || m_languagFilter.filePattern != settings->m_languageFilter.filePattern;
}

QList<Diagnostic> Client::diagnosticsAt(const DocumentUri &uri, const Range &range) const
{
    QList<Diagnostic> diagnostics;
    for (const Diagnostic &diagnostic : m_diagnostics[uri]) {
        if (diagnostic.range().overlaps(range))
            diagnostics << diagnostic;
    }
    return diagnostics;
}

bool Client::start()
{
    return m_clientInterface->start();
}

bool Client::reset()
{
    if (!m_restartsLeft)
        return false;
    --m_restartsLeft;
    m_state = Uninitialized;
    m_responseHandlers.clear();
    m_clientInterface->resetBuffer();
    updateEditorToolBar(m_openedDocument.keys());
    m_serverCapabilities = ServerCapabilities();
    m_dynamicCapabilities.reset();
    for (const DocumentUri &uri : m_diagnostics.keys())
        removeDiagnostics(uri);
    for (TextEditor::TextDocument *document : m_openedDocument.keys())
        document->disconnect(this);
    for (TextEditor::TextDocument *document : m_resetAssistProvider.keys())
        resetAssistProviders(document);
    return true;
}

void Client::setError(const QString &message)
{
    log(message);
    m_state = Error;
}

void Client::handleMessage(const BaseMessage &message)
{
    if (auto handler = m_contentHandler[message.mimeType]) {
        QString parseError;
        handler(message.content, message.codec, parseError,
                [this](MessageId id, const QByteArray &content, QTextCodec *codec){
                    this->handleResponse(id, content, codec);
                },
                [this](const QString &method, MessageId id, const IContent *content){
                    this->handleMethod(method, id, content);
                });
        if (!parseError.isEmpty())
            log(parseError);
    } else {
        log(tr("Cannot handle content of type: %1").arg(QLatin1String(message.mimeType)));
    }
}

void Client::log(const QString &message, Core::MessageManager::PrintToOutputPaneFlag flag)
{
    Core::MessageManager::write(QString("LanguageClient %1: %2").arg(name(), message), flag);
}

void Client::showDiagnostics(Core::IDocument *doc)
{
    showDiagnostics(DocumentUri::fromFilePath(doc->filePath()));
}

void Client::hideDiagnostics(TextEditor::TextDocument *doc)
{
    if (!doc)
        return;
    qDeleteAll(Utils::filtered(doc->marks(), Utils::equal(&TextEditor::TextMark::category, id())));
}

const ServerCapabilities &Client::capabilities() const
{
    return m_serverCapabilities;
}

const DynamicCapabilities &Client::dynamicCapabilities() const
{
    return m_dynamicCapabilities;
}

const BaseClientInterface *Client::clientInterface() const
{
    return m_clientInterface.data();
}

DocumentSymbolCache *Client::documentSymbolCache()
{
    return &m_documentSymbolCache;
}

HoverHandler *Client::hoverHandler()
{
    return &m_hoverHandler;
}

void Client::log(const ShowMessageParams &message,
                     Core::MessageManager::PrintToOutputPaneFlag flag)
{
    log(message.toString(), flag);
}

void Client::showMessageBox(const ShowMessageRequestParams &message, const MessageId &id)
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
        for (const MessageActionItem &action : actions.value())
            itemForButton.insert(box->addButton(action.title(), QMessageBox::InvalidRole), action);
    }
    box->setModal(true);
    connect(box, &QMessageBox::finished, this, [=]{
        ShowMessageRequest::Response response(id);
        const MessageActionItem &item = itemForButton.value(box->clickedButton());
        response.setResult(item.isValid(nullptr) ? LanguageClientValue<MessageActionItem>(item)
                                                 : LanguageClientValue<MessageActionItem>());
        sendContent(response);
    });
    box->show();
}

void Client::showDiagnostics(const DocumentUri &uri)
{
    const FilePath &filePath = uri.toFilePath();
    if (TextEditor::TextDocument *doc = TextEditor::TextDocument::textDocumentForFilePath(
            uri.toFilePath())) {
        for (const Diagnostic &diagnostic : m_diagnostics.value(uri))
            doc->addMark(new TextMark(filePath, diagnostic, id()));
    }
}

void Client::removeDiagnostics(const DocumentUri &uri)
{
    hideDiagnostics(TextEditor::TextDocument::textDocumentForFilePath(uri.toFilePath()));
    m_diagnostics.remove(uri);
}

void Client::resetAssistProviders(TextEditor::TextDocument *document)
{
    const AssistProviders providers = m_resetAssistProvider.take(document);
    if (document->completionAssistProvider() == m_clientProviders.completionAssistProvider)
        document->setCompletionAssistProvider(providers.completionAssistProvider);
    if (document->functionHintAssistProvider() == m_clientProviders.functionHintProvider)
        document->setFunctionHintAssistProvider(providers.functionHintProvider);
    if (document->quickFixAssistProvider() == m_clientProviders.quickFixAssistProvider)
        document->setQuickFixAssistProvider(providers.quickFixAssistProvider);
}

void Client::handleResponse(const MessageId &id, const QByteArray &content, QTextCodec *codec)
{
    if (auto handler = m_responseHandlers[id])
        handler(content, codec);
}

void Client::handleMethod(const QString &method, MessageId id, const IContent *content)
{
    QStringList error;
    bool paramsValid = true;
    if (method == PublishDiagnosticsNotification::methodName) {
        auto params = dynamic_cast<const PublishDiagnosticsNotification *>(content)->params().value_or(PublishDiagnosticsParams());
        paramsValid = params.isValid(&error);
        if (paramsValid)
            handleDiagnostics(params);
    } else if (method == LogMessageNotification::methodName) {
        auto params = dynamic_cast<const LogMessageNotification *>(content)->params().value_or(LogMessageParams());
        paramsValid = params.isValid(&error);
        if (paramsValid)
            log(params, Core::MessageManager::Flash);
    } else if (method == SemanticHighlightNotification::methodName) {
        auto params = dynamic_cast<const SemanticHighlightNotification *>(content)->params().value_or(SemanticHighlightingParams());
        paramsValid = params.isValid(&error);
        if (paramsValid)
            handleSemanticHighlight(params);
    } else if (method == ShowMessageNotification::methodName) {
        auto params = dynamic_cast<const ShowMessageNotification *>(content)->params().value_or(ShowMessageParams());
        paramsValid = params.isValid(&error);
        if (paramsValid)
            log(params);
    } else if (method == ShowMessageRequest::methodName) {
        auto request = dynamic_cast<const ShowMessageRequest *>(content);
        auto params = request->params().value_or(ShowMessageRequestParams());
        paramsValid = params.isValid(&error);
        if (paramsValid) {
            showMessageBox(params, request->id());
        } else {
            ShowMessageRequest::Response response(request->id());
            ResponseError<std::nullptr_t> error;
            const QString errorMessage =
                    QString("Could not parse ShowMessageRequest parameter of '%1': \"%2\"")
                    .arg(request->id().toString(),
                         QString::fromUtf8(QJsonDocument(params).toJson()));
            error.setMessage(errorMessage);
            response.setError(error);
            sendContent(response);
        }
    } else if (method == RegisterCapabilityRequest::methodName) {
        auto params = dynamic_cast<const RegisterCapabilityRequest *>(content)->params().value_or(RegistrationParams());
        paramsValid = params.isValid(&error);
        if (paramsValid)
            m_dynamicCapabilities.registerCapability(params.registrations());
    } else if (method == UnregisterCapabilityRequest::methodName) {
        auto params = dynamic_cast<const UnregisterCapabilityRequest *>(content)->params().value_or(UnregistrationParams());
        paramsValid = params.isValid(&error);
        if (paramsValid)
            m_dynamicCapabilities.unregisterCapability(params.unregistrations());
    } else if (method == ApplyWorkspaceEditRequest::methodName) {
        auto params = dynamic_cast<const ApplyWorkspaceEditRequest *>(content)->params().value_or(ApplyWorkspaceEditParams());
        paramsValid = params.isValid(&error);
        if (paramsValid)
            applyWorkspaceEdit(params.edit());
    } else if (method == WorkSpaceFolderRequest::methodName) {
        WorkSpaceFolderRequest::Response response(dynamic_cast<const WorkSpaceFolderRequest *>(content)->id());
        const QList<ProjectExplorer::Project *> projects
            = ProjectExplorer::SessionManager::projects();
        WorkSpaceFolderResult result;
        if (projects.isEmpty()) {
            result = nullptr;
        } else {
            result = Utils::transform(projects, [](ProjectExplorer::Project *project) {
                return WorkSpaceFolder(project->projectDirectory().toString(),
                                       project->displayName());
            });
        }
        response.setResult(result);
        sendContent(response);
    } else if (id.isValid(&error)) {
        Response<JsonObject, JsonObject> response(id);
        ResponseError<JsonObject> error;
        error.setCode(ResponseError<JsonObject>::MethodNotFound);
        response.setError(error);
        sendContent(response);
    }
    std::reverse(error.begin(), error.end());
    if (!paramsValid) {
        log(tr("Invalid parameter in \"%1\": %2").arg(method, error.join("->")),
            Core::MessageManager::Flash);
    }
    delete content;
}

void Client::handleDiagnostics(const PublishDiagnosticsParams &params)
{
    const DocumentUri &uri = params.uri();

    removeDiagnostics(uri);
    const QList<Diagnostic> &diagnostics = params.diagnostics();
    m_diagnostics[uri] = diagnostics;
    if (LanguageClientManager::clientForUri(uri) == this) {
        showDiagnostics(uri);
        requestCodeActions(uri, diagnostics);
    }
}

void Client::handleSemanticHighlight(const SemanticHighlightingParams &params)
{
    const DocumentUri &uri = params.textDocument().uri();
    m_highlights[uri].clear();
    const LanguageClientValue<int> &version = params.textDocument().version();
    TextEditor::TextDocument *doc = TextEditor::TextDocument::textDocumentForFilePath(
        uri.toFilePath());

    if (!doc || LanguageClientManager::clientForDocument(doc) != this
        || (!version.isNull() && doc->document()->revision() != version.value())) {
        return;
    }

    const TextEditor::HighlightingResults results = SemanticHighligtingSupport::generateResults(
        params.lines());

    m_highlights[uri] = results;

    SemanticHighligtingSupport::applyHighlight(doc, results, capabilities());
}

void Client::rehighlight()
{
    using namespace TextEditor;
    for (auto it = m_highlights.begin(), end = m_highlights.end(); it != end; ++it) {
        if (TextDocument *doc = TextDocument::textDocumentForFilePath(it.key().toFilePath())) {
            if (LanguageClientManager::clientForDocument(doc) == this)
                SemanticHighligtingSupport::applyHighlight(doc, it.value(), capabilities());
        }
    }
}

void Client::initializeCallback(const InitializeRequest::Response &initResponse)
{
    QTC_ASSERT(m_state == InitializeRequested, return);
    if (optional<ResponseError<InitializeError>> error = initResponse.error()) {
        if (error.value().data().has_value()
                && error.value().data().value().retry().value_or(false)) {
            const QString title(tr("Language Server \"%1\" Initialize Error").arg(m_displayName));
            auto result = QMessageBox::warning(Core::ICore::dialogParent(),
                                               title,
                                               error.value().message(),
                                               QMessageBox::Retry | QMessageBox::Cancel,
                                               QMessageBox::Retry);
            if (result == QMessageBox::Retry) {
                m_state = Uninitialized;
                initialize();
                return;
            }
        }
        setError(tr("Initialize error: ") + error.value().message());
        emit finished();
        return;
    }
    const optional<InitializeResult> &_result = initResponse.result();
    if (!_result.has_value()) {// continue on ill formed result
        log(tr("No initialize result."));
    } else {
        const InitializeResult &result = _result.value();
        QStringList error;
        if (!result.isValid(&error)) { // continue on ill formed result
            std::reverse(error.begin(), error.end());
            log(tr("Initialize result is not valid: ") + error.join("->"));
        }

        m_serverCapabilities = result.capabilities().value_or(ServerCapabilities());
    }

    if (auto completionProvider = qobject_cast<LanguageClientCompletionAssistProvider *>(
            m_clientProviders.completionAssistProvider)) {
        completionProvider->setTriggerCharacters(
            m_serverCapabilities.completionProvider()
                .value_or(ServerCapabilities::CompletionOptions())
                .triggerCharacters()
                .value_or(QList<QString>()));
    }
    if (auto functionHintAssistProvider = qobject_cast<FunctionHintAssistProvider *>(
            m_clientProviders.completionAssistProvider)) {
        functionHintAssistProvider->setTriggerCharacters(
            m_serverCapabilities.signatureHelpProvider()
                .value_or(ServerCapabilities::SignatureHelpOptions())
                .triggerCharacters()
                .value_or(QList<QString>()));
    }

    qCDebug(LOGLSPCLIENT) << "language server " << m_displayName << " initialized";
    m_state = Initialized;
    sendContent(InitializeNotification());
    if (m_dynamicCapabilities.isRegistered(DocumentSymbolsRequest::methodName)
            .value_or(capabilities().documentSymbolProvider().value_or(false))) {
        TextEditor::IOutlineWidgetFactory::updateOutline();
    }

    for (TextEditor::TextDocument *document : m_openedDocument.keys())
        openDocument(document);

    emit initialized(m_serverCapabilities);
}

void Client::shutDownCallback(const ShutdownRequest::Response &shutdownResponse)
{
    QTC_ASSERT(m_state == ShutdownRequested, return);
    QTC_ASSERT(m_clientInterface, return);
    optional<ShutdownRequest::Response::Error> errorValue = shutdownResponse.error();
    if (errorValue.has_value()) {
        ShutdownRequest::Response::Error error = errorValue.value();
        qDebug() << error;
        return;
    }
    // directly send data otherwise the state check would fail;
    m_clientInterface->sendMessage(ExitNotification().toBaseMessage());
    qCDebug(LOGLSPCLIENT) << "language server " << m_displayName << " shutdown";
    m_state = Shutdown;
}

bool Client::sendWorkspceFolderChanges() const
{
    if (m_dynamicCapabilities.isRegistered(
                DidChangeWorkspaceFoldersNotification::methodName).value_or(false)) {
        return true;
    }
    if (auto workspace = m_serverCapabilities.workspace()) {
        if (auto folder = workspace.value().workspaceFolders()) {
            if (folder.value().supported().value_or(false)) {
                // holds either the Id for deregistration or whether it is registered
                auto notification = folder.value().changeNotifications().value_or(false);
                return holds_alternative<QString>(notification)
                        || (holds_alternative<bool>(notification) && get<bool>(notification));
            }
        }
    }
    return false;
}

} // namespace LanguageClient
