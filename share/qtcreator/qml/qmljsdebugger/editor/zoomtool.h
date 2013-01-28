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

#ifndef ZOOMTOOL_H
#define ZOOMTOOL_H

#include "abstractliveedittool.h"
#include "liverubberbandselectionmanipulator.h"

QT_FORWARD_DECLARE_CLASS(QAction)

namespace QmlJSDebugger {

class ZoomTool : public AbstractLiveEditTool
{
    Q_OBJECT
public:
    enum ZoomDirection {
        ZoomIn,
        ZoomOut
    };

    explicit ZoomTool(QDeclarativeViewInspector *view);

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
    LiveRubberBandSelectionManipulator *m_rubberbandManipulator;

    qreal m_smoothZoomMultiplier;
    qreal m_currentScale;

};

} // namespace QmlJSDebugger

#endif // ZOOMTOOL_H
