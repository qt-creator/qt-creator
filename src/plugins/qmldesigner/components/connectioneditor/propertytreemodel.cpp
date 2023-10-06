// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertytreemodel.h"
#include "connectionview.h"

#include <bindingproperty.h>
#include <designeralgorithm.h>
#include <exception.h>
#include <model/modelutils.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <rewritertransaction.h>
#include <rewriterview.h>
#include <signalhandlerproperty.h>
#include <variantproperty.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QByteArrayView>
#include <QMessageBox>
#include <QTimer>

namespace QmlDesigner {

const std::vector<PropertyName> blockListProperties = {"children",
                                                       "data",
                                                       "childrenRect",
                                                       "icon",
                                                       "left",
                                                       "top",
                                                       "bottom",
                                                       "right",
                                                       "locale",
                                                       "objectName",
                                                       "transitions",
                                                       "states",
                                                       "resources",
                                                       "data",
                                                       "transformOrigin",
                                                       "transformOriginPoint",
                                                       "verticalCenter",
                                                       "horizontalCenter",
                                                       "anchors.bottom",
                                                       "anchors.top",
                                                       "anchors.left",
                                                       "anchors.right",
                                                       "anchors.fill",
                                                       "anchors.horizontalCenter",
                                                       "anchors.verticalCenter",
                                                       "anchors.centerIn",
                                                       "transform",
                                                       "visibleChildren"};

const std::vector<PropertyName> blockListSlots = {"childAt",
                                                  "contains",
                                                  "destroy",
                                                  "dumpItemTree",
                                                  "ensurePolished",
                                                  "grabToImage",
                                                  "mapFromGlobal",
                                                  "mapFromItem",
                                                  "mapToGlobal",
                                                  "mapToItem",
                                                  "valueAt",
                                                  "toString",
                                                  "getText",
                                                  "inputMethodQuery",
                                                  "positionAt",
                                                  "positionToRectangle",
                                                  "isRightToLeft"

};

const std::vector<PropertyName> priorityListSignals = {"clicked",
                                                       "doubleClicked",
                                                       "pressed",
                                                       "released",
                                                       "toggled",
                                                       "valueModified",
                                                       "valueChanged",
                                                       "checkedChanged",
                                                       "moved",
                                                       "accepted",
                                                       "editingFinished",
                                                       "entered",
                                                       "exited",
                                                       "canceled",
                                                       "triggered",
                                                       "pressAndHold",
                                                       "started",
                                                       "stopped",
                                                       "finished"
                                                       "stateChanged",
                                                       "enabledChanged",
                                                       "visibleChanged",
                                                       "opacityChanged",
                                                       "rotationChanged"};

const std::vector<PropertyName> priorityListProperties = {"opacity",
                                                          "checked",
                                                          "hovered",
                                                          "visible",
                                                          "value",
                                                          "down",
                                                          "x",
                                                          "y",
                                                          "width",
                                                          "height",
                                                          "from",
                                                          "to",
                                                          "rotation",
                                                          "color",
                                                          "scale",
                                                          "state",
                                                          "enabled",
                                                          "z",
                                                          "text",
                                                          "pressed",
                                                          "containsMouse",
                                                          "down",
                                                          "clip",
                                                          "parent",
                                                          "radius",
                                                          "smooth",
                                                          "true",
                                                          "focus",
                                                          "border.width",
                                                          "border.color",
                                                          "eulerRotation.x",
                                                          "eulerRotation.y",
                                                          "eulerRotation.z",
                                                          "scale.x",
                                                          "scale.y",
                                                          "scale.z",
                                                          "position.x",
                                                          "position.y",
                                                          "position.z"};

const std::vector<PropertyName> priorityListSlots = {"toggle",
                                                     "increase",
                                                     "decrease",
                                                     "clear",
                                                     "complete",
                                                     "pause",
                                                     "restart",
                                                     "resume",
                                                     "start",
                                                     "stop",
                                                     "forceActiveFocus"};

std::vector<PropertyName> properityLists()
{
    std::vector<PropertyName> result;

    result.insert(result.end(), priorityListSignals.begin(), priorityListSignals.end());
    result.insert(result.end(), priorityListProperties.begin(), priorityListProperties.end());
    result.insert(result.end(), priorityListSlots.begin(), priorityListSlots.end());

    return result;
}

PropertyTreeModel::PropertyTreeModel(ConnectionView *parent)
    : QAbstractItemModel(parent), m_connectionView(parent)
{}

void PropertyTreeModel::resetModel()
{
    beginResetModel();

    m_sortedAndFilteredPropertyNamesSignalsSlots.clear();

    m_indexCache.clear();
    m_indexHash.clear();
    m_indexCount = 0;
    m_nodeList = allModelNodesWithIdsSortedByDisplayName();

    if (!m_filter.isEmpty()) { //This could be a bit slow for large projects, but we have to check everynode to "hide" it.
        m_nodeList = Utils::filtered(m_nodeList, [this](const ModelNode &node) {
            return node.displayName().contains(m_filter)
                   || !sortedAndFilteredPropertyNamesSignalsSlots(node).empty();
        });
    }

    endResetModel();
}

QString stripQualification(const QString &string)
{
    return string.split(".").last();
}
QVariant PropertyTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    auto internalId = index.internalId();

