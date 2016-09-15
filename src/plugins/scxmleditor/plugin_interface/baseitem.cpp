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

#include "baseitem.h"
#include "actionhandler.h"
#include "actionprovider.h"
#include "graphicsscene.h"
#include "sceneutils.h"
#include "scxmldocument.h"
#include "scxmleditorconstants.h"
#include "scxmltagutils.h"
#include "scxmluifactory.h"
#include "stateitem.h"

#include <QCoreApplication>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QUndoStack>

using namespace ScxmlEditor::PluginInterface;

BaseItem::BaseItem(BaseItem *parent)
    : QGraphicsObject(parent)
{
    setFlag(ItemIsFocusable, true);
    setItemBoundingRect(QRectF(-60, -50, 120, 100));

    m_scene = static_cast<GraphicsScene*>(scene());
    if (m_scene)
        m_scene->addChild(this);
}

BaseItem::~BaseItem()
{
    if (m_scene)
        m_scene->removeChild(this);
}

void BaseItem::checkParentBoundingRect()
{
    BaseItem *parentBaseItem = this->parentBaseItem();
    if (parentBaseItem && type() >= InitialStateType && !parentBaseItem->blockUpdates()) {
        auto parentStateItem = qgraphicsitem_cast<StateItem*>(parentBaseItem);
        if (parentStateItem && (parentStateItem->type() >= StateType))
            parentStateItem->updateBoundingRect();
    }
}

void BaseItem::updatePolygon()
{
    m_polygon.clear();
    m_polygon << m_boundingRect.topLeft()
              << m_boundingRect.topRight()
              << m_boundingRect.bottomRight()
              << m_boundingRect.bottomLeft()
              << m_boundingRect.topLeft();
}

void BaseItem::updateDepth()
{
    const BaseItem *baseItem = parentBaseItem();
    m_depth = baseItem ? baseItem->depth() + 1 : 0;
    update();
}

void BaseItem::setItemBoundingRect(const QRectF &r)
{
    if (m_boundingRect != r) {
        prepareGeometryChange();

        m_boundingRect = r;

        if (!m_blockUpdates)
            checkParentBoundingRect();

        updatePolygon();
        emit geometryChanged();
    }
}

QVariant BaseItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
    case ItemSceneHasChanged:
        m_scene = static_cast<GraphicsScene*>(scene());
        if (m_scene)
            m_scene->addChild(this);
        break;
    case ItemSelectedHasChanged: {
        emit selectedStateChanged(value.toBool());
        if (value.toBool() && tag())
            tag()->document()->setCurrentTag(tag());
        break;
    }
    case ItemPositionChange:
        if (m_scene && type() >= InitialStateType) {
            // Snap to items
            QPointF currentCenter = sceneCenter();
            QPointF targetCenter;
            QPair<bool, bool> res = m_scene->checkSnapToItem(this, currentCenter, targetCenter);

            QPointF val = value.toPointF();
            QPointF change = val - pos();
            if (res.first && qAbs(change.x()) < 12)
                val.setX(pos().x() + targetCenter.x() - currentCenter.x());
            if (res.second && qAbs(change.y()) < 12)
                val.setY(pos().y() + targetCenter.y() - currentCenter.y());

            return val;
        }
        break;
    case ItemParentChange:
    case ItemPositionHasChanged:
    case ItemTransformHasChanged:
        updateUIProperties();
        break;
    default:
        break;
    }

    return QGraphicsObject::itemChange(change, value);
}

void BaseItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        e->accept();
        showContextMenu(e);
    } else {
        QGraphicsObject::mousePressEvent(e);
    }
}

void BaseItem::showContextMenu(QGraphicsSceneMouseEvent *e)
{
    checkSelectionBeforeContextMenu(e);
    QMenu menu;
    createContextMenu(&menu);
    selectedMenuAction(menu.exec(e->screenPos()));
}

void BaseItem::checkSelectionBeforeContextMenu(QGraphicsSceneMouseEvent *e)
{
    if (!isSelected() && !(e->modifiers() & Qt::ControlModifier) && m_scene)
        m_scene->unselectAll();

    if (m_tag)
        m_tag->document()->setCurrentTag(m_tag);
}

void BaseItem::createContextMenu(QMenu *menu)
{
    if (!menu || !tag())
        return;

    if (m_scene) {
        menu->addAction(m_scene->actionHandler()->action(ActionCopy));
        menu->addAction(m_scene->actionHandler()->action(ActionPaste));
        menu->addSeparator();
        const ScxmlUiFactory *uiFactory = m_scene->uiFactory();
        if (uiFactory) {
            auto actionProvider = static_cast<ActionProvider*>(uiFactory->object(Constants::C_UI_FACTORY_OBJECT_ACTIONPROVIDER));
            if (actionProvider) {
                actionProvider->initStateMenu(tag(), menu);
                menu->addSeparator();
            }
        }
    }
    TagUtils::createChildMenu(tag(), menu);
}

