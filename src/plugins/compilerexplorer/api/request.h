// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QCoreApplication>
#include <QFuture>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPromise>
#include <QString>

#include <utils/appinfo.h>

static Q_LOGGING_CATEGORY(apiLog, "qtc.compilerexplorer.api", QtWarningMsg);

namespace CompilerExplorer::Api {

static inline QString toString(QNetworkAccessManager::Operation op)
{
    switch (op) {
    case QNetworkAccessManager::GetOperation:
        return QStringLiteral("GET");
    case QNetworkAccessManager::PostOperation:
        return QStringLiteral("POST");
    case QNetworkAccessManager::PutOperation:
        return QStringLiteral("PUT");
    case QNetworkAccessManager::DeleteOperation:
        return QStringLiteral("DELETE");
    case QNetworkAccessManager::HeadOperation:
        return QStringLiteral("HEAD");
    case QNetworkAccessManager::CustomOperation:
        return QStringLiteral("CUSTOM");
    case QNetworkAccessManager::UnknownOperation:
        break;
    }

    return "<unknown>";
}

static int debugRequestId = 0;

template<typename Result>
QFuture<Result> request(
    QNetworkAccessManager *networkManager,
    QNetworkRequest &req,
    std::function<void(const QByteArray &, QSharedPointer<QPromise<Result>>)> callback,
    QNetworkAccessManager::Operation op = QNetworkAccessManager::GetOperation,
    const QByteArray &payload = {})
{
    static const QByteArray userAgent = QString("%1/%2 (%3)")
                                            .arg(QCoreApplication::applicationName())
                                            .arg(QCoreApplication::applicationVersion())
                                            .arg(Utils::appInfo().author)
                                            .toUtf8();
    req.setRawHeader("User-Agent", userAgent);

    QSharedPointer<QPromise<Result>> p(new QPromise<Result>);
    p->start();

    // For logging purposes only
    debugRequestId += 1;

    const auto reqId = [r = debugRequestId] { return QString("[%1]").arg(r); };

    if (payload.isEmpty())
        qCDebug(apiLog).noquote() << reqId() << "Requesting" << toString(op)
                                  << req.url().toString();
    else
        qCDebug(apiLog).noquote() << reqId() << "Requesting" << toString(op) << req.url().toString()
                                  << "with payload:" << QString::fromUtf8(payload);

    QNetworkReply *reply = nullptr;

    switch (op) {
    case QNetworkAccessManager::GetOperation:
        reply = networkManager->get(req);
        break;
    case QNetworkAccessManager::PostOperation:
        reply = networkManager->post(req, payload);
        break;
    case QNetworkAccessManager::PutOperation:
        reply = networkManager->put(req, payload);
        break;
    case QNetworkAccessManager::DeleteOperation:
        reply = networkManager->deleteResource(req);
        break;
    default:
        return QtFuture::makeExceptionalFuture<Result>(
            std::make_exception_ptr(std::runtime_error("Unsupported operation")));
    }

    QObject::connect(reply, &QNetworkReply::finished, [p, reply, callback, reqId] {
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(apiLog).noquote()
                << reqId() << "Request failed:" << reply->error() << reply->errorString();

            QString errorMessage;
            if (reply->error() == QNetworkReply::ContentNotFoundError) {
                errorMessage = QCoreApplication::translate("QtC::CompilerExplorer", "Not found");
            } else
                errorMessage = reply->errorString();

            p->setException(std::make_exception_ptr(std::runtime_error(errorMessage.toUtf8())));
            reply->deleteLater();
            p->finish();
            return;
        }
        QByteArray data = reply->readAll();
        qCDebug(apiLog).noquote() << reqId() << "Request finished:" << data;

        callback(data, p);

        reply->deleteLater();
        p->finish();
    });

    return p->future();
}

template<typename Result>
QFuture<Result> jsonRequest(QNetworkAccessManager *networkManager,
                            const QUrl &url,
                            std::function<Result(QJsonDocument)> callback,
                            QNetworkAccessManager::Operation op
                            = QNetworkAccessManager::GetOperation,
                            const QByteArray &payload = {})
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");

    return request<Result>(
        networkManager,
        req,
        [callback](const QByteArray &reply, auto promise) {
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(reply, &error);
            if (error.error != QJsonParseError::NoError) {
                promise->setException(
                    std::make_exception_ptr(std::runtime_error(error.errorString().toUtf8())));
                return;
            }
            promise->addResult(callback(doc));
        },
        op,
        payload);
}

} // namespace CompilerExplorer::Api
