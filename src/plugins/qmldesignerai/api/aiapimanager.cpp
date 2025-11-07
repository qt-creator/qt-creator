// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiapimanager.h"

#include "airesponse.h"
#include "openaicompletionsapi.h"

#include <QNetworkReply>

namespace QmlDesigner {

AiApiManager::AiApiManager()
    : m_networkManager(Utils::makeUniqueObjectPtr<QNetworkAccessManager>())
{
    m_registeredApis.append(new OpenAiCompletionsApi());
}

AiApiManager::~AiApiManager()
{
    qDeleteAll(m_registeredApis);
}

void AiApiManager::request(const Data &requestData, const AiModelInfo &modelInfo)
{
    AbstractAiApi *api = findApi(modelInfo);
    if (!api) {
        qWarning() << Q_FUNC_INFO << "No valid api found";
        return;
    }

    QNetworkRequest networkRequest(modelInfo.url);
    api->setRequestHeader(&networkRequest, modelInfo);
    const QByteArray content = api->createContent(requestData, modelInfo);
    QNetworkReply *reply = m_networkManager->post(networkRequest, content);
    emit started(requestData, modelInfo);
    QObject::connect(
        reply, &QNetworkReply::finished, reply, [reply, api = api, requestData, modelInfo, this] {
            if (reply->error() != QNetworkReply::NoError)
                emit responseError(requestData, modelInfo, reply->errorString());
            else
                emit responseReady(requestData, modelInfo, api->interpretResponse(reply->readAll()));

            emit finished(requestData, modelInfo);
            reply->deleteLater();
        });
}

AbstractAiApi *AiApiManager::findApi(const AiModelInfo &info) const
{
    for (AbstractAiApi *api : std::as_const(m_registeredApis)) {
        if (api->accepts(info))
            return api;
    }
    return nullptr;
}

} // namespace QmlDesigner
