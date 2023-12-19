// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectioneditorutils.h"

#include "model.h"
#include "nodemetainfo.h"
#include "propertymetainfo.h"

#include <variant>

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <QColor>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>

namespace {

using CollectionDataVariant = std::variant<QString, bool, double, QUrl, QColor>;

inline bool operator<(const QColor &a, const QColor &b)
{
    return a.name(QColor::HexArgb) < b.name(QColor::HexArgb);
}

inline CollectionDataVariant valueToVariant(const QVariant &value,
                                            QmlDesigner::CollectionDetails::DataType type)
{
    using DataType = QmlDesigner::CollectionDetails::DataType;
    switch (type) {
    case DataType::String:
        return value.toString();
    case DataType::Number:
        return value.toDouble();
    case DataType::Boolean:
        return value.toBool();
    case DataType::Color:
        return value.value<QColor>();
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

Utils::FilePath dataStoreDir()
{
    using Utils::FilePath;
    ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectManager::startupProject();

    if (!currentProject)
        return {};

    return currentProject->projectDirectory().pathAppended("/imports/"
                                                           + currentProject->displayName());
}

inline Utils::FilePath collectionPath(const QString &filePath)
{
    return dataStoreDir().pathAppended("/" + filePath);
}

inline Utils::FilePath qmlDirFilePath()
{
    return collectionPath("qmldir");
}

} // namespace

namespace QmlDesigner::CollectionEditor {

bool variantIslessThan(const QVariant &a, const QVariant &b, CollectionDetails::DataType type)
{
    return std::visit(LessThanVisitor{}, valueToVariant(a, type), valueToVariant(b, type));
}

SourceFormat getSourceCollectionFormat(const ModelNode &node)
{
    using namespace QmlDesigner;
    if (node.type() == CollectionEditor::JSONCOLLECTIONMODEL_TYPENAME)
        return CollectionEditor::SourceFormat::Json;

    if (node.type() == CollectionEditor::CSVCOLLECTIONMODEL_TYPENAME)
        return CollectionEditor::SourceFormat::Csv;

    return CollectionEditor::SourceFormat::Unknown;
}

QString getSourceCollectionType(const ModelNode &node)
{
    using namespace QmlDesigner;
    if (node.type() == CollectionEditor::JSONCOLLECTIONMODEL_TYPENAME)
        return "json";

    if (node.type() == CollectionEditor::CSVCOLLECTIONMODEL_TYPENAME)
        return "csv";

    return {};
}

Utils::FilePath dataStoreJsonFilePath()
{
    return collectionPath("models.json");
}

Utils::FilePath dataStoreQmlFilePath()
{
    return collectionPath("DataStore.qml");
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

QString getSourceCollectionPath(const ModelNode &dataStoreNode)
{
    using Utils::FilePath;
    if (!dataStoreNode.isValid())
        return {};

    const FilePath expectedFile = dataStoreJsonFilePath();

    if (expectedFile.exists())
        return expectedFile.toFSPathString();

    return {};
}

bool isDataStoreNode(const ModelNode &dataStoreNode)
{
    using Utils::FilePath;

    if (!dataStoreNode.isValid())
        return false;

    const FilePath expectedFile = dataStoreQmlFilePath();

    if (!expectedFile.exists())
        return false;

    FilePath modelPath = FilePath::fromUserInput(dataStoreNode.model()->fileUrl().toLocalFile());

    return modelPath.isSameFile(expectedFile);
}

QJsonArray defaultCollectionArray()
{
    QJsonObject initialObject;
    QJsonArray initialCollection;

    initialObject.insert("Column1", "");
    initialCollection.append(initialObject);
    return initialCollection;
}

bool ensureDataStoreExists(bool &justCreated)
{
    using Utils::FilePath;
    using Utils::FileReader;
    using Utils::FileSaver;

    FilePath qmlDestinationPath = dataStoreQmlFilePath();
    justCreated = false;

    auto extractDependency = [&justCreated](const FilePath &filePath) -> bool {
        if (filePath.exists())
            return true;

        const QString templateFileName = filePath.fileName() + u".tpl";
        const FilePath templatePath = findFile(Core::ICore::resourcePath(), templateFileName);
        if (!templatePath.exists()) {
            qWarning() << Q_FUNC_INFO << __LINE__ << templateFileName << "does not exist";
            return false;
        }

        templatePath.copyFile(filePath);
        if (filePath.exists()) {
            justCreated = true;
            return true;
        }

        qWarning() << Q_FUNC_INFO << __LINE__ << "Cannot copy" << templateFileName << "to" << filePath;
        return false;
    };

    if (!extractDependency(dataStoreJsonFilePath()))
        return false;

    if (!extractDependency(collectionPath("data.json")))
        return false;

    if (!extractDependency(collectionPath("JsonData.qml")))
        return false;

    if (!qmlDestinationPath.exists()) {
        if (qmlDestinationPath.ensureExistingFile()) {
            justCreated = true;
        } else {
            qWarning() << Q_FUNC_INFO << __LINE__ << "Can't create DataStore Qml File";
            return false;
        }
    }

    FilePath qmlDirPath = qmlDirFilePath();
    qmlDirPath.ensureExistingFile();

    FileReader qmlDirReader;
    if (!qmlDirReader.fetch(qmlDirPath)) {
        qWarning() << Q_FUNC_INFO << __LINE__ << "Can't read the content of the qmldir";
        return false;
    }

    QByteArray qmlDirContent = qmlDirReader.data();
    const QList<QByteArray> qmlDirLines = qmlDirContent.split('\n');
    for (const QByteArray &line : qmlDirLines) {
        if (line.startsWith("singleton DataStore "))
            return true;
    }

    if (!qmlDirContent.isEmpty() && qmlDirContent.back() != '\n')
        qmlDirContent.append("\n");
    qmlDirContent.append("singleton DataStore 1.0 DataStore.qml\n");

    FileSaver qmlDirSaver(qmlDirPath);
    qmlDirSaver.write(qmlDirContent);

    if (qmlDirSaver.finalize()) {
        justCreated = true;
        return true;
    }

    qWarning() << Q_FUNC_INFO << __LINE__ << "Can't write to the qmldir file";
    return false;
}

} // namespace QmlDesigner::CollectionEditor
