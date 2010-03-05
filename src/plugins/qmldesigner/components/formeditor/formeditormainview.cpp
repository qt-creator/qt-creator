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

#include "formeditormainview.h"

#include "formeditorview.h"
#include "formeditorwidget.h"
#include <QPixmapCache>
#include <QtDebug>

#include "zoomaction.h"

namespace QmlDesigner {


FormEditorMainView::FormEditorMainView()
   : m_formMainEditorWidget(new FormEditorMainWidget(this))
{
    QPixmapCache::setCacheLimit(1024 * 100);
}

FormEditorMainView::~FormEditorMainView()
{
    resetViews();

    if (m_formMainEditorWidget)
        m_formMainEditorWidget->deleteLater();
}

void FormEditorMainView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    setupSubViews();

    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->modelAttached(model);
    }

    m_formMainEditorWidget->setModel(model);
}

void FormEditorMainView::createSubView(const QmlModelState &state)
{
    FormEditorView *subView = new FormEditorView(this);
    subView->setCurrentState(state);
    m_formEditorViewList.append(subView);

    m_formMainEditorWidget->addWidget(subView->widget());
    m_subWindowMap.insert(state, subView->widget());

    subView->setZoomLevel(zoomAction()->zoomLevel());
    connect(zoomAction(), SIGNAL(zoomLevelChanged(double)), subView, SLOT(setZoomLevel(double)));
}

void FormEditorMainView::removeSubView(const ModelState &state)
{
    QWidget *subWindow = m_subWindowMap.take(state).data();
    Q_ASSERT(subWindow);
    if (subWindow == m_formMainEditorWidget->currentWidget())
        setCurrentState();
    delete subWindow;

    FormEditorView *editorView = 0;
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view->modelState() == state) {
            editorView = view.data();
            break;
        }
    }
    Q_ASSERT(editorView);
    m_formEditorViewList.removeOne(editorView);
    delete editorView;
}

static bool modelStateLessThan(const ModelState &firstState, const ModelState &secondState)
 {
    if (firstState.isBaseState())
        return false;

    return firstState.name().toLower() > secondState.name().toLower();
 }

void FormEditorMainView::setupSubViews()
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList)
        delete view.data();
    m_formEditorViewList.clear();

    foreach(const QWeakPointer<QWidget> &view, m_subWindowMap.values())
        delete view.data();
    m_subWindowMap.clear();

    QList<ModelState> invertedStates(model()->modelStates());
    qSort(invertedStates.begin(), invertedStates.end(), modelStateLessThan);
    foreach(const ModelState &state, invertedStates)
        createSubView(state);
}

void FormEditorMainView::resetViews()
{
    m_subWindowMap.clear();
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList)
        delete view.data();
    m_formEditorViewList.clear();
}

void FormEditorMainView::nodeCreated(const ModelNode &createdNode)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->nodeCreated(createdNode);
    }
}

void FormEditorMainView::modelAboutToBeDetached(Model *model)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->modelAboutToBeDetached(model);
    }

    resetViews();

    m_formMainEditorWidget->setModel(0);

    AbstractView::modelAboutToBeDetached(model);
}

void FormEditorMainView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->nodeAboutToBeRemoved(removedNode);
    }
}

void FormEditorMainView::propertiesAdded(const NodeState &state, const QList<NodeProperty>& propertyList)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->propertiesAdded(state, propertyList);
    }
}

void FormEditorMainView::propertiesAboutToBeRemoved(const NodeState &state, const QList<NodeProperty>& propertyList)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->propertiesAboutToBeRemoved(state, propertyList);
    }
}

void FormEditorMainView::propertyValuesChanged(const NodeState &state, const QList<NodeProperty>& propertyList)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->propertyValuesChanged(state, propertyList);
    }
}

void FormEditorMainView::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->nodeIdChanged(node, newId, oldId);
    }
}

void FormEditorMainView::nodeReparented(const ModelNode &node, const ModelNode &oldParent, const ModelNode &newParent)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->nodeReparented(node, oldParent, newParent);
    }
}

FormEditorMainWidget *FormEditorMainView::widget() const
{
    return m_formMainEditorWidget.data();
}

