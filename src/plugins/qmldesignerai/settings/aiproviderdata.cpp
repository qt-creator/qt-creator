// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiproviderdata.h"

namespace QmlDesigner {

// TODO: move to a json file
const QList<AiProviderData> AiProviderData::defaultProviders()
{
    static const QList<AiProviderData> providers{
        AiProviderData{
            .name = "Groq",
            .url = QUrl{"https://api.groq.com/openai/v1/chat/completions"},
            .models = {
                "llama-3.1-8b-instant",
                "llama-3.3-70b-versatile",
                "openai/gpt-oss-120b",
                "openai/gpt-oss-20b",
                "groq/compound",
                "groq/compound-mini",
                "moonshotai/kimi-k2-instruct-0905",
                "qwen/qwen3-32b"
            },
        },
    };
    return providers;
}

const AiProviderData AiProviderData::findProvider(const QString &providerName)
{
    const QList<AiProviderData> &providers = defaultProviders();
    if (auto itr = std::ranges::find(providers, providerName, &AiProviderData::name);
        itr != providers.end()) {
        return *itr;
    }
    return {};
}

} // namespace QmlDesigner
