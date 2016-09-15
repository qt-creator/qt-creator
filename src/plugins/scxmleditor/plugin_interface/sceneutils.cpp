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

#include "sceneutils.h"
#include "connectableitem.h"
#include "finalstateitem.h"
#include "graphicsscene.h"
#include "historyitem.h"
#include "initialstateitem.h"
#include "parallelitem.h"
#include "scxmldocument.h"
#include "scxmleditorconstants.h"
#include "scxmltag.h"
#include "scxmltagutils.h"
#include "stateitem.h"
#include "transitionitem.h"

#include <QGuiApplication>
#include <QGraphicsScene>
#include <QtMath>

namespace ScxmlEditor {

namespace PluginInterface {

namespace SceneUtils {

ConnectableItem *createItem(ItemType type, const QPointF &pos)
{
    switch (type) {
    case InitialStateType:
        return new InitialStateItem(pos);
    case FinalStateType:
        return new FinalStateItem(pos);
    case StateType:
        return new StateItem(pos);
    case HistoryType:
        return new HistoryItem(pos);
    case ParallelType:
        return new ParallelItem(pos);
    default:
        break;
    }

    return nullptr;
}

ConnectableItem *createItemByTagType(TagType type, const QPointF &pos)
{
    switch (type) {
    case Initial:
        return createItem(InitialStateType, pos);
    case Final:
        return createItem(FinalStateType, pos);
    case State:
        return createItem(StateType, pos);
    case History:
        return createItem(HistoryType, pos);
    case Parallel:
        return createItem(ParallelType, pos);
    default:
        return nullptr;
    }
}

ScxmlTag *createTag(ItemType type, ScxmlDocument *document)
{
    TagType t = UnknownTag;
    switch (type) {
    case InitialStateType:
        t = Initial;
        break;
    case FinalStateType:
        t = Final;
        break;
    case HistoryType:
        t = History;
        break;
    case StateType:
        t = State;
        break;
    case ParallelType:
        t = Parallel;
        break;
    default:
        break;
    }

    if (t != UnknownTag)
        return new ScxmlTag(t, document);

    return nullptr;
}

bool canDrop(int parentType, int childType)
{
    switch (parentType) {
    case StateType: {
        switch (childType) {
        case InitialStateType:
        case FinalStateType:
        case StateType:
        case ParallelType:
        case HistoryType:
            return true;
        default:
            return false;
        }
    }
    case ParallelType: {
        switch (childType) {
        case StateType:
        case ParallelType:
        case HistoryType:
            return true;
        default:
            return false;
        }
    }
    default:
        return false;
    }
}

QVector<ScxmlTag*> findCopyTags(const QVector<BaseItem*> &items, QPointF &minPos)
{
    QPointF pp(0, 0);
    QVector<ScxmlTag*> tags;
    foreach (BaseItem *it, items) {
        if (it->type() >= InitialStateType && it->isSelected()) {
            BaseItem *parent = it->parentBaseItem();
            BaseItem *lastSelectedParent = it;
            while (parent) {
                if (parent->isSelected())
                    lastSelectedParent = parent;
                parent = parent->parentBaseItem();
            }

            if (!tags.contains(lastSelectedParent->tag())) {
                QPointF p = lastSelectedParent->sceneBoundingRect().topLeft();
                if (tags.isEmpty()) {
                    pp = p;
                } else {
                    pp.setX(qMin(pp.x(), p.x()));
                    pp.setY(qMin(pp.y(), p.y()));
                }
                lastSelectedParent->updateUIProperties();
                tags << lastSelectedParent->tag();
            }
        }
    }

    minPos = pp;
    return tags;
}

QVector<ScxmlTag*> findRemovedTags(const QVector<BaseItem*> &items)
{
    // Find right tags
    QVector<ScxmlTag*> tags;
    foreach (BaseItem *it, items) {
        if (it->isSelected()) {
            // Find the last selected parent
            BaseItem *parent = it->parentBaseItem();
            BaseItem *lastSelectedParent = it;
            while (parent != 0) {
                if (parent->isSelected())
                    lastSelectedParent = parent;
                parent = parent->parentBaseItem();
            }

            // Add tag to the list
            if (!tags.contains(lastSelectedParent->tag()))
                tags << lastSelectedParent->tag();
        }
    }

    return tags;
}

void layout(const QList<QGraphicsItem*> &items)
{
    // Collect child items
    QList<ConnectableItem*> childItems;
    ConnectableItem *initialItem = nullptr;
    ConnectableItem *finalItem = nullptr;
    foreach (QGraphicsItem *item, items) {
        auto connectableItem = qgraphicsitem_cast<ConnectableItem*>(item);
        if (connectableItem) {
            if (connectableItem->type() == InitialStateType)
                initialItem = connectableItem;
            else if (connectableItem->type() == FinalStateType)
                finalItem = connectableItem;
            else if (connectableItem->type() >= HistoryType)
                childItems << connectableItem;
        }
    }

    // Change initial-item position
    ConnectableItem *firstItem = nullptr;
    if (initialItem && initialItem->outputTransitionCount() == 1) {
        firstItem = initialItem->outputTransitions()[0]->connectedItem(initialItem);
        int index = childItems.indexOf(firstItem);
        if (index > 0)
            childItems.swap(index, 0);
    }

    // Search final-item
    ConnectableItem *lastItem = nullptr;
    if (finalItem && finalItem->inputTransitionCount() > 0)
        lastItem = finalItem->inputTransitions()[0]->connectedItem(finalItem);

    int startAngle = qrand() % 2 == 0 ? 180 : 90;
    int startDistance = 40 + childItems.count() * 10;
    if (childItems.count() > 0) {
        // Init position of the items
        int angleDiff = 360 / (childItems.count() + 1);
        for (int i = 0; i < childItems.count(); ++i) {
            int angle = startAngle + i * angleDiff;
            QLineF line = QLineF::fromPolar(startDistance, angle);
            childItems[i]->setPos(line.p2());
        }

        // Then grow the distances so much that there is no any overlapped items
        for (int i = 0; i < childItems.count(); ++i) {
            int angle = startAngle + i * angleDiff;
            QLineF line = QLineF::fromPolar(startDistance, angle);
            ConnectableItem *movingItem = childItems[i];
            QRectF r2 = movingItem->boundingRect();
            r2.moveTopLeft(r2.topLeft() + movingItem->pos());

            bool intersects = true;

            while (intersects) {
                intersects = false;
                for (int j = 0; j < childItems.count(); ++j) {
                    if (j != i) {
                        QRectF r1 = childItems[j]->boundingRect();
                        r1.moveTopLeft(r1.topLeft() + childItems[j]->pos());

                        if (r2.intersects(r1)) {
                            intersects = true;
                            break;
                        }
                    }
                }

                if (intersects) {
                    line.setLength(line.length() + 50);
                    movingItem->setPos(line.p2());
                    r2 = movingItem->boundingRect();
                    r2.moveTopLeft(r2.topLeft() + movingItem->pos());
                }
            }
        }

        // Then decrease the distances so much as possible
        for (int i = 0; i < childItems.count(); ++i) {
            ConnectableItem *movingItem = childItems[i];
            QPointF p = movingItem->pos();
            QLineF line(QPointF(0, 0), p);
            QRectF r2 = movingItem->boundingRect();
            r2.moveTopLeft(r2.topLeft() + p);

            bool cont = true;
            while (cont) {
                bool intersects = false;
                for (int j = 0; j < childItems.count(); ++j) {
                    if (j != i) {
                        QRectF r1 = childItems[j]->boundingRect();
                        r1.moveTopLeft(r1.topLeft() + childItems[j]->pos());

                        if (r2.intersects(r1)) {
                            intersects = true;
                            cont = false;
                            line.setLength(line.length() + 20);
                            movingItem->setPos(line.p2());
                            r2 = movingItem->boundingRect();
                            r2.moveTopLeft(r2.topLeft() + movingItem->pos());
                            break;
                        }
                    }
                }

                if (!intersects) {
                    line.setLength(line.length() - 20);
                    movingItem->setPos(line.p2());
                    r2 = movingItem->boundingRect();
                    r2.moveTopLeft(r2.topLeft() + movingItem->pos());
                    if (line.length() < 100)
                        cont = false;
                }
            }
        }

        // Finally set initial and final positions
        foreach (ConnectableItem *item, childItems) {
            if (item == firstItem)
                initialItem->setPos(firstItem->pos() + firstItem->boundingRect().topLeft() - QPointF(50, 50));
            else if (item == lastItem) {
                int angle = startAngle + childItems.indexOf(item) * angleDiff;
                QLineF line = QLineF::fromPolar(qMax(lastItem->boundingRect().width() / 2, lastItem->boundingRect().height() / 2) + 20, angle);
                finalItem->setPos(lastItem->pos() + lastItem->boundingRect().center() + line.p2());
            }
        }
    }
}

bool isChild(const QGraphicsItem *parent, const QGraphicsItem *child)
{
    while (child != 0) {
        if (parent == child)
            return true;
        child = child->parentItem();
    }

    return false;
}

bool isSomeSelected(QGraphicsItem *item)
{
    while (item != 0) {
        if (item->isSelected())
            return true;
        item = item->parentItem();
    }

    return false;
}

void moveTop(BaseItem *item, GraphicsScene *scene)
{
    if (item && scene) {
        // Make the current item to the topmost of all
        QGraphicsItem *parentItem = item->parentItem();
        QList<QGraphicsItem*> children;
        if (parentItem)
            children = parentItem->childItems();
        else
            children = scene->sceneItems(Qt::DescendingOrder);

        // Remove unnecessary items
        for (int i = children.count(); i--;) {
            if (children[i]->type() < InitialStateType)
                children.takeAt(i);
        }

        // Change stack order
        const int ind = parentItem ? children.indexOf(item) : 0;
        for (int i = ind; i < children.count(); ++i)
            children[i]->stackBefore(item);
    }
}

ScxmlTag *addNewTag(ScxmlTag *parent, TagType type, GraphicsScene *scene)
{
    if (parent) {
        ScxmlDocument *document = parent->document();
        auto newTag = new ScxmlTag(type, document);
        document->addTag(parent, newTag);

        if (scene)
            scene->unselectAll();

        document->setCurrentTag(newTag);
        return newTag;
    }

    return nullptr;
}

ScxmlTag *addChild(ScxmlTag *tag, const QVariantMap &data, GraphicsScene *scene)
{
    TagType newTagType = (TagType)data.value(Constants::C_SCXMLTAG_TAGTYPE, 0).toInt();
    TagType subMenuTagType = (TagType)data.value(Constants::C_SCXMLTAG_PARENTTAG, 0).toInt();
    if (newTagType >= UnknownTag) {
        // Check if we must create or add submenu
        if (subMenuTagType > UnknownTag && subMenuTagType != tag->tagType()) {
            // Check if submenu-tag is already available
            ScxmlTag *subMenuTag = TagUtils::findChild(tag, subMenuTagType);
            if (subMenuTag)
                return addNewTag(subMenuTag, newTagType, scene);
            else {
                // If dont, create new submenutag and add new child
                subMenuTag = addNewTag(tag, subMenuTagType, scene);
                return addNewTag(subMenuTag, newTagType, scene);
            }
        } else
            return addNewTag(tag, newTagType, scene);
    }

    return nullptr;
}

} // namespace SceneUtils
} // namespace PluginInterface
} // namespace ScxmlEditor
