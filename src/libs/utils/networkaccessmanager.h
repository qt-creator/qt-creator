// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QNetworkAccessManager>

namespace Utils {

class QTCREATOR_UTILS_EXPORT NetworkAccessManager : public QNetworkAccessManager
{
public:
    NetworkAccessManager(QObject *parent = nullptr);

    static NetworkAccessManager *instance();

protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest &request,
                                 QIODevice *outgoingData) override;
};


} // namespace utils
