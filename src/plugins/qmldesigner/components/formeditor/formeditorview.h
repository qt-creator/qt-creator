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

#ifndef FORMEDITORVIEW_H
#define FORMEDITORVIEW_H

#include <qmlmodelview.h>

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

namespace Utils {
class CrumblePath;
}

namespace QmlDesigner {

class FormEditorWidget;
class FormEditorNodeInstanceView;
class FormEditorScene;
class NodeInstanceView;

class AbstractFormEditorTool;
class MoveTool;
class SelectionTool;
class ResizeTool;
class DragTool;
class ItemLibraryEntry;
class QmlItemNode;

class  FormEditorView : public QmlModelView
{
    Q_OBJECT

public:
    FormEditorView(QObject *parent = 0);
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
    QWidget *widget();
    FormEditorWidget *formEditorWidget();
    AbstractFormEditorTool *currentTool() const;
    FormEditorScene *scene() const;

    bool changeToMoveTool();
    bool changeToMoveTool(const QPointF &beginPoint);
    void changeToDragTool();
    void changeToSelectionTool();
    void changeToSelectionTool(QGraphicsSceneMouseEvent *event);
    void changeToResizeTool();
    void changeToTransformTools();

    void nodeSlidedToIndex(const NodeListProperty &listProperty, int newIndex, int oldIndex);
    void auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data);

    void instancesCompleted(const QVector<ModelNode> &completedNodeList);
    void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash);
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList);
    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList);
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList);
    void instancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList);
    void instancesToken(const QString &tokenName, int tokenNumber, const QVector<ModelNode> &nodeVector);

    void rewriterBeginTransaction();
    void rewriterEndTransaction();

    double margins() const;
    double spacing() const;
    void deActivateItemCreator();

    void actualStateChanged(const ModelNode &node);

protected:
    void reset();

protected slots:
    void delayedReset();
    QList<ModelNode> adjustStatesForModelNodes(const QList<ModelNode> &nodeList) const;
    void updateGraphicsIndicators();
    void setSelectOnlyContentItemsAction(bool selectOnlyContentItems);
    bool isMoveToolAvailable() const;

private: //functions
    void setupFormEditorItemTree(const QmlItemNode &qmlItemNode);
    void removeNodeFromScene(const QmlItemNode &qmlItemNode);
    void hideNodeFromScene(const QmlItemNode &qmlItemNode);

private: //variables
    QWeakPointer<FormEditorWidget> m_formEditorWidget;
    QWeakPointer<FormEditorScene> m_scene;
    MoveTool *m_moveTool;
    SelectionTool *m_selectionTool;
    ResizeTool *m_resizeTool;
    DragTool *m_dragTool;
    AbstractFormEditorTool *m_currentTool;
    int m_transactionCounter;
};

}

#endif //FORMEDITORVIEW_H
