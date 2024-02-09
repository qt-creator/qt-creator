// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QHash>

#include "modelnode.h"

namespace QmlDesigner {

class CollectionListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int selectedIndex MEMBER m_selectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(QString sourceType MEMBER m_sourceType CONSTANT)

public:
    enum Roles { IdRole = Qt::UserRole + 1, NameRole, SourceRole, SelectedRole, CollectionsRole };

    explicit CollectionListModel(const ModelNode &sourceModel);
    QHash<int, QByteArray> roleNames() const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    bool removeRows(int row, int count, const QModelIndex &parent = {}) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void resetModelData(const QStringList &collectionsList);

    Q_INVOKABLE int selectedIndex() const;
    Q_INVOKABLE ModelNode sourceNode() const;
    Q_INVOKABLE QString sourceAddress() const;
    Q_INVOKABLE bool contains(const QString &collectionName) const;
    Q_INVOKABLE QStringList collections() const;

    void selectCollectionIndex(int idx, bool selectAtLeastOne = false);
    void selectCollectionName(const QString &collectionName);
    QString collectionNameAt(int idx) const;
    void addCollection(const QString &collectionName);

signals:
    void selectedIndexChanged(int idx);
    void isEmptyChanged(bool);
    void collectionNameChanged(const QString &oldName, const QString &newName);
    void collectionsRemoved(const QStringList &names);
    void collectionAdded(const QString &name);

private:
    void setSelectedIndex(int idx);

    void updateEmpty();

    using Super = QAbstractListModel;
    int m_selectedIndex = -1;
    bool m_isEmpty = false;
    const ModelNode m_sourceNode;
    const QString m_sourceType;

    QStringList m_data;
};

} // namespace QmlDesigner
