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

#ifndef DRAGTOOL_H
#define DRAGTOOL_H

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
    virtual ~DragTool();

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

protected:
    void abort();
    void createQmlItemNode(const ItemLibraryEntry &itemLibraryEntry, const QmlItemNode &parentNode, const QPointF &scenePos);
    void createQmlItemNodeFromImage(const QString &imageName, const QmlItemNode &parentNode, const QPointF &scenePos);
    FormEditorItem *targetContainerOrRootItem(const QList<QGraphicsItem*> &itemList, FormEditorItem *urrentItem = 0);
    void begin(QPointF scenePos);
    void end();
    void end(Snapper::Snapping useSnapping);
    void move(const QPointF &scenePos, const QList<QGraphicsItem *> &itemList);
    void createDragNode(const QMimeData *mimeData, const QPointF &scenePosition, const QList<QGraphicsItem *> &itemList);
    void commitTransaction();

private:
    MoveManipulator m_moveManipulator;
    SelectionIndicator m_selectionIndicator;
    QPointer<FormEditorItem> m_movingItem;
    RewriterTransaction m_rewriterTransaction;
    QmlItemNode m_dragNode;
    bool m_blockMove;
    QPointF m_startPoint;
    bool m_isAborted;
};


}
#endif // DRAGTOOL_H
