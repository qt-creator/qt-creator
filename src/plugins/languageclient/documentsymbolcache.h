// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"
#include "languageclientutils.h"

#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/lsptypes.h>

#include <QMap>
#include <QObject>
#include <QSet>
#include <QTimer>

#include <optional>

namespace LanguageClient {

class Client;

class LANGUAGECLIENT_EXPORT DocumentSymbolCache : public QObject
{
    Q_OBJECT
public:
    DocumentSymbolCache(Client *client);

    void requestSymbols(const LanguageServerProtocol::DocumentUri &uri, Schedule schedule);

signals:
    void gotSymbols(const LanguageServerProtocol::DocumentUri &uri,
                    const LanguageServerProtocol::DocumentSymbolsResult &symbols);

private:
    void requestSymbolsImpl();
    void handleResponse(const LanguageServerProtocol::DocumentUri &uri,
                        const LanguageServerProtocol::DocumentSymbolsRequest::Response &response);

    QMap<LanguageServerProtocol::DocumentUri, LanguageServerProtocol::DocumentSymbolsResult> m_cache;
    QMap<LanguageServerProtocol::DocumentUri, LanguageServerProtocol::MessageId> m_runningRequests;
    Client *m_client = nullptr;
    QTimer m_compressionTimer;
    QSet<LanguageServerProtocol::DocumentUri> m_compressedUris;
};

} // namespace LanguageClient
