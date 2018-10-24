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

#include "baseclient.h"
#include "languageclientmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>
#include <languageserverprotocol/diagnostics.h>
#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/messages.h>
#include <languageserverprotocol/workspace.h>
#include <texteditor/semantichighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

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
static Q_LOGGING_CATEGORY(LOGLSPCLIENTV, "qtc.languageclient.messages", QtWarningMsg);

BaseClient::BaseClient()
    : m_id(Core::Id::fromString(QUuid::createUuid().toString()))
    , m_completionProvider(this)
{
    m_buffer.open(QIODevice::ReadWrite | QIODevice::Append);
    m_contentHandler.insert(JsonRpcMessageHandler::jsonRpcMimeType(),
                            &JsonRpcMessageHandler::parseContent);
}

BaseClient::~BaseClient()
{
    m_buffer.close();
    // FIXME: instead of replacing the completion provider in the text document store the
    // completion provider as a prioritised list in the text document
    for (TextEditor::TextDocument *document : m_resetCompletionProvider)
        document->setCompletionAssistProvider(nullptr);
}

void BaseClient::initialize()
{
    using namespace ProjectExplorer;
    QTC_ASSERT(m_state == Uninitialized, return);
    qCDebug(LOGLSPCLIENT) << "initializing language server " << m_displayName;
    auto initRequest = new InitializeRequest();
    if (auto startupProject = SessionManager::startupProject()) {
        auto params = initRequest->params().value_or(InitializeParams());
        params.setRootUri(DocumentUri::fromFileName(startupProject->projectDirectory()));
        initRequest->setParams(params);
        params.setWorkSpaceFolders(Utils::transform(SessionManager::projects(), [](Project *pro){
            return WorkSpaceFolder(pro->projectDirectory().toString(), pro->displayName());
        }));
    }
    initRequest->setResponseCallback([this](const InitializeResponse &initResponse){
        intializeCallback(initResponse);
    });
    // directly send data otherwise the state check would fail;
    initRequest->registerResponseHandler(&m_responseHandlers);
    sendData(initRequest->toBaseMessage().toData());
    m_state = InitializeRequested;
}

void BaseClient::shutdown()
{
    QTC_ASSERT(m_state == Initialized, emit finished(); return);
    qCDebug(LOGLSPCLIENT) << "shutdown language server " << m_displayName;
    ShutdownRequest shutdown;
    shutdown.setResponseCallback([this](const ShutdownResponse &shutdownResponse){
        shutDownCallback(shutdownResponse);
    });
    sendContent(shutdown);
    m_state = ShutdownRequested;
}

BaseClient::State BaseClient::state() const
{
    return m_state;
}

void BaseClient::openDocument(Core::IDocument *document)
{
    using namespace TextEditor;
    if (!isSupportedDocument(document))
        return;
    const FileName &filePath = document->filePath();
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
    auto textDocument = qobject_cast<TextDocument *>(document);
    TextDocumentItem item;
    item.setLanguageId(TextDocumentItem::mimeTypeToLanguageId(document->mimeType()));
    item.setUri(DocumentUri::fromFileName(filePath));
    item.setText(QString::fromUtf8(document->contents()));
    item.setVersion(textDocument ? textDocument->document()->revision() : 0);

    connect(document, &Core::IDocument::contentsChanged, this,
            [this, document](){
        documentContentsChanged(document);
    });
    if (textDocument) {
        m_resetCompletionProvider << textDocument;
        textDocument->setCompletionAssistProvider(&m_completionProvider);
        connect(textDocument, &QObject::destroyed, this, [this, textDocument]{
            m_resetCompletionProvider.remove(textDocument);
        });
        if (BaseTextEditor *editor = BaseTextEditor::textEditorForDocument(textDocument)) {
            if (QPointer<TextEditorWidget> widget = editor->editorWidget()) {
                connect(widget, &TextEditorWidget::cursorPositionChanged, this, [this, widget](){
                    // TODO This would better be a compressing timer
                    QTimer::singleShot(50, this, [this, widget]() {
                        if (widget)
                            cursorPositionChanged(widget);
                    });
                });
            }
        }
    }

    m_openedDocument.append(document->filePath());
    sendContent(DidOpenTextDocumentNotification(DidOpenTextDocumentParams(item)));
    if (textDocument)
        requestDocumentSymbols(textDocument);
}

