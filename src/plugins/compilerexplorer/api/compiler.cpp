// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compiler.h"

#include "request.h"

#include <QFutureWatcher>
#include <QUrlQuery>

namespace CompilerExplorer::Api {

QFuture<Compilers> compilers(const Config &config,
                             const QString &languageId,
                             const QSet<QString> &extraFields)
{
    QUrl url = config.url(QStringList{"api/compilers"}
                          << (languageId.isEmpty() ? QString() : languageId));

    QString fieldParam;

    if (!extraFields.isEmpty()) {
        QSet<QString> fields = {"id", "name", "lang", "compilerType", "semver"};
        fields.unite(extraFields);
        for (const auto &field : fields) {
            if (!fieldParam.isEmpty())
                fieldParam += ",";
            fieldParam += field;
        }
    }

    if (!fieldParam.isEmpty())
        url.setQuery(QUrlQuery{{"fields", fieldParam}});

    auto fromJson = [extraFields](const QJsonDocument &doc) {
        QJsonArray compilers = doc.array();
        Compilers result;

        for (const auto &compiler : compilers) {
            QJsonObject obj = compiler.toObject();
            Compiler c;
            c.id = obj["id"].toString();
            c.name = obj["name"].toString();
            c.languageId = obj["lang"].toString();
            c.compilerType = obj["compilerType"].toString();
            c.version = obj["semver"].toString();
            c.instructionSet = obj["instructionSet"].toString();

            for (const auto &field : extraFields) {
                c.extraFields[field] = obj[field].toString();
            }

            result.append(c);
        }

        return result;
    };

    return jsonRequest<Compilers>(config.networkManager, url, fromJson);
}

} // namespace CompilerExplorer::Api
