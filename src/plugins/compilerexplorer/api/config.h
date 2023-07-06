// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QNetworkAccessManager>
#include <QUrl>

namespace CompilerExplorer::Api {

struct Config
{
    Config(QNetworkAccessManager *networkManager)
        : networkManager(networkManager)
    {}
    Config(QNetworkAccessManager *networkManager, const QUrl &baseUrl)
        : networkManager(networkManager)
        , baseUrl(baseUrl){};

    Config(QNetworkAccessManager *networkManager, const QString &baseUrl)
        : networkManager(networkManager)
        , baseUrl(QUrl::fromUserInput(baseUrl)){};

    QNetworkAccessManager *networkManager;
    QUrl baseUrl{"https://godbolt.org/"};

    QUrl url(const QStringList &paths) const { return baseUrl.resolved(paths.join("/")); }
};

} // namespace CompilerExplorer::Api
