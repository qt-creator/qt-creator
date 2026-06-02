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
    static QList<Schema::Tool> registeredTools();

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
