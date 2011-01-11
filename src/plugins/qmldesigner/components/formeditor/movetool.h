/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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

    void itemsAboutToRemoved(const QList<FormEditorItem*> &itemList);

    void selectedItemsChanged(const QList<FormEditorItem*> &itemList);

    void instancesCompleted(const QList<FormEditorItem*> &/*itemList*/);

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
