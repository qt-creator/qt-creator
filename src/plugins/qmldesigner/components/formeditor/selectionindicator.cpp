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

#include "selectionindicator.h"

#include <designeractionmanager.h>

#include <QPen>
#include <QGraphicsScene>
#include <QGraphicsTextItem>

#include <abstractview.h>

#include <designeractionmanager.h>

#include <utils/theme/theme.h>

namespace QmlDesigner {

SelectionIndicator::SelectionIndicator(LayerItem *layerItem)
    : m_layerItem(layerItem)
{
}

SelectionIndicator::~SelectionIndicator()
{
    clear();
}

void SelectionIndicator::show()
{
    foreach (QGraphicsPolygonItem *item, m_indicatorShapeHash)
        item->show();
}

void SelectionIndicator::hide()
{
    foreach (QGraphicsPolygonItem *item, m_indicatorShapeHash)
        item->hide();
}

void SelectionIndicator::clear()
{
    if (m_layerItem) {
        foreach (QGraphicsItem *item, m_indicatorShapeHash) {
            m_layerItem->scene()->removeItem(item);
            delete item;
        }
    }
    m_labelItem.reset(nullptr);
    m_indicatorShapeHash.clear();
}

static QPolygonF boundingRectInLayerItemSpaceForItem(FormEditorItem *item, QGraphicsItem *layerItem)
{
     QPolygonF boundingRectInSceneSpace(item->mapToScene(item->qmlItemNode().instanceBoundingRect()));
     return layerItem->mapFromScene(boundingRectInSceneSpace);
}

static bool checkSingleSelection(const QList<FormEditorItem*> &itemList)
{
    return !itemList.isEmpty()
            && itemList.first()
            && itemList.first()->qmlItemNode().view()->singleSelectedModelNode().isValid();
}

const int labelHeight = 16;

void SelectionIndicator::setItems(const QList<FormEditorItem*> &itemList)
{
    clear();

    static QColor selectionColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_FormEditorSelectionColor);

    foreach (FormEditorItem *item, itemList) {
        if (!item->qmlItemNode().isValid())
            continue;

        QGraphicsPolygonItem *newSelectionIndicatorGraphicsItem = new QGraphicsPolygonItem(m_layerItem.data());
        m_indicatorShapeHash.insert(item, newSelectionIndicatorGraphicsItem);
        newSelectionIndicatorGraphicsItem->setPolygon(boundingRectInLayerItemSpaceForItem(item, m_layerItem.data()));
        newSelectionIndicatorGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, false);

        QPen pen;
        pen.setCosmetic(true);
        pen.setWidth(2);
        pen.setJoinStyle(Qt::MiterJoin);
        pen.setColor(selectionColor);
        newSelectionIndicatorGraphicsItem->setPen(pen);
        newSelectionIndicatorGraphicsItem->setCursor(m_cursor);
    }

    if (checkSingleSelection(itemList)) {
        FormEditorItem *selectedItem = itemList.first();
        m_labelItem.reset(new QGraphicsPolygonItem(m_layerItem.data()));

        QGraphicsWidget *toolbar = DesignerActionManager::instance().createFormEditorToolBar(m_labelItem.get());
        toolbar->setPos(1, -1);

        ModelNode modelNode = selectedItem->qmlItemNode().modelNode();
        QGraphicsTextItem *textItem = new QGraphicsTextItem(modelNode.simplifiedTypeName(), m_labelItem.get());

        if (modelNode.hasId())
            textItem->setPlainText(modelNode.id());

        static QColor textColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_FormEditorForegroundColor);

        textItem->setDefaultTextColor(textColor);
        QPolygonF labelPolygon = boundingRectInLayerItemSpaceForItem(selectedItem, m_layerItem.data());
        QRectF labelRect = labelPolygon.boundingRect();
        labelRect.setHeight(labelHeight);
        labelRect.setWidth(textItem->boundingRect().width() + toolbar->size().width());
        QPointF pos = labelRect.topLeft();
        labelRect.moveTo(0, 0);
        m_labelItem->setPolygon(labelRect);
        m_labelItem->setPos(pos + QPointF(0, -labelHeight));
        int offset = labelHeight + 2 - textItem->boundingRect().height();
        textItem->setPos(QPointF(toolbar->size().width(), offset));
        m_labelItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
        QPen pen;
        pen.setCosmetic(true);
        pen.setWidth(2);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::BevelJoin);
        pen.setColor(selectionColor);
        m_labelItem->setPen(pen);
        m_labelItem->setBrush(selectionColor);
        m_labelItem->update();
    }
}

void SelectionIndicator::updateItems(const QList<FormEditorItem*> &itemList)
{
    foreach (FormEditorItem *item, itemList) {
        if (m_indicatorShapeHash.contains(item)) {
            QGraphicsPolygonItem *indicatorGraphicsItem =  m_indicatorShapeHash.value(item);
            indicatorGraphicsItem->setPolygon(boundingRectInLayerItemSpaceForItem(item, m_layerItem.data()));
        }
    }

    if (checkSingleSelection(itemList)
            && m_labelItem) {
        FormEditorItem *selectedItem = itemList.first();
        QPolygonF labelPolygon = boundingRectInLayerItemSpaceForItem(selectedItem, m_layerItem.data());
        QRectF labelRect = labelPolygon.boundingRect();
        QPointF pos = labelRect.topLeft();
        m_labelItem->setPos(pos + QPointF(0, -labelHeight));
        m_layerItem->update();
    }
}

void SelectionIndicator::setCursor(const QCursor &cursor)
{
    m_cursor = cursor;

    foreach (QGraphicsItem  *item, m_indicatorShapeHash)
        item->setCursor(cursor);
}

}

