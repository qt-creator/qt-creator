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
                {"gemini-3.5-flash",       "Gemini 3.5 Flash"},
                {"gemini-3.1-pro-preview", "Gemini 3.1 Pro"},
                {"gemini-3.1-flash-lite",  "Gemini 3.1 Flash Lite"},
                {"gemini-3-flash-preview", "Gemini 3 Flash"}
            }
        }},
        {"Claude", AiProviderData{
            .url = "https://api.anthropic.com/v1/messages",
            .models = {
                {"claude-opus-4-8",           "Claude Opus 4.8"},
                {"claude-sonnet-4-6",         "Claude Sonnet 4.6"},
                {"claude-haiku-4-5-20251001", "Claude Haiku 4.5"}
            },
        }},
        {"OpenAI", AiProviderData{
            .url = "https://api.openai.com/v1/responses",
            .models = {
                {"gpt-5.5",      "GPT 5.5"},
                {"gpt-5.4",      "GPT 5.4"},
                {"gpt-5.4-mini", "GPT 5.4 Mini"},
                {"gpt-5.4-nano", "GPT 5.4 Nano"}
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
                           .command    = QML_MCP_SERVER_EXECUTABLE,
                           .args       = {"${PROJECT_PATH}"},
                           .workingDir = {},
                       }}
    };
    return servers.value(name);
}

} // namespace QmlDesigner
