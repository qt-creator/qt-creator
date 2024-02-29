// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "collectiondetails.h"
#include "collectioneditorconstants.h"

QT_BEGIN_NAMESPACE
class QJsonArray;
class QJsonObject;
QT_END_NAMESPACE

namespace Utils {
class FilePath;
}

namespace QmlDesigner::CollectionEditorUtils {

bool variantIslessThan(const QVariant &a, const QVariant &b, CollectionDetails::DataType type);

QString getSourceCollectionType(const QmlDesigner::ModelNode &node);

QString getSourceCollectionPath(const QmlDesigner::ModelNode &dataStoreNode);

Utils::FilePath dataStoreJsonFilePath();

Utils::FilePath dataStoreQmlFilePath();

bool writeToJsonDocument(const Utils::FilePath &path,
                         const QJsonDocument &document,
                         QString *errorString = nullptr);

bool isDataStoreNode(const ModelNode &dataStoreNode);

bool ensureDataStoreExists(bool &justCreated);

bool canAcceptCollectionAsModel(const ModelNode &node);

bool hasTextRoleProperty(const ModelNode &node);

QJsonObject defaultCollection();

QJsonObject defaultColorCollection();

QString dataTypeToString(CollectionDetails::DataType dataType);

CollectionDetails::DataType dataTypeFromString(const QString &dataType);

QStringList dataTypesStringList();

} // namespace QmlDesigner::CollectionEditorUtils
