// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compile.h"

#include "request.h"

namespace CompilerExplorer::Api {
QFuture<CompileResult> compile(const Config &config, const CompileParameters &parameters)
{
    const QUrl url = config.url({"api/compiler", parameters.compilerId, "compile"});

    return jsonRequest<CompileResult>(
        config.networkManager,
        url,
        [](const QJsonDocument &response) {
            CompileResult result;

            QJsonObject obj = response.object();
            return CompileResult::fromJson(obj);
        },
        QNetworkAccessManager::PostOperation,
        parameters.toByteArray());
}

QtTaskTree::ExecutableItem compileTask(const Config &config,
                                       const CompileParameters &parameters,
                                       const ResultStorage<CompileResult> &result)
{
    const QUrl url = config.url({"api/compiler", parameters.compilerId, "compile"});

    return jsonRequestTask(
        config.networkManager,
        url,
        [result](const Utils::Result<QJsonDocument> &response) {
            if (response)
                *result = CompileResult::fromJson(response->object());
            else
                *result = Utils::ResultError(response.error());
        },
        QNetworkAccessManager::PostOperation,
        parameters.toByteArray());
}
} // namespace CompilerExplorer::Api
