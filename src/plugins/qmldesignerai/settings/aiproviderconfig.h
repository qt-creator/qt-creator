// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QUrl>

namespace Utils {
class Key;
class QtcSettings;
} // namespace Utils

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

    void toSettings(const Utils::Key &category, Utils::QtcSettings *settings) const;
    void fromSettings(const Utils::Key &category, Utils::QtcSettings *settings);

    static AiProviderConfig fromSettings(
        const Utils::Key &category, Utils::QtcSettings *settings, const QString &providerName);

    void setUrl(const QUrl &url);
    void setApiKey(const QString &newApiKey);
    void setModelIds(const QStringList &modelIds);

private:
    QString m_providerName;
    QUrl m_url;
    QStringList m_modelIds;
    QString m_apiKey;
};

}; // namespace QmlDesigner
