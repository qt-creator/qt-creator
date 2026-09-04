// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <utils/result.h>

#include <QJsonObject>

#include <functional>

namespace LanguageClient {

// The MCP tools answered through language servers. Each function takes the
// tool's arguments as an MCP client would send them and calls the handler
// exactly once, on the GUI thread, with the structured result or an error
// message. They are exported so that tests can drive them without a server.
using ToolResultHandler = std::function<void(const Utils::Result<QJsonObject> &)>;

LANGUAGECLIENT_EXPORT void lspHover(const QJsonObject &args, const ToolResultHandler &handler);
LANGUAGECLIENT_EXPORT void lspCallHierarchy(const QJsonObject &args,
                                            const ToolResultHandler &handler);
LANGUAGECLIENT_EXPORT void lspTypeHierarchy(const QJsonObject &args,
                                            const ToolResultHandler &handler);
LANGUAGECLIENT_EXPORT void lspReferences(const QJsonObject &args,
                                         const ToolResultHandler &handler);

// Closes every document the tools opened for their own purposes. An idle timer
// does this for documents nobody has asked about for a while; tests call it.
LANGUAGECLIENT_EXPORT void closeDocumentsOpenedByMcpTools();

void registerMcpTools();

} // namespace LanguageClient
