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

#ifndef RESIZETOOL_H
#define RESIZETOOL_H

#include "abstractformeditortool.h"
#include "selectionindicator.h"
#include "resizeindicator.h"
#include "resizemanipulator.h"


namespace QmlDesigner {

class ResizeTool : public AbstractFormEditorTool
{
public:
    ResizeTool(FormEditorView* editorView);
    ~ResizeTool();

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

    void dragLeaveEvent(QGraphicsSceneDragDropEvent * event);
    void dragMoveEvent(QGraphicsSceneDragDropEvent * event);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *keyEvent);

    void itemsAboutToRemoved(const QList<FormEditorItem*> &itemList);

    void selectedItemsChanged(const QList<FormEditorItem*> &itemList);

    void clear();

    void formEditorItemsChanged(const QList<FormEditorItem*> &itemList);
    void instancesParentChanged(const QList<FormEditorItem *> &itemList);

    void instancesCompleted(const QList<FormEditorItem*> &itemList);


private:
    SelectionIndicator m_selectionIndicator;
    ResizeIndicator m_resizeIndicator;
    ResizeManipulator m_resizeManipulator;
};

}

#endif // RESIZETOOL_H
