/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DRAGTOOL_H
#define DRAGTOOL_H

#include "abstractformeditortool.h"
#include "movemanipulator.h"
#include "selectionindicator.h"
#include "resizeindicator.h"

#include <QHash>


namespace QmlDesigner {


class DragTool : public AbstractFormEditorTool
{
public:
    DragTool(FormEditorView* editorView);
    ~DragTool();

    void mousePressEvent(const QList<QGraphicsItem*> &itemList,
                         QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                           QGraphicsSceneMouseEvent *event);
    void mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList,
                               QGraphicsSceneMouseEvent *event);
    void hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent *event);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *keyEvent);

    void itemsAboutToRemoved(const QList<FormEditorItem*> &itemList);

    void selectedItemsChanged(const QList<FormEditorItem*> &itemList);

    void updateMoveManipulator();

    void beginWithPoint(const QPointF &beginPoint);


    virtual void dropEvent(QGraphicsSceneDragDropEvent * event);
    virtual void dragEnterEvent(QGraphicsSceneDragDropEvent * event);
    virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent * event);
    virtual void dragMoveEvent(QGraphicsSceneDragDropEvent * event);

    //void beginWithPoint(const QPointF &beginPoint);

    void clear();

    void formEditorItemsChanged(const QList<FormEditorItem*> &itemList);

protected:


private:

    void createQmlItemNode(const ItemLibraryInfo &ItemLibraryRepresentation, QmlItemNode parentNode, QPointF scenePos);
    void createQmlItemNodeFromImage(const QString &imageName, QmlItemNode parentNode, QPointF scenePos);
    FormEditorItem* calculateContainer(const QPointF &point, FormEditorItem * currentItem = 0);

    void begin(QPointF scenePos);
    void end(QPointF scenePos);
    void move(QPointF scenePos);

    MoveManipulator m_moveManipulator;
    SelectionIndicator m_selectionIndicator;
    QWeakPointer<FormEditorItem> m_movingItem;
    RewriterTransaction m_rewriterTransaction;
    QmlItemNode m_dragNode;
};

}
#endif // DRAGTOOL_H
