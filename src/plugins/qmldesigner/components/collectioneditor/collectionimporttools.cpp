// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionimporttools.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QUrl>

namespace QmlDesigner::CollectionEditor::ImportTools {

QJsonArray loadAsSingleJsonCollection(const QUrl &url)
{
    QFile file(url.isLocalFile() ? url.toLocalFile() : url.toString());
    QJsonArray collection;
    QByteArray jsonData;
    if (file.open(QFile::ReadOnly))
        jsonData = file.readAll();

    file.close();
    if (jsonData.isEmpty())
        return {};

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return {};

    auto refineJsonArray = [](const QJsonArray &array) -> QJsonArray {
        QJsonArray resultArray;
        for (const QJsonValue &collectionData : array) {
            if (!collectionData.isObject())
                resultArray.push_back(collectionData);
        }
        return resultArray;
    };

    if (document.isArray()) {
        collection = refineJsonArray(document.array());
    } else if (document.isObject()) {
        QJsonObject documentObject = document.object();
        const QStringList mainKeys = documentObject.keys();

        bool arrayFound = false;
        for (const QString &key : mainKeys) {
            const QJsonValue &value = documentObject.value(key);
            if (value.isArray()) {
                arrayFound = true;
                collection = refineJsonArray(value.toArray());
                break;
            }
        }

        if (!arrayFound) {
            QJsonObject singleObject;
            for (const QString &key : mainKeys) {
                const QJsonValue value = documentObject.value(key);

                if (!value.isObject())
                    singleObject.insert(key, value);
            }
            collection.push_back(singleObject);
        }
    }
    return collection;
}

QJsonArray loadAsCsvCollection(const QUrl &url)
{
    QFile sourceFile(url.isLocalFile() ? url.toLocalFile() : url.toString());
    QStringList headers;
    QJsonArray elements;

    if (sourceFile.open(QFile::ReadOnly)) {
        QTextStream stream(&sourceFile);

        if (!stream.atEnd())
            headers = stream.readLine().split(',');

        for (QString &header : headers)
            header = header.trimmed();

        if (!headers.isEmpty()) {
            while (!stream.atEnd()) {
                const QStringList recordDataList = stream.readLine().split(',');
                int column = -1;
                QJsonObject recordData;
                for (const QString &cellData : recordDataList) {
                    if (++column == headers.size())
                        break;
                    recordData.insert(headers.at(column), cellData);
                }
                elements.append(recordData);
            }
        }
    }

    return elements;
}

} // namespace QmlDesigner::CollectionEditor::ImportTools
