// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiproviderconfig.h"

#include "aiassistantconstants.h"

#include <coreplugin/icore.h>

namespace QmlDesigner {

struct ScopeGroup
{
    ScopeGroup(const Utils::Key &category, Utils::QtcSettings *settings, const QString &providerName)
        : category(category)
        , settings(settings)
        , providerName(providerName)
    {
        if (!category.isEmpty())
            settings->beginGroup(category);
        settings->beginGroup(Constants::aiAssistantProviderConfigKey);
        settings->beginGroup(providerName.toUtf8());
    }

    ~ScopeGroup()
    {
        if (!category.isEmpty())
            settings->endGroup();
        settings->endGroup();
        settings->endGroup();
    }

    const Utils::Key &category;
    Utils::QtcSettings *settings = nullptr;
    const QString &providerName;
};

AiProviderConfig::AiProviderConfig(const QString &providerName)
    : m_providerName(providerName)
{}

bool AiProviderConfig::isValid() const
{
    return !m_modelIds.isEmpty() && m_url.isValid() && !m_apiKey.isEmpty();
}

QList<AiModelInfo> AiProviderConfig::allValidModels() const
{
    if (!isValid())
        return {};

    const QStringList &models = modelIds();
    QList<AiModelInfo> infos;
    infos.reserve(models.size());

    AiModelInfo info{
        .providerName = providerName(),
        .url = url(),
        .apiKey = apiKey(),
    };

    for (const QString &modelId : models) {
        if (modelId.isEmpty())
            continue;

        info.modelId = modelId;
        infos.append(info);
    }

    return infos;
}

void AiProviderConfig::toSettings(const Utils::Key &category, Utils::QtcSettings *settings) const
{
    ScopeGroup group(category, settings, m_providerName);

    settings->setValue("url", m_url);
    settings->setValue("apiKey", m_apiKey);
    settings->setValue("modelIds", m_modelIds);
}

void AiProviderConfig::fromSettings(const Utils::Key &category, Utils::QtcSettings *settings)
{
    ScopeGroup group(category, settings, m_providerName);

    m_url = settings->value("url").toUrl();
    m_apiKey = settings->value("apiKey").toString();
    m_modelIds = settings->value("modelIds").toStringList();
}

AiProviderConfig AiProviderConfig::fromSettings(
    const Utils::Key &category, Utils::QtcSettings *settings, const QString &providerName)
{
    AiProviderConfig config(providerName);
    config.fromSettings(category, settings);
    return config;
}

void AiProviderConfig::setUrl(const QUrl &url)
{
    m_url = url;
}

void AiProviderConfig::setApiKey(const QString &newApiKey)
{
    m_apiKey = newApiKey;
}

void AiProviderConfig::setModelIds(const QStringList &modelIds)
{
    m_modelIds = modelIds;
}

bool AiModelInfo::isValid() const
{
    return !providerName.isEmpty() && !url.isEmpty() && !apiKey.isEmpty() && !modelId.isEmpty();
}

}; // namespace QmlDesigner
