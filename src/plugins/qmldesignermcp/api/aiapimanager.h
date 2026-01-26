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
    explicit AiApiManager();
    ~AiApiManager();

    void request(const AbstractAiApi::Data &requestData, const AiModelInfo &modelInfo);

signals:
    void started(const QmlDesigner::AbstractAiApi::Data &requestData,
                 const QmlDesigner::AiModelInfo &modelInfo);
    void finished(const QmlDesigner::AbstractAiApi::Data &requestData,
                  const QmlDesigner::AiModelInfo &modelInfo);
    void responseError(const QmlDesigner::AbstractAiApi::Data &requestData,
                       const QmlDesigner::AiModelInfo &modelInfo,
                       const QString &error);
    void responseReady(const QmlDesigner::AbstractAiApi::Data &requestData,
                       const QmlDesigner::AiModelInfo &modelInfo,
                       const QmlDesigner::AiResponse &response);

private: // functions
    AbstractAiApi *findApi(const AiModelInfo &info) const;

private: // variables
    Utils::UniqueObjectPtr<QNetworkAccessManager> m_networkManager;
    QList<AbstractAiApi *> m_registeredApis;
};

} // namespace QmlDesigner
