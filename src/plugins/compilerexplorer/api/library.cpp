// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "library.h"

#include "request.h"

#include <utils/qtcassert.h>

namespace CompilerExplorer::Api {

QFuture<Libraries> libraries(const Config &config, const QString &languageId)
{
    QTC_ASSERT(!languageId.isEmpty(),
               return QtFuture::makeExceptionalFuture<Libraries>(
                   std::make_exception_ptr(std::runtime_error("Language ID is empty."))));

    const QUrl url = config.url({"api/libraries", languageId});

    return jsonRequest<Libraries>(config.networkManager, url, [](const QJsonDocument &doc) {
        const QJsonArray libraries = doc.array();
        Libraries result;

        for (const auto &library : libraries) {
            QJsonObject obj = library.toObject();
            Library l;
            l.id = obj["id"].toString();
            l.name = obj["name"].toString();
            l.url = QUrl::fromUserInput(obj["url"].toString());

            const QJsonArray versions = obj["versions"].toArray();
            for (const auto &version : versions) {
                l.versions.append({
                    version.toObject()["version"].toString(),
                    version.toObject()["id"].toString(),
                });
            }

            result.append(l);
        }

        return result;
    });
}

QtTaskTree::ExecutableItem librariesTask(const Config &config,
                                         const QString &languageId,
                                         const ResultStorage<Libraries> &result)
{
    QTC_ASSERT(!languageId.isEmpty(), return QtTaskTree::QSyncTask([result] {
        *result = Utils::ResultError(QString("Language ID is empty."));
        return false;
    }));

    const QUrl url = config.url({"api/libraries", languageId});

    return jsonRequestTask(
        config.networkManager, url, [result](const Utils::Result<QJsonDocument> &response) {
            if (!response) {
                *result = Utils::ResultError(response.error());
                return;
            }
            Libraries libraries;
            for (const auto &library : response->array()) {
                QJsonObject obj = library.toObject();
                Library l;
                l.id = obj["id"].toString();
                l.name = obj["name"].toString();
                l.url = QUrl::fromUserInput(obj["url"].toString());

                for (const auto &version : obj["versions"].toArray()) {
                    l.versions.append({
                        version.toObject()["version"].toString(),
                        version.toObject()["id"].toString(),
                    });
                }

                libraries.append(l);
            }
            *result = libraries;
        });
}
} // namespace CompilerExplorer::Api
