/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "livesingleselectionmanipulator.h"
#include "qdeclarativeviewinspector.h"
#include "../qdeclarativeviewinspector_p.h"
#include <QtDebug>

namespace QmlJSDebugger {

LiveSingleSelectionManipulator::LiveSingleSelectionManipulator(QDeclarativeViewInspector *editorView)
    : m_editorView(editorView),
      m_isActive(false)
{
}


void LiveSingleSelectionManipulator::begin(const QPointF &beginPoint)
{
    m_beginPoint = beginPoint;
    m_isActive = true;
    m_oldSelectionList = QDeclarativeViewInspectorPrivate::get(m_editorView)->selectedItems();
}

void LiveSingleSelectionManipulator::update(const QPointF &/*updatePoint*/)
{
    m_oldSelectionList.clear();
}

void LiveSingleSelectionManipulator::clear()
{
    m_beginPoint = QPointF();
    m_oldSelectionList.clear();
}


void LiveSingleSelectionManipulator::end(const QPointF &/*updatePoint*/)
{
    m_oldSelectionList.clear();
    m_isActive = false;
}

void LiveSingleSelectionManipulator::select(SelectionType selectionType,
                                            const QList<QGraphicsItem*> &items,
                                            bool /*selectOnlyContentItems*/)
{
    QGraphicsItem *selectedItem = 0;

    foreach (QGraphicsItem* item, items)
    {
        //FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);
        if (item
            /*&& !formEditorItem->qmlItemNode().isRootNode()
               && (formEditorItem->qmlItemNode().hasShowContent() || !selectOnlyContentItems)*/)
        {
            selectedItem = item;
            break;
        }
    }

    QList<QGraphicsItem*> resultList;

    switch(selectionType) {
    case AddToSelection: {
        resultList.append(m_oldSelectionList);
        if (selectedItem && !m_oldSelectionList.contains(selectedItem))
            resultList.append(selectedItem);
    }
        break;
    case ReplaceSelection: {
        if (selectedItem)
            resultList.append(selectedItem);
    }
        break;
    case RemoveFromSelection: {
        resultList.append(m_oldSelectionList);
        if (selectedItem)
            resultList.removeAll(selectedItem);
    }
        break;
    case InvertSelection: {
        if (selectedItem
                && !m_oldSelectionList.contains(selectedItem))
        {
            resultList.append(selectedItem);
        }
    }
    }

    m_editorView->setSelectedItems(resultList);
}

void LiveSingleSelectionManipulator::select(SelectionType selectionType, bool selectOnlyContentItems)
{
    QDeclarativeViewInspectorPrivate *inspectorPrivate =
            QDeclarativeViewInspectorPrivate::get(m_editorView);
    QList<QGraphicsItem*> itemList = inspectorPrivate->selectableItems(m_beginPoint);
    select(selectionType, itemList, selectOnlyContentItems);
}


bool LiveSingleSelectionManipulator::isActive() const
{
    return m_isActive;
}

QPointF LiveSingleSelectionManipulator::beginPoint() const
{
    return m_beginPoint;
}

} // namespace QmlJSDebugger
