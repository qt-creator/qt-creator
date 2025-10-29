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
    };
    return providers;
}

} // namespace QmlDesigner
