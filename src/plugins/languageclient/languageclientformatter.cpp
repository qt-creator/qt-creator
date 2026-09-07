// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientformatter.h"

#include "client.h"
#include "dynamiccapabilities.h"
#include "languageclientutils.h"

#include <languageserverprotocol/lsputils.h>

#include <texteditor/tabsettings.h>
#include <texteditor/textdocument.h>
#include <utils/mimeutils.h>

#include <QTextDocument>

using namespace LanguageServerProtocol;
using namespace Utils;

namespace LanguageClient {

LanguageClientFormatter::LanguageClientFormatter(TextEditor::TextDocument *document, Client *client)
    : m_client(client)
    , m_document(document)
    , m_formattingRequester(std::make_unique<RangeFormattingRequest>(m_client, m_document))
{
    m_cancelConnection = QObject::connect(document->document(),
                                          &QTextDocument::contentsChanged,
                                          [this] {
        if (m_ignoreCancel)
            m_ignoreCancel = false;
        else
            cancelCurrentRequest();
    });
}

LanguageClientFormatter::~LanguageClientFormatter()
{
    QObject::disconnect(m_cancelConnection);
    cancelCurrentRequest();
}

void LanguageClientFormatter::handleResponse(const Utils::Result<ResultType> &result)
{
    m_currentRequest = std::nullopt;
    if (!result) {
        if (QTC_GUARD(m_client))
            m_client->log(QtMsgType::QtCriticalMsg, result.error());
    }
    Utils::ChangeSet changeSet;
    if (result) {
        if (const auto edits = std::get_if<QList<TextEdit>>(&*result))
            changeSet = editsToChangeSet(*edits, m_document->document());
    }
    if (m_formatCallback)
        m_formatCallback(changeSet);
}

static FormattingOptions formattingOptions(const TextEditor::TabSettingsData &settings)
{
    return FormattingOptions()
        .tabSize(settings.m_tabSize)
        .insertSpaces(settings.m_tabPolicy == TextEditor::TabSettingsData::SpacesOnlyTabPolicy);
}

template <typename RequestType>
bool canRequest(QPointer<Client> client, TextEditor::TextDocument *document)
{
    if (!client || !document)
        return false;
    const FilePath &filePath = document->filePath();
    const DynamicCapabilities dynamicCapabilities = client->dynamicCapabilities();

    const QString method = RequestType::method;

    if (std::optional<bool> registered = dynamicCapabilities.isRegistered(method)) {
        if (!*registered)
            return false;
        if (!registrationApplies(dynamicCapabilities.option(method), filePath,
                                 document->mimeType())) {
            return false;
        }
    } else {
        bool supported = false;
        if constexpr (std::is_same_v<RequestType, DocumentFormattingRequest>) {
            if (const auto &provider = client->capabilities().documentFormattingProvider())
                supported = !std::holds_alternative<bool>(*provider) || std::get<bool>(*provider);
        } else {
            if (const auto &provider = client->capabilities().documentRangeFormattingProvider())
                supported = !std::holds_alternative<bool>(*provider) || std::get<bool>(*provider);
        }
        if (!supported)
            return false;
    }

    return true;
}

void LanguageClientFormatter::setMode(FormatMode mode)
{
    switch (mode) {
    case FormatMode::FullDocument:
        m_formattingRequester = std::make_unique<FullFormattingRequest>(m_client, m_document);
        break;
    case FormatMode::Range:
        m_formattingRequester = std::make_unique<RangeFormattingRequest>(m_client, m_document);
        break;
    }
}

void LanguageClientFormatter::format(const QTextCursor &cursor,
        const TextEditor::TabSettingsData &tabSettings,
        const TextEditor::FormatCallback &callback)
{
    QTC_ASSERT(m_client, return);
    cancelCurrentRequest();

    m_formatCallback = callback;
    m_currentRequest = m_formattingRequester->sendRequest(cursor, tabSettings, this);
    if (m_currentRequest) {
        // ignore first contents changed, because this function is called inside a begin/endEdit block
        m_ignoreCancel = true;
    }
}

void LanguageClientFormatter::cancelCurrentRequest()
{
    if (QTC_GUARD(m_client) && m_currentRequest.has_value()) {
        m_formatCallback = {};
        m_client->cancelRequest(*m_currentRequest);
        m_ignoreCancel = false;
        m_currentRequest = std::nullopt;
    }
}

IFormattingRequest::IFormattingRequest(Client *client, TextEditor::TextDocument *document)
    : m_client(client)
    , m_document(document)
{
}

RangeFormattingRequest::RangeFormattingRequest(Client *client, TextEditor::TextDocument *document)
    : IFormattingRequest(client, document)
{
}

std::optional<MessageId> RangeFormattingRequest::sendRequest(
    const QTextCursor &cursor,
    const TextEditor::TabSettingsData &settings,
    LanguageClientFormatter *formatter)
{
    if (!canRequest<DocumentRangeFormattingRequest>(m_client, m_document))
        return {};
    DocumentRangeFormattingParams params;
    params.textDocument(TextDocumentIdentifier().uri(m_client->uriFor(m_document->filePath())));
    params.options(formattingOptions(settings));
    if (cursor.hasSelection()) {
        params.range(rangeOf(cursor));
    } else {
        QTextCursor c = cursor;
        c.select(QTextCursor::LineUnderCursor);
        params.range(rangeOf(c));
    }
    return m_client->sendRequest<DocumentRangeFormattingRequest>(
        params, [formatter](const Utils::Result<LanguageClientFormatter::ResultType> &result) {
            formatter->handleResponse(result);
        });
}

FullFormattingRequest::FullFormattingRequest(Client *client, TextEditor::TextDocument *document)
    : IFormattingRequest(client, document)
{
}

std::optional<MessageId> FullFormattingRequest::sendRequest(
    const QTextCursor &cursor,
    const TextEditor::TabSettingsData &settings,
    LanguageClientFormatter *formatter)
{
    Q_UNUSED(cursor)
    if (!canRequest<DocumentFormattingRequest>(m_client, m_document))
        return {};
    DocumentFormattingParams params;
    params.textDocument(TextDocumentIdentifier().uri(m_client->uriFor(m_document->filePath())));
    params.options(formattingOptions(settings));
    return m_client->sendRequest<DocumentFormattingRequest>(
        params, [formatter](const Utils::Result<LanguageClientFormatter::ResultType> &result) {
            formatter->handleResponse(result);
        });
}

} // namespace LanguageClient
