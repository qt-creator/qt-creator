// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "currentdocumentsymbolsrequest.h"

#include "documentsymbolcache.h"
#include "languageclientmanager.h"

#include <coreplugin/editormanager/editormanager.h>

using namespace Core;
using namespace LanguageServerProtocol;
using namespace QtTaskTree;
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
        emit done(DoneResult::Error);
        return;
    }

    DocumentSymbolCache *symbolCache = client->documentSymbolCache();
    const QString currentUri = client->uriFor(document->filePath());
    const UriToFilePath uriToFilePath = [client = QPointer<Client>(client)](const QString &uri) {
        return client ? client->filePathFor(uri) : FilePath();
    };

    const auto reportFailure = [this] {
        clearConnections();
        emit done(DoneResult::Error);
    };

    const auto updateSymbols =
        [this,
         currentUri,
         uriToFilePath](const QString &uri, const DocumentSymbolRequestResult &symbols) {
            if (uri != currentUri) // We might get updates for not current editor
                return;

            m_currentDocumentSymbolsData = {uriToFilePath(currentUri), uriToFilePath, symbols};
            clearConnections();
            emit done(DoneResult::Success);
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

} // namespace LanguageClient
