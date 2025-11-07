// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiproviderconfig.h"

#include "aiassistantconstants.h"

#include <coreplugin/icore.h>

namespace QmlDesigner {

struct GroupScope
{
    GroupScope(const QString &providerName)
    {
        Core::ICore::settings()->beginGroup(Constants::aiAssistantProviderKey);
        Core::ICore::settings()->beginGroup(providerName.toUtf8());
    }

    ~GroupScope()
    {
        Core::ICore::settings()->endGroup();
        Core::ICore::settings()->endGroup();
    }
};

AiProviderConfig::AiProviderConfig(const QString &providerName)
    : m_providerName(providerName)
{
    GroupScope group(m_providerName);

    m_isChecked = Core::ICore::settings()->contains("checked")
                      ? Core::ICore::settings()->value("checked").toBool() : true;
    m_url = Core::ICore::settings()->value("url").toUrl();
    m_apiKey = Core::ICore::settings()->value("apiKey").toString();
    m_modelIds = Core::ICore::settings()->value("modelIds").toStringList();
}

bool AiProviderConfig::isValid() const
{
    return !m_modelIds.isEmpty() && m_url.isValid() && !m_apiKey.isEmpty();
}

QList<AiModelInfo> AiProviderConfig::allValidModels() const
{
    if (!isValid())
        return {};

    const QStringList models = modelIds();
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

void AiProviderConfig::save(bool checked, const QString &url, const QString &apiKey,
                            const QStringList &modelIds)
{
    m_isChecked = checked;
    m_url = url;
    m_apiKey = apiKey;
    m_modelIds = modelIds;

    GroupScope group(m_providerName);
    Core::ICore::settings()->setValue("checked", m_isChecked);
    Core::ICore::settings()->setValue("url", m_url.toString());
    Core::ICore::settings()->setValue("apiKey", m_apiKey);
    Core::ICore::settings()->setValue("modelIds", m_modelIds);
}

bool AiModelInfo::isValid() const
{
    return !providerName.isEmpty() && !url.isEmpty() && !apiKey.isEmpty() && !modelId.isEmpty();
}

}; // namespace QmlDesigner
