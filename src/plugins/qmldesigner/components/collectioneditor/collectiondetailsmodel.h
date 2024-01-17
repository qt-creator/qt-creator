// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "collectiondetails.h"

#include <QAbstractTableModel>
#include <QHash>

namespace QmlDesigner {

class ModelNode;

class CollectionDetailsModel : public QAbstractTableModel
{
    Q_OBJECT

    Q_PROPERTY(QString collectionName MEMBER m_collectionName NOTIFY collectionNameChanged)
    Q_PROPERTY(int selectedColumn READ selectedColumn WRITE selectColumn NOTIFY selectedColumnChanged)
    Q_PROPERTY(int selectedRow READ selectedRow WRITE selectRow NOTIFY selectedRowChanged)
    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)

public:
    enum DataRoles { SelectedRole = Qt::UserRole + 1, DataTypeRole, ColumnDataTypeRole, DataTypeWarningRole };
    explicit CollectionDetailsModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool setHeaderData(int section,
                       Qt::Orientation orientation,
                       const QVariant &value,
                       int role = Qt::EditRole) override;
    bool insertRows(int row, int count, const QModelIndex &parent = {}) override;
    bool removeColumns(int column, int count, const QModelIndex &parent = {}) override;
    bool removeRows(int row, int count, const QModelIndex &parent = {}) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    CollectionDetails::DataType propertyDataType(int column) const;

    int selectedColumn() const;
    int selectedRow() const;
    Q_INVOKABLE QString propertyName(int column) const;
    Q_INVOKABLE QString propertyType(int column) const;

    Q_INVOKABLE bool isPropertyAvailable(const QString &name);
    Q_INVOKABLE bool addColumn(int column, const QString &name, const QString &propertyType = {});
    Q_INVOKABLE bool selectColumn(int section);
    Q_INVOKABLE bool renameColumn(int section, const QString &newValue);
    Q_INVOKABLE bool setPropertyType(int column, const QString &newValue);
    Q_INVOKABLE bool selectRow(int row);
    Q_INVOKABLE void deselectAll();
    Q_INVOKABLE QString warningToString(DataTypeWarning::Warning warning) const;
    static Q_INVOKABLE QStringList typesList();

    void loadCollection(const ModelNode &sourceNode, const QString &collection);

    Q_INVOKABLE bool saveDataStoreCollections();
    Q_INVOKABLE bool exportCollection(const QUrl &url);

signals:
    void collectionNameChanged(const QString &collectionName);
    void selectedColumnChanged(int);
    void selectedRowChanged(int);
    void isEmptyChanged(bool);

private slots:
    void updateEmpty();

private:
    void switchToCollection(const CollectionReference &collection);
    void closeCollectionIfSaved(const CollectionReference &collection);
    void closeCurrentCollectionIfSaved();
    void setCollectionName(const QString &newCollectionName);
    void loadJsonCollection(const QString &source, const QString &collection);
    void loadCsvCollection(const QString &source, const QString &collectionName);
    QVariant variantFromString(const QString &value);

    QHash<CollectionReference, CollectionDetails> m_openedCollections;
    CollectionDetails m_currentCollection;
    bool m_isEmpty = true;
    int m_selectedColumn = -1;
    int m_selectedRow = -1;

    QString m_collectionName;
};

} // namespace QmlDesigner
