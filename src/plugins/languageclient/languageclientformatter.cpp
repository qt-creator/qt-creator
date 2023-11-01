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
{
    m_cancelConnection = QObject::connect(document->document(),
                                          &QTextDocument::contentsChanged,
                                          [this]() {
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

static const FormattingOptions formattingOptions(const TextEditor::TabSettings &settings)
{
    FormattingOptions options;
    options.setTabSize(settings.m_tabSize);
    options.setInsertSpace(settings.m_tabPolicy == TextEditor::TabSettings::SpacesOnlyTabPolicy);
    return options;
}

QFutureWatcher<ChangeSet> *LanguageClientFormatter::format(
        const QTextCursor &cursor, const TextEditor::TabSettings &tabSettings)
{
    QTC_ASSERT(m_client, return nullptr);
    cancelCurrentRequest();
    m_progress = QFutureInterface<ChangeSet>();

    const FilePath &filePath = m_document->filePath();
    const DynamicCapabilities dynamicCapabilities = m_client->dynamicCapabilities();
    const QString method(DocumentRangeFormattingRequest::methodName);
    if (std::optional<bool> registered = dynamicCapabilities.isRegistered(method)) {
        if (!*registered)
            return nullptr;
        const TextDocumentRegistrationOptions option(dynamicCapabilities.option(method).toObject());
        if (option.isValid()
                && !option.filterApplies(filePath, Utils::mimeTypeForName(m_document->mimeType()))) {
            return nullptr;
        }
    } else {
        const std::optional<std::variant<bool, WorkDoneProgressOptions>> &provider
            = m_client->capabilities().documentRangeFormattingProvider();
        if (!provider.has_value())
            return nullptr;
        if (std::holds_alternative<bool>(*provider) && !std::get<bool>(*provider))
            return nullptr;
    }
    DocumentRangeFormattingParams params;
    const DocumentUri uri = m_client->hostPathToServerUri(filePath);
    params.setTextDocument(TextDocumentIdentifier(uri));
    params.setOptions(formattingOptions(tabSettings));
    if (!cursor.hasSelection()) {
        QTextCursor c = cursor;
        c.select(QTextCursor::LineUnderCursor);
        params.setRange(Range(c));
    } else {
        params.setRange(Range(cursor));
    }
    DocumentRangeFormattingRequest request(params);
    request.setResponseCallback([this](const DocumentRangeFormattingRequest::Response &response) {
        handleResponse(response);
    });
    m_currentRequest = request.id();
    m_client->sendMessage(request);
    // ignore first contents changed, because this function is called inside a begin/endEdit block
    m_ignoreCancel = true;
    m_progress.reportStarted();
    auto watcher = new QFutureWatcher<ChangeSet>();
    QObject::connect(watcher, &QFutureWatcher<ChangeSet>::canceled, [this]() {
        cancelCurrentRequest();
    });
    watcher->setFuture(m_progress.future());
    return watcher;
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

void LanguageClientFormatter::handleResponse(const DocumentRangeFormattingRequest::Response &response)
{
    m_currentRequest = std::nullopt;
    const std::optional<DocumentRangeFormattingRequest::Response::Error> &error = response.error();
    if (QTC_GUARD(m_client) && error)
        m_client->log(*error);
    ChangeSet changeSet;
    if (std::optional<LanguageClientArray<TextEdit>> result = response.result()) {
        if (!result->isNull())
            changeSet = editsToChangeSet(result->toList(), m_document->document());
    }
    m_progress.reportResult(changeSet);
    m_progress.reportFinished();
}

} // namespace LanguageClient
