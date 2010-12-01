/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "singleselectionmanipulator.h"
#include "qdeclarativeviewobserver.h"
#include "../qdeclarativeviewobserver_p.h"
#include <QtDebug>

namespace QmlJSDebugger {

SingleSelectionManipulator::SingleSelectionManipulator(QDeclarativeViewObserver *editorView)
    : m_editorView(editorView),
      m_isActive(false)
{
}


void SingleSelectionManipulator::begin(const QPointF &beginPoint)
{
    m_beginPoint = beginPoint;
    m_isActive = true;
    m_oldSelectionList = QDeclarativeViewObserverPrivate::get(m_editorView)->selectedItems();
}

void SingleSelectionManipulator::update(const QPointF &/*updatePoint*/)
{
    m_oldSelectionList.clear();
}

void SingleSelectionManipulator::clear()
{
    m_beginPoint = QPointF();
    m_oldSelectionList.clear();
}


void SingleSelectionManipulator::end(const QPointF &/*updatePoint*/)
{
    m_oldSelectionList.clear();
    m_isActive = false;
}

void SingleSelectionManipulator::select(SelectionType selectionType,
                                        const QList<QGraphicsItem*> &items,
                                        bool /*selectOnlyContentItems*/)
{
    QGraphicsItem *selectedItem = 0;

    foreach(QGraphicsItem* item, items)
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

void SingleSelectionManipulator::select(SelectionType selectionType, bool selectOnlyContentItems)
{
    QDeclarativeViewObserverPrivate *observerPrivate =
            QDeclarativeViewObserverPrivate::get(m_editorView);
    QList<QGraphicsItem*> itemList = observerPrivate->selectableItems(m_beginPoint);
    select(selectionType, itemList, selectOnlyContentItems);
}


bool SingleSelectionManipulator::isActive() const
{
    return m_isActive;
}

QPointF SingleSelectionManipulator::beginPoint() const
{
    return m_beginPoint;
}

} // namespace QmlJSDebugger
