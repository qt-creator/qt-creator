// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mcp/mcphost.h"

#include <QString>

namespace QmlDesigner {

struct AiProviderData
{
    QString url;
    QStringList models;
};

struct McpServerData
{
    McpServerConfig::Transport transport = McpServerConfig::Stdio;
    QString command;
    QStringList args;
    QString workingDir;
    QString url;
};

class AiDefaultSettings
{
public:
    // AI providers
    static QStringList providerNames();
    static AiProviderData providerData(const QString &name);

    // MCP servers
    static QStringList mcpServerNames();
    static McpServerData mcpServerData(const QString &name);
};

} // namespace QmlDesigner
