#pragma once

/*
 * Copyright (C) 2022-current by Axivion GmbH
 * https://www.axivion.com/
 *
 * SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
 */

#include "dashboard/dto.h"

#include <QNetworkReply>

#include <variant>

namespace Axivion::Internal
{

class CommunicationError
{
public:
    QUrl replyUrl;

    CommunicationError(QUrl replyUrl);
};

class GeneralError : public CommunicationError
{
public:
    QString message;

    GeneralError(QUrl replyUrl, QString message);
};

class NetworkError : public CommunicationError
{
public:
    QNetworkReply::NetworkError networkError;
    QString networkErrorString;

    NetworkError(QUrl replyUrl, QNetworkReply::NetworkError networkError, QString networkErrorString);
};

class HttpError : public CommunicationError
{
public:
    int httpStatusCode;
    QString httpReasonPhrase;
    QString body;

    HttpError(QUrl replyUrl, int httpStatusCode, QString httpReasonPhrase, QString body);
};

class DashboardError : public CommunicationError
{
public:
    int httpStatusCode;
    QString httpReasonPhrase;
    std::optional<QString> dashboardVersion;
    QString type;
    QString message;

    DashboardError(QUrl replyUrl, int httpStatusCode, QString httpReasonPhrase, Dto::ErrorDto error);
};

class Error
{
public:
    Error(GeneralError error);

    Error(NetworkError error);

    Error(HttpError error);

    Error(DashboardError error);

    QString message() const;

    bool isInvalidCredentialsError();

private:
    std::variant<GeneralError,
                 NetworkError,
                 HttpError,
                 DashboardError> m_error;
};

} // namespace Axivion::Internal
