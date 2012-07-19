/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef ANCHORTOOL_H
#define ANCHORTOOL_H

#include <QTimeLine>

#include "abstractformeditortool.h"

#include "anchorlineindicator.h"
#include "anchorindicator.h"
#include "anchormanipulator.h"

namespace QmlDesigner {

class AnchorLineHandleItem;

class AnchorTool : public QObject, public AbstractFormEditorTool
{
    Q_OBJECT
public:
    AnchorTool(FormEditorView* editorView);
    ~AnchorTool();

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

    void clear();

    void formEditorItemsChanged(const QList<FormEditorItem*> &itemList);

    void instancesCompleted(const QList<FormEditorItem*> &itemList);
    void instancesParentChanged(const QList<FormEditorItem *> &itemList);


    static AnchorLineHandleItem* topAnchorLineHandleItem(const QList<QGraphicsItem*> & itemList);

private slots:
    void checkIfStillHovering();

private: //variables
    AnchorLineIndicator m_anchorLineIndicator;
    AnchorIndicator m_anchorIndicator;
    AnchorManipulator m_anchorManipulator;
    QTimeLine m_hoverTimeLine;
    QPointF m_lastMousePosition;
    AnchorLineHandleItem *m_lastAnchorLineHandleItem;
};

}
#endif // ANCHORTOOL_H
