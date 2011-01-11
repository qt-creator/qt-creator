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

#ifndef SELECTIONTOOL_H
#define SELECTIONTOOL_H


#include "abstractformeditortool.h"
#include "rubberbandselectionmanipulator.h"
#include "singleselectionmanipulator.h"
#include "selectionindicator.h"

#include <QHash>
#include <QList>
#include <QTime>
#include <QAction>

QT_FORWARD_DECLARE_CLASS(QGraphicsItem);
QT_FORWARD_DECLARE_CLASS(QMouseEvent);
QT_FORWARD_DECLARE_CLASS(QKeyEvent);

namespace QmlJSDebugger {

class SelectionTool : public AbstractFormEditorTool
{
    Q_OBJECT

public:
    SelectionTool(QDeclarativeViewObserver* editorView);
    ~SelectionTool();

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
    SingleSelectionManipulator::SelectionType getSelectionType(Qt::KeyboardModifiers modifiers);
    bool alreadySelected(const QList<QGraphicsItem*> &itemList) const;

private:
    bool m_rubberbandSelectionMode;
    RubberBandSelectionManipulator m_rubberbandSelectionManipulator;
    SingleSelectionManipulator m_singleSelectionManipulator;
    SelectionIndicator m_selectionIndicator;
    //ResizeIndicator m_resizeIndicator;
    QTime m_mousePressTimer;
    bool m_selectOnlyContentItems;

    QList<QWeakPointer<QGraphicsObject> > m_selectedItemList;

    QList<QGraphicsItem*> m_contextMenuItemList;
};

}

#endif // SELECTIONTOOL_H