void BaseClient::sendContent(const IContent &content)
{
    QTC_ASSERT(m_state == Initialized, return);
    content.registerResponseHandler(&m_responseHandlers);
    QString error;
    if (!QTC_GUARD(content.isValid(&error)))
        Core::MessageManager::write(error);
    sendData(content.toBaseMessage().toData());
}

void BaseClient::sendContent(const DocumentUri &uri, const IContent &content)
{
    if (!m_openedDocument.contains(uri.toFileName()))
        return;
    sendContent(content);
}

void BaseClient::cancelRequest(const MessageId &id)
{
    m_responseHandlers.remove(id);
    sendContent(CancelRequest(CancelParameter(id)));
}

void BaseClient::closeDocument(const DidCloseTextDocumentParams &params)
{
    sendContent(params.textDocument().uri(), DidCloseTextDocumentNotification(params));
}

void BaseClient::documentContentsSaved(Core::IDocument *document)
{
    if (!m_openedDocument.contains(document->filePath()))
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
                TextDocumentIdentifier(DocumentUri::fromFileName(document->filePath())));
    if (includeText)
        params.setText(QString::fromUtf8(document->contents()));
    sendContent(DidSaveTextDocumentNotification(params));
}

void BaseClient::documentWillSave(Core::IDocument *document)
{
    const FileName &filePath = document->filePath();
    if (!m_openedDocument.contains(filePath))
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
                TextDocumentIdentifier(DocumentUri::fromFileName(document->filePath())));
    sendContent(WillSaveTextDocumentNotification(params));
}

void BaseClient::documentContentsChanged(Core::IDocument *document)
{
    if (!m_openedDocument.contains(document->filePath()))
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
    if (syncKind != TextDocumentSyncKind::None) {
        const auto uri = DocumentUri::fromFileName(document->filePath());
        VersionedTextDocumentIdentifier docId(uri);
        docId.setVersion(textDocument ? textDocument->document()->revision() : 0);
        const DidChangeTextDocumentParams params(docId, QString::fromUtf8(document->contents()));
        sendContent(DidChangeTextDocumentNotification(params));
    }
    if (textDocument)
        requestDocumentSymbols(textDocument);
}

void BaseClient::registerCapabilities(const QList<Registration> &registrations)
{
    m_dynamicCapabilities.registerCapability(registrations);
}

void BaseClient::unregisterCapabilities(const QList<Unregistration> &unregistrations)
{
    m_dynamicCapabilities.unregisterCapability(unregistrations);
}

bool BaseClient::findLinkAt(GotoDefinitionRequest &request)
{
    bool sendMessage = m_dynamicCapabilities.isRegistered(
                GotoDefinitionRequest::methodName).value_or(false);
    if (sendMessage) {
        const TextDocumentRegistrationOptions option(
                    m_dynamicCapabilities.option(GotoDefinitionRequest::methodName));
        if (option.isValid(nullptr))
            sendMessage = option.filterApplies(Utils::FileName::fromString(QUrl(request.params()->textDocument().uri()).adjusted(QUrl::PreferLocalFile).toString()));
    } else {
        sendMessage = m_serverCapabilities.definitionProvider().value_or(sendMessage);
    }
    if (sendMessage)
        sendContent(request);
    return sendMessage;
}

TextEditor::HighlightingResult createHighlightingResult(const SymbolInformation &info)
{
    if (!info.isValid(nullptr))
        return {};
    const Position &start = info.location().range().start();
    return TextEditor::HighlightingResult(start.line() + 1, start.character() + 1,
                                          info.name().length(), info.kind());
}

