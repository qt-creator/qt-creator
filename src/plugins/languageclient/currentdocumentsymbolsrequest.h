// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <languageserverprotocol/lsptypes.h>

#include <utils/filepath.h>

#include <functional>

#include <QtTaskTree/QTaskTree>

namespace LanguageClient {

/// Maps a URI as the server spells it to the path on the host.
using UriToFilePath = std::function<Utils::FilePath(const QString &uri)>;

class LANGUAGECLIENT_EXPORT CurrentDocumentSymbolsData
{
public:
    Utils::FilePath m_filePath;
    UriToFilePath m_uriToFilePath;
    LanguageServerProtocol::DocumentSymbolRequestResult m_symbols;
};

class LANGUAGECLIENT_EXPORT CurrentDocumentSymbolsRequest : public QObject
{
    Q_OBJECT

public:
    void start();
    bool isRunning() const;
    CurrentDocumentSymbolsData currentDocumentSymbolsData() const { return m_currentDocumentSymbolsData; }

signals:
    void done(QtTaskTree::DoneResult result);

private:
    void clearConnections();

    CurrentDocumentSymbolsData m_currentDocumentSymbolsData;
    QList<QMetaObject::Connection> m_connections;
};

using CurrentDocumentSymbolsRequestTask = QtTaskTree::QCustomTask<CurrentDocumentSymbolsRequest>;

} // namespace LanguageClient
