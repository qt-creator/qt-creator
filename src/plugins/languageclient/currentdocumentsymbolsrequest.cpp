// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "currentdocumentsymbolsrequest.h"

#include "documentsymbolcache.h"
#include "languageclientmanager.h"

#include <coreplugin/editormanager/editormanager.h>

using namespace Core;
using namespace LanguageServerProtocol;
using namespace TextEditor;
using namespace Utils;

namespace LanguageClient {

void CurrentDocumentSymbolsRequest::start()
{
    QTC_ASSERT(!isRunning(), return);

    m_currentDocumentSymbolsData = {};

    TextDocument *document = TextDocument::currentTextDocument();
    Client *client = LanguageClientManager::clientForDocument(document);
    if (!client) {
        emit done(false);
        return;
    }

    DocumentSymbolCache *symbolCache = client->documentSymbolCache();
    DocumentUri currentUri = client->hostPathToServerUri(document->filePath());
    DocumentUri::PathMapper pathMapper = client->hostPathMapper();

    const auto reportFailure = [this] {
        clearConnections();
        emit done(false);
    };

    const auto updateSymbols = [this, currentUri, pathMapper](const DocumentUri &uri,
                                                              const DocumentSymbolsResult &symbols)
    {
        if (uri != currentUri) // We might get updates for not current editor
            return;

        const FilePath filePath = pathMapper ? currentUri.toFilePath(pathMapper) : FilePath();
        m_currentDocumentSymbolsData = {filePath, pathMapper, symbols};
        clearConnections();
        emit done(true);
    };

    m_connections.append(connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
                                 this, reportFailure));
    m_connections.append(connect(client, &Client::finished, this, reportFailure));
    m_connections.append(connect(document, &IDocument::contentsChanged, this, reportFailure));
    m_connections.append(connect(symbolCache, &DocumentSymbolCache::gotSymbols,
                                 this, updateSymbols));
    symbolCache->requestSymbols(currentUri, Schedule::Now);
}

bool CurrentDocumentSymbolsRequest::isRunning() const
{
    return !m_connections.isEmpty();
}

void CurrentDocumentSymbolsRequest::clearConnections()
{
    for (const QMetaObject::Connection &connection : std::as_const(m_connections))
        disconnect(connection);
    m_connections.clear();
}

CurrentDocumentSymbolsRequestTaskAdapter::CurrentDocumentSymbolsRequestTaskAdapter()
{
    connect(task(), &CurrentDocumentSymbolsRequest::done, this, &TaskInterface::done);
}

void CurrentDocumentSymbolsRequestTaskAdapter::start()
{
    task()->start();
}

} // namespace LanguageClient
