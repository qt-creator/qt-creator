// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMap>
#include <QString>
#include <QUrl>

namespace QmlDesigner {

struct AiProviderData
{
    QUrl url;
    QStringList models; // TODO: add AiModelTraits{modelName, modelId, supportsJsonSchema}

    static const QMap<QString, AiProviderData> defaultProviders();
};

} // namespace QmlDesigner
