// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionview.h"
#include "collectionmodel.h"
#include "collectionwidget.h"
#include "designmodecontext.h"
#include "nodelistproperty.h"
#include "nodemetainfo.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "singlecollectionmodel.h"
#include "variantproperty.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace {
using Data = std::variant<bool, double, QString, QDateTime>;
using DataRecord = QMap<QString, Data>;

struct DataHeader
{
    enum class Type { Unknown, Bool, Numeric, String, DateTime };
    Type type;
    QString name;
};

using DataHeaderMap = QMap<QString, DataHeader>; // Lowercase Name - Header Data

inline constexpr QStringView BoolDataType{u"Bool"};
inline constexpr QStringView NumberDataType{u"Number"};
inline constexpr QStringView StringDataType{u"String"};
inline constexpr QStringView DateTimeDataType{u"Date/Time"};

QString removeSpaces(QString string)
{
    string.replace(" ", "_");
    string.replace("-", "_");
    return string;
}

DataHeader getDataType(const QString &type, const QString &name)
{
    static const QMap<QString, DataHeader::Type> typeMap = {
        {BoolDataType.toString().toLower(), DataHeader::Type::Bool},
        {NumberDataType.toString().toLower(), DataHeader::Type::Numeric},
        {StringDataType.toString().toLower(), DataHeader::Type::String},
        {DateTimeDataType.toString().toLower(), DataHeader::Type::DateTime}};
    if (name.isEmpty())
        return {};

    if (type.isEmpty())
        return {DataHeader::Type::String, removeSpaces(name)};

    return {typeMap.value(type.toLower(), DataHeader::Type::Unknown), removeSpaces(name)};
}

struct JsonDocumentError : public std::exception
{
    enum Error {
        InvalidDocumentType,
        InvalidCollectionName,
        InvalidCollectionId,
        InvalidCollectionObject,
        InvalidArrayPosition,
        InvalidLiteralType,
        InvalidCollectionHeader,
        IsNotJsonArray,
        CollectionHeaderNotFound
    };

    const Error error;

    JsonDocumentError(Error error)
        : error(error)
    {}

    const char *what() const noexcept override
    {
        switch (error) {
        case InvalidDocumentType:
            return "Current JSON document contains errors.";
        case InvalidCollectionName:
            return "Invalid collection name.";
        case InvalidCollectionId:
            return "Invalid collection Id.";
        case InvalidCollectionObject:
            return "A collection should be a json object.";
        case InvalidArrayPosition:
            return "Arrays are not supported inside the collection.";
        case InvalidLiteralType:
            return "Invalid literal type for collection items";
        case InvalidCollectionHeader:
            return "Invalid Collection Header";
        case IsNotJsonArray:
            return "Json file should be an array";
        case CollectionHeaderNotFound:
            return "Collection Header not found";
        default:
            return "Unknown Json Error";
        }
    }
};

struct CsvDocumentError : public std::exception
{
    enum Error {
        HeaderNotFound,
        DataNotFound,
    };

    const Error error;

    CsvDocumentError(Error error)
        : error(error)
    {}

    const char *what() const noexcept override
    {
        switch (error) {
        case HeaderNotFound:
            return "CSV Header not found";
        case DataNotFound:
            return "CSV data not found";
        default:
            return "Unknown CSV Error";
        }
    }
};

Data getLiteralDataValue(const QVariant &value, const DataHeader &header, bool *typeWarningCheck = nullptr)
{
    if (header.type == DataHeader::Type::Bool)
        return value.toBool();

    if (header.type == DataHeader::Type::Numeric)
        return value.toDouble();

    if (header.type == DataHeader::Type::String)
        return value.toString();

    if (header.type == DataHeader::Type::DateTime) {
        QDateTime dateTimeStr = QDateTime::fromString(value.toString());
        if (dateTimeStr.isValid())
            return dateTimeStr;
    }

    if (typeWarningCheck)
        *typeWarningCheck = true;

    return value.toString();
}

void loadJsonHeaders(QList<DataHeader> &collectionHeaders,
                     DataHeaderMap &headerDataMap,
                     const QJsonObject &collectionJsonObject)
{
    const QJsonArray collectionHeader = collectionJsonObject.value("headers").toArray();
    for (const QJsonValue &headerValue : collectionHeader) {
        const QJsonObject headerJsonObject = headerValue.toObject();
        DataHeader dataHeader = getDataType(headerJsonObject.value("type").toString(),
                                            headerJsonObject.value("name").toString());

        if (dataHeader.type == DataHeader::Type::Unknown)
            throw JsonDocumentError{JsonDocumentError::InvalidCollectionHeader};

        collectionHeaders.append(dataHeader);
        headerDataMap.insert(dataHeader.name.toLower(), dataHeader);
    }

    if (collectionHeaders.isEmpty())
        throw JsonDocumentError{JsonDocumentError::CollectionHeaderNotFound};
}