void BaseClient::requestDocumentSymbols(TextEditor::TextDocument *document)
{
    // TODO: Do not use this information for highlighting but the overview model
    return;
    const FileName &filePath = document->filePath();
    bool sendMessage = m_dynamicCapabilities.isRegistered(DocumentSymbolsRequest::methodName).value_or(false);
    if (sendMessage) {
        const TextDocumentRegistrationOptions option(m_dynamicCapabilities.option(DocumentSymbolsRequest::methodName));
        if (option.isValid(nullptr))
            sendMessage = option.filterApplies(filePath, Utils::mimeTypeForName(document->mimeType()));
    } else {
        sendMessage = m_serverCapabilities.documentSymbolProvider().value_or(false);
    }
    if (!sendMessage)
        return;
    DocumentSymbolsRequest request(
                DocumentSymbolParams(TextDocumentIdentifier(DocumentUri::fromFileName(filePath))));
    request.setResponseCallback(
                [doc = QPointer<TextEditor::TextDocument>(document)]
                (Response<DocumentSymbolsResult, LanguageClientNull> response){
        if (!doc)
            return;
        const DocumentSymbolsResult result = response.result().value_or(DocumentSymbolsResult());
        if (!holds_alternative<QList<SymbolInformation>>(result))
            return;
        const auto &symbols = get<QList<SymbolInformation>>(result);

        QFutureInterface<TextEditor::HighlightingResult> future;
        for (const SymbolInformation &symbol : symbols)
            future.reportResult(createHighlightingResult(symbol));

        const TextEditor::FontSettings &fs = doc->fontSettings();
        QHash<int, QTextCharFormat> formatMap;
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::File         )]
                = fs.toTextCharFormat(TextEditor::C_STRING);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Module       )]
                = fs.toTextCharFormat(TextEditor::C_STRING);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Namespace    )]
                = fs.toTextCharFormat(TextEditor::C_STRING);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Package      )]
                = fs.toTextCharFormat(TextEditor::C_STRING);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Class        )]
                = fs.toTextCharFormat(TextEditor::C_TYPE);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Method       )]
                = fs.toTextCharFormat(TextEditor::C_FUNCTION);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Property     )]
                = fs.toTextCharFormat(TextEditor::C_FIELD);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Field        )]
                = fs.toTextCharFormat(TextEditor::C_FIELD);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Constructor  )]
                = fs.toTextCharFormat(TextEditor::C_FUNCTION);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Enum         )]
                = fs.toTextCharFormat(TextEditor::C_TYPE);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Interface    )]
                = fs.toTextCharFormat(TextEditor::C_TYPE);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Function     )]
                = fs.toTextCharFormat(TextEditor::C_FUNCTION);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Variable     )]
                = fs.toTextCharFormat(TextEditor::C_LOCAL);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Constant     )]
                = fs.toTextCharFormat(TextEditor::C_LOCAL);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::String       )]
                = fs.toTextCharFormat(TextEditor::C_STRING);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Number       )]
                = fs.toTextCharFormat(TextEditor::C_NUMBER);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Boolean      )]
                = fs.toTextCharFormat(TextEditor::C_KEYWORD);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Array        )]
                = fs.toTextCharFormat(TextEditor::C_STRING);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Object       )]
                = fs.toTextCharFormat(TextEditor::C_LOCAL);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Key          )]
                = fs.toTextCharFormat(TextEditor::C_LOCAL);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Null         )]
                = fs.toTextCharFormat(TextEditor::C_KEYWORD);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::EnumMember   )]
                = fs.toTextCharFormat(TextEditor::C_ENUMERATION);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Struct       )]
                = fs.toTextCharFormat(TextEditor::C_TYPE);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Event        )]
                = fs.toTextCharFormat(TextEditor::C_STRING);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::Operator     )]
                = fs.toTextCharFormat(TextEditor::C_OPERATOR);
        formatMap[static_cast<int>(LanguageServerProtocol::SymbolKind::TypeParameter)]
                = fs.toTextCharFormat(TextEditor::C_LOCAL);

        TextEditor::SemanticHighlighter::incrementalApplyExtraAdditionalFormats(
                    doc->syntaxHighlighter(), future.future(), 0, future.resultCount() - 1,
                    formatMap);
    });
    sendContent(request);
}