void BaseItem::selectedMenuAction(const QAction *action)
{
    if (!action)
        return;

    ScxmlTag *tag = this->tag();

    if (tag) {
        const QVariantMap data = action->data().toMap();
        int actionType = data.value(Constants::C_SCXMLTAG_ACTIONTYPE, -1).toInt();

        switch (actionType) {
        case TagUtils::AddChild: {
            const ScxmlDocument *document = tag->document();
            if (m_scene && document) {
                document->undoStack()->beginMacro(tr("Add child"));
                SceneUtils::addChild(tag, data, m_scene);
                document->undoStack()->endMacro();
            }
            break;
        }
        case TagUtils::Remove:
            postDeleteEvent();
            break;
        default:
            break;
        }
    }
}

void BaseItem::postDeleteEvent()
{
    QCoreApplication::postEvent(scene(), new QKeyEvent(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier));
}

void BaseItem::setEditorInfo(const QString &key, const QString &value, bool block)
{
    if (m_tag && m_tag->editorInfo(key) != value) {
        if (!m_blockUpdates && !block && m_tag->document())
            m_tag->document()->setEditorInfo(m_tag, key, value);
        else
            m_tag->setEditorInfo(key, value);
    }
}

QString BaseItem::editorInfo(const QString &key) const
{
    if (m_tag)
        return m_tag->editorInfo(key);

    return QString();
}

void BaseItem::setTagValue(const QString &key, const QString &value)
{
    if (m_tag && m_tag->attribute(key) != value) {
        if (!m_blockUpdates && m_tag->document())
            m_tag->document()->setValue(m_tag, key, value);
        else
            m_tag->setAttribute(key, value);
    }
}

QString BaseItem::tagValue(const QString &key, bool useNameSpace) const
{
    return m_tag ? m_tag->attribute(key, useNameSpace) : QString();
}

ScxmlTag *BaseItem::tag() const
{
    return m_tag;
}

void BaseItem::setItemSelected(bool sel, bool unselectOthers)
{
    if (sel) {
        if (unselectOthers && m_scene)
            m_scene->unselectAll();

        if (m_tag)
            m_tag->document()->setCurrentTag(m_tag);
    } else {
        setSelected(false);
    }
}

QRectF BaseItem::boundingRect() const
{
    return m_boundingRect;
}

void BaseItem::setTag(ScxmlTag *tag)
{
    m_tag = tag;
}

void BaseItem::init(ScxmlTag *tag, BaseItem *parentItem, bool /*initChildren*/, bool /*blockUpdates*/)
{
    setTag(tag);
    readUISpecifiedProperties(tag);
    setParentItem(parentItem);
}

void BaseItem::setBlockUpdates(bool block)
{
    m_blockUpdates = block;
}

void BaseItem::setHighlight(bool hl)
{
    if (hl != m_highlight) {
        m_highlight = hl;
        update();
    }
}

void BaseItem::setOverlapping(bool ol)
{
    if (ol != m_overlapping) {
        m_overlapping = ol;
        update();
    }
}

bool BaseItem::highlight() const
{
    return m_highlight;
}

bool BaseItem::overlapping() const
{
    return m_overlapping;
}

bool BaseItem::isActiveScene() const
{
    return m_scene ? m_scene->topMostScene() : false;
}

ScxmlUiFactory *BaseItem::uiFactory() const
{
    return m_scene ? m_scene->uiFactory() : 0;
}

GraphicsScene *BaseItem::graphicsScene() const
{
    return m_scene;
}

void BaseItem::setParentItem(BaseItem *item)
{
    QGraphicsObject::setParentItem(item);
    updateColors();
}

void BaseItem::checkWarnings()
{
}

void BaseItem::checkOverlapping()
{
}

void BaseItem::checkVisibility(double scaleFactor)
{
    Q_UNUSED(scaleFactor)
}

void BaseItem::updateUIProperties()
{
}

void BaseItem::readUISpecifiedProperties(const ScxmlTag *tag)
{
    Q_UNUSED(tag)
}

void BaseItem::updateAttributes()
{
}

void BaseItem::updateColors()
{
}

void BaseItem::updateEditorInfo(bool /*allChildren*/)
{
    readUISpecifiedProperties(m_tag);
}

void BaseItem::moveStateBy(qreal /*dx*/, qreal /*dy*/)
{
}

void BaseItem::finalizeCreation()
{
}

void BaseItem::doLayout(int depth)
{
    Q_UNUSED(depth)
}

void BaseItem::checkInitial(bool parent)
{
    Q_UNUSED(parent)
}

bool BaseItem::containsScenePoint(const QPointF &p) const
{
    return sceneBoundingRect().contains(p);
}

QString BaseItem::itemId() const
{
    if (m_tag) {
        if (m_tag->tagType() == Transition)
            return m_tag->attribute("event");
        else
            return m_tag->attribute("id", true);
    }

    return QString();
}

bool BaseItem::blockUpdates() const
{
    return m_blockUpdates;
}

BaseItem *BaseItem::parentBaseItem() const
{
    QGraphicsItem *parent = parentItem();
    return (parent && parent->type() > UnknownType)
            ? qgraphicsitem_cast<BaseItem*>(parent) : nullptr;
}
