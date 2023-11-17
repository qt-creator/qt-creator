// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "modelnode.h"

#include <QAbstractListModel>
#include <QHash>
#include <QSortFilterProxyModel>

namespace QmlDesigner {

class CollectionJsonSourceFilterModel;
class CollectionListModel;

class CollectionSourceModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int selectedIndex MEMBER m_selectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        NodeRole,
        CollectionTypeRole,
        SourceRole,
        SelectedRole,
        CollectionsRole
    };

    explicit CollectionSourceModel(QObject *parent = nullptr);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index,
                         const QVariant &value,
                         int role = Qt::EditRole) override;

    Q_INVOKABLE virtual bool removeRows(int row,
                                        int count = 1,
                                        const QModelIndex &parent = QModelIndex()) override;

    virtual QHash<int, QByteArray> roleNames() const override;

    void setSources(const ModelNodes &sources);
    void removeSource(const ModelNode &node);
    int sourceIndex(const ModelNode &node) const;
    void addSource(const ModelNode &node);
    void selectSource(const ModelNode &node);

    bool collectionExists(const ModelNode &node, const QString &collectionName) const;
    bool addCollectionToSource(const ModelNode &node,
                               const QString &collectionName,
                               QString *errorString = nullptr);

    ModelNode sourceNodeAt(int idx);
    CollectionListModel *selectedCollectionList();

    Q_INVOKABLE void selectSourceIndex(int idx, bool selectAtLeastOne = false);
    Q_INVOKABLE void deselect();
    Q_INVOKABLE void updateSelectedSource(bool selectAtLeastOne = false);
    Q_INVOKABLE bool collectionExists(const QVariant &node, const QString &collectionName) const;

    void updateNodeName(const ModelNode &node);
    void updateNodeSource(const ModelNode &node);

signals:
    void selectedIndexChanged(int idx);
    void collectionSelected(const ModelNode &sourceNode, const QString &collectionName);
    void isEmptyChanged(bool);
    void warning(const QString &title, const QString &body);

private slots:
    void onSelectedCollectionChanged(int collectionIndex);
    void onCollectionNameChanged(const QString &oldName, const QString &newName);
    void onCollectionsRemoved(const QStringList &removedCollections);

private:
    void setSelectedIndex(int idx);
    void updateEmpty();
    void updateCollectionList(QModelIndex index);
    void registerCollection(const QSharedPointer<CollectionListModel> &collection);
    QModelIndex indexOfNode(const ModelNode &node) const;

    using Super = QAbstractListModel;

    ModelNodes m_collectionSources;
    QHash<qint32, int> m_sourceIndexHash; // internalId -> index
    QList<QSharedPointer<CollectionListModel>> m_collectionList;
    QPointer<CollectionListModel> m_previousSelectedList;
    int m_selectedIndex = -1;
    bool m_isEmpty = true;
};

class CollectionJsonSourceFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    static void registerDeclarativeType();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &) const override;
};

} // namespace QmlDesigner
