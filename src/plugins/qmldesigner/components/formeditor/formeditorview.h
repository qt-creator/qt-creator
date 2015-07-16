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

#ifndef FORMEDITORVIEW_H
#define FORMEDITORVIEW_H

#include <abstractview.h>

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

namespace Utils { class CrumblePath; }

namespace QmlDesigner {

class FormEditorWidget;
class FormEditorNodeInstanceView;
class FormEditorScene;
class NodeInstanceView;

class AbstractFormEditorTool;
class AbstractCustomTool;
class MoveTool;
class SelectionTool;
class ResizeTool;
class DragTool;
class ItemLibraryEntry;
class QmlItemNode;

class QMLDESIGNERCORE_EXPORT FormEditorView : public AbstractView
{
    Q_OBJECT

public:
    FormEditorView(QObject *parent = 0);
    ~FormEditorView();

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;

    void nodeCreated(const ModelNode &createdNode) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;
    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;

    // FormEditorView
    WidgetInfo widgetInfo() override;

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
    void changeToCustomTool();
    void changeToCustomTool(AbstractCustomTool *customTool);

    void registerTool(AbstractCustomTool *tool);

    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;

    void instancesCompleted(const QVector<ModelNode> &completedNodeList) override;
    void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash) override;
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList) override;
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList) override;
    void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void rewriterBeginTransaction() override;
    void rewriterEndTransaction() override;

    double containerPadding() const;
    double spacing() const;
    void deActivateItemCreator();

protected:
    void reset();

protected slots:
    void delayedReset();
    QList<ModelNode> adjustStatesForModelNodes(const QList<ModelNode> &nodeList) const;
    void updateGraphicsIndicators();
    bool isMoveToolAvailable() const;

private: //functions
    void setupFormEditorItemTree(const QmlItemNode &qmlItemNode);
    void removeNodeFromScene(const QmlItemNode &qmlItemNode);
    void hideNodeFromScene(const QmlItemNode &qmlItemNode);

private: //variables
    QPointer<FormEditorWidget> m_formEditorWidget;
    QPointer<FormEditorScene> m_scene;
    QList<AbstractCustomTool*> m_customToolList;
    MoveTool *m_moveTool;
    SelectionTool *m_selectionTool;
    ResizeTool *m_resizeTool;
    DragTool *m_dragTool;
    AbstractFormEditorTool *m_currentTool;
    int m_transactionCounter;
};

}

#endif //FORMEDITORVIEW_H
