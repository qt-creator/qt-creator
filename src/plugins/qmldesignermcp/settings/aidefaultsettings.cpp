// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "aidefaultsettings.h"

#include <QHash>

namespace QmlDesigner {

QStringList AiDefaultSettings::providerNames()
{
    static const QStringList names{"Claude", "OpenAI", "Gemini"};
    return names;
}

AiProviderData AiDefaultSettings::providerData(const QString &name)
{
    static const QHash<QString, AiProviderData> providers{
        {"Gemini", AiProviderData{
            .url = "https://generativelanguage.googleapis.com/v1beta/models/{modelId}:generateContent",
            .models = {
                "gemini-3.1-pro-preview",
                "gemini-3-flash-preview",
                "gemini-3.1-flash-lite-preview",
                "gemini-2.5-pro",
                "gemini-2.5-flash",
                "gemini-2.5-flash-lite",
            }
        }},
        {"Claude", AiProviderData{
                       .url = "https://api.anthropic.com/v1/messages",
                       .models = {
                           "claude-opus-4-6",
                           "claude-sonnet-4-6",
                           "claude-haiku-4-5-20251001",
                       },
                   }},
        {"OpenAI", AiProviderData{
                       .url = "https://api.openai.com/v1/chat/completions",
                       .models = {
                           "gpt-5.4",
                           "gpt-5.4-mini",
                           "gpt-5.4-nano",
                       },
                   }},
    };
    return providers.value(name);
}

QStringList AiDefaultSettings::mcpServerNames()
{
    static const QStringList names{"qml_server"};
    return names;
}

McpServerData AiDefaultSettings::mcpServerData(const QString &name)
{
    static const QHash<QString, McpServerData> servers{
        {"qml_server", McpServerData{
                           .transport  = McpServerConfig::Stdio,
                           .command    = "qml_mcp_server.exe",
                           .args       = {"${PROJECT_PATH}"},
                           .workingDir = "../share/qtcreator/mcp",
                       }}
    };
    return servers.value(name);
}

} // namespace QmlDesigner
