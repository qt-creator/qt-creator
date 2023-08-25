// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include "modelnode.h"

#include <QAbstractListModel>
#include <QHash>

QT_BEGIN_NAMESPACE
class QJsonArray;
QT_END_NAMESPACE

namespace QmlDesigner {

class CollectionModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int selectedIndex MEMBER m_selectedIndex NOTIFY selectedIndexChanged)

public:
    enum Roles { IdRole = Qt::UserRole + 1, NameRole, SelectedRole };

    explicit CollectionModel();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index,
                         const QVariant &value,
                         int role = Qt::EditRole) override;

    Q_INVOKABLE virtual bool removeRows(int row,
                                        int count = 1,
                                        const QModelIndex &parent = QModelIndex()) override;

    virtual QHash<int, QByteArray> roleNames() const override;

    void setCollections(const ModelNodes &collections);
    void removeCollection(const ModelNode &node);
    int collectionIndex(const ModelNode &node) const;
    void selectCollection(const ModelNode &node);

    Q_INVOKABLE bool isEmpty() const;
    Q_INVOKABLE void selectCollectionIndex(int idx, bool selectAtLeastOne = false);
    Q_INVOKABLE void deselect();
    Q_INVOKABLE void updateSelectedCollection(bool selectAtLeastOne = false);
    void updateNodeName(const ModelNode &node);
    void updateNodeId(const ModelNode &node);

signals:
    void selectedIndexChanged(int idx);
    void renameCollectionTriggered(const QmlDesigner::ModelNode &collection, const QString &newName);
    void addNewCollectionTriggered();
    void isEmptyChanged(bool);

private:
    void setSelectedIndex(int idx);
    void updateEmpty();

    using Super = QAbstractListModel;

    QModelIndex indexOfNode(const ModelNode &node) const;
    ModelNodes m_collections;
    QHash<qint32, int> m_collectionsIndexHash; // internalId -> index
    int m_selectedIndex = -1;
    bool m_isEmpty = true;
};

} // namespace QmlDesigner
