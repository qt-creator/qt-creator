// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "aidefaultsettings.h"

namespace QmlDesigner {

struct AiModelInfo
{
    QString provider;
    QString url;
    QString apiKey;
    QString modelId;
    QString modelName;

    bool isValid() const;
};

class AiProviderConfig
{
public:
    AiProviderConfig(const QString &providerName = {});

    QString providerName() const { return m_providerName; }
    bool isChecked() const { return m_isChecked; }
    QString url() const { return m_url; };
    QString apiKey() const { return m_apiKey; };
    QList<AiModelData> models() const { return m_models; };
    bool isValid() const;
    QList<AiModelInfo> allValidModels() const;

    void save(bool checked, const QString &url, const QString &apiKey, const QList<AiModelData> &models);

private:
    QString m_providerName;
    bool m_isChecked = true;
    QString m_url;
    QList<AiModelData> m_models;
    QString m_apiKey;
};

}; // namespace QmlDesigner
