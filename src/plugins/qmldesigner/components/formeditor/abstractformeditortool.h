// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "formeditoritem.h"

#include <qmldesignercomponents_global.h>

#include "snapper.h"

QT_BEGIN_NAMESPACE
class QGraphicsItem;
QT_END_NAMESPACE

namespace QmlDesigner {

class FormEditorView;

class QMLDESIGNERCOMPONENTS_EXPORT AbstractFormEditorTool
{
    friend FormEditorView;

public:
    AbstractFormEditorTool(FormEditorView* view);

    virtual ~AbstractFormEditorTool();

    virtual void mousePressEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseReleaseEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event);
    virtual void mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event);

    virtual void hoverMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneMouseEvent *event) = 0;

    virtual void dropEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event);
    virtual void dragEnterEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event);
    virtual void dragLeaveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event) = 0;
    virtual void dragMoveEvent(const QList<QGraphicsItem*> &itemList, QGraphicsSceneDragDropEvent *event) = 0;

    virtual void keyPressEvent(QKeyEvent *event) = 0;
    virtual void keyReleaseEvent(QKeyEvent *keyEvent) = 0;

    virtual void itemsAboutToRemoved(const QList<FormEditorItem*> &itemList) = 0;
    //    virtual QVariant itemChange(QList<QGraphicsItem*> itemList,
//                        QGraphicsItem::GraphicsItemChange change,
//                        const QVariant &value ) = 0;
//    virtual void update() = 0;
    virtual void clear();
    virtual void start();

    virtual void formEditorItemsChanged(const QList<FormEditorItem*> &itemList) = 0;

    virtual void instancesCompleted(const QList<FormEditorItem*> &itemList) = 0;
    virtual void instancesParentChanged(const QList<FormEditorItem*> &itemList) = 0;
    virtual void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList) = 0;

    virtual void focusLost() = 0;

    void setItems(const QList<FormEditorItem*> &itemList);
    QList<FormEditorItem*> items() const;

    static QList<FormEditorItem*> toFormEditorItemList(const QList<QGraphicsItem*> &itemList);
    static QGraphicsItem* topMovableGraphicsItem(const QList<QGraphicsItem*> &itemList);
    static FormEditorItem* topMovableFormEditorItem(const QList<QGraphicsItem*> &itemList, bool selectOnlyContentItems);
    bool topItemIsMovable(const QList<QGraphicsItem*> &itemList);
    bool topSelectedItemIsMovable(const QList<QGraphicsItem*> &itemList);
    bool selectedItemCursorInMovableArea(const QPointF &pos);
    bool topItemIsResizeHandle(const QList<QGraphicsItem*> &itemList);

    QList<FormEditorItem*> filterSelectedModelNodes(const QList<FormEditorItem*> &itemList) const;

    FormEditorItem *nearestFormEditorItem(const QPointF &point, const QList<QGraphicsItem *> &itemList);

protected:
    virtual void selectedItemsChanged(const QList<FormEditorItem*> &itemList) = 0;
    virtual void showContextMenu(QGraphicsSceneMouseEvent *event);
    Snapper::Snapping generateUseSnapping(Qt::KeyboardModifiers keyboardModifier) const;
    FormEditorItem *containerFormEditorItem(const QList<QGraphicsItem*> &itemUnderMouseList, const QList<FormEditorItem*> &selectedItemList) const;

    FormEditorView *view() const;
    void setView(FormEditorView *view);
    FormEditorScene* scene() const;
private:
    FormEditorView *m_view;
    QList<FormEditorItem*> m_itemList;
};

} // namespace QmlDesigner
