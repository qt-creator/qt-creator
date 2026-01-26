// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "aiproviderconfig.h"

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QNetworkRequest)

namespace QmlDesigner {

class AiResponse;

class AbstractAiApi
{
public:
    struct Data
    {
        QString manifest;
        QString qml;
        QString userPrompt;
        QUrl attachedImage;
    };

    virtual ~AbstractAiApi() = default;
    virtual void setRequestHeader(QNetworkRequest *networkRequest, const AiModelInfo &modelInfo) = 0;
    virtual QByteArray createContent(const Data &data, const AiModelInfo &modelInfo) = 0;
    virtual AiResponse interpretResponse(const QByteArray &response) = 0;
    virtual bool accepts(const AiModelInfo &info) = 0;
};

} // namespace QmlDesigner