void BaseClient::cursorPositionChanged(TextEditor::TextEditorWidget *widget)
{
    const auto uri = DocumentUri::fromFileName(widget->textDocument()->filePath());
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
                (Response<DocumentHighlightsResult, LanguageClientNull> response)
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

void BaseClient::projectOpened(ProjectExplorer::Project *project)
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

void BaseClient::projectClosed(ProjectExplorer::Project *project)
{
    if (!sendWorkspceFolderChanges())
        return;
    WorkspaceFoldersChangeEvent event;
    event.setRemoved({WorkSpaceFolder(project->projectDirectory().toString(), project->displayName())});
    DidChangeWorkspaceFoldersParams params;
    params.setEvent(event);
    DidChangeWorkspaceFoldersNotification change(params);
    sendContent(change);
}

void BaseClient::setSupportedLanguage(const LanguageFilter &filter)
{
    m_languagFilter = filter;
}

bool BaseClient::isSupportedDocument(const Core::IDocument *document) const
{
    QTC_ASSERT(document, return false);
    if (m_languagFilter.mimeTypes.isEmpty() || m_languagFilter.mimeTypes.contains(document->mimeType()))
        return true;
    auto regexps = Utils::transform(m_languagFilter.filePattern, [](const QString &pattern){
        return QRegExp(pattern, Utils::HostOsInfo::fileNameCaseSensitivity(), QRegExp::Wildcard);
    });
    return Utils::anyOf(regexps, [filePath = document->filePath()](const QRegExp &reg){
        return reg.exactMatch(filePath.toString()) || reg.exactMatch(filePath.fileName());
    });
}

bool BaseClient::needsRestart(const BaseSettings *settings) const
{
    QTC_ASSERT(settings, return false);
    return m_languagFilter.mimeTypes != settings->m_languageFilter.mimeTypes
            || m_languagFilter.filePattern != settings->m_languageFilter.filePattern;
}

bool BaseClient::reset()
{
    if (!m_restartsLeft)
        return false;
    --m_restartsLeft;
    m_state = Uninitialized;
    m_responseHandlers.clear();
    m_buffer.close();
    m_buffer.setData(nullptr);
    m_buffer.open(QIODevice::ReadWrite | QIODevice::Append);
    m_openedDocument.clear();
    m_serverCapabilities = ServerCapabilities();
    m_dynamicCapabilities.reset();
    return true;
}

void BaseClient::setError(const QString &message)
{
    log(message);
    m_state = Error;
}

void BaseClient::log(const QString &message, Core::MessageManager::PrintToOutputPaneFlag flag)
{
    Core::MessageManager::write(QString("LanguageClient %1: %2").arg(name(), message), flag);
}

void BaseClient::log(const ShowMessageParams &message,
                     Core::MessageManager::PrintToOutputPaneFlag flag)
{
    log(message.toString(), flag);
}

void BaseClient::showMessageBox(const ShowMessageRequestParams &message, const MessageId &id)
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
        Response<LanguageClientValue<MessageActionItem>, LanguageClientNull> response;
        response.setId(id);
        const MessageActionItem &item = itemForButton.value(box->clickedButton());
        response.setResult(item.isValid(nullptr) ? LanguageClientValue<MessageActionItem>(item)
                                                 : LanguageClientValue<MessageActionItem>());
        sendContent(response);
    });
    box->show();
}

void BaseClient::handleResponse(const MessageId &id, const QByteArray &content, QTextCodec *codec)
{
    if (auto handler = m_responseHandlers[id])
        handler(content, codec);
}

