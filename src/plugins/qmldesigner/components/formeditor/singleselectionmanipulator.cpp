/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "singleselectionmanipulator.h"

#include "formeditorscene.h"
#include "formeditorview.h"
#include <QDebug>

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
    m_oldSelectionList = toQmlItemNodeList(m_editorView->selectedModelNodes());
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

void SingleSelectionManipulator::select(SelectionType selectionType)
{
    QList<QGraphicsItem*> itemList = m_editorView->scene()->items(m_beginPoint);

    QmlItemNode selectedNode;

    FormEditorItem *formEditorItem = m_editorView->currentTool()->nearestFormEditorItem(m_beginPoint, itemList);

    if (formEditorItem && formEditorItem->qmlItemNode().isValid())
        selectedNode = formEditorItem->qmlItemNode();

    QList<QmlItemNode> nodeList;

    switch (selectionType) {
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

    m_editorView->setSelectedModelNodes(toModelNodeList(nodeList));
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
