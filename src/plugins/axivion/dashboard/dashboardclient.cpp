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

#include <memory>

namespace Axivion::Internal
{

DashboardClient::DashboardClient(Utils::NetworkAccessManager &networkAccessManager)
    : m_networkAccessManager(networkAccessManager)
{
}

static void deleteLater(QObject *obj)
{
    obj->deleteLater();
}

using RawBody = Utils::expected_str<DataWithOrigin<QByteArray>>;

class RawBodyReader final
{
public:
    RawBodyReader(std::shared_ptr<QNetworkReply> reply)
        : m_reply(std::move(reply))
    { }

    ~RawBodyReader() { }

    RawBody operator()()
    {
        QNetworkReply::NetworkError error = m_reply->error();
        if (error != QNetworkReply::NetworkError::NoError)
            return tl::make_unexpected(QString::number(error)
                                       + QLatin1String(": ")
                                       + m_reply->errorString());
        return DataWithOrigin(m_reply->url(), m_reply->readAll());
    }

private:
    std::shared_ptr<QNetworkReply> m_reply;
};

template<typename T>
static Utils::expected_str<DataWithOrigin<T>> RawBodyParser(RawBody rawBody)
{
    if (!rawBody)
        return tl::make_unexpected(std::move(rawBody.error()));
    try {
        T data = T::deserialize(rawBody.value().data);
        return DataWithOrigin(std::move(rawBody.value().origin),
                              std::move(data));
    } catch (const Dto::invalid_dto_exception &e) {
        return tl::make_unexpected(QString::fromUtf8(e.what()));
    }
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
    QNetworkRequest request{ url };
    request.setRawHeader(QByteArrayLiteral(u8"Accept"),
                         QByteArrayLiteral(u8"Application/Json"));
    request.setRawHeader(QByteArrayLiteral(u8"Authorization"),
                         QByteArrayLiteral(u8"AxToken ") + server.token.toUtf8());
    QByteArray ua = QByteArrayLiteral(u8"Axivion")
                    + QCoreApplication::applicationName().toUtf8()
                    + QByteArrayLiteral(u8"Plugin/")
                    + QCoreApplication::applicationVersion().toUtf8();
    request.setRawHeader(QByteArrayLiteral(u8"X-Axivion-User-Agent"), ua);
    std::shared_ptr<QNetworkReply> reply{ this->m_networkAccessManager.get(request), deleteLater };
    return QtFuture::connect(reply.get(), &QNetworkReply::finished)
        .onCanceled(reply.get(), [reply] { reply->abort(); })
        .then(RawBodyReader(reply))
        .then(QtFuture::Launch::Async, &RawBodyParser<Dto::ProjectInfoDto>);
}

}
