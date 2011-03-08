/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef FORMEDITORVIEW_H
#define FORMEDITORVIEW_H

#include <qmlmodelview.h>

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

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
class ItemLibraryEntry;
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

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports);

    void nodeCreated(const ModelNode &createdNode);
    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList);
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);
    void propertiesRemoved(const QList<AbstractProperty> &propertyList);

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

    void instancesCompleted(const QVector<ModelNode> &completedNodeList);
    void instanceInformationsChange(const QVector<ModelNode> &nodeList);
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList);
    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList);
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList);

    void rewriterBeginTransaction();
    void rewriterEndTransaction();

    double margins() const;
    double spacing() const;
    void deActivateItemCreator();

public slots:
    void activateItemCreator(const QString &name);

signals:
    void ItemCreatorDeActivated();

protected:
    void otherPropertyChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName);
    void stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState);
    void reset();

protected slots:
    void delayedReset();
    QList<ModelNode> adjustStatesForModelNodes(const QList<ModelNode> &nodeList) const;
    void updateGraphicsIndicators();
    void setSelectOnlyContentItemsAction(bool selectOnlyContentItems);
    bool isMoveToolAvailable() const;

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
    int m_transactionCounter;
};

}

#endif //FORMEDITORVIEW_H