    if (role == InternalIdRole)
        return internalId;

    if (role == RowRole)
        return index.row();

    if (role == PropertyNameRole || role == PropertyPriorityRole || role == ExpressionRole
        || role == ChildCountRole) {
        if (internalId == internalRootIndex) {
            if (role == PropertyNameRole)
                return "--root item--";
            if (role == ChildCountRole)
                return rowCount(index);

            return {};
        }

        DataCacheItem item = m_indexHash[index.internalId()];

        if (role == ChildCountRole)
            return rowCount(index);

        if (role == ExpressionRole)
            return QString(item.modelNode.id() + "." + item.propertyName);

        if (role == PropertyNameRole) {
            if (!item.propertyName.isEmpty())
                return stripQualification(QString::fromUtf8(item.propertyName));
            else
                return item.modelNode.displayName();
        }

        static const auto priority = properityLists();
        if (std::find(priority.begin(), priority.end(), item.propertyName) != priority.end())
            return true; // listed priority properties

        auto dynamic = getDynamicProperties(item.modelNode);
        if (std::find(dynamic.begin(), dynamic.end(), item.propertyName) != dynamic.end())
            return true; // dynamic properties have priority

        if (item.propertyName.isEmpty()) {
            return true; // nodes are always shown
        }

        return false;
    }

    return {};
}

Qt::ItemFlags PropertyTreeModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled;
}

QModelIndex PropertyTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    auto internalId = parent.internalId();
    if (!m_connectionView->isAttached())
        return {};

    if (!parent.isValid())
        return createIndex(0, 0, internalRootIndex);

    if (!hasIndex(row, column, parent))
        return {};

    if (internalId == internalRootIndex) { //root level model node
        const ModelNode modelNode = m_nodeList[row];
        return ensureModelIndex(modelNode, row);
    }

    //property

    QTC_ASSERT(internalId != internalRootIndex, return {});

    DataCacheItem item = m_indexHash[internalId];
    QTC_ASSERT(item.modelNode.isValid(), return {});

    if (!item.propertyName.isEmpty()) {
        // "." aka sub property
        auto properties = sortedDotPropertyNamesSignalsSlots(item.modelNode.metaInfo(),
                                                             item.propertyName);
        PropertyName propertyName = properties[row];
        return ensureModelIndex(item.modelNode, propertyName, row);
    }

    auto properties = sortedAndFilteredPropertyNamesSignalsSlots(item.modelNode);

    PropertyName propertyName = properties[row];

    return ensureModelIndex(item.modelNode, propertyName, row);
}

QModelIndex PropertyTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    auto internalId = index.internalId();

    if (internalId == internalRootIndex)
        return {};

    QTC_ASSERT(internalId < m_indexCount, return {});

    const DataCacheItem item = m_indexHash[index.internalId()];

    // no property means the parent is the root item
    if (item.propertyName.isEmpty())
        return createIndex(0, 0, internalRootIndex);

    if (item.propertyName.contains(".")) {
        auto list = item.propertyName.split('.');
        DataCacheItem parent;
        parent.modelNode = item.modelNode;
        parent.propertyName = list.first();
        if (auto iter = m_indexCache.find(parent); iter != m_indexCache.end()) {
            const auto vector = sortedAndFilteredPropertyNamesSignalsSlots(item.modelNode);
            QList<PropertyName> list(vector.begin(), vector.end());
            int row = list.indexOf(parent.propertyName);
            return createIndex(row, 0, iter->internalIndex);
        }
    }

    // find the parent

    int row = m_nodeList.indexOf(item.modelNode);
    return ensureModelIndex(item.modelNode, row);
}

