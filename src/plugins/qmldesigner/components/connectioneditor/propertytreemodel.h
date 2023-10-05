// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "propertymetainfo.h"
#include <modelnode.h>

#include <studioquickwidget.h>
#include <QAbstractItemModel>
#include <QAbstractListModel>

#include <set>
#include <vector>

namespace QmlDesigner {

inline constexpr quintptr internalRootIndex = std::numeric_limits<quintptr>::max();

class AbstractProperty;
class ModelNode;
class BindingProperty;
class SignalHandlerProperty;
class VariantProperty;

class ConnectionView;

class PropertyTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum UserRoles {
        PropertyNameRole = Qt::DisplayRole,
        PropertyPriorityRole = Qt::UserRole + 1,
        ExpressionRole,
        ChildCountRole,
        RowRole,
        InternalIdRole
    };

    enum PropertyTypes {
        AllTypes,
        NumberType,
        StringType,
        ColorType,
        SignalType,
        SlotType,
        UrlType,
        BoolType
    };

    PropertyTreeModel(ConnectionView *parent = nullptr);

    void resetModel();

    QVariant data(const QModelIndex &index, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    QPersistentModelIndex indexForInternalIdAndRow(quintptr internalId, int row);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void setIncludeDotPropertiesOnFirstLevel(bool b);

    struct DataCacheItem
    {
        ModelNode modelNode;
        PropertyName propertyName;
        std::size_t internalIndex = internalRootIndex;
    };

    void setPropertyType(PropertyTypes type);
    Q_INVOKABLE void setFilter(const QString &filter);

    QList<ModelNode> nodeList() const;

    const std::vector<PropertyName> getProperties(const ModelNode &modelNode) const;
    ModelNode getModelNodeForId(const QString &id) const;

    QHash<int, QByteArray> roleNames() const override;

private:
    QModelIndex ensureModelIndex(const ModelNode &node, int row) const;
    QModelIndex ensureModelIndex(const ModelNode &node, const PropertyName &name, int row) const;
    void testModel();
    const QList<ModelNode> allModelNodesWithIdsSortedByDisplayName() const;
    const std::vector<PropertyName> sortedAndFilteredPropertyNamesSignalsSlots(
        const ModelNode &modelNode) const;

    const std::vector<PropertyName> getDynamicProperties(const ModelNode &modelNode) const;
    const std::vector<PropertyName> sortedAndFilteredPropertyNames(const NodeMetaInfo &metaInfo,
                                                                   bool recursive = false) const;

    const std::vector<PropertyName> sortedAndFilteredSignalNames(const NodeMetaInfo &metaInfo,
                                                                 bool recursive = false) const;

    const std::vector<PropertyName> sortedAndFilteredSlotNames(const NodeMetaInfo &metaInfo,
                                                               bool recursive = false) const;

    const std::vector<PropertyName> sortedDotPropertyNames(const NodeMetaInfo &metaInfo,
                                                           const PropertyName &propertyName) const;

    const std::vector<PropertyName> sortedDotPropertySignals(const NodeMetaInfo &metaInfo,
                                                             const PropertyName &propertyName) const;

    const std::vector<PropertyName> sortedDotPropertySlots(const NodeMetaInfo &metaInfo,
                                                           const PropertyName &propertyName) const;

    const std::vector<PropertyName> sortedDotPropertyNamesSignalsSlots(
        const NodeMetaInfo &metaInfo, const PropertyName &propertyName) const;

    bool filterProperty(const PropertyName &name,
                        const PropertyMetaInfo &metaInfo,
                        bool recursive) const;

    ConnectionView *m_connectionView;

    mutable std::set<DataCacheItem> m_indexCache;
    mutable std::vector<DataCacheItem> m_indexHash;
    mutable std::size_t m_indexCount = 0;
    QList<ModelNode> m_nodeList;
    PropertyTypes m_type = AllTypes;
    QString m_filter;
    mutable QHash<ModelNode, std::vector<PropertyName>> m_sortedAndFilteredPropertyNamesSignalsSlots;
    bool m_includeDotPropertiesOnFirstLevel = false;
};

class PropertyListProxyModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString parentName READ parentName NOTIFY parentNameChanged)

public:
    PropertyListProxyModel(PropertyTreeModel *parent);

    void resetModel();

    void setRowAndInternalId(int row, quintptr internalId);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void goInto(int row);
    Q_INVOKABLE void goUp();
    Q_INVOKABLE void reset();

    QString parentName() const;

signals:
    void parentNameChanged();

private:
    ModelNode m_modelNode;
    PropertyName m_propertyName;
    QPersistentModelIndex m_parentIndex;

    PropertyTreeModel *m_treeModel = nullptr;
};


inline bool operator==(const PropertyTreeModel::DataCacheItem &lhs,
                       const PropertyTreeModel::DataCacheItem &rhs)
{
    return lhs.modelNode == rhs.modelNode && lhs.propertyName == rhs.propertyName;
}

inline bool operator<(const PropertyTreeModel::DataCacheItem &lhs,
                      const PropertyTreeModel::DataCacheItem &rhs)
{
    return (lhs.modelNode.id() + lhs.propertyName) < (rhs.modelNode.id() + rhs.propertyName);
}

class PropertyTreeModelDelegate : public QObject
{
    Q_OBJECT

    Q_PROPERTY(StudioQmlComboBoxBackend *name READ nameCombboBox CONSTANT)
    Q_PROPERTY(StudioQmlComboBoxBackend *id READ idCombboBox CONSTANT)

public:
    explicit PropertyTreeModelDelegate(ConnectionView *parent = nullptr);
    void setPropertyType(PropertyTreeModel::PropertyTypes type);
    void setup(const QString &id, const QString &name, bool *nameExists = nullptr);
    void setupNameComboBox(const QString &id, const QString &name, bool *nameExists);
    QString id() const;
    QString name() const;
    NodeMetaInfo propertyMetaInfo() const;

signals:
    void commitData();

private:
    void handleNameChanged();
    void handleIdChanged();

    StudioQmlComboBoxBackend *nameCombboBox();
    StudioQmlComboBoxBackend *idCombboBox();

    StudioQmlComboBoxBackend m_nameCombboBox;
    StudioQmlComboBoxBackend m_idCombboBox;
    PropertyTreeModel::PropertyTypes m_type;
    PropertyTreeModel m_model;
};

} // namespace QmlDesigner
