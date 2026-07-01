// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "language.h"

#include "request.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>

#include <algorithm>

namespace CompilerExplorer::Api {

QtTaskTree::ExecutableItem languagesTask(const Config &config, const ResultStorage<Languages> &result)
{
    QUrl url = config.url({"api/languages"});
    url.setQuery(QUrlQuery{{"fields", "id,name,extensions,logoUrl"}});

    return jsonRequestTask(
        config.networkManager, url, [result](const Utils::Result<QJsonDocument> &response) {
            if (!response) {
                *result = Utils::ResultError(response.error());
                return;
            }
            Languages languages;
            for (const auto &language : response->array()) {
                QJsonObject obj = language.toObject();
                Language l;
                l.id = obj["id"].toString();
                l.name = obj["name"].toString();
                l.logoUrl = obj["logoUrl"].toString();
                for (const auto &extension : obj["extensions"].toArray())
                    l.extensions.append(extension.toString());

                languages.append(l);
            }
            std::sort(languages.begin(), languages.end(), [](const Language &a, const Language &b) {
                return a.name < b.name;
            });
            *result = languages;
        });
}

} // namespace CompilerExplorer::Api
