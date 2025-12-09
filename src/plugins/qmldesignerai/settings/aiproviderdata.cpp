// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiproviderdata.h"

#include <QHash>

namespace QmlDesigner {

const QStringList AiProviderData::defaultProvidersNames()
{
    static const QStringList names {"Groq", "Gemini", "OpenAI", "Claude"};
    return names;
}

AiProviderData AiProviderData::defaultProviderData(const QString &providerName)
{
    static const QHash<QString, AiProviderData> providers{
        {"Groq", AiProviderData{
            .url = QUrl{"https://api.groq.com/openai/v1/chat/completions"},
            .models = {
                "llama-3.3-70b-versatile",
                "llama-3.1-8b-instant",
                "openai/gpt-oss-120b",
                "openai/gpt-oss-20b",
                "groq/compound",
                "groq/compound-mini",
                "moonshotai/kimi-k2-instruct-0905",
                "qwen/qwen3-32b",
            }
        }},
        {"Gemini", AiProviderData{
            .url = QUrl{"https://generativelanguage.googleapis.com/v1beta/openai/chat/completions"},
            .models = {
                "gemini-2.5-pro",
                "gemini-2.5-flash",
                "gemini-2.5-flash-lite",
                "gemini-2.0-flash",
                "gemini-2.0-flash-lite",
            }
        }},
        {"OpenAI", AiProviderData{
            .url = QUrl{"https://api.openai.com/v1/chat/completions"},
            .models = {
                "gpt-5",
                "gpt-5-mini",
                "gpt-5-nano",
                // "gpt-5-pro", Code execution is not supported https://platform.openai.com/docs/models/gpt-5-pro/?utm_source=chatgpt.com
                "gpt-5-codex",
                "gpt-5.1-codex",
                "gpt-5.1-codex-mini",
                "gpt-4.1",
                "gpt-4.1-mini",
                "gpt-4.1-nano",
                "gpt-4o",
                "gpt-4o-mini",
            }
        }},
        {"Claude", AiProviderData{
            .url = QUrl{"https://api.anthropic.com/v1/messages"},
            .models = {
                "claude-sonnet-4-5-20250929",
                "claude-sonnet-4-20250514",
                "claude-haiku-4-5-20251001",
                "claude-3-5-haiku-20241022",
                "claude-opus-4-20250514",
                "claude-opus-4-1-20250514", // Premium model
            }
        }},
    };

    return providers.value(providerName);
}

} // namespace QmlDesigner
