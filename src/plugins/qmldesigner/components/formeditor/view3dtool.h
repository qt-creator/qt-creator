// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "abstractcustomtool.h"

#include "modelnode.h"

namespace QmlDesigner {

class View3DTool : public QObject, public AbstractCustomTool
{
    Q_OBJECT
public:
    View3DTool();
    ~View3DTool();

    void mouseMoveEvent(const QList<QGraphicsItem *> &itemList,
                        QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(const QList<QGraphicsItem *> &itemList,
                        QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(const QList<QGraphicsItem *> &itemList,
                           QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

    void dragLeaveEvent(const QList<QGraphicsItem *> &itemList,
                        QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(const QList<QGraphicsItem *> &itemList,
                       QGraphicsSceneDragDropEvent *event) override;

    void itemsAboutToRemoved(const QList<FormEditorItem *> &itemList) override;

    void instancesCompleted(const QList<FormEditorItem *> &itemList) override;
    void instancesParentChanged(const QList<FormEditorItem *> &itemList) override;
    void instancePropertyChange(const QList<QPair<ModelNode, PropertyName>> &propertyList) override;

    void clear() override;

    void formEditorItemsChanged(const QList<FormEditorItem *> &itemList) override;

    int wantHandleItem(const ModelNode &modelNode) const override;

    QString name() const override;

    void selectedItemsChanged(const QList<FormEditorItem *> &itemList) override;

private:
    ModelNode m_view3dNode;
};

} // namespace QmlDesigner
