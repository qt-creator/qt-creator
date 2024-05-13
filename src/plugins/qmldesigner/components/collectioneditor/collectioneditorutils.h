// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "collectiondetails.h"

QT_BEGIN_NAMESPACE
class QJsonArray;
class QJsonObject;
QT_END_NAMESPACE

namespace Utils {
class FilePath;
}

namespace QmlDesigner::CollectionEditorUtils {

bool variantIslessThan(const QVariant &a, const QVariant &b, CollectionDetails::DataType type);

QString getSourceCollectionPath(const QmlDesigner::ModelNode &dataStoreNode);

Utils::FilePath findFile(const Utils::FilePath &path, const QString &fileName);

bool writeToJsonDocument(const Utils::FilePath &path,
                         const QJsonDocument &document,
                         QString *errorString = nullptr);

bool canAcceptCollectionAsModel(const ModelNode &node);

bool hasTextRoleProperty(const ModelNode &node);

QJsonObject defaultCollection();

QJsonObject defaultColorCollection();

} // namespace QmlDesigner::CollectionEditorUtils
