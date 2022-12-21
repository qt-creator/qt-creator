// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rubberbandselectionmanipulator.h"

#include "formeditorscene.h"

#include <utils/algorithm.h>

namespace QmlDesigner {

RubberBandSelectionManipulator::RubberBandSelectionManipulator(LayerItem *layerItem, FormEditorView *editorView)
    : m_selectionRectangleElement(layerItem),
    m_editorView(editorView),
    m_beginFormEditorItem(nullptr),
    m_isActive(false)
{
    m_selectionRectangleElement.hide();
}

void RubberBandSelectionManipulator::clear()
{
    m_selectionRectangleElement.clear();
    m_isActive = false;
    m_beginPoint = QPointF();
    m_itemList.clear();
    m_oldSelectionList.clear();
}

FormEditorItem *RubberBandSelectionManipulator::topFormEditorItem(const QList<QGraphicsItem*> &itemList)
{
    for (QGraphicsItem *item : itemList) {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);

        if (formEditorItem)
        {
            return formEditorItem;
        }
    }

    return m_editorView->scene()->rootFormEditorItem();
}

void RubberBandSelectionManipulator::begin(const QPointF& beginPoint)
{
    m_beginPoint = beginPoint;
    m_selectionRectangleElement.setRect(m_beginPoint, m_beginPoint);
    m_selectionRectangleElement.show();
    m_isActive = true;
    m_beginFormEditorItem = topFormEditorItem(m_editorView->scene()->items(beginPoint));
    m_oldSelectionList = toQmlItemNodeList(m_editorView->selectedModelNodes());
}

void RubberBandSelectionManipulator::update(const QPointF& updatePoint)
{
    m_selectionRectangleElement.setRect(m_beginPoint, updatePoint);
}

void RubberBandSelectionManipulator::end()
{
    m_oldSelectionList.clear();
    m_selectionRectangleElement.hide();
    m_isActive = false;
}

void RubberBandSelectionManipulator::select(SelectionType selectionType)
{
    if (!m_beginFormEditorItem)
        return;

    QList<QGraphicsItem *> itemList = m_editorView->scene()->items(m_selectionRectangleElement.rect(), Qt::IntersectsItemBoundingRect);
    QList<QmlItemNode> newNodeList;

    for (QGraphicsItem *item : itemList)
    {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);

        if (formEditorItem
                && formEditorItem->qmlItemNode().isValid()
                && formEditorItem->qmlItemNode().instanceIsMovable()
                && formEditorItem->qmlItemNode().modelIsMovable()
                && !formEditorItem->qmlItemNode().instanceIsInLayoutable())
        {
            newNodeList.append(formEditorItem->qmlItemNode());
        }
    }

    if (newNodeList.isEmpty()
            && m_beginFormEditorItem->qmlItemNode().isValid()
            && m_beginFormEditorItem->qmlItemNode().instanceIsMovable()
            && m_beginFormEditorItem->qmlItemNode().modelIsMovable()
            && !m_beginFormEditorItem->qmlItemNode().instanceIsInLayoutable())
        newNodeList.append(m_beginFormEditorItem->qmlItemNode());

    QList<QmlItemNode> nodeList;

    switch (selectionType) {
    case AddToSelection: {
            nodeList.append(m_oldSelectionList);
            nodeList.append(newNodeList);
        }
        break;
    case ReplaceSelection: {
            nodeList.append(newNodeList);
        }
        break;
    case RemoveFromSelection: {
            QSet<QmlItemNode> oldSelectionSet = Utils::toSet(m_oldSelectionList);
            const QSet<QmlItemNode> newSelectionSet = Utils::toSet(newNodeList);
            nodeList.append(Utils::toList(oldSelectionSet.subtract(newSelectionSet)));
        }
    }

    m_editorView->setSelectedModelNodes(toModelNodeList(nodeList));
}


void RubberBandSelectionManipulator::setItems(const QList<FormEditorItem *> &itemList)
{
    m_itemList = itemList;
}

QPointF RubberBandSelectionManipulator::beginPoint() const
{
    return m_beginPoint;
}

bool RubberBandSelectionManipulator::isActive() const
{
    return m_isActive;
}

}
