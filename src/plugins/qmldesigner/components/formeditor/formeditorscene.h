// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <qmlitemnode.h>
#include "abstractformeditortool.h"

#include <QGraphicsScene>
#include <QPointer>
#include <QHash>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

namespace QmlDesigner {

class FormEditorWidget;
class FormEditorItem;
class FormEditorView;
class LayerItem;

class QMLDESIGNERCOMPONENTS_EXPORT FormEditorScene : public QGraphicsScene
{
    Q_OBJECT

    friend FormEditorItem;
    friend FormEditorView;

public:

    enum ItemType {
        Default,
        Flow,
        FlowAction,
        FlowTransition,
        FlowDecision,
        FlowWildcard,
        Preview3d
    };

    FormEditorScene(FormEditorWidget *widget, FormEditorView *editorView);
    ~FormEditorScene() override;
    FormEditorItem *addFormEditorItem(const QmlItemNode &qmlItemNode, ItemType type = Default);

    FormEditorItem* itemForQmlItemNode(const QmlItemNode &qmlItemNode) const;

    QList<FormEditorItem*> itemsForQmlItemNodes(const QList<QmlItemNode> &nodeList) const;
    QList<FormEditorItem*> allFormEditorItems() const;

    void updateAllFormEditorItems();

    void setupScene();
    void resetScene();

    double canvasWidth() const;
    double canvasHeight() const;

    void synchronizeTransformation(FormEditorItem *item);
    void synchronizeParent(const QmlItemNode &qmlItemNode);
    void synchronizeOtherProperty(FormEditorItem *item, const QByteArray &propertyName);

    FormEditorItem* calulateNewParent(FormEditorItem *widget);
    LayerItem* manipulatorLayerItem() const;
    LayerItem* formLayerItem() const;
    FormEditorView *editorView() const;

    FormEditorItem *rootFormEditorItem() const;

    void reparentItem(const QmlItemNode &node, const QmlItemNode &newParent);

    void clearFormEditorItems();

    void highlightBoundingRect(FormEditorItem *formEditorItem);

    void setShowBoundingRects(bool show);
    bool showBoundingRects() const;

    bool annotationVisibility() const;
    void setAnnotationVisibility(bool status);

protected:
    bool event(QEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent * event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent * event) override;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent * event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent * event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

    void keyPressEvent(QKeyEvent *keyEvent) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

    void focusOutEvent(QFocusEvent *focusEvent) override;
    void focusInEvent(QFocusEvent *focusEvent) override;

private:
    QList<QGraphicsItem *> removeLayerItems(const QList<QGraphicsItem *> &itemList);
    QList<QGraphicsItem *> itemsAt(const QPointF &pos);

    AbstractFormEditorTool* currentTool() const;
    void removeItemFromHash(FormEditorItem*);

    FormEditorView *m_editorView;
    AbstractFormEditorTool *m_currentTool;
    QHash<QmlItemNode, FormEditorItem*> m_qmlItemNodeItemHash;
    QPointer<LayerItem> m_formLayerItem;
    QPointer<LayerItem> m_manipulatorLayerItem;
    ModelNode m_dragNode;
    bool m_showBoundingRects;
    bool m_annotationVisibility;
    QElapsedTimer m_usageTimer;
};

} // namespace QmlDesigner
