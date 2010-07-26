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

#ifndef ABSTRACTFORMEDITORTOOL_H
#define ABSTRACTFORMEDITORTOOL_H

#include <qglobal.h>
#include <QList>
#include <QObject>

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QGraphicsItem;
class QDeclarativeItem;
class QKeyEvent;
class QGraphicsScene;
class QGraphicsObject;
class QWheelEvent;
QT_END_NAMESPACE

namespace QmlViewer {

class QDeclarativeDesignView;


class FormEditorView;

class AbstractFormEditorTool : public QObject
{
    Q_OBJECT
public:
    AbstractFormEditorTool(QDeclarativeDesignView* view);

    virtual ~AbstractFormEditorTool();

    virtual void mousePressEvent(QMouseEvent *event) = 0;
    virtual void mouseMoveEvent(QMouseEvent *event) = 0;
    virtual void mouseReleaseEvent(QMouseEvent *event) = 0;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) = 0;

    virtual void hoverMoveEvent(QMouseEvent *event) = 0;
    virtual void wheelEvent(QWheelEvent *event) = 0;

    virtual void keyPressEvent(QKeyEvent *event) = 0;
    virtual void keyReleaseEvent(QKeyEvent *keyEvent) = 0;
    virtual void itemsAboutToRemoved(const QList<QGraphicsItem*> &itemList) = 0;

    virtual void clear() = 0;
    virtual void graphicsObjectsChanged(const QList<QGraphicsObject*> &itemList) = 0;

    void updateSelectedItems();
    QList<QGraphicsItem*> items() const;

    bool topItemIsMovable(const QList<QGraphicsItem*> &itemList);
    bool topItemIsResizeHandle(const QList<QGraphicsItem*> &itemList);
    bool topSelectedItemIsMovable(const QList<QGraphicsItem*> &itemList);

    static QString titleForItem(QGraphicsItem *item);
    static QList<QObject*> toObjectList(const QList<QGraphicsItem*> &itemList);
    static QList<QGraphicsObject*> toGraphicsObjectList(const QList<QGraphicsItem*> &itemList);
    static QGraphicsItem* topMovableGraphicsItem(const QList<QGraphicsItem*> &itemList);
    static QDeclarativeItem* topMovableDeclarativeItem(const QList<QGraphicsItem*> &itemList);
    static QDeclarativeItem *toQDeclarativeItem(QGraphicsItem *item);

protected:
    virtual void selectedItemsChanged(const QList<QGraphicsItem*> &objectList) = 0;

    QDeclarativeDesignView *view() const;
    QGraphicsScene* scene() const;

private:
    QDeclarativeDesignView *m_view;
    QList<QGraphicsItem*> m_itemList;
};

}

#endif // ABSTRACTFORMEDITORTOOL_H
