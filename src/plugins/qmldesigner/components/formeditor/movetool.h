/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MOVETOOL_H
#define MOVETOOL_H

#include "abstractformeditortool.h"
#include "movemanipulator.h"
#include "selectionindicator.h"
#include "resizeindicator.h"

#include <QHash>


namespace QmlDesigner {


class MoveTool : public AbstractFormEditorTool
{
public:
    MoveTool(FormEditorView* editorView);
    ~MoveTool();

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

    void dragLeaveEvent(QGraphicsSceneDragDropEvent * event);
    void dragMoveEvent(QGraphicsSceneDragDropEvent * event);

    void itemsAboutToRemoved(const QList<FormEditorItem*> &itemList);

    void selectedItemsChanged(const QList<FormEditorItem*> &itemList);

    void instancesCompleted(const QList<FormEditorItem*> &itemList);
    void instancesParentChanged(const QList<FormEditorItem *> &itemList);

    void updateMoveManipulator();

    void beginWithPoint(const QPointF &beginPoint);

    void clear();

    void formEditorItemsChanged(const QList<FormEditorItem*> &itemList);

protected:
    static bool haveSameParent(const QList<FormEditorItem*> &itemList);

    static QList<FormEditorItem*> movingItems(const QList<FormEditorItem*> &selectedItemList);

    static FormEditorItem* containerFormEditorItem(const QList<QGraphicsItem*> &itemUnderMouseList,
                                            const QList<FormEditorItem*> &selectedItemList);

    static bool isAncestorOfAllItems(FormEditorItem* maybeAncestorItem,
                                    const QList<FormEditorItem*> &itemList);
    static FormEditorItem* ancestorIfOtherItemsAreChild(const QList<FormEditorItem*> &itemList);

private:
    MoveManipulator m_moveManipulator;
    SelectionIndicator m_selectionIndicator;
    ResizeIndicator m_resizeIndicator;
    QList<FormEditorItem*> m_movingItems;
};

}
#endif // MOVETOOL_H