void loadJsonRecords(QList<DataRecord> &collectionItems,
                     DataHeaderMap &headerDataMap,
                     const QJsonObject &collectionJsonObject)
{
    auto addItemFromValue = [&headerDataMap, &collectionItems](const QJsonValue &jsonValue) {
        const QVariantMap dataMap = jsonValue.toObject().toVariantMap();
        DataRecord recordData;
        for (const auto &dataPair : dataMap.asKeyValueRange()) {
            const DataHeader correspondingHeader = headerDataMap.value(removeSpaces(
                                                                           dataPair.first.toLower()),
                                                                       {});

            const QString &fieldName = correspondingHeader.name;
            if (fieldName.size())
                recordData.insert(fieldName,
                                  getLiteralDataValue(dataPair.second, correspondingHeader));
        }
        if (!recordData.isEmpty())
            collectionItems.append(recordData);
    };

    const QJsonValue jsonDataValue = collectionJsonObject.value("data");
    if (jsonDataValue.isObject()) {
        addItemFromValue(jsonDataValue);
    } else if (jsonDataValue.isArray()) {
        const QJsonArray jsonDataArray = jsonDataValue.toArray();
        for (const QJsonValue &jsonItem : jsonDataArray) {
            if (jsonItem.isObject())
                addItemFromValue(jsonItem);
        }
    }
}

inline bool isCollectionLib(const QmlDesigner::ModelNode &node)
{
    return node.parentProperty().parentModelNode().isRootNode()
           && node.id() == QmlDesigner::Constants::COLLECTION_LIB_ID;
}

inline bool isListModel(const QmlDesigner::ModelNode &node)
{
    return node.metaInfo().isQtQuickListModel();
}

inline bool isListElement(const QmlDesigner::ModelNode &node)
{
    return node.metaInfo().isQtQuickListElement();
}

inline bool isCollection(const QmlDesigner::ModelNode &node)
{
    return isCollectionLib(node.parentProperty().parentModelNode()) && isListModel(node);
}

inline bool isCollectionElement(const QmlDesigner::ModelNode &node)
{
    return isListElement(node) && isCollection(node.parentProperty().parentModelNode());
}

} // namespace

namespace QmlDesigner {

struct Collection
{
    QString name;
    QString id;
    QList<DataHeader> headers;
    QList<DataRecord> items;
};

CollectionView::CollectionView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
{}

bool CollectionView::loadJson(const QByteArray &data)
{
    try {
        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError)
            throw JsonDocumentError{JsonDocumentError::InvalidDocumentType};

        QList<Collection> collections;
        if (document.isArray()) {
            const QJsonArray collectionsJsonArray = document.array();

            for (const QJsonValue &collectionJson : collectionsJsonArray) {
                Collection collection;
                if (!collectionJson.isObject())
                    throw JsonDocumentError{JsonDocumentError::InvalidCollectionObject};

                QJsonObject collectionJsonObject = collectionJson.toObject();

                const QString &collectionName = collectionJsonObject.value(u"name").toString();
                if (!collectionName.size())
                    throw JsonDocumentError{JsonDocumentError::InvalidCollectionName};

                const QString &collectionId = collectionJsonObject.value(u"id").toString();
                if (!collectionId.size())
                    throw JsonDocumentError{JsonDocumentError::InvalidCollectionId};

                DataHeaderMap headerDataMap;

                loadJsonHeaders(collection.headers, headerDataMap, collectionJsonObject);
                loadJsonRecords(collection.items, headerDataMap, collectionJsonObject);

                if (collection.items.count())
                    collections.append(collection);
            }
        } else {
            throw JsonDocumentError{JsonDocumentError::InvalidDocumentType};
        }

        addLoadedModel(collections);
    } catch (const std::exception &error) {
        m_widget->warn("Json Import Problem", QString::fromLatin1(error.what()));
        return false;
    }

    return true;
}

bool CollectionView::loadCsv(const QString &collectionName, const QByteArray &data)
{
    QTextStream stream(data);
    Collection collection;
    collection.name = collectionName;

    try {
        if (!stream.atEnd()) {
            const QStringList recordData = stream.readLine().split(',');
            for (const QString &name : recordData)
                collection.headers.append(getDataType({}, name));
        }
        if (collection.headers.isEmpty())
            throw CsvDocumentError{CsvDocumentError::HeaderNotFound};

        while (!stream.atEnd()) {
            const QStringList recordDataList = stream.readLine().split(',');
            DataRecord recordData;
            int column = -1;
            for (const QString &cellData : recordDataList) {
                if (++column == collection.headers.size())
                    break;
                recordData.insert(collection.headers.at(column).name, cellData);
            }
            if (recordData.count())
                collection.items.append(recordData);
        }

        if (collection.items.isEmpty())
            throw CsvDocumentError{CsvDocumentError::DataNotFound};

        addLoadedModel({collection});
    } catch (const std::exception &error) {
        m_widget->warn("Json Import Problem", QString::fromLatin1(error.what()));
        return false;
    }

    return true;
}

