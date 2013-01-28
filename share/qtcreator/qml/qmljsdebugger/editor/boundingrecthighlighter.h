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

#ifndef BOUNDINGRECTHIGHLIGHTER_H
#define BOUNDINGRECTHIGHLIGHTER_H

#include "livelayeritem.h"

#include <QObject>
#include <QWeakPointer>

QT_FORWARD_DECLARE_CLASS(QGraphicsItem)
QT_FORWARD_DECLARE_CLASS(QPainter)
QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QStyleOptionGraphicsItem)
QT_FORWARD_DECLARE_CLASS(QTimer)

namespace QmlJSDebugger {

class QDeclarativeViewInspector;
class BoundingBox;

class BoundingRectHighlighter : public LiveLayerItem
{
    Q_OBJECT
public:
    explicit BoundingRectHighlighter(QDeclarativeViewInspector *view);
    ~BoundingRectHighlighter();
    void clear();
    void highlight(QList<QGraphicsObject*> items);
    void highlight(QGraphicsObject* item);

private slots:
    void refresh();
    void itemDestroyed(QObject *);

private:
    BoundingBox *boxFor(QGraphicsObject *item) const;
    void highlightAll();
    BoundingBox *createBoundingBox(QGraphicsObject *itemToHighlight);
    void removeBoundingBox(BoundingBox *box);
    void freeBoundingBox(BoundingBox *box);

private:
    Q_DISABLE_COPY(BoundingRectHighlighter)

    QDeclarativeViewInspector *m_view;
    QList<BoundingBox* > m_boxes;
    QList<BoundingBox* > m_freeBoxes;
};

class BoundingBox : public QObject
{
    Q_OBJECT
public:
    explicit BoundingBox(QGraphicsObject *itemToHighlight, QGraphicsItem *parentItem,
                         QObject *parent = 0);
    ~BoundingBox();
    QWeakPointer<QGraphicsObject> highlightedObject;
    QGraphicsPolygonItem *highlightPolygon;
    QGraphicsPolygonItem *highlightPolygonEdge;

private:
    Q_DISABLE_COPY(BoundingBox)

};

class BoundingBoxPolygonItem : public QGraphicsPolygonItem
{
public:
    explicit BoundingBoxPolygonItem(QGraphicsItem *item);
    int type() const;
};

} // namespace QmlJSDebugger

#endif // BOUNDINGRECTHIGHLIGHTER_H
