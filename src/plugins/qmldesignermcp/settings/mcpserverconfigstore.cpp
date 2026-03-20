// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcpserverconfigstore.h"

#include "aiassistantconstants.h"
#include "aidefaultsettings.h"

#include <coreplugin/icore.h>

namespace QmlDesigner {

struct McpGroupScope
{
    explicit McpGroupScope(const QString &serverName)
    {
        Core::ICore::settings()->beginGroup(Constants::aiAssistantMcpServerKey);
        Core::ICore::settings()->beginGroup(serverName.toLatin1());
    }

    ~McpGroupScope()
    {
        Core::ICore::settings()->endGroup();
        Core::ICore::settings()->endGroup();
    }
};

McpServerConfigStore::McpServerConfigStore(const QString &serverName)
    : m_serverName(serverName)
{
    McpGroupScope g(m_serverName);
    auto *s = Core::ICore::settings();

    m_enabled = s->value("enabled", true).toBool();
    m_transport = static_cast<McpServerConfig::Transport>(s->value("transport",
                                                                   McpServerConfig::Stdio).toInt());
    m_command = s->value("command").toString();
    m_args = s->value("args").toStringList();
    m_workingDir = s->value("workingDir").toString();
    m_url = s->value("url").toString();
    m_bearerToken = s->value("bearerToken").toString();
}

void McpServerConfigStore::save(bool enabled,
                                McpServerConfig::Transport transport,
                                const QString &command,
                                const QStringList &args,
                                const QString &workingDir,
                                const QString &url,
                                const QString &bearerToken)
{
    m_enabled = enabled;
    m_transport = transport;
    m_command = command;
    m_args = args;
    m_workingDir = workingDir;
    m_url = url;
    m_bearerToken = bearerToken;

    McpGroupScope g(m_serverName);
    auto *s = Core::ICore::settings();

    s->setValue("enabled", m_enabled);
    s->setValue("transport", static_cast<int>(m_transport));
    s->setValue("command", m_command);
    s->setValue("args", m_args);
    s->setValue("workingDir", m_workingDir);
    s->setValue("url", m_url);
    s->setValue("bearerToken", m_bearerToken);
}

McpServerConfig McpServerConfigStore::toMcpServerConfig(const QString &resolvedProjectPath) const
{
    McpServerConfig cfg;
    cfg.name = m_serverName;
    cfg.transport = m_transport;
    cfg.command = m_command;
    cfg.workingDir = m_workingDir;
    cfg.url = m_url;
    cfg.bearerToken = m_bearerToken;

    // Resolve ${PROJECT_PATH} placeholder in args
    QStringList resolvedArgs;
    for (const QString &arg : m_args) {
        if (!resolvedProjectPath.isEmpty())
            resolvedArgs.append(QString(arg).replace("${PROJECT_PATH}", resolvedProjectPath));
        else
            resolvedArgs.append(arg);
    }
    cfg.args = resolvedArgs;

    return cfg;
}

void McpServerConfigStore::remove(const QString &serverName)
{
    McpGroupScope g(serverName);
    Core::ICore::settings()->remove("");
}

QStringList McpServerConfigStore::savedServerNames()
{
    Core::ICore::settings()->beginGroup(Constants::aiAssistantMcpServerKey);
    const QStringList names = Core::ICore::settings()->childGroups();
    Core::ICore::settings()->endGroup();

    return names;
}

void McpServerConfigStore::loadDefaults()
{
    if (!savedServerNames().isEmpty())
        return;

    const QStringList names = AiDefaultSettings::mcpServerNames();
    for (const QString &name : names) {
        const McpServerData data = AiDefaultSettings::mcpServerData(name);
        McpServerConfigStore store(name);
        store.save(true, data.transport, data.command, data.args,
                   data.workingDir, data.url, {});
    }
}

} // namespace QmlDesigner
