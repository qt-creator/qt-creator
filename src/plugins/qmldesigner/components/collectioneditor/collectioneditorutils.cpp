// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectioneditorutils.h"

#include "collectiondatatypemodel.h"
#include "collectioneditorconstants.h"
#include "model.h"
#include "nodemetainfo.h"
#include "propertymetainfo.h"
#include "variantproperty.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <utils/qtcassert.h>

#include <variant>

#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QUrl>

using DataType = QmlDesigner::CollectionDetails::DataType;

namespace {

using CollectionDataVariant = std::variant<QString, bool, double, int, QUrl, QColor>;

inline bool operator<(const QColor &a, const QColor &b)
{
    return a.name(QColor::HexArgb) < b.name(QColor::HexArgb);
}

inline CollectionDataVariant valueToVariant(const QVariant &value, DataType type)
{
    switch (type) {
    case DataType::String:
        return value.toString();
    case DataType::Real:
        return value.toDouble();
    case DataType::Integer:
        return value.toInt();
    case DataType::Boolean:
        return value.toBool();
    case DataType::Color:
        return value.value<QColor>();
    case DataType::Image:
    case DataType::Url:
        return value.value<QUrl>();
    default:
        return false;
    }
}

struct LessThanVisitor
{
    template<typename T1, typename T2>
    bool operator()(const T1 &a, const T2 &b) const
    {
        return CollectionDataVariant(a).index() < CollectionDataVariant(b).index();
    }

    template<typename T>
    bool operator()(const T &a, const T &b) const
    {
        return a < b;
    }
};

} // namespace

namespace QmlDesigner::CollectionEditorUtils {

bool variantIslessThan(const QVariant &a, const QVariant &b, DataType type)
{
    return std::visit(LessThanVisitor{}, valueToVariant(a, type), valueToVariant(b, type));
}

bool canAcceptCollectionAsModel(const ModelNode &node)
{
    const NodeMetaInfo nodeMetaInfo = node.metaInfo();
    if (!nodeMetaInfo.isValid())
        return false;

    const PropertyMetaInfo modelProperty = nodeMetaInfo.property("model");
    if (!modelProperty.isValid())
        return false;

    return modelProperty.isWritable() && !modelProperty.isPrivate()
           && modelProperty.propertyType().isVariant();
}

bool hasTextRoleProperty(const ModelNode &node)
{
    const NodeMetaInfo nodeMetaInfo = node.metaInfo();
    if (!nodeMetaInfo.isValid())
        return false;

    const PropertyMetaInfo textRoleProperty = nodeMetaInfo.property("textRole");
    if (!textRoleProperty.isValid())
        return false;

    return textRoleProperty.isWritable() && !textRoleProperty.isPrivate()
           && textRoleProperty.propertyType().isString();
}

QString getSourceCollectionPath(const ModelNode &dataStoreNode)
{
    using Utils::FilePath;
    if (!dataStoreNode.isValid())
        return {};

    const QUrl dataStoreUrl = dataStoreNode.model()->fileUrl();
    QUrl sourceValue = dataStoreNode.property("source").toVariantProperty().value().toUrl();

    QUrl sourceUrl = sourceValue.isRelative() ? dataStoreUrl.resolved(sourceValue) : sourceValue;

    const FilePath expectedFile = FilePath::fromUrl(sourceUrl);

    if (expectedFile.isFile() && expectedFile.exists())
        return expectedFile.toFSPathString();

    const FilePath defaultJsonFile = FilePath::fromUrl(
        dataStoreUrl.resolved(CollectionEditorConstants::DEFAULT_MODELS_JSON_FILENAME.toString()));

    if (defaultJsonFile.exists())
        return defaultJsonFile.toFSPathString();

    return {};
}

QJsonObject defaultCollection()
{
    QJsonObject collectionObject;

    QJsonArray columns;
    QJsonObject defaultColumn;
    defaultColumn.insert("name", "Column 1");
    defaultColumn.insert("type", CollectionDataTypeModel::dataTypeToString(DataType::String));
    columns.append(defaultColumn);

    QJsonArray collectionData;
    QJsonArray cellData;
    cellData.append(QString{});
    collectionData.append(cellData);

    collectionObject.insert("columns", columns);
    collectionObject.insert("data", collectionData);

    return collectionObject;
}

QJsonObject defaultColorCollection()
{
    using Utils::FilePath;
    using Utils::FileReader;
    const FilePath templatePath = findFile(Core::ICore::resourcePath(), "Colors.json.tpl");

    FileReader fileReader;
    if (!fileReader.fetch(templatePath)) {
        qWarning() << __FUNCTION__ << "Can't read the content of the file" << templatePath;
        return {};
    }

    QJsonParseError parseError;
    const QList<CollectionDetails> collections = CollectionDetails::fromImportedJson(fileReader.data(),
                                                                                     &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << __FUNCTION__ << "Error in template file" << parseError.errorString();
        return {};
    }

    if (!collections.size()) {
        qWarning() << __FUNCTION__ << "Can not generate collections from template file!";
        return {};
    }

    const CollectionDetails collection = collections.first();
    return collection.toLocalJson();
}

Utils::FilePath findFile(const Utils::FilePath &path, const QString &fileName)
{
    QDirIterator it(path.toString(), QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QFileInfo file(it.next());
        if (file.isDir())
            continue;

        if (file.fileName() == fileName)
            return Utils::FilePath::fromFileInfo(file);
    }
    return {};
}

bool writeToJsonDocument(const Utils::FilePath &path, const QJsonDocument &document, QString *errorString)
{
    Core::FileChangeBlocker fileBlocker(path);
    Utils::FileSaver jsonFile(path);
    if (jsonFile.write(document.toJson()))
        jsonFile.finalize();
    if (errorString)
        *errorString = jsonFile.errorString();

    return !jsonFile.hasError();
}

} // namespace QmlDesigner::CollectionEditorUtils
