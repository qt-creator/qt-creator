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
            foreach (QGraphicsItem *it, item->scene()->items()) {
                if (!it->parentItem())
                    children << it;
            }
        }

        foreach (QGraphicsItem *it, children) {
            if (it != item && it->type() == item->type()) {
                return true;
            }
        }
    }

    return false;
}

} // namespace SceneUtils
} // namespace PluginInterface
} // namespace ScxmlEditor
