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
            const auto uri = DocumentUri::fromFilePath(document->filePath());
            m_cache.remove(DocumentUri::fromFilePath(document->filePath()));
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
    const auto doc = TextEditor::TextDocument::textDocumentForFilePath(uri.toFilePath());
    return client->supportsDocumentSymbols(doc);
}

void DocumentSymbolCache::requestSymbolsImpl()
{
    if (!m_client->reachable()) {
        m_compressionTimer.start(200);
        return;
    }
    for (const DocumentUri &uri : qAsConst(m_compressedUris)) {
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
        m_client->sendContent(request);
    }
    m_compressedUris.clear();
}

void DocumentSymbolCache::handleResponse(const DocumentUri &uri,
                                         const DocumentSymbolsRequest::Response &response)
{
    m_runningRequests.remove(uri);
    if (Utils::optional<DocumentSymbolsRequest::Response::Error> error = response.error()) {
        if (m_client)
            m_client->log(*error);
    }
    const DocumentSymbolsResult &symbols = response.result().value_or(DocumentSymbolsResult());
    m_cache[uri] = symbols;
    emit gotSymbols(uri, symbols);
}

} // namespace LanguageClient