QPersistentModelIndex PropertyTreeModel::indexForInternalIdAndRow(quintptr internalId, int row)
{
    return createIndex(row, 0, internalId);
}

int PropertyTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!m_connectionView->isAttached() || parent.column() > 0)
        return 0;

    if (!parent.isValid())
        return 1; //m_nodeList.size();

    auto internalId = parent.internalId();

    if (internalId == internalRootIndex)
        return m_nodeList.size();

    QTC_ASSERT(internalId < m_indexCount, return 0);

    DataCacheItem item = m_indexHash[internalId];
    if (!item.propertyName.isEmpty()) {
        if (item.modelNode.metaInfo().property(item.propertyName).isPointer()) {
            auto subProbs = sortedDotPropertyNamesSignalsSlots(item.modelNode.metaInfo(),
                                                               item.propertyName);

            return static_cast<int>(subProbs.size());
        }

        return 0;
    }

    return static_cast<int>(sortedAndFilteredPropertyNamesSignalsSlots(item.modelNode).size());
}
int PropertyTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

void PropertyTreeModel::setIncludeDotPropertiesOnFirstLevel(bool b)
{
    m_includeDotPropertiesOnFirstLevel = b;
}

void PropertyTreeModel::setPropertyType(PropertyTypes type)
{
    if (m_type == type)
        return;

    m_type = type;
    resetModel();
}

void PropertyTreeModel::setFilter(const QString &filter)
{
    if (m_filter == filter)
        return;

    m_filter = filter;
    resetModel();
}

QList<ModelNode> PropertyTreeModel::nodeList() const
{
    return m_nodeList;
}

const std::vector<PropertyName> PropertyTreeModel::getProperties(const ModelNode &modelNode) const
{
    return sortedAndFilteredPropertyNamesSignalsSlots(modelNode);
}

ModelNode PropertyTreeModel::getModelNodeForId(const QString &id) const
{
    if (!m_connectionView->isAttached())
        return {};

    return m_connectionView->modelNodeForId(id);
}

QModelIndex PropertyTreeModel::ensureModelIndex(const ModelNode &node, int row) const
{
    DataCacheItem item;
    item.modelNode = node;

    auto iter = m_indexCache.find(item);
    if (iter != m_indexCache.end())
        return createIndex(row, 0, iter->internalIndex);

    item.internalIndex = m_indexCount;
    m_indexCount++;
    m_indexHash.push_back(item);
    m_indexCache.insert(item);

    return createIndex(row, 0, item.internalIndex);
}
QModelIndex PropertyTreeModel::ensureModelIndex(const ModelNode &node,
                                                const PropertyName &name,
                                                int row) const
{
    DataCacheItem item;
    item.modelNode = node;
    item.propertyName = name;

    auto iter = m_indexCache.find(item);
    if (iter != m_indexCache.end())
        return createIndex(row, 0, iter->internalIndex);

    item.internalIndex = m_indexCount;
    m_indexCount++;
    m_indexHash.push_back(item);
    m_indexCache.insert(item);

    return createIndex(row, 0, item.internalIndex);
}

void PropertyTreeModel::testModel()
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << rowCount({});

    QModelIndex rootIndex = index(0, 0);

    qDebug() << rowCount(rootIndex);
    QModelIndex firstItem = index(0, 0, rootIndex);

    qDebug() << "fi" << data(firstItem, Qt::DisplayRole) << rowCount(firstItem);

    firstItem = index(1, 0, rootIndex);
    qDebug() << "fi" << data(firstItem, Qt::DisplayRole) << rowCount(firstItem);

    firstItem = index(2, 0, rootIndex);
    qDebug() << "fi" << data(firstItem, Qt::DisplayRole) << rowCount(firstItem);

    QModelIndex firstProperty = index(0, 0, firstItem);

    qDebug() << "fp" << data(firstProperty, Qt::DisplayRole) << rowCount(firstProperty);

    qDebug() << m_indexCount << m_indexHash.size() << m_indexCache.size();
}

