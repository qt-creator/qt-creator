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
        QJsonArray libraries = doc.array();
        Libraries result;

        for (const auto &library : libraries) {
            QJsonObject obj = library.toObject();
            Library l;
            l.id = obj["id"].toString();
            l.name = obj["name"].toString();
            l.url = QUrl::fromUserInput(obj["url"].toString());

            QJsonArray versions = obj["versions"].toArray();
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
} // namespace CompilerExplorer::Api
