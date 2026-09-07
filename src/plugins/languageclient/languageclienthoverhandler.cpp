// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclienthoverhandler.h"

#include "client.h"
#include "languageclientutils.h"
#include "dynamiccapabilities.h"

#include <languageserverprotocol/lsputils.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/mimeutils.h>

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
    m_pendingHover.reset();
}

void HoverHandler::setPreferDiagnosticts(bool prefer)
{
    m_preferDiagnostics = prefer;
}

void HoverHandler::setHelpItem(const MessageId &msgId, const Core::HelpItem &help)
{
    if (!m_pendingHover || !(msgId == m_pendingHover->first))
        return;
    setContent(m_pendingHover->second.contents());
    m_pendingHover.reset();
    setLastHelpItemIdentified(help);
    m_report(priority());
}

bool HoverHandler::reportDiagnostics(const QTextCursor &cursor)
{
    const QList<Diagnostic> diagnostics = m_client->diagnosticsAt(m_filePath, cursor);
    if (diagnostics.isEmpty())
        return false;

    const QStringList messages = Utils::transform(diagnostics, [](const Diagnostic &diagnostic) {
        return plainText(diagnostic.message());
    });
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
    m_pendingHover.reset();
    m_report = report;

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(pos);
    if (m_preferDiagnostics && reportDiagnostics(cursor))
        return;

    const std::optional<ServerCapabilitiesHoverProvider> &provider
        = m_client->capabilities().hoverProvider();
    const bool *boolvalue = provider.has_value() ? std::get_if<bool>(&*provider) : nullptr;
    bool sendMessage = provider.has_value() && (!boolvalue || *boolvalue);
    if (std::optional<bool> registered = m_client->dynamicCapabilities().isRegistered(
            HoverRequest::method)) {
        sendMessage = *registered;
        if (sendMessage) {
            sendMessage = registrationApplies(
                m_client->dynamicCapabilities().option(HoverRequest::method),
                editorWidget->textDocument()->filePath(),
                editorWidget->textDocument()->mimeType());
        }
    }
    if (!sendMessage) {
        report(Priority_None);
        return;
    }

    HoverParams params;
    params.textDocument(TextDocumentIdentifier().uri(m_client->uriFor(m_filePath)));
    params.position(positionOf(cursor));
    // The id is only known after sending, and the callback needs it to tell the
    // help item provider which request it is answering.
    const auto id = std::make_shared<MessageId>();
    *id = m_client->sendRequest<HoverRequest>(
        params, [this, cursor, id](const Utils::Result<HoverRequestResult> &result) {
            handleResponse(*id, result, cursor);
        });
    m_currentRequest = *id;
}

void HoverHandler::handleResponse(
    const MessageId &id, const Utils::Result<HoverRequestResult> &result, const QTextCursor &cursor)
{
    m_currentRequest.reset();
    if (!result) {
        if (m_client)
            m_client->log(QtMsgType::QtCriticalMsg, result.error());
    } else if (const auto hover = std::get_if<Hover>(&*result)) {
        if (m_helpItemProvider) {
            m_pendingHover = {id, *hover};
            m_helpItemProvider(id, *hover, m_filePath);
            return;
        }
        setContent(hover->contents());
    } else if (!m_preferDiagnostics && reportDiagnostics(cursor)) {
        return;
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
        else if (auto string = std::get_if<MarkedStringWithLanguage>(&markedString))
            tooltip += string->value() + " [" + string->language() + ']';
    }
    return tooltip;
}

void HoverHandler::setContent(const HoverContents &contents)
{
    if (auto markupContent = std::get_if<MarkupContent>(&contents)) {
        setToolTip(
            markupContent->value(),
            markupContent->kind() == MarkupKind::markdown ? Qt::MarkdownText : Qt::PlainText);
    } else if (auto markedString = std::get_if<MarkedString>(&contents)) {
        setToolTip(toolTipForMarkedStrings({*markedString}), Qt::MarkdownText);
    } else if (auto markedStrings = std::get_if<QList<MarkedString>>(&contents)) {
        setToolTip(toolTipForMarkedStrings(*markedStrings), Qt::MarkdownText);
    }
}

} // namespace LanguageClient
