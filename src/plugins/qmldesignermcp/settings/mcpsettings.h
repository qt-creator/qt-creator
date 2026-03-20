// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mcp/mcphost.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QList>

namespace QmlDesigner {

class AiAssistantView;
class McpServerSettingsWidget;

/**
 * @brief The settings page that lets users view and edit MCP server configurations.
 *
 * Registered under the same category as AiProviderSettings so both pages appear
 * together in Qt Creator's Options dialog.
 *
 * Usage:
 *   // One-time initialisation (e.g. in plugin initialise()):
 *   McpProviderSettings::seedFromJson(loader.getConfigs());
 *
 *   // When building McpHost:
 *   m_mcpHost->addServers(McpProviderSettings::allEnabledConfigs(projectPath));
 */
class McpSettings : public Core::IOptionsPage
{
public:
    explicit McpSettings();
    ~McpSettings() override;

    void setView(AiAssistantView *view);

    /**
     * Return all enabled McpServerConfig entries from QSettings,
     * with @p projectPath substituted for ${PROJECT_PATH} in args.
     */
    static QList<McpServerConfig> allEnabledConfigs(const QString &projectPath = {});

private:
    AiAssistantView *m_view = nullptr;
};

class McpSettingsTab : public Core::IOptionsPageWidget
{
    Q_OBJECT
public:
    explicit McpSettingsTab(AiAssistantView *view);
    ~McpSettingsTab() override;

    void apply() override;
    void cancel() override;

private:
    void addServer(const QString &serverName);
    void removeServer(const QString &serverName);
    void promptAddServer();

    AiAssistantView *m_view = nullptr;
    QList<McpServerSettingsWidget *> m_serverWidgets;
    QWidget *m_serversContainer = nullptr;
};

} // namespace QmlDesigner
