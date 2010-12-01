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

#ifndef ZOOMTOOL_H
#define ZOOMTOOL_H

#include "abstractformeditortool.h"
#include "rubberbandselectionmanipulator.h"

QT_FORWARD_DECLARE_CLASS(QAction);

namespace QmlJSDebugger {

class ZoomTool : public AbstractFormEditorTool
{
    Q_OBJECT
public:
    enum ZoomDirection {
        ZoomIn,
        ZoomOut
    };

    explicit ZoomTool(QDeclarativeViewObserver *view);

    virtual ~ZoomTool();

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

    void hoverMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *keyEvent);
    void itemsAboutToRemoved(const QList<QGraphicsItem*> &itemList);

    void clear();
protected:
    void selectedItemsChanged(const QList<QGraphicsItem*> &itemList);

private slots:
    void zoomTo100();
    void zoomIn();
    void zoomOut();

private:
    qreal nextZoomScale(ZoomDirection direction) const;
    void scaleView(const QPointF &centerPos);

private:
    bool m_dragStarted;
    QPoint m_mousePos; // in view coords
    QPointF m_dragBeginPos;
    QAction *m_zoomTo100Action;
    QAction *m_zoomInAction;
    QAction *m_zoomOutAction;
    RubberBandSelectionManipulator *m_rubberbandManipulator;

    qreal m_smoothZoomMultiplier;
    qreal m_currentScale;

};

} // namespace QmlJSDebugger

#endif // ZOOMTOOL_H
