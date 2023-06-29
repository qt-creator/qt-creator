// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <qmldesignercomponents_global.h>

#include <abstractview.h>

#include <QPicture>

#include <functional>
#include <memory>
#include <vector>

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
class RotationTool;
class ResizeTool;
class DragTool;
class ItemLibraryEntry;
class QmlItemNode;

class QMLDESIGNERCOMPONENTS_EXPORT FormEditorView : public AbstractView
{
    Q_OBJECT

public:
    FormEditorView(ExternalDependenciesInterface &externalDependencies);
    ~FormEditorView() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void importsChanged(const Imports &addedImports, const Imports &removedImports) override;

    void nodeCreated(const ModelNode &createdNode) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeSourceChanged(const ModelNode &node, const QString &newNodeSource) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;

    void variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;

    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;

    void documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings) override;

    void customNotification(const AbstractView *view,
                            const QString &identifier,
                            const QList<ModelNode> &nodeList,
                            const QList<QVariant> &data) override;

    void currentStateChanged(const ModelNode &node) override;

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
    void resetToSelectionTool();
    void changeToRotationTool();
    void changeToResizeTool();
    void changeToTransformTools();
    void changeToCustomTool();
    void changeCurrentToolTo(AbstractFormEditorTool *customTool);

    void registerTool(std::unique_ptr<AbstractCustomTool> &&tool);

    void auxiliaryDataChanged(const ModelNode &node,
                              AuxiliaryDataKeyView name,
                              const QVariant &data) override;

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
    QImage takeFormEditorScreenshot();
    QPicture renderToPicture() const;

    void setupFormEditorWidget();
    void cleanupToolsAndScene();

protected:
    void reset();
    void delayedReset();
    bool isMoveToolAvailable() const;

private:
    void setupFormEditorItemTree(const QmlItemNode &qmlItemNode);
    void removeNodeFromScene(const QmlItemNode &qmlItemNode);
    void createFormEditorWidget();
    void temporaryBlockView(int duration = 100);
    void resetNodeInstanceView();
    void addOrRemoveFormEditorItem(const ModelNode &node);
    void checkRootModelNode();
    void setupFormEditor3DView();
    void setupRootItemSize();

    QPointer<FormEditorWidget> m_formEditorWidget;
    QPointer<FormEditorScene> m_scene;
    std::vector<std::unique_ptr<AbstractCustomTool>> m_customTools;
    std::unique_ptr<MoveTool> m_moveTool;
    std::unique_ptr<SelectionTool> m_selectionTool;
    std::unique_ptr<RotationTool> m_rotationTool;
    std::unique_ptr<ResizeTool> m_resizeTool;
    std::unique_ptr<DragTool> m_dragTool;
    AbstractFormEditorTool *m_currentTool = nullptr;
    int m_transactionCounter = 0;
    std::function<void(int, int)> m_gotoErrorCallback;
};

} // namespace QmlDesigner
