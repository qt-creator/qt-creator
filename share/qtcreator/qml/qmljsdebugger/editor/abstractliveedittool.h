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

#ifndef ABSTRACTLIVEEDITTOOL_H
#define ABSTRACTLIVEEDITTOOL_H

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
class QDeclarativeView;
QT_END_NAMESPACE

namespace QmlJSDebugger {

class QDeclarativeViewInspector;

class FormEditorView;

class AbstractLiveEditTool : public QObject
{
    Q_OBJECT
public:
    AbstractLiveEditTool(QDeclarativeViewInspector *inspector);

    virtual ~AbstractLiveEditTool();

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

    void updateSelectedItems();
    QList<QGraphicsItem*> items() const;

    void enterContext(QGraphicsItem *itemToEnter);

    bool topItemIsMovable(const QList<QGraphicsItem*> &itemList);
    bool topItemIsResizeHandle(const QList<QGraphicsItem*> &itemList);
    bool topSelectedItemIsMovable(const QList<QGraphicsItem*> &itemList);

    static QString titleForItem(QGraphicsItem *item);
    static QList<QGraphicsObject*> toGraphicsObjectList(const QList<QGraphicsItem*> &itemList);
    static QGraphicsItem* topMovableGraphicsItem(const QList<QGraphicsItem*> &itemList);
    static QDeclarativeItem* topMovableDeclarativeItem(const QList<QGraphicsItem*> &itemList);
    static QDeclarativeItem *toQDeclarativeItem(QGraphicsItem *item);

protected:
    virtual void selectedItemsChanged(const QList<QGraphicsItem*> &objectList) = 0;

    QDeclarativeViewInspector *inspector() const;
    QDeclarativeView *view() const;
    QGraphicsScene *scene() const;

private:
    QDeclarativeViewInspector *m_inspector;
    QList<QGraphicsItem*> m_itemList;
};

}

#endif // ABSTRACTLIVEEDITTOOL_H
