/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "languageclienthoverhandler.h"

#include "client.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/mimetypes/mimedatabase.h>
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
    if (m_client && m_client->reachable() && m_currentRequest.has_value())
        m_client->cancelRequest(*m_currentRequest);
    m_currentRequest.reset();
}

void HoverHandler::identifyMatch(TextEditor::TextEditorWidget *editorWidget,
                                 int pos,
                                 TextEditor::BaseHoverHandler::ReportPriority report)
{
    if (m_currentRequest.has_value())
        abort();
    if (m_client.isNull()
        || !m_client->documentOpen(editorWidget->textDocument())
        || !m_client->capabilities().hoverProvider().value_or(false)) {
        report(Priority_None);
        return;
    }
    auto uri = DocumentUri::fromFilePath(editorWidget->textDocument()->filePath());
    QTextCursor tc = editorWidget->textCursor();
    tc.setPosition(pos);
    QList<Diagnostic> diagnostics = m_client->diagnosticsAt(uri, Range(Position(tc), Position(tc)));
    if (!diagnostics.isEmpty()) {
        const QStringList messages = Utils::transform(diagnostics, &Diagnostic::message);
        setToolTip(messages.join('\n'));
        report(Priority_Diagnostic);
        return;
    }

    bool sendMessage = m_client->capabilities().hoverProvider().value_or(false);
    if (Utils::optional<bool> registered = m_client->dynamicCapabilities().isRegistered(
            HoverRequest::methodName)) {
        sendMessage = registered.value();
        if (sendMessage) {
            const TextDocumentRegistrationOptions option(
                m_client->dynamicCapabilities().option(HoverRequest::methodName).toObject());
            if (option.isValid(nullptr)) {
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

    m_report = report;
    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(pos);
    HoverRequest request((TextDocumentPositionParams(TextDocumentIdentifier(uri), Position(cursor))));
    request.setResponseCallback(
        [this](const HoverRequest::Response &response) { handleResponse(response); });
    m_client->sendContent(request);
}

void HoverHandler::handleResponse(const HoverRequest::Response &response)
{
    m_currentRequest.reset();
    if (Utils::optional<HoverRequest::Response::Error> error = response.error()) {
        if (m_client)
            m_client->log(error.value());
    }
    if (Utils::optional<Hover> result = response.result())
        setContent(result.value().content());
    m_report(priority());
}

static QString toolTipForMarkedStrings(const QList<MarkedString> &markedStrings)
{
    QString tooltip;
    for (const MarkedString &markedString : markedStrings) {
        if (!tooltip.isEmpty())
            tooltip += '\n';
        if (auto string = Utils::get_if<QString>(&markedString))
            tooltip += *string;
        else if (auto string = Utils::get_if<MarkedLanguageString>(&markedString))
            tooltip += string->value() + " [" + string->language() + ']';
    }
    return tooltip;
}

void HoverHandler::setContent(const HoverContent &hoverContent)
{
    if (auto markupContent = Utils::get_if<MarkupContent>(&hoverContent))
        setToolTip(markupContent->content(), markupContent->textFormat());
    else if (auto markedString = Utils::get_if<MarkedString>(&hoverContent))
        setToolTip(toolTipForMarkedStrings({*markedString}));
    else if (auto markedStrings = Utils::get_if<QList<MarkedString>>(&hoverContent))
        setToolTip(toolTipForMarkedStrings(*markedStrings));
}

} // namespace LanguageClient
