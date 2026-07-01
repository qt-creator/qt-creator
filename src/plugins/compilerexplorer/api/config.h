// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/result.h>

#include <QNetworkAccessManager>
#include <QUrl>

#include <QtTaskTree/QTaskTree>

namespace CompilerExplorer::Api {

// Storage for the outcome of an API task: either the parsed result, or an error message.
template<typename T>
using ResultStorage = QtTaskTree::Storage<Utils::Result<T>>;

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
