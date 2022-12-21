// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractcustomtool.h>
#include "selectionindicator.h"

#include <QHash>
#include <QPointer>
#include <QFileDialog>

namespace QmlDesigner {

class SelectionContext;

class SourceTool : public QObject, public AbstractCustomTool
{
    Q_OBJECT
public:
    SourceTool();
    ~SourceTool() override;

    void mousePressEvent(const QList<QGraphicsItem*> &itemList,
                         QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                           QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList,
                               QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

    void dragLeaveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneDragDropEvent * event) override;
    void dragMoveEvent(const QList<QGraphicsItem*> &itemList,
                       QGraphicsSceneDragDropEvent * event) override;

    void itemsAboutToRemoved(const QList<FormEditorItem*> &itemList) override;

    void selectedItemsChanged(const QList<FormEditorItem*> &itemList) override;

    void instancesCompleted(const QList<FormEditorItem*> &itemList) override;
    void instancesParentChanged(const QList<FormEditorItem *> &itemList) override;
    void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void clear() override;

    void formEditorItemsChanged(const QList<FormEditorItem*> &itemList) override;

    int wantHandleItem(const ModelNode &modelNode) const override;

    QString name() const override;

private:
    /* private methods */
    void fileSelected(const QString &fileName);


    /* members */
    FormEditorItem *m_formEditorItem = nullptr;
    QString m_oldFileName;
};

}
