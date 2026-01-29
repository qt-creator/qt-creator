// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mcphost.h"

#include <QMap>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QJsonObject)
QT_FORWARD_DECLARE_CLASS(QJsonValue)

namespace QmlDesigner {

/*!
 * \class McpConfigLoader
 * \brief Loader for MCP server configuration files (.qds/mcp.json).
 *
 * Responsibilities:
 *  - Read and parse MCP configuration JSON (supports comments via jsonc stripping).
 *  - Build McpServerConfig entries for McpHost.
 *  - Apply precedence: global (~/.qds/mcp.json) + project (./.qds/mcp.json).
 *  - Configure allow-lists and expose require-consent sets per server.
 */
class McpConfigLoader
{
public:
    explicit McpConfigLoader();

    /*!
     * \brief The map helps resolve stringtokens of the json configuration files.
     * Each key in the map represents a token in the JSON data, and its associated
     * value provides the resolved string that should replace it.
     * \param map A QMap where each key is a token name and the value is the
     * resolved string that should be substituted during configuration loading.
     */
    void setResolveMap(const QMap<QString, QString> &map);

    /*!
     * \brief Load a single configuration file and apply it to the host.
     * \param host      Target host to configure.
     * \param path      Path to JSON/JSONC configuration file.
     * \return true on success.
     */
    bool loadFile(McpHost *host, const QString &path);

    /*!
     * \brief Load with precedence: global then project.
     *
     * - If both exist, project overrides/augments global server entries.
     * - Merges env and allowed/require lists; project wins on conflicts.
     *
     * \param host            Target host to configure.
     * \param globalPath      e.g., "~/.qds/mcp.json"
     * \param projectPath     e.g., "./.qds/mcp.json"
     * \return true if at least one file loaded successfully; false otherwise.
     */
    bool loadPrecedence(McpHost *host, const QString &globalPath, const QString &projectPath);

private:
    void applyServers(McpHost *host, const QJsonObject &serversObj);
    QJsonObject loadJsonObject(const QString &path, QString *errOut = nullptr) const;
    QJsonValue resolveStringTokens(const QJsonValue &value) const;

    QMap<QString, QString> m_resolveMap;
};

} // namespace QmlDesigner
