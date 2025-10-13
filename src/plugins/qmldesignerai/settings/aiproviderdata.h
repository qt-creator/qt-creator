// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QString>
#include <QUrl>

namespace QmlDesigner {

struct AiProviderData
{
    QString name;
    QUrl defaultUrl;

    // TODO: provide it with AiModelTraits{modelName, modelId, supportsJsonSchema}
    QList<QString> models;

    static const QList<AiProviderData> defaultProviders();
    static const AiProviderData findProvider(const QString &providerName);
};

} // namespace QmlDesigner
