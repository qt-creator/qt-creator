/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
#pragma once

#include <abstractview.h>

#include <functional>
#include <memory>

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

    virtual void documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings) override;

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
    void changeCurrentToolTo(AbstractFormEditorTool *customTool);

    void registerTool(AbstractCustomTool *tool);

    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;

    void instancesCompleted(const QVector<ModelNode> &completedNodeList) override;
    void instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangedHash) override;
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList) override;
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList) override;
    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void rewriterBeginTransaction() override;
    void rewriterEndTransaction() override;

    double containerPadding() const;
    double spacing() const;
    void deActivateItemCreator();
    void gotoError(int, int);
    void setGotoErrorCallback(std::function<void(int, int)> gotoErrorCallback);

    void exportAsImage();

protected:
    void reset();

protected slots:
    void delayedReset();
    bool isMoveToolAvailable() const;

private: //functions
    void setupFormEditorItemTree(const QmlItemNode &qmlItemNode);
    void removeNodeFromScene(const QmlItemNode &qmlItemNode);
    void hideNodeFromScene(const QmlItemNode &qmlItemNode);
    void createFormEditorWidget();

private: //variables
    QPointer<FormEditorWidget> m_formEditorWidget;
    QPointer<FormEditorScene> m_scene;
    QList<AbstractCustomTool*> m_customToolList;
    std::unique_ptr<MoveTool> m_moveTool;
    std::unique_ptr<SelectionTool> m_selectionTool;
    std::unique_ptr<ResizeTool> m_resizeTool;
    std::unique_ptr<DragTool> m_dragTool;
    AbstractFormEditorTool *m_currentTool = nullptr;
    int m_transactionCounter = 0;
    std::function<void(int, int)> m_gotoErrorCallback;
};

} // namespace QmlDesigner
