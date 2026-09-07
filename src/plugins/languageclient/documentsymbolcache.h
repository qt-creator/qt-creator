// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"
#include "languageclientutils.h"

#include <languageserverprotocol/lspjsonrpc.h>

#include <QMap>
#include <QObject>
#include <QSet>
#include <QTimer>

namespace LanguageClient {

class Client;

class LANGUAGECLIENT_EXPORT DocumentSymbolCache : public QObject
{
    Q_OBJECT
public:
    DocumentSymbolCache(Client *client);

    void requestSymbols(const QString &uri, Schedule schedule);

signals:
    void gotSymbols(
        const QString &uri, const LanguageServerProtocol::DocumentSymbolRequestResult &symbols);

private:
    void requestSymbolsImpl();
    void handleResponse(
        const QString &uri,
        const Utils::Result<LanguageServerProtocol::DocumentSymbolRequestResult> &result);

    QMap<QString, LanguageServerProtocol::DocumentSymbolRequestResult> m_cache;
    QMap<QString, LanguageServerProtocol::MessageId> m_runningRequests;
    Client *m_client = nullptr;
    QTimer m_compressionTimer;
    QSet<QString> m_compressedUris;
};

} // namespace LanguageClient
