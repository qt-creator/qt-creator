// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclienthoverhandler.h"

#include "client.h"
#include "dynamiccapabilities.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/tooltip/tooltip.h>

using namespace LanguageServerProtocol;

namespace LanguageClient {

HoverHandler::HoverHandler(Client *client)
    : m_client(client)
{}

HoverHandler::~HoverHandler()
{
    abort();
}

void HoverHandler::abort()
{
    if (m_client && m_currentRequest.has_value()) {
        m_client->cancelRequest(*m_currentRequest);
        m_currentRequest.reset();
    }
    m_response = {};
}

void HoverHandler::setPreferDiagnosticts(bool prefer)
{
    m_preferDiagnostics = prefer;
}

void HoverHandler::setHelpItem(const LanguageServerProtocol::MessageId &msgId,
                               const Core::HelpItem &help)
{
    if (msgId == m_response.id()) {
        if (std::optional<HoverResult> result = m_response.result()) {
            if (auto hover = std::get_if<Hover>(&(*result)))
                setContent(hover->content());
        }
        m_response = {};
        setLastHelpItemIdentified(help);
        m_report(priority());
    }
}

bool HoverHandler::reportDiagnostics(const QTextCursor &cursor)
{
    const QList<Diagnostic> &diagnostics = m_client->diagnosticsAt(m_filePath, cursor);
    if (diagnostics.isEmpty())
        return false;

    const QStringList messages = Utils::transform(diagnostics, &Diagnostic::message);
    setToolTip(messages.join('\n'));
    m_report(Priority_Diagnostic);
    return true;
}

void HoverHandler::identifyMatch(TextEditor::TextEditorWidget *editorWidget,
                                 int pos,
                                 TextEditor::BaseHoverHandler::ReportPriority report)
{
    if (m_currentRequest.has_value())
        abort();
    if (m_client.isNull() || !m_client->documentOpen(editorWidget->textDocument())
        || !m_client->reachable()) {
        report(Priority_None);
        return;
    }
    m_filePath = editorWidget->textDocument()->filePath();
    m_response = {};
    m_report = report;

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(pos);
    if (m_preferDiagnostics && reportDiagnostics(cursor))
        return;

    const std::optional<std::variant<bool, WorkDoneProgressOptions>> &provider
        = m_client->capabilities().hoverProvider();
    bool sendMessage = provider.has_value();
    if (sendMessage && std::holds_alternative<bool>(*provider))
        sendMessage = std::get<bool>(*provider);
    if (std::optional<bool> registered = m_client->dynamicCapabilities().isRegistered(
            HoverRequest::methodName)) {
        sendMessage = *registered;
        if (sendMessage) {
            const TextDocumentRegistrationOptions option(
                m_client->dynamicCapabilities().option(HoverRequest::methodName).toObject());
            if (option.isValid()) {
                sendMessage = option.filterApplies(editorWidget->textDocument()->filePath(),
                                                   Utils::mimeTypeForName(
                                                       editorWidget->textDocument()->mimeType()));
            }
        }
    }
    if (!sendMessage) {
        report(Priority_None);
        return;
    }

    HoverRequest request{
        TextDocumentPositionParams(TextDocumentIdentifier(m_client->hostPathToServerUri(m_filePath)),
                                   Position(cursor))};
    m_currentRequest = request.id();
    request.setResponseCallback(
        [this, cursor](const HoverRequest::Response &response) { handleResponse(response, cursor); });
    m_client->sendMessage(request);
}

void HoverHandler::handleResponse(const HoverRequest::Response &response, const QTextCursor &cursor)
{
    m_currentRequest.reset();
    if (std::optional<HoverRequest::Response::Error> error = response.error()) {
        if (m_client)
            m_client->log(*error);
    }
    if (std::optional<HoverResult> result = response.result()) {
        if (auto hover = std::get_if<Hover>(&(*result))) {
            if (m_helpItemProvider) {
                m_response = response;
                m_helpItemProvider(response, m_filePath);
                return;
            }
            setContent(hover->content());
        } else if (!m_preferDiagnostics && reportDiagnostics(cursor)) {
            return;
        }
    }
    m_report(priority());
}

static QString toolTipForMarkedStrings(const QList<MarkedString> &markedStrings)
{
    QString tooltip;
    for (const MarkedString &markedString : markedStrings) {
        if (!tooltip.isEmpty())
            tooltip += '\n';
        if (auto string = std::get_if<QString>(&markedString))
            tooltip += *string;
        else if (auto string = std::get_if<MarkedLanguageString>(&markedString))
            tooltip += string->value() + " [" + string->language() + ']';
    }
    return tooltip;
}

void HoverHandler::setContent(const HoverContent &hoverContent)
{
    if (auto markupContent = std::get_if<MarkupContent>(&hoverContent))
        setToolTip(markupContent->content(), markupContent->textFormat());
    else if (auto markedString = std::get_if<MarkedString>(&hoverContent))
        setToolTip(toolTipForMarkedStrings({*markedString}));
    else if (auto markedStrings = std::get_if<QList<MarkedString>>(&hoverContent))
        setToolTip(toolTipForMarkedStrings(*markedStrings));
}

} // namespace LanguageClient