const QList<ModelNode> PropertyTreeModel::allModelNodesWithIdsSortedByDisplayName() const
{
    if (!m_connectionView->isAttached())
        return {};

    return Utils::sorted(ModelUtils::allModelNodesWithId(m_connectionView),
                         [](const ModelNode &lhs, const ModelNode &rhs) {
                             return lhs.displayName() < rhs.displayName();
                         });
}

const std::vector<PropertyName> PropertyTreeModel::sortedAndFilteredPropertyNamesSignalsSlots(
    const ModelNode &modelNode) const
{
    std::vector<PropertyName> returnValue;

    returnValue = m_sortedAndFilteredPropertyNamesSignalsSlots.value(modelNode);

    if (!returnValue.empty())
        return returnValue;

    if (m_type == SignalType) {
        returnValue = sortedAndFilteredSignalNames(modelNode.metaInfo());
    } else if (m_type == SlotType) {
        returnValue = sortedAndFilteredSlotNames(modelNode.metaInfo());
    } else {
        auto list = sortedAndFilteredPropertyNames(modelNode.metaInfo());
        returnValue = getDynamicProperties(modelNode);
        std::move(list.begin(), list.end(), std::back_inserter(returnValue));
    }

    if (m_filter.isEmpty() || modelNode.displayName().contains(m_filter))
        return returnValue;

    const auto filtered = Utils::filtered(returnValue, [this](const PropertyName &name) {
        return name.contains(m_filter.toUtf8()) || name == m_filter.toUtf8();
    });

    m_sortedAndFilteredPropertyNamesSignalsSlots.insert(modelNode, filtered);

    return filtered;
}

const std::vector<PropertyName> PropertyTreeModel::getDynamicProperties(
    const ModelNode &modelNode) const
{
    QList<PropertyName> list = Utils::transform(modelNode.dynamicProperties(),
                                                [](const AbstractProperty &property) {
                                                    return property.name();
                                                });

    QList<PropertyName> filtered
        = Utils::filtered(list, [this, modelNode](const PropertyName &propertyName) {
              PropertyName propertyType = modelNode.property(propertyName).dynamicTypeName();
              switch (m_type) {
              case AllTypes:
                  return true;
              case NumberType:
                  return propertyType == "float" || propertyType == "double"
                         || propertyType == "int";
              case StringType:
                  return propertyType == "string";
              case UrlType:
                  return propertyType == "url";
              case ColorType:
                  return propertyType == "color";
              case BoolType:
                  return propertyType == "bool";
              default:
                  break;
              }
              return true;
          });

    return Utils::sorted(std::vector<PropertyName>(filtered.begin(), filtered.end()));
}

const std::vector<PropertyName> PropertyTreeModel::sortedAndFilteredPropertyNames(
    const NodeMetaInfo &metaInfo, bool recursive) const
{
    auto filtered = Utils::filtered(metaInfo.properties(),
                                    [this, recursive](const PropertyMetaInfo &metaInfo) {
                                        // if (!metaInfo.isWritable()) - lhs/rhs

                                        const PropertyName name = metaInfo.name();

                                        if (!m_includeDotPropertiesOnFirstLevel
                                            && name.contains("."))
                                            return false;

                                        return filterProperty(name, metaInfo, recursive);
                                    });

    auto sorted = Utils::sorted(
        Utils::transform(filtered, [](const PropertyMetaInfo &metaInfo) -> PropertyName {
            return metaInfo.name();
        }));

    std::set<PropertyName> set(std::make_move_iterator(sorted.begin()),
                               std::make_move_iterator(sorted.end()));

    auto checkedPriorityList = Utils::filtered(priorityListProperties,
                                               [&set](const PropertyName &name) {
                                                   auto it = set.find(name);
                                                   const bool b = it != set.end();
                                                   if (b)
                                                       set.erase(it);

                                                   return b;
                                               });

    //const int priorityLength = checkedPriorityList.size(); We eventually require this to get the prioproperties

    std::vector<PropertyName> final(set.begin(), set.end());

    std::move(final.begin(), final.end(), std::back_inserter(checkedPriorityList));

    return checkedPriorityList;
}

