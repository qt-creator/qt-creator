/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef NAVIGATORTREEMODEL_H
#define NAVIGATORTREEMODEL_H

#include <modelnode.h>
#include <nodemetainfo.h>

#include <QStandardItem>
#include <QStandardItemModel>

namespace QmlDesigner {

class Model;
class QmlModelView;
class ModelNode;

class NavigatorTreeModel : public QStandardItemModel
{
    Q_OBJECT

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
        ItemRow(QStandardItem *id, QStandardItem *visibility, const QMap<QString, QStandardItem *> &properties)
            : idItem(id), visibilityItem(visibility), propertyItems(properties) {}

        QList<QStandardItem*> toList() const {
            return QList<QStandardItem*>() << idItem << visibilityItem;
        }

        QStandardItem *idItem;
        QStandardItem *visibilityItem;
        QMap<QString, QStandardItem *> propertyItems;
    };
#endif

    static const int NavigatorRole;

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

    void setView(QmlModelView *view);
    void clearView();

    QModelIndex indexForNode(const ModelNode &node) const;
    ModelNode nodeForIndex(const QModelIndex &index) const;

    bool isInTree(const ModelNode &node) const;
    void propagateInvisible(const ModelNode &node, const bool &invisible);
    bool isNodeInvisible(const QModelIndex &index) const;
    bool isNodeInvisible(const ModelNode &node) const;

    void addSubTree(const ModelNode &node);
    void removeSubTree(const ModelNode &node);
    void updateItemRow(const ModelNode &node);
    void updateItemRowOrder(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);

    void setId(const QModelIndex &index, const QString &id);
    void setVisible(const QModelIndex &index, bool visible);

    void openContextMenu(const QPoint &p);
private slots:
    void handleChangedItem(QStandardItem *item);

private:
    bool containsNodeHash(uint hash) const;
    ModelNode nodeForHash(uint hash) const;

    bool containsNode(const ModelNode &node) const;
    ItemRow itemRowForNode(const ModelNode &node);

    ItemRow createItemRow(const ModelNode &node);
    void updateItemRow(const ModelNode &node, ItemRow row);

    void moveNodesInteractive(NodeAbstractProperty parentProperty, const QList<ModelNode> &modelNodes, int targetIndex);

    QList<ModelNode> modelNodeChildren(const ModelNode &parentNode);

    QString qmlTypeInQtContainer(const QString &qtContainerType) const;
    QStringList visibleProperties(const ModelNode &node) const;

    bool blockItemChangedSignal(bool block);

private:
    QHash<ModelNode, ItemRow> m_nodeItemHash;
    QHash<uint, ModelNode> m_nodeHash;
    QWeakPointer<QmlModelView> m_view;

    bool m_blockItemChangedSignal;

    QStringList m_hiddenProperties;
};

} // namespace QmlDesigner

#endif // NAVIGATORTREEMODEL_H