bool CollectionView::hasWidget() const
{
    return true;
}

QmlDesigner::WidgetInfo CollectionView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new CollectionWidget(this);

        auto collectionEditorContext = new Internal::CollectionEditorContext(m_widget.data());
        Core::ICore::addContextObject(collectionEditorContext);
        CollectionModel *collectionModel = m_widget->collectionModel().data();

        connect(collectionModel, &CollectionModel::selectedIndexChanged, this, [&](int selectedIndex) {
            m_widget->singleCollectionModel()->setCollection(
                m_widget->collectionModel()->collectionNodeAt(selectedIndex));
        });
    }

    return createWidgetInfo(m_widget.data(),
                            "CollectionEditor",
                            WidgetInfo::LeftPane,
                            0,
                            tr("Collection Editor"),
                            tr("Collection Editor view"));
}

void CollectionView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    refreshModel();
}

void CollectionView::nodeReparented(const ModelNode &node,
                                    const NodeAbstractProperty &newPropertyParent,
                                    const NodeAbstractProperty &oldPropertyParent,
                                    [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    if (!isListModel(node))
        return;

    ModelNode newParentNode = newPropertyParent.parentModelNode();
    ModelNode oldParentNode = oldPropertyParent.parentModelNode();
    bool added = isCollectionLib(newParentNode);
    bool removed = isCollectionLib(oldParentNode);

    if (!added && !removed)
        return;

    refreshModel();

    if (isCollection(node))
        m_widget->collectionModel()->selectCollection(node);
}

void CollectionView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    // removing the collections lib node
    if (isCollectionLib(removedNode)) {
        m_widget->collectionModel()->setCollections({});
        return;
    }

    if (isCollection(removedNode))
        m_widget->collectionModel()->removeCollection(removedNode);
}

void CollectionView::nodeRemoved([[maybe_unused]] const ModelNode &removedNode,
                                 const NodeAbstractProperty &parentProperty,
                                 [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    if (parentProperty.parentModelNode().id() != Constants::COLLECTION_LIB_ID)
        return;

    m_widget->collectionModel()->updateSelectedCollection(true);
}

void CollectionView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                              [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    for (const VariantProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());
        if (isCollection(node)) {
            if (property.name() == "objectName")
                m_widget->collectionModel()->updateNodeName(node);
            else if (property.name() == "id")
                m_widget->collectionModel()->updateNodeId(node);
        }
    }
}

void CollectionView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                          [[maybe_unused]] const QList<ModelNode> &lastSelectedNodeList)
{
    QList<ModelNode> selectedCollections = Utils::filtered(selectedNodeList, &isCollection);

    // More than one collections are selected. So ignore them
    if (selectedCollections.size() > 1)
        return;

    if (selectedCollections.size() == 1) { // If exactly one collection is selected
        m_widget->collectionModel()->selectCollection(selectedCollections.first());
        return;
    }

    // If no collection is selected, check the elements
    QList<ModelNode> selectedElements = Utils::filtered(selectedNodeList, &isCollectionElement);
    if (selectedElements.size()) {
        const ModelNode parentElement = selectedElements.first().parentProperty().parentModelNode();
        bool haveSameParent = Utils::allOf(selectedElements, [&parentElement](const ModelNode &element) {
            return element.parentProperty().parentModelNode() == parentElement;
        });
        if (haveSameParent)
            m_widget->collectionModel()->selectCollection(parentElement);
    }
}

void CollectionView::addNewCollection(const QString &name)
{
    executeInTransaction(__FUNCTION__, [&] {
        ensureCollectionLibraryNode();
        ModelNode collectionLib = collectionLibraryNode();
        if (!collectionLib.isValid())
            return;

        NodeMetaInfo listModelMetaInfo = model()->qtQmlModelsListModelMetaInfo();
        ModelNode collectionNode = createModelNode(listModelMetaInfo.typeName(),
                                                   listModelMetaInfo.majorVersion(),
                                                   listModelMetaInfo.minorVersion());
        QString collectionName = name.isEmpty() ? "Collection" : name;
        renameCollection(collectionNode, collectionName);

        QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_PROPERTY_ADDED);

        auto headersProperty = collectionNode.variantProperty("headers");
        headersProperty.setDynamicTypeNameAndValue("string", {});

        collectionLib.defaultNodeListProperty().reparentHere(collectionNode);
    });
}

