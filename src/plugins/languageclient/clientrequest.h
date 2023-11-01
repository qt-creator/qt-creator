// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include "client.h"

#include <languageserverprotocol/lsptypes.h>
#include <languageserverprotocol/lsputils.h>
#include <languageserverprotocol/workspace.h>

#include <solutions/tasking/tasktree.h>

namespace LanguageClient {

template <typename Request>
class LANGUAGECLIENT_EXPORT ClientRequest
{
public:
    virtual ~ClientRequest()
    {
        if (m_id)
            m_client->cancelRequest(*m_id); // In order to not to invoke a response callback anymore
    }

    void setClient(Client *client) { m_client = client; }
    Client *client() const { return m_client; }
    void setParams(const typename Request::Parameters &params) { m_params = params; }

    void start()
    {
        QTC_ASSERT(!isRunning(), return);
        if (!preStartCheck()) {
            m_callback({});
            return;
        }
        Request request(m_params);
        request.setResponseCallback([this](const typename Request::Response &response) {
            m_response = response;
            m_id = {};
            m_callback(response);
        });
        m_id = request.id();
        m_client->sendMessage(request);
    }

    bool isRunning() const { return m_id.has_value(); }
    virtual bool preStartCheck() { return m_client && m_client->reachable() && m_params.isValid(); }

    typename Request::Response response() const { return m_response; }
    void setResponseCallback(typename Request::ResponseCallback callback) { m_callback = callback; }

private:
    Client *m_client = nullptr;
    typename Request::Parameters m_params;
    typename Request::ResponseCallback m_callback;
    std::optional<LanguageServerProtocol::MessageId> m_id;
    typename Request::Response m_response;
};

class LANGUAGECLIENT_EXPORT ClientWorkspaceSymbolRequest
    : public ClientRequest<LanguageServerProtocol::WorkspaceSymbolRequest>
{
public:
    bool preStartCheck() override;
};

class LANGUAGECLIENT_EXPORT ClientWorkspaceSymbolRequestTaskAdapter
    : public Tasking::TaskAdapter<ClientWorkspaceSymbolRequest>
{
public:
    ClientWorkspaceSymbolRequestTaskAdapter();
    void start() final;
};

using ClientWorkspaceSymbolRequestTask
    = Tasking::CustomTask<ClientWorkspaceSymbolRequestTaskAdapter>;

} // namespace LanguageClient
