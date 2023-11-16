// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QHash>
#include <QStringListModel>

#include "modelnode.h"

namespace QmlDesigner {

class CollectionListModel : public QStringListModel
{
    Q_OBJECT

    Q_PROPERTY(int selectedIndex MEMBER m_selectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(QString sourceType MEMBER m_sourceType CONSTANT)

public:
    enum Roles { IdRole = Qt::UserRole + 1, NameRole, SourceRole, SelectedRole, CollectionsRole };

    explicit CollectionListModel(const ModelNode &sourceModel);
    virtual QHash<int, QByteArray> roleNames() const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    bool removeRows(int row, int count, const QModelIndex &parent = {}) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE int selectedIndex() const;
    Q_INVOKABLE ModelNode sourceNode() const;
    Q_INVOKABLE QString sourceAddress() const;
    Q_INVOKABLE bool contains(const QString &collectionName) const;

    void selectCollectionIndex(int idx, bool selectAtLeastOne = false);
    void selectCollectionName(const QString &collectionName);
    QString collectionNameAt(int idx) const;

signals:
    void selectedIndexChanged(int idx);
    void isEmptyChanged(bool);
    void collectionNameChanged(const QString &oldName, const QString &newName);
    void collectionsRemoved(const QStringList &names);

private:
    void setSelectedIndex(int idx);

    void updateEmpty();

    using Super = QStringListModel;
    int m_selectedIndex = -1;
    bool m_isEmpty = false;
    const ModelNode m_sourceNode;
    const QString m_sourceType;
};

} // namespace QmlDesigner
