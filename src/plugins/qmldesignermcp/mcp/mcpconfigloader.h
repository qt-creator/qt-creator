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
 *  - Read and parse MCP server configuration JSON file.
 *  - Configure tools allow-lists and require-consent sets.
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
     * \brief Load a configuration file.
     * \param path      Path to JSON/JSONC configuration file.
     * \return true on success.
     */
    bool loadFile(const QString &path);

    /*!
     * \brief Load with precedence: global then project.
     *
     * - If both exist, project overrides/augments global server entries.
     * - Merges env and allowed/require lists; project wins on conflicts.
     *
     * \param globalPath      e.g., "~/.qds/mcp.json"
     * \param projectPath     e.g., "./.qds/mcp.json"
     * \return true if at least one file loaded successfully; false otherwise.
     */
    bool loadPrecedence(const QString &globalPath, const QString &projectPath);

    QList<McpServerConfig> getConfigs() const;

private:
    QJsonObject loadJsonObject(const QString &path, QString *errOut = nullptr) const;
    QJsonValue resolveStringTokens(const QJsonValue &value) const;

    QJsonObject m_serversObj;
    QMap<QString, QString> m_resolveMap;
};

} // namespace QmlDesigner
