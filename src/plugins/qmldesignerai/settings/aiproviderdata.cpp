// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiproviderdata.h"

namespace QmlDesigner {

// TODO: move to a json file
const QMap<QString, AiProviderData> AiProviderData::defaultProviders()
{
    static const QMap<QString, AiProviderData> providers{
        {"Groq", AiProviderData{
            .url = QUrl{"https://api.groq.com/openai/v1/chat/completions"},
            .models = {
                "llama-3.1-8b-instant",
                "llama-3.3-70b-versatile",
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
                "gemini-2.0-flash",
                "gemini-2.0-flash-lite",
                "gemini-2.5-pro",
                "gemini-2.5-flash",
                "gemini-2.5-flash-lite",
            }
        }},
        {"Open AI", AiProviderData{
            .url = QUrl{"https://api.openai.com/v1/chat/completions"},
            .models = {
                // TODO: commented out models require Responses API which will be implemented soon
                // https://platform.openai.com/docs/guides/migrate-to-responses
                "gpt-5",
                "gpt-5-mini",
                "gpt-5-nano",
                // "gpt-5-pro",
                "gpt-4.1",
                "gpt-4.1-mini",
                "gpt-4.1-nano",
                // "gpt-4o",
                // "gpt-4o-mini",
            }
        }},
        {"Claude", AiProviderData{
            .url = QUrl{"https://api.anthropic.com/v1/messages"},
            .models = {
                "claude-sonnet-4-5",
                "claude-sonnet-4",
                "claude-haiku-4-5",
                "claude-haiku-3-5",
                "claude-opus-4-1",
                "claude-opus-4",
            }
        }},
    };
    return providers;
}

} // namespace QmlDesigner
