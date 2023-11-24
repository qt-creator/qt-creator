/*
 * Copyright (C) 2022-current by Axivion GmbH
 * https://www.axivion.com/
 *
 * SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
 */

#include "dashboard/error.h"

#include <type_traits>
#include <utility>

namespace Axivion::Internal
{

CommunicationError::CommunicationError(QUrl replyUrl)
    : replyUrl(std::move(replyUrl))
{
}

GeneralError::GeneralError(QUrl replyUrl, QString message)
    : CommunicationError(replyUrl),
    message(std::move(message))
{
}

NetworkError::NetworkError(QUrl replyUrl,
                           QNetworkReply::NetworkError networkError,
                           QString networkErrorString)
    : CommunicationError(std::move(replyUrl)),
      networkError(std::move(networkError)),
      networkErrorString(std::move(networkErrorString))
{
}

HttpError::HttpError(QUrl replyUrl, int httpStatusCode, QString httpReasonPhrase, QString body)
    : CommunicationError(std::move(replyUrl)),
      httpStatusCode(httpStatusCode),
      httpReasonPhrase(std::move(httpReasonPhrase)),
      body(std::move(body))
{
}

DashboardError::DashboardError(QUrl replyUrl, int httpStatusCode, QString httpReasonPhrase, Dto::ErrorDto error)
    : CommunicationError(std::move(replyUrl)),
      httpStatusCode(httpStatusCode),
      httpReasonPhrase(std::move(httpReasonPhrase)),
      dashboardVersion(std::move(error.dashboardVersionNumber)),
      type(std::move(error.type)),
      message(std::move(error.message))
{
}

Error::Error(GeneralError error) : m_error(std::move(error))
{
}

Error::Error(NetworkError error) : m_error(std::move(error))
{
}

Error::Error(HttpError error) : m_error(std::move(error))
{
}

Error::Error(DashboardError error) : m_error(std::move(error))
{
}

// https://www.cppstories.com/2018/09/visit-variants/
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>; // line not needed in C++20...

QString Error::message() const
{
    return std::visit(
        overloaded{
            [](const GeneralError &error) {
                return QStringLiteral("GeneralError (%1) %2")
                    .arg(error.replyUrl.toString(),
                         error.message);
            },
            [](const NetworkError &error) {
                return QStringLiteral("NetworkError (%1) %2: %3")
                    .arg(error.replyUrl.toString(),
                         QString::number(error.networkError),
                         error.networkErrorString);
            },
            [](const HttpError &error) {
                return QStringLiteral("HttpError (%1) %2: %3\n%4")
                    .arg(error.replyUrl.toString(),
                         QString::number(error.httpStatusCode),
                         error.httpReasonPhrase,
                         error.body);
            },
            [](const DashboardError &error) {
                return QStringLiteral("DashboardError (%1) [%2 %3] %4: %5")
                    .arg(error.replyUrl.toString(),
                         QString::number(error.httpStatusCode),
                         error.httpReasonPhrase,
                         error.type,
                         error.message);
            },
        }, this->m_error);
}

bool Error::isInvalidCredentialsError()
{
    DashboardError *dashboardError = std::get_if<DashboardError>(&this->m_error);
    return dashboardError != nullptr
           && dashboardError->type == QLatin1String("InvalidCredentialsException");
}

} // namespace Axivion::Internal
