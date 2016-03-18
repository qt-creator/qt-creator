/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <modelnode.h>
#include <nodemetainfo.h>

#include <QStandardItem>
#include <QStandardItemModel>
#include <QPointer>

namespace QmlDesigner {

class Model;
class AbstractView;
class ModelNode;

#ifdef _LOCK_ITEMS_
    struct ItemRow {
        ItemRow()
            : idItem(0), lockItem(0), visibilityItem(0) {}
        ItemRow(QStandardItem *id, QStandardItem *lock, QStandardItem *visibility, const QMap<QString, QStandardItem *> &properties)
            : idItem(id), lockItem(lock), visibilityItem(visibility), propertyItems(properties) {}

        QList<QStandardItem*> toList() const {
            return QList<QStandardItem*>() << idItem << lockItem << visibilityItem;
        }

        QStandardItem *idItem;
        QStandardItem *lockItem;
        QStandardItem *visibilityItem;
        QMap<QString, QStandardItem *> propertyItems;
    };
#else
    struct ItemRow {
        ItemRow()
            : idItem(0), visibilityItem(0) {}
        ItemRow(QStandardItem *id, QStandardItem *exportI, QStandardItem *visibility, const QMap<QString, QStandardItem *> &properties)
            : idItem(id), exportItem(exportI), visibilityItem(visibility), propertyItems(properties) {}

        QList<QStandardItem*> toList() const {
            return QList<QStandardItem*>() << idItem << exportItem << visibilityItem;
        }

        QStandardItem *idItem;
        QStandardItem *exportItem;
        QStandardItem *visibilityItem;
        QMap<QString, QStandardItem *> propertyItems;
    };
#endif

class NavigatorTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum {
         InternalIdRole = Qt::UserRole
        ,InvisibleRole = Qt::UserRole + 1
        ,SimplifiedTypeNameRole = Qt::UserRole + 2
        ,ErrorRole = Qt::UserRole + 3
    };


    NavigatorTreeModel(QObject *parent = 0);
    ~NavigatorTreeModel();

    Qt::DropActions supportedDropActions() const;
    Qt::DropActions supportedDragActions() const;

    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex &parent);

    void setView(AbstractView *view);
    void clearView();

    QModelIndex indexForNode(const ModelNode &node) const;
    ModelNode nodeForIndex(const QModelIndex &index) const;
    bool hasNodeForIndex(const QModelIndex &index) const;

    bool isInTree(const ModelNode &node) const;
    bool isNodeInvisible(const QModelIndex &index) const;
    bool isNodeInvisible(const ModelNode &node) const;

    void addSubTree(const ModelNode &node);
    void removeSubTree(const ModelNode &node);
    void updateItemRow(const ModelNode &node);

    void setId(const QModelIndex &index, const QString &id);
    void setExported(const QModelIndex &index, bool exported);
    void setVisible(const QModelIndex &index, bool visible);

    void openContextMenu(const QPoint &p);

    ItemRow itemRowForNode(const ModelNode &node);
    bool blockItemChangedSignal(bool block);

private slots:
    void handleChangedItem(QStandardItem *item);

private:
    ItemRow createItemRow(const ModelNode &node);
    void updateItemRow(const ModelNode &node, ItemRow row);
    void handleChangedIdItem(QStandardItem *idItem, ModelNode &modelNode);
    void handleChangedExportItem(QStandardItem *exportItem, ModelNode &modelNode);
    void handleChangedVisibilityItem(QStandardItem *visibilityItem, ModelNode &modelNode);

    void moveNodesInteractive(NodeAbstractProperty &parentProperty, const QList<ModelNode> &modelNodes, int targetIndex);
    void handleInternalDrop(const QMimeData *mimeData, int rowNumber, const QModelIndex &dropModelIndex);
    void handleItemLibraryItemDrop(const QMimeData *mimeData, int rowNumber, const QModelIndex &dropModelIndex);
    void handleItemLibraryImageDrop(const QMimeData *mimeData, int rowNumber, const QModelIndex &dropModelIndex);

private:
    QHash<ModelNode, ItemRow> m_nodeItemHash;
    QPointer<AbstractView> m_view;

    bool m_blockItemChangedSignal;
};

} // namespace QmlDesigner
