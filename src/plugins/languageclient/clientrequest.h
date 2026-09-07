// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include "client.h"

#include <QtTaskTree/QTaskTree>

namespace LanguageClient {

/// A workspace symbol request as a task, which cancels the request when it goes away.
class LANGUAGECLIENT_EXPORT ClientWorkspaceSymbolRequest
{
public:
    ~ClientWorkspaceSymbolRequest();

    void setClient(Client *client) { m_client = client; }
    Client *client() const { return m_client; }
    void setParams(const LanguageServerProtocol::WorkspaceSymbolParams &params)
    { m_params = params; }
    /// The clangd extension capping the number of symbols the server looks for.
    void setLimit(int limit) { m_limit = limit; }
    using ResultHandler = std::function<void(
        const Utils::Result<LanguageServerProtocol::WorkspaceSymbolRequestResult> &)>;
    void setResultHandler(const ResultHandler &handler) { m_handler = handler; }

    void start();
    bool isRunning() const { return m_id.has_value(); }
    bool supported() const;

    const Utils::Result<LanguageServerProtocol::WorkspaceSymbolRequestResult> &result() const
    { return m_result; }

private:
    Client *m_client = nullptr;
    LanguageServerProtocol::WorkspaceSymbolParams m_params;
    ResultHandler m_handler;
    std::optional<LanguageServerProtocol::MessageId> m_id;
    int m_limit = -1;
    Utils::Result<LanguageServerProtocol::WorkspaceSymbolRequestResult> m_result
        = Utils::ResultError(QString());
};

class ClientWorkspaceSymbolRequestTaskAdapter final
{
public:
    LANGUAGECLIENT_EXPORT void operator()(ClientWorkspaceSymbolRequest *task,
                                          QtTaskTree::QTaskInterface *iface);
};

using ClientWorkspaceSymbolRequestTask
    = QtTaskTree::QCustomTask<ClientWorkspaceSymbolRequest, ClientWorkspaceSymbolRequestTaskAdapter>;

} // namespace LanguageClient
