// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "language.h"

#include "request.h"

#include <QUrlQuery>

namespace CompilerExplorer::Api {

QFuture<Languages> languages(const Config &config)
{
    QUrl url = config.url({"api/languages"});
    url.setQuery(QUrlQuery{{"fields", "id,name,extensions,logoUrl"}});

    return jsonRequest<Languages>(config.networkManager, url, [](const QJsonDocument &doc) {
        QJsonArray languages = doc.array();
        Languages result;
        for (const auto &language : languages) {
            QJsonObject obj = language.toObject();
            Language l;
            l.id = obj["id"].toString();
            l.name = obj["name"].toString();
            l.logoUrl = obj["logoUrl"].toString();
            QJsonArray extensions = obj["extensions"].toArray();
            for (const auto &extension : extensions) {
                l.extensions.append(extension.toString());
            }

            result.append(l);
        }
        std::sort(result.begin(), result.end(), [](const Language &a, const Language &b) {
            return a.name < b.name;
        });

        return result;
    });
}

} // namespace CompilerExplorer::Api
