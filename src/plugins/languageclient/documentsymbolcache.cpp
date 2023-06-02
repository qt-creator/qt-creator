// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "documentsymbolcache.h"

#include "client.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/textdocument.h>

using namespace LanguageServerProtocol;

namespace LanguageClient {

DocumentSymbolCache::DocumentSymbolCache(Client *client)
    : QObject(client)
    , m_client(client)
{
    auto connectDocument = [this](Core::IDocument *document) {
        connect(document, &Core::IDocument::contentsChanged, this, [document, this]() {
            const auto uri = m_client->hostPathToServerUri(document->filePath());
            m_cache.remove(uri);
            auto requestIdIt = m_runningRequests.find(uri);
            if (requestIdIt != m_runningRequests.end()) {
                m_client->cancelRequest(requestIdIt.value());
                m_runningRequests.erase(requestIdIt);
            }
        });
    };

    for (Core::IDocument *document : Core::DocumentModel::openedDocuments())
        connectDocument(document);
    connect(Core::EditorManager::instance(),
            &Core::EditorManager::documentOpened,
            this,
            connectDocument);
    m_compressionTimer.setSingleShot(true);
    connect(&m_compressionTimer, &QTimer::timeout, this, &DocumentSymbolCache::requestSymbolsImpl);
}

void DocumentSymbolCache::requestSymbols(const DocumentUri &uri, Schedule schedule)
{
    if (m_runningRequests.contains(uri))
        return;
    m_compressedUris.insert(uri);
    switch (schedule) {
    case Schedule::Now:
        requestSymbolsImpl();
        break;
    case Schedule::Delayed:
        m_compressionTimer.start(200);
        break;
    }
}

bool clientSupportsDocumentSymbols(const Client *client, const DocumentUri &uri)
{
    QTC_ASSERT(client, return false);
    const auto doc = TextEditor::TextDocument::textDocumentForFilePath(
        uri.toFilePath(client->hostPathMapper()));
    return client->supportsDocumentSymbols(doc);
}

void DocumentSymbolCache::requestSymbolsImpl()
{
    if (!m_client->reachable()) {
        m_compressionTimer.start(200);
        return;
    }
    for (const DocumentUri &uri : std::as_const(m_compressedUris)) {
        auto entry = m_cache.find(uri);
        if (entry != m_cache.end()) {
            emit gotSymbols(uri, entry.value());
            continue;
        }

        if (!LanguageClient::clientSupportsDocumentSymbols(m_client, uri)) {
            emit gotSymbols(uri, nullptr);
            continue;
        }

        const DocumentSymbolParams params((TextDocumentIdentifier(uri)));
        DocumentSymbolsRequest request(params);
        request.setResponseCallback([uri, self = QPointer<DocumentSymbolCache>(this)](
                                        const DocumentSymbolsRequest::Response &response) {
            if (self)
                self->handleResponse(uri, response);
        });
        m_runningRequests[uri] = request.id();
        m_client->sendMessage(request);
    }
    m_compressedUris.clear();
}

void DocumentSymbolCache::handleResponse(const DocumentUri &uri,
                                         const DocumentSymbolsRequest::Response &response)
{
    m_runningRequests.remove(uri);
    if (std::optional<DocumentSymbolsRequest::Response::Error> error = response.error()) {
        if (m_client)
            m_client->log(*error);
    }
    const DocumentSymbolsResult &symbols = response.result().value_or(DocumentSymbolsResult());
    m_cache[uri] = symbols;
    emit gotSymbols(uri, symbols);
}

} // namespace LanguageClient
