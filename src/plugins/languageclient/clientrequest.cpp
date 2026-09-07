// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clientrequest.h"

using namespace QtTaskTree;

namespace LanguageClient {

ClientWorkspaceSymbolRequest::~ClientWorkspaceSymbolRequest()
{
    if (m_id)
        m_client->cancelRequest(*m_id); // In order to not to invoke a response callback anymore
}

bool ClientWorkspaceSymbolRequest::supported() const
{
    if (!m_client || !m_client->reachable())
        return false;
    const std::optional<LanguageServerProtocol::ServerCapabilitiesWorkspaceSymbolProvider> capability
        = m_client->capabilities().workspaceSymbolProvider();
    if (!capability.has_value())
        return false;
    const auto enabled = std::get_if<bool>(&*capability);
    return !enabled || *enabled;
}

void ClientWorkspaceSymbolRequest::start()
{
    QTC_ASSERT(!isRunning(), return);
    if (!supported()) {
        if (m_handler)
            m_handler(m_result);
        return;
    }
    QJsonObject params = toJson(m_params);
    if (m_limit > 0)
        params.insert("limit", m_limit);
    m_id = m_client->sendRawRequest(
        params,
        LanguageServerProtocol::WorkspaceSymbolRequest::method,
        [this](const QJsonObject &response) {
            m_result
                = LanguageServerProtocol::result<LanguageServerProtocol::WorkspaceSymbolRequest>(
                    response);
            m_id = {};
            if (m_handler)
                m_handler(m_result);
        });
}

void ClientWorkspaceSymbolRequestTaskAdapter::operator()(ClientWorkspaceSymbolRequest *task,
                                                         QTaskInterface *iface)
{
    task->setResultHandler(
        [iface](const Utils::Result<LanguageServerProtocol::WorkspaceSymbolRequestResult> &result) {
            iface->reportDone(toDoneResult(result.has_value()));
        });
    task->start();
}

} // namespace LanguageClient
