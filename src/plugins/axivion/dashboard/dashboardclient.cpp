/*
 * Copyright (C) 2022-current by Axivion GmbH
 * https://www.axivion.com/
 *
 * SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
 */

#include "dashboardclient.h"

#include "axivionsettings.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QLatin1String>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPromise>

#include <memory>

namespace Axivion::Internal
{

DashboardClient::DashboardClient(Utils::NetworkAccessManager &networkAccessManager)
    : m_networkAccessManager(networkAccessManager)
{
}

using ResponseData = Utils::expected<DataWithOrigin<QByteArray>, Error>;

static constexpr int httpStatusCodeOk = 200;
static const QLatin1String jsonContentType{ "application/json" };

ResponseData readResponse(QNetworkReply& reply, QAnyStringView expectedContentType)
{
    QNetworkReply::NetworkError error = reply.error();
    int statusCode = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString contentType = reply.header(QNetworkRequest::ContentTypeHeader)
                              .toString()
                              .split(';')
                              .constFirst()
                              .trimmed()
                              .toLower();
    if (error == QNetworkReply::NetworkError::NoError
            && statusCode == httpStatusCodeOk
            && contentType == expectedContentType) {
        return DataWithOrigin(reply.url(), reply.readAll());
    }
    if (contentType == jsonContentType) {
        try {
            return tl::make_unexpected(DashboardError(
                reply.url(),
                statusCode,
                reply.attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString(),
                Dto::ErrorDto::deserialize(reply.readAll())));
        } catch (const Dto::invalid_dto_exception &) {
            // ignore
        }
    }
    if (statusCode != 0) {
        return tl::make_unexpected(HttpError(
            reply.url(),
            statusCode,
            reply.attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString(),
            QString::fromUtf8(reply.readAll()))); // encoding?
    }
    return tl::make_unexpected(
        NetworkError(reply.url(), error, reply.errorString()));
}

template<typename T>
static Utils::expected<DataWithOrigin<T>, Error> parseResponse(ResponseData rawBody)
{
    if (!rawBody)
        return tl::make_unexpected(std::move(rawBody.error()));
    try {
        T data = T::deserialize(rawBody.value().data);
        return DataWithOrigin(std::move(rawBody.value().origin),
                              std::move(data));
    } catch (const Dto::invalid_dto_exception &e) {
        return tl::make_unexpected(GeneralError(std::move(rawBody.value().origin),
                                                QString::fromUtf8(e.what())));
    }
}

QFuture<ResponseData> fetch(Utils::NetworkAccessManager &networkAccessManager,
                            const std::optional<QUrl> &base,
                            const QUrl &target)
{
    QPromise<ResponseData> promise;
    promise.start();
    const AxivionServer &server = settings().server;
    QUrl url = base ? base->resolved(target) : target;
    QNetworkRequest request{ url };
    request.setRawHeader(QByteArrayLiteral(u8"Accept"),
                         QByteArray(jsonContentType.data(), jsonContentType.size()));
    request.setRawHeader(QByteArrayLiteral(u8"Authorization"),
                         QByteArrayLiteral(u8"AxToken ") + server.token.toUtf8());
    QByteArray ua = QByteArrayLiteral(u8"Axivion")
                    + QCoreApplication::applicationName().toUtf8()
                    + QByteArrayLiteral(u8"Plugin/")
                    + QCoreApplication::applicationVersion().toUtf8();
    request.setRawHeader(QByteArrayLiteral(u8"X-Axivion-User-Agent"), ua);
    QFuture<ResponseData> future = promise.future();
    QNetworkReply* reply = networkAccessManager.get(request);
    QObject::connect(reply,
                     &QNetworkReply::finished,
                     reply,
                     [promise = std::move(promise), reply]() mutable {
                         promise.addResult(readResponse(*reply, jsonContentType));
                         promise.finish();
                         reply->deleteLater();
                     });
    return future;
}

QFuture<DashboardClient::RawProjectInfo> DashboardClient::fetchProjectInfo(const QString &projectName)
{
    const AxivionServer &server = settings().server;
    QString dashboard = server.dashboard;
    if (!dashboard.endsWith(QLatin1Char('/')))
        dashboard += QLatin1Char('/');
    QUrl url = QUrl(dashboard)
        .resolved(QUrl(QStringLiteral(u"api/projects/")))
        .resolved(QUrl(projectName));
    return fetch(this->m_networkAccessManager, std::nullopt, url)
        .then(QtFuture::Launch::Async, &parseResponse<Dto::ProjectInfoDto>);
}

} // namespace Axivion::Internal
