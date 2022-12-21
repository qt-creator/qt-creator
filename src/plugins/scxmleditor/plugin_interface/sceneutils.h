// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "baseitem.h"

#include <QGraphicsScene>

QT_FORWARD_DECLARE_CLASS(QGraphicsItem)

namespace ScxmlEditor {

namespace PluginInterface {

class GraphicsScene;
class ConnectableItem;
class InitialStateItem;
class BaseItem;
class ScxmlTag;

/**
 * Namespace SceneUtils includes some usable function to manipulate the data of the items.
 */
namespace SceneUtils {

ScxmlTag *addChild(ScxmlTag *tag, const QVariantMap &data, GraphicsScene *scene);
ScxmlTag *addSibling(ScxmlTag *tag, const QVariantMap &data, GraphicsScene *scene);
ScxmlTag *addNewTag(ScxmlTag *parent, TagType type, GraphicsScene *scene);
ConnectableItem *createItem(ItemType type, const QPointF &pos = QPointF());
ConnectableItem *createItemByTagType(TagType type, const QPointF &pos = QPointF());
ScxmlTag *createTag(ItemType type, ScxmlDocument *document);
bool canDrop(int parentType, int childType);
QVector<ScxmlTag*> findCopyTags(const QVector<BaseItem*> &items, QPointF &minPos);
QVector<ScxmlTag*> findRemovedTags(const QVector<BaseItem*> &items);
void layout(const QList<QGraphicsItem*> &items);
bool isSomeSelected(QGraphicsItem *item);
void moveTop(BaseItem *item, GraphicsScene *scene);

bool isChild(const QGraphicsItem *parent, const QGraphicsItem *child);

template <class T>
bool hasSiblingStates(T *item)
{
    if (item) {
        QList<QGraphicsItem*> children;
        QGraphicsItem *parentItem = item->parentItem();
        if (parentItem) {
            children = parentItem->childItems();
        } else if (item->scene()) {
            const QList<QGraphicsItem *> items = item->scene()->items();
            for (QGraphicsItem *it : items)
                if (!it->parentItem())
                    children << it;
        }

        for (QGraphicsItem *it : std::as_const(children))
            if (it != item && it->type() == item->type())
                return true;
    }

    return false;
}

} // namespace SceneUtils
} // namespace PluginInterface
} // namespace ScxmlEditor
