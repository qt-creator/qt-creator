// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/result.h>

#include <QByteArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QUrl>

#include <QtTaskTree/QTaskTree>

#include <functional>

namespace CompilerExplorer::Api {

// TaskTree-based JSON request: performs the request as a QNetworkReplyWrapperTask and hands the
// outcome to \a onResponse as a single Result: either the parsed JSON document, or an error
// message (network failure or JSON parse error). The callback typically converts and stores it
// into its own Storage<Result<...>>.
QtTaskTree::ExecutableItem jsonRequestTask(
    QNetworkAccessManager *networkManager,
    const QUrl &url,
    const std::function<void(const Utils::Result<QJsonDocument> &)> &onResponse,
    QNetworkAccessManager::Operation op = QNetworkAccessManager::GetOperation,
    const QByteArray &payload = {});

} // namespace CompilerExplorer::Api
