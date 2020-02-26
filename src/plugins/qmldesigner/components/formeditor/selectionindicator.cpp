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

#include "annotation.h"
#include <designeractionmanager.h>
#include <formeditorannotationicon.h>

#include <QPen>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <memory>

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
    if (m_labelItem)
        m_labelItem->show();
}

void SelectionIndicator::hide()
{
    foreach (QGraphicsPolygonItem *item, m_indicatorShapeHash)
        item->hide();
    if (m_labelItem)
        m_labelItem->hide();
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
    m_annotationItem = nullptr;
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
            && itemList.constFirst()
            && itemList.constFirst()->qmlItemNode().view()->singleSelectedModelNode().isValid();
}

const int labelHeight = 18;

void SelectionIndicator::setItems(const QList<FormEditorItem*> &itemList)
{
    clear();

    static QColor selectionColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_FormEditorSelectionColor);

    foreach (FormEditorItem *item, itemList) {
        if (!item->qmlItemNode().isValid())
            continue;

        auto newSelectionIndicatorGraphicsItem = new QGraphicsPolygonItem(m_layerItem.data());
        m_indicatorShapeHash.insert(item, newSelectionIndicatorGraphicsItem);
        newSelectionIndicatorGraphicsItem->setPolygon(boundingRectInLayerItemSpaceForItem(item, m_layerItem.data()));
        newSelectionIndicatorGraphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, false);

        QPen pen;
        pen.setCosmetic(true);
        pen.setWidth(2);
        pen.setJoinStyle(Qt::MiterJoin);
        pen.setColor(selectionColor);
        newSelectionIndicatorGraphicsItem->setPen(pen);
    }

    if (checkSingleSelection(itemList)) {
        FormEditorItem *selectedItem = itemList.constFirst();
        m_labelItem = std::make_unique<QGraphicsPolygonItem>(m_layerItem.data());
        const qreal scaleFactor = m_layerItem->viewportTransform().m11();

        QGraphicsWidget *toolbar = DesignerActionManager::instance().createFormEditorToolBar(m_labelItem.get());
        toolbar->setPos(1, -1);

        ModelNode modelNode = selectedItem->qmlItemNode().modelNode();
        QGraphicsTextItem *textItem = new QGraphicsTextItem(modelNode.simplifiedTypeName(), m_labelItem.get());

        if (modelNode.hasId())
            textItem->setPlainText(modelNode.id());

        if (modelNode.hasAnnotation() || modelNode.hasCustomId()) {
            m_annotationItem = new FormEditorAnnotationIcon(modelNode, m_labelItem.get());
            m_annotationItem->update();
        }
        else {
            m_annotationItem = nullptr;
        }

        static QColor textColor = Utils::creatorTheme()->color(Utils::Theme::QmlDesigner_FormEditorForegroundColor);

        textItem->setDefaultTextColor(textColor);
        QPolygonF labelPolygon = boundingRectInLayerItemSpaceForItem(selectedItem, m_layerItem.data());
        QRectF labelRect = labelPolygon.boundingRect();
        labelRect.setHeight(labelHeight);
        labelRect.setWidth(textItem->boundingRect().width() + toolbar->size().width());
        QPointF pos = labelRect.topLeft();
        labelRect.moveTo(0, 0);
        m_labelItem->setPolygon(labelRect);
        const int scaledHeight = labelHeight / scaleFactor;
        m_labelItem->setPos(pos + QPointF(0, -scaledHeight));
        const int offset = (labelHeight - textItem->boundingRect().height()) / 2;
        textItem->setPos(QPointF(toolbar->size().width(), offset));
        m_labelItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
        m_labelItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

        QPen pen;
        pen.setCosmetic(true);
        pen.setWidth(2);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::BevelJoin);
        pen.setColor(selectionColor);

        if (m_annotationItem) {
            m_annotationItem->setFlags(QGraphicsItem::ItemIgnoresTransformations);
            adjustAnnotationPosition(labelPolygon.boundingRect(), m_labelItem->boundingRect(), scaleFactor);
        }

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
        FormEditorItem *selectedItem = itemList.constFirst();
        QPolygonF labelPolygon = boundingRectInLayerItemSpaceForItem(selectedItem, m_layerItem.data());
        QRectF labelRect = labelPolygon.boundingRect();
        QPointF pos = labelRect.topLeft();
        const qreal scaleFactor = m_layerItem->viewportTransform().m11();
        const int scaledHeight = labelHeight / scaleFactor;
        m_labelItem->setPos(pos + QPointF(0, -scaledHeight));

        if (m_annotationItem) {
            adjustAnnotationPosition(labelPolygon.boundingRect(), m_labelItem->boundingRect(), scaleFactor);
        }

        m_layerItem->update();
    }
}

void SelectionIndicator::setCursor(const QCursor &cursor)
{
    m_cursor = cursor;

    foreach (QGraphicsItem  *item, m_indicatorShapeHash)
        item->setCursor(cursor);
}

void SelectionIndicator::adjustAnnotationPosition(const QRectF &itemRect, const QRectF &labelRect, qreal scaleFactor)
{
    if (!m_annotationItem) return;

    const qreal iconWShift = m_annotationItem->iconWidth() * 0.5;
    const qreal iconHShift = (m_annotationItem->iconHeight() * 0.45)/scaleFactor;
    qreal iconX = 0.0;
    qreal iconY = -(iconHShift);

    if (((labelRect.width() + iconWShift)/scaleFactor) > itemRect.width())
        iconY -= labelRect.height()/scaleFactor;

    if ((iconWShift/scaleFactor) > itemRect.width())
        iconX = 0.0;
    else
        iconX = (itemRect.width()) - (iconWShift/scaleFactor);

    m_annotationItem->setPos(iconX*scaleFactor, iconY*scaleFactor);
}

}

