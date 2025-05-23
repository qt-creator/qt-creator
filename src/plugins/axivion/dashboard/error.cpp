/*
 * Copyright (C) 2022-current by Axivion GmbH
 * https://www.axivion.com/
 *
 * SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
 */

#include "dashboard/error.h"

namespace Axivion::Internal {

QString networkErrorMessage(const QUrl &replyUrl,
                            QNetworkReply::NetworkError networkError,
                            const QString &networkErrorString)
{
    return QStringLiteral("NetworkError (%1) %2: %3")
        .arg(replyUrl.toString()).arg(networkError).arg(networkErrorString);
}

QString httpErrorMessage(const QUrl &replyUrl,
                         int httpStatusCode,
                         const QString &httpReasonPhrase,
                         const QString &body)
{
    return QStringLiteral("HttpError (%1) %2: %3\n%4")
        .arg(replyUrl.toString()).arg(httpStatusCode)
        .arg(httpReasonPhrase, body);
}

QString dashboardErrorMessage(const QUrl &replyUrl,
                              int httpStatusCode,
                              const QString &httpReasonPhrase,
                              const Dto::ErrorDto &error)
{
    return QStringLiteral("DashboardError (%1) [%2 %3] %4: %5")
        .arg(replyUrl.toString()).arg(httpStatusCode)
        .arg(httpReasonPhrase, error.type, error.message);
}

} // namespace Axivion::Internal
