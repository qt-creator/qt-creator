// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clientrequest.h"

using namespace LanguageServerProtocol;
using namespace Tasking;

namespace LanguageClient {

void ClientWorkspaceSymbolRequestTaskAdapter::operator()(ClientWorkspaceSymbolRequest *task,
                                                         TaskInterface *iface)
{
    task->setResponseCallback([iface](const WorkspaceSymbolRequest::Response &response) {
        iface->reportDone(toDoneResult(response.result().has_value()));
    });
    task->start();
}

bool ClientWorkspaceSymbolRequest::preStartCheck()
{
    if (!ClientRequest::preStartCheck())
        return false;

    const std::optional<std::variant<bool, WorkDoneProgressOptions>> capability
        = client()->capabilities().workspaceSymbolProvider();
    if (!capability.has_value())
        return false;
    const auto boolvalue = std::get_if<bool>(&*capability);
    if (boolvalue && !*boolvalue)
        return false;

    return true;
}

} // namespace LanguageClient
