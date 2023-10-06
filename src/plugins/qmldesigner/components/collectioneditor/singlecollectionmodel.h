// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "collectiondetails.h"

#include <QAbstractTableModel>
#include <QHash>

namespace QmlDesigner {

class ModelNode;

class SingleCollectionModel : public QAbstractTableModel
{
    Q_OBJECT

    Q_PROPERTY(QString collectionName MEMBER m_collectionName NOTIFY collectionNameChanged)
    Q_PROPERTY(int selectedColumn READ selectedColumn WRITE selectColumn NOTIFY selectedColumnChanged)

public:
    enum DataRoles { SelectedRole = Qt::UserRole + 1 };

    explicit SingleCollectionModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool setHeaderData(int section,
                       Qt::Orientation orientation,
                       const QVariant &value,
                       int role = Qt::EditRole) override;
    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    int selectedColumn() const;

    Q_INVOKABLE bool isPropertyAvailable(const QString &name);
    Q_INVOKABLE bool addColumn(int column, const QString &name);
    Q_INVOKABLE bool selectColumn(int section);
    Q_INVOKABLE bool renameColumn(int section, const QString &newValue);

    void loadCollection(const ModelNode &sourceNode, const QString &collection);

signals:
    void collectionNameChanged(const QString &collectionName);
    void selectedColumnChanged(int);

private:
    void switchToCollection(const CollectionReference &collection);
    void closeCollectionIfSaved(const CollectionReference &collection);
    void closeCurrentCollectionIfSaved();
    void setCollectionName(const QString &newCollectionName);
    void loadJsonCollection(const QString &source, const QString &collection);
    void loadCsvCollection(const QString &source, const QString &collectionName);
    bool saveCollectionAsJson(const QString &collection, const QJsonArray &content, const QString &source);

    QHash<CollectionReference, CollectionDetails> m_openedCollections;
    CollectionDetails m_currentCollection;
    int m_selectedColumn = -1;

    QString m_collectionName;
};

} // namespace QmlDesigner
