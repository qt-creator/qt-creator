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

#ifndef FORMEDITORMAINVIEW_H
#define FORMEDITORMAINVIEW_H

#include <forwardview.h>

#include <formeditormainwidget.h>
#include <formeditorview.h>
#include <QPointer>


namespace QmlDesigner {
class AbstractFormEditorTool;

class FormEditorMainView : public ForwardView<FormEditorView>
{
    Q_OBJECT

public:
    FormEditorMainView();
    ~FormEditorMainView();

    // AbstractView
    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);
    void nodeCreated(const ModelNode &createdNode);
    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeReparented(const ModelNode &node, const ModelNode &oldParent, const ModelNode &newParent);
    void propertiesAdded(const NodeState &state, const QList<NodeProperty>& propertyList);
    void propertiesAboutToBeRemoved(const NodeState &state, const QList<NodeProperty>& propertyList);
    void propertyValuesChanged(const NodeState& state, const QList<NodeProperty> &propertyList);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList);

    void modelStateAboutToBeRemoved(const ModelState &modelState);
    void modelStateAdded(const ModelState &modelState);

    void nodeStatesAboutToBeRemoved(const QList<NodeState> &nodeStateList);
    void nodeStatesAdded(const QList<NodeState> &nodeStateList);

    void auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data);
    // FormEditorMainView
    FormEditorMainWidget *widget() const;
    NodeInstanceView *nodeInstanceView(const ModelState &modelState) const;

    void setCurrentState(const ModelState &state);
    ModelState currentState() const;

    enum EditorTool {
        MoveTool,
        DragTool,
        SelectTool,
        ResizeTool,
        AnchorTool
    };

    EditorTool currentTool() const;
    void setCurrentTool(EditorTool tool);

    void changeToMoveTool();
    void changeToMoveTool(const QPointF &beginPoint);
    void changeToDragTool();
    void changeToSelectionTool();
    void changeToResizeTool();
    void changeToTransformTools();
    void changeToAnchorTool();

    void anchorsChanged(const NodeState &nodeState);
    void nodeSlidedToIndex(const ModelNode &node, int newIndex, int oldIndex);

    ComponentAction *componentAction() const;
    ZoomAction *zoomAction() const;

signals:
    void stateChanged(const ModelState &state);
    void toolChanged(EditorTool tool);

protected:
    void setupSubViews();
    void createSubView(const QmlModelState &state);
    void removeSubView(const QmlModelState &state);
    void resetViews();

private:
    QWeakPointer<FormEditorMainWidget> m_formMainEditorWidget;
    QList<QWeakPointer<FormEditorView> > m_formEditorViewList;
    QMap<ModelState, QWeakPointer<QWidget> > m_subWindowMap;
    EditorTool m_currentTool;
};

} // namespace QmlDesigner

#endif // FORMEDITORMAINVIEW_H
