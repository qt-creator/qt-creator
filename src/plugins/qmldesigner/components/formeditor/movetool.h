// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "abstractformeditortool.h"
#include "movemanipulator.h"
#include "selectionindicator.h"
#include "resizeindicator.h"
#include "rotationindicator.h"
#include "anchorindicator.h"
#include "bindingindicator.h"
#include "contentnoteditableindicator.h"

namespace QmlDesigner {

class MoveTool : public AbstractFormEditorTool
{
public:
    MoveTool(FormEditorView* editorView);
    ~MoveTool() override;

    void mousePressEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

    void dragLeaveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent * event) override;
    void dragMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent * event) override;

    void itemsAboutToRemoved(const QList<FormEditorItem*> &itemList) override;

    void selectedItemsChanged(const QList<FormEditorItem*> &itemList) override;

    void instancesCompleted(const QList<FormEditorItem*> &itemList) override;
    void instancesParentChanged(const QList<FormEditorItem *> &itemList) override;
    void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void updateMoveManipulator();

    void beginWithPoint(const QPointF &beginPoint);

    void clear() override;
    void start() override;

    void formEditorItemsChanged(const QList<FormEditorItem*> &itemList) override;

    void focusLost() override;

protected:
    static bool haveSameParent(const QList<FormEditorItem*> &itemList);

    static QList<FormEditorItem*> movingItems(const QList<FormEditorItem*> &selectedItemList);


    static bool isAncestorOfAllItems(FormEditorItem* maybeAncestorItem,
                                    const QList<FormEditorItem*> &itemList);
    static FormEditorItem* ancestorIfOtherItemsAreChild(const QList<FormEditorItem*> &itemList);

private:
    MoveManipulator m_moveManipulator;
    SelectionIndicator m_selectionIndicator;
    ResizeIndicator m_resizeIndicator;
    RotationIndicator m_rotationIndicator;
    AnchorIndicator m_anchorIndicator;
    BindingIndicator m_bindingIndicator;
    ContentNotEditableIndicator m_contentNotEditableIndicator;
    QList<FormEditorItem*> m_movingItems;
};

} // namespace QmlDesigner