const std::vector<PropertyName> PropertyTreeModel::sortedAndFilteredSignalNames(
    const NodeMetaInfo &metaInfo, bool recursive) const
{
    Q_UNUSED(recursive);

    auto filtered = Utils::filtered(metaInfo.signalNames(), [](const PropertyName &name) {
        if (std::find(priorityListSignals.cbegin(), priorityListSignals.cend(), name)
            != priorityListSignals.cend())
            return true;

        if (name.endsWith("Changed")) //option?
            return false;

        return true;
    });

    auto sorted = Utils::sorted(filtered);

    std::set<PropertyName> set(std::make_move_iterator(sorted.begin()),
                               std::make_move_iterator(sorted.end()));

    auto checkedPriorityList = Utils::filtered(priorityListSignals,
                                               [&set](const PropertyName &name) {
                                                   auto it = set.find(name);
                                                   const bool b = it != set.end();
                                                   if (b)
                                                       set.erase(it);

                                                   return b;
                                               });

    //const int priorityLength = checkedPriorityList.size(); We eventually require this to get the prioproperties

    std::vector<PropertyName> finalPropertyList(set.begin(), set.end());

    std::move(finalPropertyList.begin(),
              finalPropertyList.end(),
              std::back_inserter(checkedPriorityList));

    return checkedPriorityList;
}

const std::vector<PropertyName> PropertyTreeModel::sortedAndFilteredSlotNames(
    const NodeMetaInfo &metaInfo, bool recursive) const
{
    Q_UNUSED(recursive);

    auto priorityList = priorityListSlots;
    auto filtered = Utils::filtered(metaInfo.slotNames(), [priorityList](const PropertyName &name) {
        if (std::find(priorityListSlots.begin(), priorityListSlots.end(), name)
            != priorityListSlots.end())
            return true;

        if (name.startsWith("_"))
            return false;
        if (name.startsWith("q_"))
            return false;

        if (name.endsWith("Changed")) //option?
            return false;

        if (std::find(blockListSlots.begin(), blockListSlots.end(), name) != blockListSlots.end())
            return false;

        return true;
    });

    auto sorted = Utils::sorted(filtered);

    std::set<PropertyName> set(std::make_move_iterator(sorted.begin()),
                               std::make_move_iterator(sorted.end()));

    auto checkedPriorityList = Utils::filtered(priorityListSlots, [&set](const PropertyName &name) {
        auto it = set.find(name);
        const bool b = it != set.end();
        if (b)
            set.erase(it);

        return b;
    });

    //const int priorityLength = checkedPriorityList.size(); We eventually require this to get the prioproperties

    std::vector<PropertyName> final(set.begin(), set.end());

    std::move(final.begin(), final.end(), std::back_inserter(checkedPriorityList));

    return checkedPriorityList;
}

const std::vector<PropertyName> PropertyTreeModel::sortedDotPropertyNames(
    const NodeMetaInfo &metaInfo, const PropertyName &propertyName) const
{
    const PropertyName prefix = propertyName + '.';
    auto filtered = Utils::filtered(metaInfo.properties(),
                                    [this, prefix](const PropertyMetaInfo &metaInfo) {
                                        const PropertyName name = metaInfo.name();

                                        if (!name.startsWith(prefix))
                                            return false;

                                        return filterProperty(name, metaInfo, true);
                                    });

    auto sorted = Utils::sorted(
        Utils::transform(filtered, [](const PropertyMetaInfo &metaInfo) -> PropertyName {
            return metaInfo.name();
        }));

    if (sorted.size() == 0 && metaInfo.property(propertyName).propertyType().isQtObject()) {
        return Utils::transform(sortedAndFilteredPropertyNames(metaInfo.property(propertyName)
                                                                   .propertyType(),
                                                               true),
                                [propertyName](const PropertyName &name) -> PropertyName {
                                    return propertyName + "." + name;
                                });
    }

    return sorted;
}

