// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "toolregistry.h"

#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QJsonObject>

namespace QmlDesigner {

ToolRegistry::ToolRegistry(QObject *parent)
    : QObject(parent)
{}

void ToolRegistry::registerServer(const QString &serverName, McpClient *client)
{
    QTC_ASSERT(client, return);

    if (m_registeredServers.contains(serverName)) {
        qWarning() << "ToolRegistry: Server" << serverName << "already registered";
        return;
    }

    if (!client->isRunning()) {
        qWarning() << "Client " << client->clientName() << " not running";
        return;
    }

    m_registeredServers.insert(serverName);

    // Request tool list
    qint64 requestId = client->listTools();
    if (requestId < 0) {
        emit discoveryFailed(serverName, "Failed to request tool list");
        return;
    }

    // Connect to response (one-shot)
    connect(client, &McpClient::toolsListed, this,
            [this, serverName](const QList<McpTool> &tools, qint64) {
                addTools(serverName, tools);
                emit toolsDiscovered(serverName, tools.size());
            }, Qt::SingleShotConnection);
}

void ToolRegistry::unregisterServer(const QString &serverName, McpClient *client)
{
    QTC_ASSERT(client, return);

    client->disconnect(this);
    removeServer(serverName);
}

void ToolRegistry::addTools(const QString &serverName, const QList<McpTool> &tools)
{
    for (const McpTool &tool : tools) {
        ToolInfo info{
            .serverName = serverName,
            .tool = tool,
            .lastUpdated = QDateTime::currentDateTime(),
            .enabled = true
        };

        m_tools[tool.name] = info;
        m_serverTools[serverName].insert(tool.name);
    }

    emit registryUpdated();
}

QString ToolRegistry::findServerForTool(const QString &toolName) const
{
    // Check if tool exists in registry
    if (m_tools.contains(toolName))
        return m_tools.value(toolName).serverName;

    // If tool name has server prefix (server_tool), extract it
    if (toolName.contains('_')) {
        QStringList parts = toolName.split('_');
        if (parts.size() == 2) {
            QString serverName = parts.first();
            QString actualToolName = parts.last();

            // Verify this server actually has this tool
            if (m_serverTools.contains(serverName) &&
                m_serverTools[serverName].contains(actualToolName)) {
                return serverName;
            }
        }
    }

    return {}; // Not found
}

ToolRegistry::ToolInfo ToolRegistry::toolInfo(const QString &toolName) const
{
    return m_tools.value(toolName);
}

QList<ToolRegistry::ToolInfo> ToolRegistry::toolsForServer(const QString &serverName) const
{
    QList<ToolInfo> result;
    const QSet<QString> &toolNames = m_serverTools.value(serverName);

    for (const QString &toolName : toolNames) {
        if (m_tools.contains(toolName))
            result.append(m_tools.value(toolName));
    }

    return result;
}

QList<ToolRegistry::ToolInfo> ToolRegistry::allTools() const
{
    return m_tools.values();
}

bool ToolRegistry::hasTool(const QString &toolName) const
{
    return m_tools.contains(toolName);
}

QJsonArray ToolRegistry::getToolsForClaude(bool prefixWithServer) const
{
    QJsonArray tools;

    for (auto it = m_tools.constBegin(); it != m_tools.constEnd(); ++it) {
        const ToolInfo &info = it.value();

        if (!info.enabled)
            continue;

        QString toolName = prefixWithServer ? QString("%1__%2").arg(info.serverName, info.tool.name)
                                            : info.tool.name;

        QJsonObject tool{
            {"name", toolName},
            {"description", info.tool.description},
            {"input_schema", info.tool.inputSchema}
        };

        tools.append(tool);
    }

    return tools;
}

QJsonArray ToolRegistry::getToolsForOpenAI(bool prefixWithServer) const
{
    QJsonArray tools;

    for (auto it = m_tools.constBegin(); it != m_tools.constEnd(); ++it) {
        const ToolInfo &info = it.value();

        if (!info.enabled)
            continue;

        QString toolName = prefixWithServer
            ? QString("%1__%2").arg(info.serverName, info.tool.name)
            : info.tool.name;

        QJsonObject tool{
            {"type", "function"},
            {"name", toolName},
            {"description", info.tool.description},
            {"parameters", info.tool.inputSchema},
        };

        tools.append(tool);
    }

    return tools;
}

void ToolRegistry::clear()
{
    m_tools.clear();
    m_serverTools.clear();
    emit registryUpdated();
}

void ToolRegistry::removeServer(const QString &serverName)
{
    // Get all tool names for this server
    const QSet<QString> toolNames = m_serverTools.value(serverName);

    // Remove each tool
    for (const QString &toolName : toolNames)
        m_tools.remove(toolName);

    // Remove server entry
    m_serverTools.remove(serverName);
    m_registeredServers.remove(serverName);

    emit registryUpdated();
}

} // namespace QmlDesigner
