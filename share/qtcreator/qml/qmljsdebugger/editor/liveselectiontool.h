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

#ifndef LIVESELECTIONTOOL_H
#define LIVESELECTIONTOOL_H


#include "abstractliveedittool.h"
#include "liverubberbandselectionmanipulator.h"
#include "livesingleselectionmanipulator.h"
#include "liveselectionindicator.h"

#include <QList>
#include <QTime>

QT_FORWARD_DECLARE_CLASS(QGraphicsItem)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QKeyEvent)
QT_FORWARD_DECLARE_CLASS(QAction)

namespace QmlJSDebugger {

class LiveSelectionTool : public AbstractLiveEditTool
{
    Q_OBJECT

public:
    LiveSelectionTool(QDeclarativeViewInspector* editorView);
    ~LiveSelectionTool();

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void hoverMoveEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *keyEvent);
    void wheelEvent(QWheelEvent *event);

    void itemsAboutToRemoved(const QList<QGraphicsItem*> &itemList);
//    QVariant itemChange(const QList<QGraphicsItem*> &itemList,
//                        QGraphicsItem::GraphicsItemChange change,
//                        const QVariant &value );

//    void update();

    void clear();

    void selectedItemsChanged(const QList<QGraphicsItem*> &itemList);

    void selectUnderPoint(QMouseEvent *event);

    void setSelectOnlyContentItems(bool selectOnlyContentItems);

    void setRubberbandSelectionMode(bool value);

private slots:
    void contextMenuElementSelected();
    void contextMenuElementHovered(QAction *action);
    void repaintBoundingRects();

private:
    void createContextMenu(QList<QGraphicsItem*> itemList, QPoint globalPos);
    LiveSingleSelectionManipulator::SelectionType getSelectionType(Qt::KeyboardModifiers modifiers);
    bool alreadySelected(const QList<QGraphicsItem*> &itemList) const;

private:
    bool m_rubberbandSelectionMode;
    LiveRubberBandSelectionManipulator m_rubberbandSelectionManipulator;
    LiveSingleSelectionManipulator m_singleSelectionManipulator;
    LiveSelectionIndicator m_selectionIndicator;
    //ResizeIndicator m_resizeIndicator;
    QTime m_mousePressTimer;
    bool m_selectOnlyContentItems;

    QList<QWeakPointer<QGraphicsObject> > m_selectedItemList;

    QList<QGraphicsItem*> m_contextMenuItemList;
};

} // namespace QmlJSDebugger

#endif // LIVESELECTIONTOOL_H
