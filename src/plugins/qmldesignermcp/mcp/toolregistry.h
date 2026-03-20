// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mcpclient.h"

#include <QDateTime>
#include <QHash>
#include <QJsonArray>
#include <QObject>
#include <QSet>

namespace QmlDesigner {

/**
 * @brief Central registry for MCP tools across all servers
 *
 * Automatically discovers and caches tools when servers connect.
 * Provides fast lookup for tool-to-server mapping.
 * Generates tool definitions in provider-specific formats (Claude, OpenAI, etc.)
 */
class ToolRegistry : public QObject
{
    Q_OBJECT

public:
    struct ToolInfo {
        QString serverName;
        McpTool tool;
        QDateTime lastUpdated;
        bool enabled = true;
    };

    explicit ToolRegistry(QObject *parent = nullptr);

    /**
     * @brief Register a server for automatic tool discovery
     *
     * Connects to the client's signals and automatically lists tools
     * when the server becomes ready.
     */
    void registerServer(const QString &serverName, McpClient *client);

    /**
     * @brief Unregister a server and remove all its associated tools
     *
     * Disconnects any signals connected to the client and clears all tools
     * that were discovered for this server.
     */
    void unregisterServer(const QString &serverName, McpClient *client);

    /**
     * @brief Manually add tools to the registry
     */
    void addTools(const QString &serverName, const QList<McpTool> &tools);

    /**
     * @brief Find which server provides a given tool
     * @return Server name, or empty string if tool not found
     */
    QString findServerForTool(const QString &toolName) const;

    /**
     * @brief Get detailed info about a specific tool
     */
    ToolInfo toolInfo(const QString &toolName) const;

    /**
     * @brief Get all tools from a specific server
     */
    QList<ToolInfo> toolsForServer(const QString &serverName) const;

    /**
     * @brief Get all registered tools
     */
    QList<ToolInfo> allTools() const;

    /**
     * @brief Get tool count
     */
    int toolCount() const { return m_tools.size(); }

    /**
     * @brief Check if a tool exists
     */
    bool hasTool(const QString &toolName) const;

    /**
     * @brief Get tools in Claude format (for Claude API)
     *
     * Returns array of objects with: name, description, input_schema
     * Optionally prefixes tool names with server name (server_tool)
     */
    QJsonArray getToolsForClaude(bool prefixWithServer = true) const;

    /**
     * @brief Get tools in OpenAI format
     *
     * Returns array of objects with: type, function.name, function.description, etc.
     */
    QJsonArray getToolsForOpenAI(bool prefixWithServer = true) const;

    /**
     * @brief Clear all tools (useful for reload)
     */
    void clear();

    /**
     * @brief Remove tools for a specific server
     */
    void removeServer(const QString &serverName);

signals:
    /**
     * @brief Emitted when tools are discovered from a server
     */
    void toolsDiscovered(const QString &serverName, int count);

    /**
     * @brief Emitted when the registry is updated
     */
    void registryUpdated();

    /**
     * @brief Emitted when tool discovery fails
     */
    void discoveryFailed(const QString &serverName, const QString &error);

private:
    // tool name -> info (unique tool names across all servers)
    QHash<QString, ToolInfo> m_tools;

    // server name -> set of tool names (for quick server-based queries)
    QHash<QString, QSet<QString>> m_serverTools;

    // Track registered clients to avoid double-registration
    QSet<QString> m_registeredServers;
};

} // namespace QmlDesigner
