// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clientrequesttask.h"

#include <QScopeGuard>

using namespace LanguageServerProtocol;

namespace LanguageClient {

WorkspaceSymbolRequestTaskAdapter::WorkspaceSymbolRequestTaskAdapter()
{
    task()->setResponseCallback([this](const WorkspaceSymbolRequest::Response &response){
        emit done(response.result().has_value());
    });
}

void WorkspaceSymbolRequestTaskAdapter::start()
{
    task()->start();
}

bool WorkspaceSymbolRequestTask::preStartCheck()
{
    if (!ClientRequestTask::preStartCheck())
        return false;

    const std::optional<std::variant<bool, WorkDoneProgressOptions>> capability
        = client()->capabilities().workspaceSymbolProvider();
    if (!capability.has_value())
        return false;
    if (std::holds_alternative<bool>(*capability) && !std::get<bool>(*capability))
        return false;

    return true;
}

} // namespace LanguageClient
