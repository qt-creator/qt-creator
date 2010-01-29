/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef NAVIGATORTREEMODEL_H
#define NAVIGATORTREEMODEL_H

#include <modelnode.h>
#include <nodemetainfo.h>

#include <QStandardItem>
#include <QStandardItemModel>

namespace QmlDesigner {

class Model;
class AbstractView;
class ModelNode;

class NavigatorTreeModel : public QStandardItemModel
{
    Q_OBJECT

#ifdef _LOCK_ITEMS_
    struct ItemRow {
        ItemRow()
            : idItem(0), lockItem(0), visibilityItem(0) {}
        ItemRow(QStandardItem *id, QStandardItem *lock, QStandardItem *visibility)
            : idItem(id), lockItem(lock), visibilityItem(visibility) {}

        QList<QStandardItem*> toList() const {
            return QList<QStandardItem*>() << idItem << lockItem << visibilityItem;
        }

        QStandardItem *idItem;
        QStandardItem *lockItem;
        QStandardItem *visibilityItem;
    };
#else
    struct ItemRow {
        ItemRow()
            : idItem(0), visibilityItem(0) {}
        ItemRow(QStandardItem *id, QStandardItem *visibility)
            : idItem(id), visibilityItem(visibility) {}

        QList<QStandardItem*> toList() const {
            return QList<QStandardItem*>() << idItem << visibilityItem;
        }

        QStandardItem *idItem;
        QStandardItem *visibilityItem;
    };
#endif

public:
    NavigatorTreeModel(QObject *parent = 0);
    ~NavigatorTreeModel();

    Qt::DropActions supportedDropActions() const;

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

    bool isInTree(const ModelNode &node) const;

    void addSubTree(const ModelNode &node);
    void removeSubTree(const ModelNode &node);
    void updateItemRow(const ModelNode &node);
    void updateItemRowOrder(const ModelNode &node);

private slots:
    void handleChangedItem(QStandardItem *item);

private:
    bool containsNodeHash(uint hash) const;
    ModelNode nodeForHash(uint hash) const;

    bool containsNode(const ModelNode &node) const;
    ItemRow itemRowForNode(const ModelNode &node);

    ItemRow createItemRow(const ModelNode &node);
    void updateItemRow(const ModelNode &node, ItemRow row);

    QList<ModelNode> modelNodeChildren(const ModelNode &parentNode);

    bool blockItemChangedSignal(bool block);

private:
    QHash<ModelNode, ItemRow> m_nodeItemHash;
    QHash<uint, ModelNode> m_nodeHash;
    QWeakPointer<AbstractView> m_view;

    bool m_blockItemChangedSignal;
};

} // namespace QmlDesigner

#endif // NAVIGATORTREEMODEL_H
