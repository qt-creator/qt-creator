// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectioneditorutils.h"

#include "model.h"
#include "nodemetainfo.h"
#include "propertymetainfo.h"

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

namespace {

using CollectionDataVariant = std::variant<QString, bool, double, int, QUrl, QColor>;

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

namespace QmlDesigner::CollectionEditorUtils {

bool variantIslessThan(const QVariant &a, const QVariant &b, CollectionDetails::DataType type)
{
    return std::visit(LessThanVisitor{}, valueToVariant(a, type), valueToVariant(b, type));
}

CollectionEditorConstants::SourceFormat getSourceCollectionFormat(const ModelNode &node)
{
    using namespace QmlDesigner;
    if (node.type() == CollectionEditorConstants::JSONCOLLECTIONMODEL_TYPENAME)
        return CollectionEditorConstants::SourceFormat::Json;

    if (node.type() == CollectionEditorConstants::CSVCOLLECTIONMODEL_TYPENAME)
        return CollectionEditorConstants::SourceFormat::Csv;

    return CollectionEditorConstants::SourceFormat::Unknown;
}

QString getSourceCollectionType(const ModelNode &node)
{
    using namespace QmlDesigner;
    if (node.type() == CollectionEditorConstants::JSONCOLLECTIONMODEL_TYPENAME)
        return "json";

    if (node.type() == CollectionEditorConstants::CSVCOLLECTIONMODEL_TYPENAME)
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

        // Force code model reset to notice changes to existing module
        auto modelManager = QmlJS::ModelManagerInterface::instance();
        if (modelManager)
            modelManager->resetCodeModel();

        return true;
    }

    qWarning() << Q_FUNC_INFO << __LINE__ << "Can't write to the qmldir file";
    return false;
}

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
            if (collectionData.isObject()) {
                QJsonObject rowObject = collectionData.toObject();
                const QStringList rowKeys = rowObject.keys();
                for (const QString &key : rowKeys) {
                    QJsonValue cellValue = rowObject.value(key);
                    if (cellValue.isArray())
                        rowObject.remove(key);
                }
                resultArray.push_back(rowObject);
            }
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

QString getFirstColumnName(const QString &collectionName)
{
    Utils::FilePath dataStorePath = CollectionEditorUtils::dataStoreJsonFilePath();

    if (!dataStorePath.exists())
        return {};

    Utils::FileReader dataStoreFile;
    if (!dataStoreFile.fetch(dataStorePath))
        return {};

    QJsonParseError jsonError;
    QJsonDocument dataStoreDocument = QJsonDocument::fromJson(dataStoreFile.data(), &jsonError);
    if (jsonError.error == QJsonParseError::NoError) {
        QJsonObject rootObject = dataStoreDocument.object();
        if (rootObject.contains(collectionName)) {
            QJsonArray collectionArray = rootObject.value(collectionName).toArray();
            for (const QJsonValue &elementValue : std::as_const(collectionArray)) {
                const QJsonObject elementObject = elementValue.toObject();
                QJsonObject::ConstIterator element = elementObject.constBegin();
                if (element != elementObject.constEnd())
                    return element.key();
            }
        } else {
            qWarning() << Q_FUNC_INFO << __LINE__
                       << QString("Collection \"%1\" not found.").arg(collectionName);
        }
    } else {
        qWarning() << Q_FUNC_INFO << __LINE__ << "Problem in reading json file."
                   << jsonError.errorString();
    }

    return {};
}

bool collectionHasColumn(const QString &collectionName, const QString &columnName)
{
    Utils::FilePath dataStorePath = CollectionEditorUtils::dataStoreJsonFilePath();

    if (!dataStorePath.exists())
        return false;

    Utils::FileReader dataStoreFile;
    if (!dataStoreFile.fetch(dataStorePath))
        return false;

    QJsonParseError jsonError;
    QJsonDocument dataStoreDocument = QJsonDocument::fromJson(dataStoreFile.data(), &jsonError);
    if (jsonError.error == QJsonParseError::NoError) {
        QJsonObject rootObject = dataStoreDocument.object();
        if (rootObject.contains(collectionName)) {
            QJsonArray collectionArray = rootObject.value(collectionName).toArray();
            for (const QJsonValue &elementValue : std::as_const(collectionArray)) {
                const QJsonObject elementObject = elementValue.toObject();
                QJsonObject::ConstIterator element = elementObject.constBegin();
                const QJsonObject::ConstIterator stopItem = elementObject.constEnd();

                while (element != stopItem) {
                    const QString keyName = element.key();
                    ++element;

                    if (columnName == keyName)
                        return true;
                }
            }
        } else {
            qWarning() << Q_FUNC_INFO << __LINE__
                       << QString("Collection \"%1\" not found.").arg(collectionName);
        }
    } else {
        qWarning() << Q_FUNC_INFO << __LINE__ << "Problem in reading json file."
                   << jsonError.errorString();
    }

    return false;
}

} // namespace QmlDesigner::CollectionEditorUtils
