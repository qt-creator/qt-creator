#pragma once

/*
 * Copyright (C) 2022-current by Axivion GmbH
 * https://www.axivion.com/
 *
 * SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
 */

#include "dashboard/dto.h"

#include <QNetworkReply>

namespace Axivion::Internal {

QString networkErrorMessage(const QUrl &replyUrl,
                            QNetworkReply::NetworkError networkError,
                            const QString &networkErrorString);

QString httpErrorMessage(const QUrl &replyUrl,
                         int httpStatusCode,
                         const QString &httpReasonPhrase,
                         const QString &body);

QString dashboardErrorMessage(const QUrl &replyUrl,
                              int httpStatusCode,
                              const QString &httpReasonPhrase,
                              const Dto::ErrorDto &error);

} // namespace Axivion::Internal
