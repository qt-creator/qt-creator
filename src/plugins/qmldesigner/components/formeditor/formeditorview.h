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

#ifndef FORMEDITORVIEW_H
#define FORMEDITORVIEW_H

#include <qmlmodelview.h>

class QGraphicsScene;
class QGraphicsSceneMouseEvent;

namespace QmlDesigner {

class FormEditorWidget;
class FormEditorNodeInstanceView;
class FormEditorScene;
class NodeInstanceView;

class AbstractFormEditorTool;
class MoveTool;
class SelectionTool;
class ResizeTool;
class AnchorTool;
class DragTool;
class ItemCreatorTool;
class ItemLibraryInfo;
class QmlItemNode;

class  FormEditorView : public QmlModelView
{
    Q_OBJECT

public:
    FormEditorView(QObject *parent);
    ~FormEditorView();

    // AbstractView
    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);

    void nodeCreated(const ModelNode &createdNode);
    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    void nodeTypeChanged(const ModelNode &node,const QString &type, int majorVersion, int minorVersion);

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList);

    // FormEditorView
    FormEditorWidget *widget() const;
    AbstractFormEditorTool *currentTool() const;
    FormEditorScene *scene() const;

    void changeToMoveTool();
    void changeToMoveTool(const QPointF &beginPoint);
    void changeToDragTool();
    void changeToSelectionTool();
    void changeToItemCreatorTool();
    void changeToSelectionTool(QGraphicsSceneMouseEvent *event);
    void changeToResizeTool();
    void changeToAnchorTool();
    void changeToTransformTools();

    void setCursor(const QCursor &cursor);

    void nodeSlidedToIndex(const NodeListProperty &listProperty, int newIndex, int oldIndex);
    void auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data);

    double margins() const;
    double spacing() const;
    void deActivateItemCreator();

public slots:
    void activateItemCreator(const QString &name);

signals:
    void ItemCreatorDeActivated();

protected:
    void transformChanged(const QmlObjectNode &qmlObjectNode);
    void parentChanged(const QmlObjectNode &qmlObjectNode);
    void otherPropertyChanged(const QmlObjectNode &qmlObjectNode);
    void updateItem(const QmlObjectNode &qmlObjectNode);
    void stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState);

protected slots:
    QList<ModelNode> adjustStatesForModelNodes(const QList<ModelNode> &nodeList) const;
    void updateGraphicsIndicators();
    void setSelectOnlyContentItemsAction(bool selectOnlyContentItems);

private: //functions
    void setupFormEditorItemTree(const QmlItemNode &qmlItemNode);


private: //variables
    QWeakPointer<FormEditorWidget> m_formEditorWidget;
    QWeakPointer<FormEditorScene> m_scene;
    MoveTool *m_moveTool;
    SelectionTool *m_selectionTool;
    ResizeTool *m_resizeTool;
    AnchorTool *m_anchorTool;
    DragTool *m_dragTool;
    ItemCreatorTool *m_itemCreatorTool;
    AbstractFormEditorTool *m_currentTool;

};

}

#endif //FORMEDITORVIEW_H
