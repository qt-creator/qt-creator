// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiapimanager.h"

#include "airesponse.h"
#include "claudemessagesapi.h"
#include "openaicompletionsapi.h"
#include "openairesponsesapi.h"

#include <utils/qtcassert.h>

#include <QNetworkReply>

namespace QmlDesigner {

AiApiManager::AiApiManager()
    : m_networkManager(Utils::makeUniqueObjectPtr<QNetworkAccessManager>())
{
    m_registeredApis.append(new OpenAiCompletionsApi()); // The first one is the default one
    m_registeredApis.append(new OpenAiResponsesApi());
    m_registeredApis.append(new ClaudeMessagesApi());
}

AiApiManager::~AiApiManager()
{
    qDeleteAll(m_registeredApis);
}

void AiApiManager::request(const AbstractAiApi::Data &requestData, const AiModelInfo &modelInfo)
{
    AbstractAiApi *api = findApi(modelInfo);
    QTC_ASSERT(api, return);

    QNetworkRequest networkRequest(modelInfo.url);
    api->setRequestHeader(&networkRequest, modelInfo);
    const QByteArray content = api->createContent(requestData, modelInfo);
    QNetworkReply *reply = m_networkManager->post(networkRequest, content);
    emit started(requestData, modelInfo);
    QObject::connect(reply, &QNetworkReply::finished, reply,
                     [reply, api = api, requestData, modelInfo, this] {
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

    if (!m_registeredApis.isEmpty())
        return m_registeredApis.first();

    return nullptr;
}

} // namespace QmlDesigner
