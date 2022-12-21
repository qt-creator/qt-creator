// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "abstractformeditortool.h"
#include "movemanipulator.h"
#include "selectionindicator.h"

#include <QObject>
#include <QScopedPointer>
#include <QPointer>

namespace QmlDesigner {

class DragTool;

class DragTool : public AbstractFormEditorTool
{
public:
    DragTool(FormEditorView *editorView);
    ~DragTool() override;

    void mousePressEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;

    void hoverMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;

    void dropEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event) override;
    void dragEnterEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event) override;
    void dragLeaveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

    void itemsAboutToRemoved(const QList<FormEditorItem*> &itemList) override;

    void selectedItemsChanged(const QList<FormEditorItem*> &itemList) override;
    void instancesParentChanged(const QList<FormEditorItem *> &itemList) override;
    void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void updateMoveManipulator();

    void beginWithPoint(const QPointF &beginPoint);

    void clear() override;

    void formEditorItemsChanged(const QList<FormEditorItem*> &itemList) override;

    void instancesCompleted(const QList<FormEditorItem*> &itemList) override;

    void clearMoveDelay();

    void focusLost() override;

protected:
    void abort();
    void createQmlItemNode(const ItemLibraryEntry &itemLibraryEntry, const QmlItemNode &parentNode, const QPointF &scenePos);
    void createQmlItemNodeFromImage(const QString &imagePath, const QmlItemNode &parentNode, const QPointF &scenePos);
    void createQmlItemNodeFromFont(const QString &fontPath, const QmlItemNode &parentNode, const QPointF &scenePos);
    FormEditorItem *targetContainerOrRootItem(const QList<QGraphicsItem *> &itemList,
                                              const QList<FormEditorItem *> &currentItems = {});
    void begin(QPointF scenePos);
    void end();
    void end(Snapper::Snapping useSnapping);
    void move(const QPointF &scenePos, const QList<QGraphicsItem *> &itemList);
    void createDragNodes(const QMimeData *mimeData, const QPointF &scenePosition, const QList<QGraphicsItem *> &itemList);
    void commitTransaction();
    void handleView3dDrop();

private:
    MoveManipulator m_moveManipulator;
    SelectionIndicator m_selectionIndicator;
    QList<FormEditorItem *> m_movingItems;
    RewriterTransaction m_rewriterTransaction;
    QList<QmlItemNode> m_dragNodes;
    bool m_blockMove;
    QPointF m_startPoint;
    bool m_isAborted = false;
};

} // namespace QmlDesigner
