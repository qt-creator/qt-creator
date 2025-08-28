// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientformatter.h"

#include "client.h"
#include "dynamiccapabilities.h"
#include "languageclientutils.h"

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

void LanguageClientFormatter::handleResponse(const ResponseType &response)
{
    m_currentRequest = std::nullopt;
    const std::optional<ResponseType::Error> &error = response.error();
    if (QTC_GUARD(m_client) && error)
        m_client->log(*error);
    Utils::ChangeSet changeSet;
    if (std::optional<LanguageClientArray<TextEdit>> result = response.result()) {
        if (!result->isNull())
            changeSet = editsToChangeSet(result->toList(), m_document->document());
    }
    m_progress.reportResult(changeSet);
    m_progress.reportFinished();
}

static const FormattingOptions formattingOptions(const TextEditor::TabSettings &settings)
{
    FormattingOptions options;
    options.setTabSize(settings.m_tabSize);
    options.setInsertSpace(settings.m_tabPolicy == TextEditor::TabSettings::SpacesOnlyTabPolicy);
    return options;
}

template <typename RequestType>
bool canRequest(QPointer<Client> client, TextEditor::TextDocument *document)
{
    if (!client || !document)
        return false;
    const FilePath &filePath = document->filePath();
    const DynamicCapabilities dynamicCapabilities = client->dynamicCapabilities();

    QString method;
    if constexpr(std::is_same_v<RequestType, DocumentFormattingRequest>) {
        method = DocumentFormattingRequest::methodName;
    } else {
        method = DocumentRangeFormattingRequest::methodName;
    }

    if (std::optional<bool> registered = dynamicCapabilities.isRegistered(method)) {
        if (!*registered)
            return false;
        const TextDocumentRegistrationOptions option(dynamicCapabilities.option(method).toObject());
        if (option.isValid()
            && !option.filterApplies(filePath, Utils::mimeTypeForName(document->mimeType()))) {
            return false;;
        }
    } else {
        std::optional<std::variant<bool, WorkDoneProgressOptions>> provider;
        if constexpr(std::is_same_v<RequestType, DocumentFormattingRequest>) {
            provider = client->capabilities().documentFormattingProvider();
        } else {
            provider = client->capabilities().documentRangeFormattingProvider();
        }
        if (!provider.has_value())
            return false;
        const auto boolvalue = std::get_if<bool>(&*provider);
        if (boolvalue && !*boolvalue)
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

QFutureWatcher<ChangeSet> *LanguageClientFormatter::format(
        const QTextCursor &cursor, const TextEditor::TabSettings &tabSettings)
{
    QTC_ASSERT(m_client, return nullptr);
    cancelCurrentRequest();

    auto request = m_formattingRequester->prepareRequest(cursor, tabSettings, this);
    return std::visit([this](auto &&value) -> QFutureWatcher<ChangeSet> *{
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return nullptr;
        } else {
            m_progress = QFutureInterface<ChangeSet>();
            m_currentRequest = value.id();
            m_client->sendMessage(value);
            // ignore first contents changed, because this function is called inside a begin/endEdit block
            m_ignoreCancel = true;
            m_progress.reportStarted();
            auto watcher = new QFutureWatcher<ChangeSet>();
            QObject::connect(watcher, &QFutureWatcher<ChangeSet>::canceled, [this] {
                cancelCurrentRequest();
            });
            watcher->setFuture(m_progress.future());
            return watcher;
        }
    }, request);
}

void LanguageClientFormatter::cancelCurrentRequest()
{
    if (QTC_GUARD(m_client) && m_currentRequest.has_value()) {
        m_progress.reportCanceled();
        m_progress.reportFinished();
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

IFormattingRequest::RequestType RangeFormattingRequest::prepareRequest(
    const QTextCursor &cursor, const TextEditor::TabSettings &settings, LanguageClientFormatter *formatter)
{
    if (!canRequest<DocumentRangeFormattingRequest>(m_client, m_document))
        return std::monostate();
    const FilePath &filePath = m_document->filePath();
    DocumentRangeFormattingParams params;
    const DocumentUri uri = m_client->hostPathToServerUri(filePath);
    params.setTextDocument(TextDocumentIdentifier(uri));
    params.setOptions(formattingOptions(settings));
    if (!cursor.hasSelection()) {
        QTextCursor c = cursor;
        c.select(QTextCursor::LineUnderCursor);
        params.setRange(Range(c));
    } else {
        params.setRange(Range(cursor));
    }
    DocumentRangeFormattingRequest request(params);
    request.setResponseCallback([formatter](const DocumentRangeFormattingRequest::Response &response) {
        formatter->handleResponse(response);
    });
    return request;
}

FullFormattingRequest::FullFormattingRequest(Client *client, TextEditor::TextDocument *document)
    : IFormattingRequest(client, document)
{
}

IFormattingRequest::RequestType FullFormattingRequest::prepareRequest(
    const QTextCursor &cursor, const TextEditor::TabSettings &settings, LanguageClientFormatter *formatter)
{
    Q_UNUSED(cursor)
    if (!canRequest<DocumentFormattingRequest>(m_client, m_document))
        return std::monostate();
    const FilePath &filePath = m_document->filePath();
    DocumentFormattingParams params;
    const DocumentUri uri = m_client->hostPathToServerUri(filePath);
    params.setTextDocument(TextDocumentIdentifier(uri));
    params.setOptions(formattingOptions(settings));

    DocumentFormattingRequest request(params);
    request.setResponseCallback([formatter](const DocumentFormattingRequest::Response &response) {
        formatter->handleResponse(response);
    });
    return request;
}

} // namespace LanguageClient
