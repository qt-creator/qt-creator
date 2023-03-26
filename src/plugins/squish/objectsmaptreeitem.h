// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "propertytreeitem.h"

#include <utils/treemodel.h>

#include <QSortFilterProxyModel>

namespace Squish {
namespace Internal {

class ObjectsMapModel;

class ObjectsMapTreeItem : public Utils::TreeItem
{
public:
    explicit ObjectsMapTreeItem(const QString &name, Qt::ItemFlags flags = Qt::ItemIsEnabled);
    ~ObjectsMapTreeItem() override;

    void initPropertyModelConnections(ObjectsMapModel *objMapModel);

    QVariant data(int column, int role) const override;
    bool setData(int column, const QVariant &data, int role) override;
    Qt::ItemFlags flags(int column) const override;
    bool isValid() const { return m_propertiesContent.isEmpty(); }
    void setPropertiesContent(const QByteArray &content);
    QByteArray propertiesContent() const { return m_propertiesContent; }
    QByteArray propertiesToByteArray() const;
    QString parentName() const;
    PropertyList properties() const;
    PropertiesModel *propertiesModel() const { return m_propertiesModel; }

    static const QChar COLON;

private:
    bool parseProperties(const QByteArray &properties);
    PropertiesModel *m_propertiesModel;
    QString m_name;
    QByteArray m_propertiesContent; // for invalid properties content
    Qt::ItemFlags m_flags = Qt::NoItemFlags;
};

class ObjectsMapModel : public Utils::TreeModel<ObjectsMapTreeItem>
{
    Q_OBJECT
public:
    ObjectsMapModel(QObject *parent = nullptr);
    bool setData(const QModelIndex &idx, const QVariant &data, int role) override;
    void addNewObject(ObjectsMapTreeItem *item);
    ObjectsMapTreeItem *findItem(const QString &search) const;

    void removeSymbolicNameResetReferences(const QString &symbolicName, const QString &newRef);
    void removeSymbolicNameInvalidateReferences(const QModelIndex &idx);
    void removeSymbolicName(const QModelIndex &idx);

    QStringList allSymbolicNames() const;
    void changeRootItem(ObjectsMapTreeItem *newRoot);

signals:
    void requestSelection(const QModelIndex &idx);
    void modelChanged();
    void nameChanged(const QString &old, const QString &modified);
    void propertyChanged(
        ObjectsMapTreeItem *item, const QString &old, const QString &modified, int row, int column);
    void propertyRemoved(ObjectsMapTreeItem *item, const Property &property);
    void propertyAdded(ObjectsMapTreeItem *item);

private:
    void onNameChanged(const QString &old, const QString &modified);
    void onPropertyChanged(
        ObjectsMapTreeItem *item, const QString &old, const QString &modified, int row, int column);
    void onPropertyRemoved(ObjectsMapTreeItem *item, const Property &property);
};

class ObjectsMapSortFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    ObjectsMapSortFilterModel(Utils::TreeModel<ObjectsMapTreeItem> *sourceModel,
                              QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

} // namespace Internal
} // namespace Squish