const std::vector<PropertyName> PropertyTreeModel::sortedDotPropertySignals(
    const NodeMetaInfo &metaInfo, const PropertyName &propertyName) const
{
    return Utils::transform(sortedAndFilteredSignalNames(metaInfo.property(propertyName)
                                                             .propertyType(),
                                                         true),
                            [propertyName](const PropertyName &name) -> PropertyName {
                                return propertyName + "." + name;
                            });
}

const std::vector<PropertyName> PropertyTreeModel::sortedDotPropertySlots(
    const NodeMetaInfo &metaInfo, const PropertyName &propertyName) const
{
    return Utils::transform(sortedAndFilteredSlotNames(metaInfo.property(propertyName).propertyType(),
                                                       true),
                            [propertyName](const PropertyName &name) -> PropertyName {
                                return propertyName + "." + name;
                            });
}

const std::vector<PropertyName> PropertyTreeModel::sortedDotPropertyNamesSignalsSlots(
    const NodeMetaInfo &metaInfo, const PropertyName &propertyName) const
{
    if (m_type == SignalType) {
        return sortedDotPropertySignals(metaInfo, propertyName);
    } else if (m_type == SlotType) {
        return sortedDotPropertySlots(metaInfo, propertyName);
    } else {
        return sortedDotPropertyNames(metaInfo, propertyName);
    }
}

bool PropertyTreeModel::filterProperty(const PropertyName &name,
                                       const PropertyMetaInfo &metaInfo,
                                       bool recursive) const
{
    if (std::find(blockListProperties.begin(), blockListProperties.end(), name)
        != blockListProperties.end())
        return false;

    const NodeMetaInfo propertyType = metaInfo.propertyType();

    if (m_includeDotPropertiesOnFirstLevel && metaInfo.isPointer())
        return false;

    //We want to keep sub items with matching properties
    if (!recursive && metaInfo.isPointer()
        && sortedAndFilteredPropertyNames(propertyType, true).size() > 0)
        return true;

    //TODO no type relaxation atm...
    switch (m_type) {
    case AllTypes:
        return true;
    case NumberType:
        return propertyType.isNumber();
    case StringType:
        return propertyType.isString();
    case UrlType:
        return propertyType.isUrl();
        //return propertyType.isString() || propertyType.isUrl();
    case ColorType:
        return propertyType.isColor();
        //return propertyType.isString() || propertyType.isColor();
    case BoolType:
        return propertyType.isBool();
    default:
        break;
    }
    return true;
}

QHash<int, QByteArray> PropertyTreeModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{PropertyNameRole, "propertyName"},
                                            {PropertyPriorityRole, "hasPriority"},
                                            {ExpressionRole, "expression"},
                                            {ChildCountRole, "childCount"}};

    return roleNames;
}

PropertyListProxyModel::PropertyListProxyModel(PropertyTreeModel *parent)
    : QAbstractListModel(), m_treeModel(parent)
{}

void PropertyListProxyModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void PropertyListProxyModel::setRowAndInternalId(int row, quintptr internalId)
{
    QTC_ASSERT(m_treeModel, return );

    if (internalId == internalRootIndex)
        m_parentIndex = m_treeModel->index(0, 0);
    else
        m_parentIndex = m_treeModel->index(row, 0, m_parentIndex);
    //m_parentIndex = m_treeModel->indexForInternalIdAndRow(internalId, row);

    resetModel();
}

int PropertyListProxyModel::rowCount(const QModelIndex &) const
{
    QTC_ASSERT(m_treeModel, return 0);
    return m_treeModel->rowCount(m_parentIndex);
}

QVariant PropertyListProxyModel::data(const QModelIndex &index, int role) const
{
    QTC_ASSERT(m_treeModel, return 0);

    auto treeIndex = m_treeModel->index(index.row(), 0, m_parentIndex);

    return m_treeModel->data(treeIndex, role);
}

QHash<int, QByteArray> PropertyListProxyModel::roleNames() const
{
    return m_treeModel->roleNames();
}

void PropertyListProxyModel::goInto(int row)
{
    setRowAndInternalId(row, 0); //m_parentIndex.internalId());

    emit parentNameChanged();
}

