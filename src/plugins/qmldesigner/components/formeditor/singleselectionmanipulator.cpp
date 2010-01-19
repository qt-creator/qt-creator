/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "model.h"
#include "nodemetainfo.h"
#include "formeditorscene.h"
#include "formeditorview.h"
#include <QtDebug>

namespace QmlDesigner {

SingleSelectionManipulator::SingleSelectionManipulator(FormEditorView *editorView)
    : m_editorView(editorView),
    m_isActive(false)
{
}


void SingleSelectionManipulator::begin(const QPointF &beginPoint)
{
    m_beginPoint = beginPoint;
    m_isActive = true;
    m_oldSelectionList = m_editorView->selectedQmlItemNodes();
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

void SingleSelectionManipulator::select(SelectionType selectionType, bool selectOnlyContentItems)
{
    QList<QGraphicsItem*> itemList = m_editorView->scene()->items(m_beginPoint);

    QmlItemNode selectedNode;

    foreach(QGraphicsItem* item, itemList)
    {
        FormEditorItem *formEditorItem = FormEditorItem::fromQGraphicsItem(item);

        if (formEditorItem
           && !formEditorItem->qmlItemNode().isRootNode()
           && (formEditorItem->qmlItemNode().hasShowContent() || !selectOnlyContentItems))
        {
            selectedNode = formEditorItem->qmlItemNode();
            break;
        }
    }

    QList<QmlItemNode> nodeList;

    switch(selectionType) {
        case AddToSelection: {
                nodeList.append(m_oldSelectionList);
                if (selectedNode.isValid())
                    nodeList.append(selectedNode);
        }
        break;
        case ReplaceSelection: {
                if (selectedNode.isValid())
                    nodeList.append(selectedNode);
        }
        break;
        case RemoveFromSelection: {
                nodeList.append(m_oldSelectionList);
                if (selectedNode.isValid())
                    nodeList.removeAll(selectedNode);
        }
        break;
        case InvertSelection: {
                if (selectedNode.isValid()
                    && !m_oldSelectionList.contains(selectedNode))
                    nodeList.append(selectedNode);
        }
    }

    m_editorView->setSelectedQmlItemNodes(nodeList);
}


bool SingleSelectionManipulator::isActive() const
{
    return m_isActive;
}

QPointF SingleSelectionManipulator::beginPoint() const
{
    return m_beginPoint;
}

}
