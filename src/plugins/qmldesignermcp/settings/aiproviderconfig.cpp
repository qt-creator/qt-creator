// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiproviderconfig.h"

#include "aiassistantconstants.h"

#include <coreplugin/icore.h>

namespace QmlDesigner {

static Utils::QtcSettings *settings()
{
    return Core::ICore::settings();
}

struct GroupScope
{
    GroupScope(const QString &providerName)
    {
        settings()->beginGroup(Constants::aiAssistantProviderKey);
        settings()->beginGroup(providerName.toUtf8());
    }

    ~GroupScope()
    {
        settings()->endGroup();
        settings()->endGroup();
    }
};

AiProviderConfig::AiProviderConfig(const QString &providerName)
    : m_providerName(providerName)
{
    GroupScope group(m_providerName);

    m_isChecked = settings()->contains("checked") ? settings()->value("checked").toBool() : true;
    m_url = settings()->value("url").toString();
    m_apiKey = settings()->value("apiKey").toString();

    int size = settings()->beginReadArray("models");
    m_models.reserve(size);

    for (int i = 0; i < size; ++i) {
        settings()->setArrayIndex(i);
        m_models << AiModelData{settings()->value("id").toString(),
                                settings()->value("name").toString()};
    }
    settings()->endArray();
}

bool AiProviderConfig::isValid() const
{
    return !m_models.isEmpty() && !m_url.isEmpty() && !m_apiKey.isEmpty();
}

QList<AiModelInfo> AiProviderConfig::allValidModels() const
{
    if (!isValid())
        return {};

    QList<AiModelInfo> infos;
    infos.reserve(m_models.size());

    for (const AiModelData &model : std::as_const(m_models)) {
        if (model.id.isEmpty())
            continue;

        infos.append({
            .provider = providerName(),
            .url = url(),
            .apiKey = apiKey(),
            .modelId = model.id,
            .modelName = model.name
        });
    }

    return infos;
}

void AiProviderConfig::save(bool checked, const QString &url, const QString &apiKey,
                            const QList<AiModelData> &models)
{
    m_isChecked = checked;
    m_url = url;
    m_apiKey = apiKey;
    m_models = models;

    GroupScope group(m_providerName);

    settings()->setValue("checked", m_isChecked);
    settings()->setValue("url", m_url);
    settings()->setValue("apiKey", m_apiKey);

    settings()->remove("models");
    settings()->beginWriteArray("models");

    for (int i = 0; i < m_models.size(); ++i) {
        settings()->setArrayIndex(i);
        settings()->setValue("id", m_models.at(i).id);
        settings()->setValue("name", m_models.at(i).name);
    }

    settings()->endArray();
}

bool AiModelInfo::isValid() const
{
    return !provider.isEmpty() && !url.isEmpty() && !apiKey.isEmpty() && !modelId.isEmpty();
}

}; // namespace QmlDesigner
