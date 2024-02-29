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
    Q_PROPERTY(QString selectedCollectionName
                    READ selectedCollectionName
                    WRITE selectCollectionName
                    NOTIFY selectedCollectionNameChanged)

public:
    enum Roles { IdRole = Qt::UserRole + 1, NameRole, SelectedRole };

    explicit CollectionListModel();
    QHash<int, QByteArray> roleNames() const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    bool removeRows(int row, int count, const QModelIndex &parent = {}) override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void setDataStoreNode(const ModelNode &dataStoreNode = {});

    Q_INVOKABLE int selectedIndex() const;
    Q_INVOKABLE ModelNode sourceNode() const;
    Q_INVOKABLE bool collectionExists(const QString &collectionName) const;
    Q_INVOKABLE QStringList collections() const;
    Q_INVOKABLE QString getUniqueCollectionName(const QString &baseName = {}) const;

    void selectCollectionIndex(int idx, bool selectAtLeastOne = false);
    void selectCollectionName(QString collectionName, bool selectAtLeastOne = false);
    QString collectionNameAt(int idx) const;
    QString selectedCollectionName() const;

    void update();
    bool addCollection(const QString &collectionName, const QJsonObject &localCollection);

signals:
    void selectedIndexChanged(int idx);
    void isEmptyChanged(bool);
    void collectionNameChanged(const QString &oldName, const QString &newName);
    void collectionNamesChanged(const QStringList &collectionNames);
    void collectionsRemoved(const QStringList &names);
    void collectionAdded(const QString &name);
    void selectedCollectionNameChanged(const QString &selectedCollectionName);
    void warning(const QString &title, const QString &body);

private:
    void setSelectedIndex(int idx);
    bool removeCollectionsFromDataStore(const QStringList &removedCollections, QString &error) const;
    bool renameCollectionInDataStore(const QString &oldName, const QString &newName, QString &error);
    bool addCollectionToDataStore(const QString &collectionName,
                                  const QJsonObject &localCollection,
                                  QString &errorString) const;

    void updateEmpty();
    void updateSelectedCollectionName();

    using Super = QAbstractListModel;
    int m_selectedIndex = -1;
    bool m_isEmpty = false;
    ModelNode m_dataStoreNode;
    QString m_selectedCollectionName;
    QStringList m_data;
};

} // namespace QmlDesigner
