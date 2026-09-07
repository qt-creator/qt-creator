// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "documentsymbolcache.h"

#include "client.h"

#include <languageserverprotocol/lspmessages.h>

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
            const QString uri = m_client->uriFor(document->filePath());
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

void DocumentSymbolCache::requestSymbols(const QString &uri, Schedule schedule)
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

static bool clientSupportsDocumentSymbols(const Client *client, const QString &uri)
{
    QTC_ASSERT(client, return false);
    const auto doc = TextEditor::TextDocument::textDocumentForFilePath(client->filePathFor(uri));
    return client->supportsDocumentSymbols(doc);
}

void DocumentSymbolCache::requestSymbolsImpl()
{
    if (!m_client->reachable()) {
        m_compressionTimer.start(200);
        return;
    }
    for (const QString &uri : std::as_const(m_compressedUris)) {
        auto entry = m_cache.find(uri);
        if (entry != m_cache.end()) {
            emit gotSymbols(uri, entry.value());
            continue;
        }

        if (!LanguageClient::clientSupportsDocumentSymbols(m_client, uri)) {
            emit gotSymbols(uri, std::monostate{});
            continue;
        }

        DocumentSymbolParams params;
        params.textDocument(TextDocumentIdentifier().uri(uri));
        m_runningRequests[uri] = m_client->sendRequest<DocumentSymbolRequest>(
            params,
            [uri, self = QPointer<DocumentSymbolCache>(this)](
                const Utils::Result<DocumentSymbolRequestResult> &result) {
                if (self)
                    self->handleResponse(uri, result);
            });
    }
    m_compressedUris.clear();
}

void DocumentSymbolCache::handleResponse(
    const QString &uri, const Utils::Result<DocumentSymbolRequestResult> &result)
{
    m_runningRequests.remove(uri);
    if (!result) {
        if (m_client)
            m_client->log(QtMsgType::QtCriticalMsg, result.error());
    }
    const DocumentSymbolRequestResult symbols = result
                                                    ? *result
                                                    : DocumentSymbolRequestResult(std::monostate{});
    m_cache[uri] = symbols;
    emit gotSymbols(uri, symbols);
}

} // namespace LanguageClient
