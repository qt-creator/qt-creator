// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "singleselectionmanipulator.h"

#include "formeditorscene.h"
#include "formeditortracing.h"
#include "formeditorview.h"

#include <QDebug>

namespace QmlDesigner {

using FormEditorTracing::category;

SingleSelectionManipulator::SingleSelectionManipulator(FormEditorView *editorView)
    : m_editorView(editorView),
    m_isActive(false)
{
    NanotraceHR::Tracer tracer{"single selection manipulator constructor", category()};
}


void SingleSelectionManipulator::begin(const QPointF &beginPoint)
{
    NanotraceHR::Tracer tracer{"single selection manipulator begin", category()};

    m_beginPoint = beginPoint;
    m_isActive = true;
    m_oldSelectionList = toQmlItemNodeList(m_editorView->selectedModelNodes());
}

void SingleSelectionManipulator::update(const QPointF &/*updatePoint*/)
{
    NanotraceHR::Tracer tracer{"single selection manipulator update", category()};

    m_oldSelectionList.clear();
}

void SingleSelectionManipulator::clear()
{
    NanotraceHR::Tracer tracer{"single selection manipulator clear", category()};

    m_beginPoint = QPointF();
    m_oldSelectionList.clear();
}


void SingleSelectionManipulator::end(const QPointF &/*updatePoint*/)
{
    NanotraceHR::Tracer tracer{"single selection manipulator end", category()};

    m_oldSelectionList.clear();
    m_isActive = false;
}

void SingleSelectionManipulator::select(SelectionType selectionType)
{
    NanotraceHR::Tracer tracer{"single selection manipulator select", category()};

    QList<QGraphicsItem*> itemList = m_editorView->scene()->items(m_beginPoint);

    QmlItemNode selectedNode;

    FormEditorItem *formEditorItem = m_editorView->currentTool()->nearestFormEditorItem(m_beginPoint, itemList);

    if (formEditorItem && formEditorItem->qmlItemNode().isValid())
        selectedNode = formEditorItem->qmlItemNode();

    QList<QmlItemNode> nodeList;

    switch (selectionType) {
        case AddToSelection: {
                nodeList.append(m_oldSelectionList);
                nodeList.append(selectedNode);
        }
        break;
        case ReplaceSelection: {
            nodeList.append(selectedNode);
        }
        break;
        case RemoveFromSelection: {
                nodeList.append(m_oldSelectionList);
                nodeList.removeAll(selectedNode);
        }
        break;
        case InvertSelection: {
            nodeList.append(m_oldSelectionList);
            if (!m_oldSelectionList.contains(selectedNode))
                nodeList.append(selectedNode);
            else
                nodeList.removeAll(selectedNode);
        }
    }

    m_editorView->setSelectedModelNodes(toModelNodeList(nodeList));
}


bool SingleSelectionManipulator::isActive() const
{
    NanotraceHR::Tracer tracer{"single selection manipulator isActive", category()};

    return m_isActive;
}

QPointF SingleSelectionManipulator::beginPoint() const
{
    NanotraceHR::Tracer tracer{"single selection manipulator beginPoint", category()};

    return m_beginPoint;
}

}
