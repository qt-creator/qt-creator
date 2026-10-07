// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <mcp/server/mcpserver.h>

#include <utils/result.h>

#include <QJsonObject>

#include <functional>

namespace Debugger::Internal {

class McpSessionState;

using McpReply = std::function<void(Utils::Result<QJsonObject>)>;
using McpHandler = std::function<void(const QJsonObject &args, const McpReply &reply)>;

void registerAsyncMcpTool(const Mcp::Schema::Tool &tool, const McpHandler &handler);

Utils::Result<McpSessionState *> activeSessionState();
Utils::Result<McpSessionState *> pausedSessionState();

int timeoutArgument(const QJsonObject &args, int defaultMs);
QJsonObject timeoutSchema(int defaultMs);
QJsonObject contextSchema();
QString notReadyMessage(const QString &what, McpSessionState *state, bool sessionEnded);

} // namespace Debugger::Internal
