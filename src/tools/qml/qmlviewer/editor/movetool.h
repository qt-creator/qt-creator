/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef MOVETOOL_H
#define MOVETOOL_H

#include "abstractformeditortool.h"
#include "movemanipulator.h"
#include "resizeindicator.h"
#include "selectionindicator.h"
#include <QHash>

namespace QmlViewer {

class MoveTool : public AbstractFormEditorTool
{
public:
    MoveTool(QDeclarativeDesignView* editorView);
    ~MoveTool();

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void hoverMoveEvent(QMouseEvent *event);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *keyEvent);
    void wheelEvent(QWheelEvent *event);

    void itemsAboutToRemoved(const QList<QGraphicsItem*> &itemList);

    void selectedItemsChanged(const QList<QGraphicsItem*> &itemList);

    void updateMoveManipulator();

    void beginWithPoint(const QPointF &beginPoint);

    void clear();

    void graphicsObjectsChanged(const QList<QGraphicsObject*> &itemList);

protected:
    static bool haveSameParent(const QList<QGraphicsItem*> &itemList);

    QList<QGraphicsItem*> movingItems(const QList<QGraphicsItem*> &selectedItemList);

    static QGraphicsItem* containerFormEditorItem(const QList<QGraphicsItem*> &itemUnderMouseList,
                                            const QList<QGraphicsItem*> &selectedItemList);

    static bool isAncestorOfAllItems(QGraphicsItem* maybeAncestorItem,
                                    const QList<QGraphicsItem*> &itemList);
    static QGraphicsItem* ancestorIfOtherItemsAreChild(const QList<QGraphicsItem*> &itemList);

private:
    bool m_moving;
    MoveManipulator m_moveManipulator;
    SelectionIndicator m_selectionIndicator;
    ResizeIndicator m_resizeIndicator;
    QList<QGraphicsItem*> m_movingItems;
};

}

#endif // MOVETOOL_H
