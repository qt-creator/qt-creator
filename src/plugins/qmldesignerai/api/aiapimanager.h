// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "aiproviderconfig.h"

#include "abstractaiapi.h"

#include <utils/uniqueobjectptr.h>

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)

namespace QmlDesigner {

class AiResponse;

class AiApiManager : public QObject
{
    Q_OBJECT
public:
    using Data = AbstractAiApi::Data;

    explicit AiApiManager();
    ~AiApiManager();

    void request(const Data &requestData, const AiModelInfo &modelInfo);

signals:
    void started(const Data &requestData, const AiModelInfo &modelInfo);
    void finished(const Data &requestData, const AiModelInfo &modelInfo);
    void responseError(const Data &requestData, const AiModelInfo &modelInfo, const QString &error);
    void responseReady(
        const Data &requestData, const AiModelInfo &modelInfo, const AiResponse &response);

private: // functions
    AbstractAiApi *findApi(const AiModelInfo &info) const;

private: // variables
    Utils::UniqueObjectPtr<QNetworkAccessManager> m_networkManager;
    QList<AbstractAiApi *> m_registeredApis;
};

} // namespace QmlDesigner
