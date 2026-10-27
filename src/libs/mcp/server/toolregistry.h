// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcpserver_global.h"

#include "../schemas/schema_2025_11_25.h"
#include "mcpserver.h"

#include <QList>

namespace Mcp {

class MCPSERVER_EXPORT ToolRegistry : public QObject
{
    Q_OBJECT
public:
    static void registerTool(
        const Generated::Schema::_2025_11_25::Tool &tool,
        const Server::ToolInterfaceCallback &callback);
    static void registerTool(
        const Generated::Schema::_2025_11_25::Tool &tool, const Server::ToolCallback &callback);

    static const ToolRegistry &instance();

    static void enableTool(const QString &toolName, bool enabled);
    static bool isToolEnabled(const QString &toolName);
    static QList<Schema::Tool> registeredTools();

    // Invokes a registered tool by name and returns its result. Intended for
    // tests; an asynchronous (ToolInterface) tool that has not answered by the
    // time its callback returns is an error.
    static Utils::Result<Schema::CallToolResult> callToolForTests(
        const QString &name, const Schema::CallToolRequestParams &params);
    // The same for a tool that may answer later. onResult is called at most
    // once, when and wherever the tool answers, and never if the tool drops its
    // ToolInterface without answering.
    static void callToolForTests(
        const QString &name,
        const Schema::CallToolRequestParams &params,
        const ToolInterface::FinishCallback &onResult);

signals:
    void toolRegistered();
    void toolEnabled(const QString &toolName, bool enabled);
};

class MCPSERVER_EXPORT AutoRegisteringServer : public Server, public QObject
{
public:
    AutoRegisteringServer(Generated::Schema::_2025_11_25::Implementation serverInfo);

private:
    std::size_t m_nTools = 0;
};

} // namespace Mcp