void PropertyListProxyModel::goUp()
{
    if (m_parentIndex.internalId() == internalRootIndex)
        return;

    m_parentIndex = m_treeModel->parent(m_parentIndex);
    resetModel();

    emit parentNameChanged();
}

void PropertyListProxyModel::reset()
{
    setRowAndInternalId(0, internalRootIndex); // TODO ???

    emit parentNameChanged();
}

QString PropertyListProxyModel::parentName() const
{
    if (!m_treeModel->parent(m_parentIndex).isValid())
        return {};

    return m_treeModel->data(m_parentIndex, PropertyTreeModel::UserRoles::PropertyNameRole).toString();
}

PropertyTreeModelDelegate::PropertyTreeModelDelegate(ConnectionView *parent) : m_model(parent)
{
    connect(&m_nameCombboBox, &StudioQmlComboBoxBackend::activated, this, [this]() {
        handleNameChanged();
    });
    connect(&m_idCombboBox, &StudioQmlComboBoxBackend::activated, this, [this]() {
        handleIdChanged();
    });

    m_model.setIncludeDotPropertiesOnFirstLevel(true);
}

void PropertyTreeModelDelegate::setPropertyType(PropertyTreeModel::PropertyTypes type)
{
    m_model.setPropertyType(type);
    setupNameComboBox(m_idCombboBox.currentText(), m_nameCombboBox.currentText(), nullptr);
}

void PropertyTreeModelDelegate::setup(const QString &id, const QString &name, bool *nameExists)
{
    m_model.resetModel();
    QStringList idLists = Utils::transform(m_model.nodeList(),
                                           [](const ModelNode &node) { return node.id(); });

    if (!idLists.contains(id))
        idLists.prepend(id);

    m_idCombboBox.setModel(idLists);
    m_idCombboBox.setCurrentText(id);
    setupNameComboBox(id, name, nameExists);
}

void PropertyTreeModelDelegate::setupNameComboBox(const QString &id,
                                                  const QString &name,
                                                  bool *nameExists)
{
    const auto modelNode = m_model.getModelNodeForId(id);
    //m_nameCombboBox
    std::vector<QString> nameVector = Utils::transform(m_model.getProperties(modelNode),
                                                       [](const PropertyName &name) {
                                                           return QString::fromUtf8(name);
                                                       });
    QStringList nameList;
    nameList.reserve(Utils::ssize(nameVector));
    std::copy(nameVector.begin(), nameVector.end(), std::back_inserter(nameList));

    if (!nameList.contains(name)) {
        if (!nameExists)
            nameList.prepend(name);
        else
            *nameExists = false;
    }

    m_nameCombboBox.setModel(nameList);
    m_nameCombboBox.setCurrentText(name);
}

QString PropertyTreeModelDelegate::id() const
{
    return m_idCombboBox.currentText();
}

QString PropertyTreeModelDelegate::name() const
{
    return m_nameCombboBox.currentText();
}

void PropertyTreeModelDelegate::handleNameChanged()
{
    const auto id = m_idCombboBox.currentText();
    const auto name = m_nameCombboBox.currentText();
    setup(id, name);

    emit commitData();

    // commit data
}

NodeMetaInfo PropertyTreeModelDelegate::propertyMetaInfo() const
{
    const auto modelNode = m_model.getModelNodeForId(m_idCombboBox.currentText());
    if (modelNode.isValid())
        return modelNode.metaInfo().property(m_nameCombboBox.currentText().toUtf8()).propertyType();
    return {};
}

void PropertyTreeModelDelegate::handleIdChanged()
{
    const auto id = m_idCombboBox.currentText();
    const auto name = m_nameCombboBox.currentText();
    bool exists = true;
    setup(id, name, &exists);
    if (!exists) {
        auto model = m_nameCombboBox.model();
        model.prepend("---");
        m_nameCombboBox.setModel(model);
        m_nameCombboBox.setCurrentText("---");
        //We do not commit invalid name
    } else {
        emit commitData();
        //commit data
    }
}

StudioQmlComboBoxBackend *PropertyTreeModelDelegate::nameCombboBox()
{
    return &m_nameCombboBox;
}

StudioQmlComboBoxBackend *PropertyTreeModelDelegate::idCombboBox()
{
    return &m_idCombboBox;
}

} // namespace QmlDesigner