void BaseClient::handleMethod(const QString &method, MessageId id, const IContent *content)
{
    QStringList error;
    bool paramsValid = true;
    if (method == PublishDiagnosticsNotification::methodName) {
        auto params = dynamic_cast<const PublishDiagnosticsNotification *>(content)->params().value_or(PublishDiagnosticsParams());
        paramsValid = params.isValid(&error);
        if (paramsValid)
            LanguageClientManager::publishDiagnostics(m_id, params);
    } else if (method == LogMessageNotification::methodName) {
        auto params = dynamic_cast<const LogMessageNotification *>(content)->params().value_or(LogMessageParams());
        paramsValid = params.isValid(&error);
        if (paramsValid)
            log(params, Core::MessageManager::Flash);
    } else if (method == ShowMessageNotification::methodName) {
        auto params = dynamic_cast<const ShowMessageNotification *>(content)->params().value_or(ShowMessageParams());
        paramsValid = params.isValid(&error);
        if (paramsValid)
            log(params);
    } else if (method == ShowMessageRequest::methodName) {
        const ShowMessageRequest *request = dynamic_cast<const ShowMessageRequest *>(content);
        auto params = request->params().value_or(ShowMessageRequestParams());
        paramsValid = params.isValid(&error);
        if (paramsValid) {
            showMessageBox(params, request->id());
        } else {
            Response<LanguageClientValue<MessageActionItem>, LanguageClientNull> response;
            response.setId(request->id());
            ResponseError<LanguageClientNull> error;
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
    } else if (id.isValid(&error)) {
        Response<JsonObject, JsonObject> response;
        response.setId(id);
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

void BaseClient::intializeCallback(const InitializeResponse &initResponse)
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
        if (!result.isValid(&error)) // continue on ill formed result
            log(tr("Initialize result is not valid: ") + error.join("->"));

        m_serverCapabilities = result.capabilities().value_or(ServerCapabilities());
    }
    qCDebug(LOGLSPCLIENT) << "language server " << m_displayName << " initialized";
    m_state = Initialized;
    sendContent(InitializeNotification());
    emit initialized(m_serverCapabilities);
    for (auto openedDocument : Core::DocumentModel::openedDocuments())
        openDocument(openedDocument);
}

void BaseClient::shutDownCallback(const ShutdownResponse &shutdownResponse)
{
    QTC_ASSERT(m_state == ShutdownRequested, return);
    optional<ResponseError<JsonObject>> errorValue = shutdownResponse.error();
    if (errorValue.has_value()) {
        ResponseError<JsonObject> error = errorValue.value();
        qDebug() << error;
        return;
    }
    // directly send data otherwise the state check would fail;
    sendData(ExitNotification().toBaseMessage().toData());
    qCDebug(LOGLSPCLIENT) << "language server " << m_displayName << " shutdown";
    m_state = Shutdown;
}

bool BaseClient::sendWorkspceFolderChanges() const
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

void BaseClient::parseData(const QByteArray &data)
{
    const qint64 preWritePosition = m_buffer.pos();
    if (!m_buffer.atEnd())
        m_buffer.seek(preWritePosition + m_buffer.bytesAvailable());
    m_buffer.write(data);
    m_buffer.seek(preWritePosition);
    while (!m_buffer.atEnd()) {
        QString parseError;
        BaseMessage::parse(&m_buffer, parseError, m_currentMessage);
        if (!parseError.isEmpty())
            log(parseError);
        if (!m_currentMessage.isComplete())
            break;
        if (auto handler = m_contentHandler[m_currentMessage.mimeType]){
            QString parseError;
            handler(m_currentMessage.content, m_currentMessage.codec, parseError,
                    [this](MessageId id, const QByteArray &content, QTextCodec *codec){
                this->handleResponse(id, content, codec);
            },
                    [this](const QString &method, MessageId id, const IContent *content){
                this->handleMethod(method, id, content);
            });
            if (!parseError.isEmpty())
                log(parseError);
        } else {
            log(tr("Cannot handle content of type: %1").arg(QLatin1String(m_currentMessage.mimeType)));
        }
        m_currentMessage = BaseMessage();
    }
}

StdIOClient::StdIOClient(const QString &executable, const QString &arguments)
    : m_executable(executable)
    , m_arguments(arguments)
{
    connect(&m_process, &QProcess::readyReadStandardError,
            this, &StdIOClient::readError);
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &StdIOClient::readOutput);
    connect(&m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &StdIOClient::onProcessFinished);

    m_process.setArguments(Utils::QtcProcess::splitArgs(m_arguments));
    m_process.setProgram(m_executable);
}

StdIOClient::~StdIOClient()
{
    Utils::SynchronousProcess::stopProcess(m_process);
}

bool StdIOClient::needsRestart(const StdIOSettings *settings)
{
    return m_executable != settings->m_executable || m_arguments != settings->m_arguments;
}

bool StdIOClient::start()
{
    m_process.start();
    if (!m_process.waitForStarted() || m_process.state() != QProcess::Running) {
        setError(m_process.errorString());
        return false;
    }
    return true;
}

void StdIOClient::setWorkingDirectory(const QString &workingDirectory)
{
    m_process.setWorkingDirectory(workingDirectory);
}

void StdIOClient::sendData(const QByteArray &data)
{
    if (m_process.state() != QProcess::Running) {
        log(tr("Cannot send data to unstarted server %1").arg(m_process.program()));
        return;
    }
    qCDebug(LOGLSPCLIENTV) << "StdIOClient send data:";
    qCDebug(LOGLSPCLIENTV).noquote() << data;
    m_process.write(data);
}

void StdIOClient::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit)
        setError(tr("Crashed with exit code %1: %2").arg(exitCode, m_process.error()));
    emit finished();
}

void StdIOClient::readError()
{
    qCDebug(LOGLSPCLIENTV) << "StdIOClient std err:\n";
    qCDebug(LOGLSPCLIENTV).noquote() << m_process.readAllStandardError();
}

void StdIOClient::readOutput()
{
    const QByteArray &out = m_process.readAllStandardOutput();
    qDebug(LOGLSPCLIENTV) << "StdIOClient std out:\n";
    qDebug(LOGLSPCLIENTV).noquote() << out;
    parseData(out);
}

} // namespace LanguageClient
