// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QUrl>

namespace QmlDesigner {

struct AiModelInfo
{
    QString providerName;
    QUrl url;
    QString apiKey;
    QString modelId;

    bool isValid() const;
};

class AiProviderConfig
{
public:
    AiProviderConfig(const QString &providerName = {});

    QString providerName() const { return m_providerName; }
    QUrl url() const { return m_url; };
    QString apiKey() const { return m_apiKey; };
    QStringList modelIds() const { return m_modelIds; };
    bool isValid() const;
    QList<AiModelInfo> allValidModels() const;

    void save(const QString &url, const QString &apiKey, const QStringList &modelIds);

private:
    QString m_providerName;
    QUrl m_url;
    QStringList m_modelIds;
    QString m_apiKey;
};

}; // namespace QmlDesigner
