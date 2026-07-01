// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "request.h"

#include <utils/appinfo.h>
#include <utils/mimeconstants.h>

#include <QtTaskTree/QNetworkReplyWrapper>

using namespace QtTaskTree;

namespace CompilerExplorer::Api {

static QByteArray userAgent()
{
    static const QByteArray agent = QString("%1/%2 (%3)")
                                        .arg(QCoreApplication::applicationName())
                                        .arg(QCoreApplication::applicationVersion())
                                        .arg(Utils::appInfo().author)
                                        .toUtf8();
    return agent;
}

ExecutableItem jsonRequestTask(
    QNetworkAccessManager *networkManager,
    const QUrl &url,
    const std::function<void(const Utils::Result<QJsonDocument> &)> &onResponse,
    QNetworkAccessManager::Operation op,
    const QByteArray &payload)
{
    const auto onSetup = [=](QNetworkReplyWrapper &task) {
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, Utils::Constants::JSON_MIMETYPE);
        req.setRawHeader("Accept", Utils::Constants::JSON_MIMETYPE);
        req.setRawHeader("User-Agent", userAgent());
        task.setNetworkAccessManager(networkManager);
        task.setRequest(req);
        task.setOperation(op);
        if (!payload.isEmpty())
            task.setData(payload);
        qCDebug(apiLog).noquote() << "Requesting" << toString(op) << url.toString();
    };
    const auto onDone = [=](const QNetworkReplyWrapper &task, DoneWith result) {
        QNetworkReply *reply = task.reply();
        if (result != DoneWith::Success) {
            const QString message
                = reply->error() == QNetworkReply::ContentNotFoundError
                      ? QCoreApplication::translate("QtC::CompilerExplorer", "Not found")
                      : reply->errorString();
            qCWarning(apiLog).noquote() << "Request failed:" << message;
            onResponse(Utils::ResultError(message));
            return false;
        }
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            onResponse(Utils::ResultError(parseError.errorString()));
            return false;
        }
        onResponse(doc);
        return true;
    };
    return QNetworkReplyWrapperTask(onSetup, onDone);
}

} // namespace CompilerExplorer::Api
