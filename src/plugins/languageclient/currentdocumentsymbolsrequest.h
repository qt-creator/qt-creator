// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/lsptypes.h>

#include <solutions/tasking/tasktree.h>

namespace LanguageClient {

class LANGUAGECLIENT_EXPORT CurrentDocumentSymbolsData
{
public:
    Utils::FilePath m_filePath;
    LanguageServerProtocol::DocumentUri::PathMapper m_pathMapper;
    LanguageServerProtocol::DocumentSymbolsResult m_symbols;
};

class LANGUAGECLIENT_EXPORT CurrentDocumentSymbolsRequest : public QObject
{
    Q_OBJECT

public:
    void start();
    bool isRunning() const;
    CurrentDocumentSymbolsData currentDocumentSymbolsData() const { return m_currentDocumentSymbolsData; }

signals:
    void done(bool success);

private:
    void clearConnections();

    CurrentDocumentSymbolsData m_currentDocumentSymbolsData;
    QList<QMetaObject::Connection> m_connections;
};

class LANGUAGECLIENT_EXPORT CurrentDocumentSymbolsRequestTaskAdapter
    : public Tasking::TaskAdapter<CurrentDocumentSymbolsRequest>
{
public:
    CurrentDocumentSymbolsRequestTaskAdapter();
    void start() final;
};

using CurrentDocumentSymbolsRequestTask
    = Tasking::CustomTask<CurrentDocumentSymbolsRequestTaskAdapter>;

} // namespace LanguageClient
