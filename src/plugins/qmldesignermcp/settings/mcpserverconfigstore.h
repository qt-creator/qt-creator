// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mcp/mcphost.h" // for McpServerConfig

#include <QSet>
#include <QString>
#include <QStringList>

namespace QmlDesigner {

/**
 * @brief Persists a single MCP server configuration to/from QSettings.
 *
 * Settings are stored under:
 *   QmlDesigner/mcpServers/<serverName>/
 */
class McpServerConfigStore
{
public:
    // Construct and immediately load from QSettings.
    explicit McpServerConfigStore(const QString &serverName);

    QString serverName() const { return m_serverName; }
    bool isEnabled() const { return m_enabled; }
    McpServerConfig::Transport transport() const { return m_transport; }

    // Stdio fields
    QString command() const { return m_command; }
    QStringList args() const { return m_args; }
    QString workingDir() const { return m_workingDir; }
    QStringList env() const { return m_env; }

    // HTTP fields
    QString url() const { return m_url; }
    QString bearerToken() const { return m_bearerToken; }

    // Persist all fields and update in-memory state.
    void save(bool enabled,
              McpServerConfig::Transport transport,
              const QString &command,
              const QStringList &args,
              const QString &workingDir,
              const QString &url,
              const QString &bearerToken);

    // Build a McpServerConfig ready for McpHost from the stored data.
    McpServerConfig toMcpServerConfig(const QString &resolvedProjectPath = {}) const;

    // Remove this server's entry from QSettings completely.
    static void remove(const QString &serverName);

    // Return the names of all servers saved in QSettings.
    static QStringList savedServerNames();

    // Seed QSettings with the built-in defaults if no servers have been saved yet.
    static void loadDefaults();

private:
    QString m_serverName;
    bool m_enabled = true;
    McpServerConfig::Transport m_transport = McpServerConfig::Stdio;

    // Stdio
    QString m_command;
    QStringList m_args;
    QString m_workingDir;
    QStringList m_env;

    // HTTP
    QString m_url;
    QString m_bearerToken;
};

} // namespace QmlDesigner
