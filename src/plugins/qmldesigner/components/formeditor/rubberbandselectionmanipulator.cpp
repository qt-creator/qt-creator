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

#include "rubberbandselectionmanipulator.h"

#include "model.h"
#include "formeditorscene.h"

namespace QmlDesigner {

RubberBandSelectionManipulator::RubberBandSelectionManipulator(LayerItem *layerItem, FormEditorView *editorView)
    : m_selectionRectangleElement(layerItem),
    m_editorView(editorView),
    m_beginFormEditorItem(0),
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
    foreach (QGraphicsItem *item, itemList) {
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
    m_oldSelectionList = m_editorView->selectedQmlItemNodes();
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

    QList<QGraphicsItem*> itemList = m_editorView->scene()->items(m_selectionRectangleElement.rect(), Qt::IntersectsItemShape);
    QList<QmlItemNode> newNodeList;

    foreach (QGraphicsItem* item, itemList)
    {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);

        if (formEditorItem
                && formEditorItem->qmlItemNode().isValid()
                && m_beginFormEditorItem->childItems().contains(formEditorItem)
                && formEditorItem->qmlItemNode().instanceIsMovable()
                && !formEditorItem->qmlItemNode().instanceIsInPositioner())
        {
            newNodeList.append(formEditorItem->qmlItemNode());
        }
    }

    if (newNodeList.isEmpty()
            && m_beginFormEditorItem->qmlItemNode().isValid()
            && m_beginFormEditorItem->qmlItemNode().instanceIsMovable()
            && !m_beginFormEditorItem->qmlItemNode().instanceIsInPositioner())
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
            QSet<QmlItemNode> oldSelectionSet(m_oldSelectionList.toSet());
            QSet<QmlItemNode> newSelectionSet(newNodeList.toSet());
            nodeList.append(oldSelectionSet.subtract(newSelectionSet).toList());
        }
    }

    m_editorView->setSelectedQmlItemNodes(nodeList);
}


void RubberBandSelectionManipulator::setItems(const QList<FormEditorItem*> &itemList)
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