NodeInstanceView *FormEditorMainView::nodeInstanceView(const ModelState &modelState) const
{
    foreach (const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view->modelState() == modelState)
            return view->nodeInstanceView();
    }

    return 0;
}

void FormEditorMainView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                          const QList<ModelNode> &lastSelectedNodeList)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->selectedNodesChanged(selectedNodeList, lastSelectedNodeList);
    }
}

void FormEditorMainView::modelStateAboutToBeRemoved(const ModelState &modelState)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->modelStateAboutToBeRemoved(modelState);
    }

    removeSubView(modelState);
}

void FormEditorMainView::modelStateAdded(const ModelState &modelState)
{
    createSubView(modelState);
    m_formEditorViewList.last()->modelAttached(model());

    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->modelStateAdded(modelState);
    }
}

void FormEditorMainView::nodeStatesAboutToBeRemoved(const QList<NodeState> &nodeStateList)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->nodeStatesAboutToBeRemoved(nodeStateList);
    }
}

void FormEditorMainView::nodeStatesAdded(const QList<NodeState> &nodeStateList)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->nodeStatesAdded(nodeStateList);
    }
}

void FormEditorMainView::setCurrentState(const QmlModelState &state)
{
    Q_ASSERT(m_subWindowMap.contains(state));
    m_formMainEditorWidget->setCurrentWidget(m_subWindowMap.value(state).data());
    emit stateChanged(state);
}

ModelState FormEditorMainView::currentState() const
{
    QWidget *currentWidget = m_formMainEditorWidget->currentWidget();
    QMapIterator<ModelState, QWeakPointer<QWidget> > iter(m_subWindowMap);
    while (iter.hasNext()) {
        iter.next();
        if (iter.value().data() == currentWidget) {
            return iter.key();
        }
    }
    Q_ASSERT_X(0, Q_FUNC_INFO, "cannot find current state");
    return ModelState();
}

FormEditorMainView::EditorTool FormEditorMainView::currentTool() const
{
    return m_currentTool;
}

void FormEditorMainView::setCurrentTool(FormEditorMainView::EditorTool tool)
{
    if (m_currentTool == tool)
        return;
    m_currentTool = tool;
    switch (tool) {
    case MoveTool: {
        foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList)
            view->changeToMoveTool();
        break;
    }
    case DragTool: {
        foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList)
            view->changeToDragTool();
        break;
    }
    case SelectTool: {
        foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList)
            view->changeToSelectionTool();
        break;
    }
    case ResizeTool: {
        foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList)
            view->changeToResizeTool();
        break;
    }
    case AnchorTool: {
        foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList)
            view->changeToAnchorTool();
        break;
    }
    default: Q_ASSERT(0);
    }
    emit toolChanged(m_currentTool);
}

void FormEditorMainView::changeToDragTool()
{
    setCurrentTool(DragTool);
}


void FormEditorMainView::changeToMoveTool()
{
    setCurrentTool(MoveTool);
}

void FormEditorMainView::changeToMoveTool(const QPointF &/*beginPoint*/)
{
    setCurrentTool(MoveTool);
}

void FormEditorMainView::changeToSelectionTool()
{
    setCurrentTool(SelectTool);
}

void FormEditorMainView::changeToResizeTool()
{
    setCurrentTool(ResizeTool);
}

void FormEditorMainView::changeToAnchorTool()
{
    setCurrentTool(AnchorTool);
}

void FormEditorMainView::changeToTransformTools()
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList)
        if (view)
            view->changeToTransformTools();
}

void FormEditorMainView::anchorsChanged(const NodeState &nodeState)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->anchorsChanged(nodeState);
    }
}


void FormEditorMainView::auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data)
{
     foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->auxiliaryDataChanged(node, name, data);
    }
}

void FormEditorMainView::nodeSlidedToIndex(const ModelNode &node, int newIndex, int oldIndex)
{
    foreach(const QWeakPointer<FormEditorView> &view, m_formEditorViewList) {
        if (view)
            view->nodeSlidedToIndex(node, newIndex, oldIndex);
    }
}

ComponentAction *FormEditorMainView::componentAction() const
{
    return m_formMainEditorWidget->componentAction();
}

ZoomAction *FormEditorMainView::zoomAction() const
{
    return m_formMainEditorWidget->zoomAction();
}

} // namespace QmlDesigner