void CollectionView::refreshModel()
{
    if (!model())
        return;

    ModelNode collectionLib = modelNodeForId(Constants::COLLECTION_LIB_ID);
    ModelNodes collections;

    if (collectionLib.isValid()) {
        const QList<ModelNode> collectionLibNodes = collectionLib.directSubModelNodes();
        for (const ModelNode &node : collectionLibNodes) {
            if (isCollection(node))
                collections.append(node);
        }
    }

    m_widget->collectionModel()->setCollections(collections);
}

ModelNode CollectionView::getNewCollectionNode(const Collection &collection)
{
    QTC_ASSERT(model(), return {});
    ModelNode collectionNode;
    executeInTransaction(__FUNCTION__, [&] {
        NodeMetaInfo listModelMetaInfo = model()->qtQmlModelsListModelMetaInfo();
        collectionNode = createModelNode(listModelMetaInfo.typeName(),
                                         listModelMetaInfo.majorVersion(),
                                         listModelMetaInfo.minorVersion());
        QString collectionName = collection.name.isEmpty() ? "Collection" : collection.name;
        renameCollection(collectionNode, collectionName);
        QStringList headers;
        for (const DataHeader &header : collection.headers)
            headers.append(header.name);

        QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_PROPERTY_ADDED);

        auto headersProperty = collectionNode.variantProperty("headers");
        headersProperty.setDynamicTypeNameAndValue("string", headers.join(","));

        NodeMetaInfo listElementMetaInfo = model()->qtQmlModelsListElementMetaInfo();
        for (const DataRecord &item : collection.items) {
            ModelNode elementNode = createModelNode(listElementMetaInfo.typeName(),
                                                    listElementMetaInfo.majorVersion(),
                                                    listElementMetaInfo.minorVersion());
            for (const auto &headerMapElement : item.asKeyValueRange()) {
                auto property = elementNode.variantProperty(headerMapElement.first.toLatin1());
                QVariant value = std::visit([](const auto &data)
                                                -> QVariant { return QVariant::fromValue(data); },
                                            headerMapElement.second);
                property.setValue(value);
            }
            collectionNode.defaultNodeListProperty().reparentHere(elementNode);
        }
    });
    return collectionNode;
}

void CollectionView::addLoadedModel(const QList<Collection> &newCollection)
{
    executeInTransaction(__FUNCTION__, [&] {
        ensureCollectionLibraryNode();
        ModelNode collectionLib = collectionLibraryNode();
        if (!collectionLib.isValid())
            return;

        for (const Collection &collection : newCollection) {
            ModelNode collectionNode = getNewCollectionNode(collection);
            collectionLib.defaultNodeListProperty().reparentHere(collectionNode);
        }
    });
}

void CollectionView::renameCollection(ModelNode &collection, const QString &newName)
{
    QTC_ASSERT(collection.isValid(), return);

    QVariant objName = collection.variantProperty("objectName").value();
    if (objName.isValid() && objName.toString() == newName)
        return;

    executeInTransaction(__FUNCTION__, [&] {
        collection.setIdWithRefactoring(model()->generateIdFromName(newName, "collection"));

        VariantProperty objNameProp = collection.variantProperty("objectName");
        objNameProp.setValue(newName);
    });
}

void CollectionView::ensureCollectionLibraryNode()
{
    ModelNode collectionLib = modelNodeForId(Constants::COLLECTION_LIB_ID);
    if (collectionLib.isValid()
        || (!rootModelNode().metaInfo().isQtQuick3DNode()
            && !rootModelNode().metaInfo().isQtQuickItem())) {
        return;
    }

    executeInTransaction(__FUNCTION__, [&] {
    // Create collection library node
#ifdef QDS_USE_PROJECTSTORAGE
        TypeName nodeTypeName = rootModelNode().metaInfo().isQtQuick3DNode() ? "Node" : "Item";
        collectionLib = createModelNode(nodeTypeName, -1, -1);
#else
            auto nodeType = rootModelNode().metaInfo().isQtQuick3DNode()
                                ? model()->qtQuick3DNodeMetaInfo()
                                : model()->qtQuickItemMetaInfo();
            collectionLib = createModelNode(nodeType.typeName(),
                                            nodeType.majorVersion(),
                                            nodeType.minorVersion());
#endif
        collectionLib.setIdWithoutRefactoring(Constants::COLLECTION_LIB_ID);
        rootModelNode().defaultNodeListProperty().reparentHere(collectionLib);
    });
}

ModelNode CollectionView::collectionLibraryNode()
{
    return modelNodeForId(Constants::COLLECTION_LIB_ID);
}
} // namespace QmlDesigner
