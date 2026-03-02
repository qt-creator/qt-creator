// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiproviderdata.h"

#include <QHash>

namespace QmlDesigner {

const QStringList AiProviderData::defaultProvidersNames()
{
    // TODO: suppor OpenAI and Gemini
    static const QStringList names {"Claude", "OpenAI", /*"Gemini"*/};
    return names;
}

AiProviderData AiProviderData::defaultProviderData(const QString &providerName)
{
    static const QHash<QString, AiProviderData> providers{
        // TODO: support Gemini
        // {"Gemini", AiProviderData{
        //     .url = QUrl{"https://generativelanguage.googleapis.com/v1beta/openai/chat/completions"},
        //     .models = {
        //         "gemini-2.5-pro",
        //         "gemini-2.5-flash",
        //         "gemini-2.5-flash-lite",
        //         "gemini-2.0-flash",
        //         "gemini-2.0-flash-lite",
        //     }
        // }},
        {"OpenAI", AiProviderData{
            .url = QUrl{"https://api.openai.com/v1/chat/completions"},
            .models = {
                "gpt-5.2",
                "gpt-5.2-pro",
                "gpt-5.2-codex",
                "gpt-5-mini",
                "gpt-5-nano",
            }
        }},
        {"Claude", AiProviderData{
            .url = QUrl{"https://api.anthropic.com/v1/messages"},
            .models = {
                "claude-opus-4-6", // Premium model
                "claude-sonnet-4-6",
                "claude-haiku-4-5-20251001",
            }
        }},
    };

    return providers.value(providerName);
}

} // namespace QmlDesigner
