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

#ifndef BOUNDINGRECTHIGHLIGHTER_H
#define BOUNDINGRECTHIGHLIGHTER_H

#include <QObject>
#include <QWeakPointer>

#include "layeritem.h"

QT_FORWARD_DECLARE_CLASS(QGraphicsItem);
QT_FORWARD_DECLARE_CLASS(QPainter);
QT_FORWARD_DECLARE_CLASS(QWidget);
QT_FORWARD_DECLARE_CLASS(QStyleOptionGraphicsItem);
QT_FORWARD_DECLARE_CLASS(QTimer);

namespace QmlJSDebugger {

class QDeclarativeViewObserver;
class BoundingBox;

class BoundingRectHighlighter : public LayerItem
{
    Q_OBJECT
public:
    explicit BoundingRectHighlighter(QDeclarativeViewObserver *view);
    ~BoundingRectHighlighter();
    void clear();
    void highlight(QList<QGraphicsObject*> items);
    void highlight(QGraphicsObject* item);

private slots:
    void refresh();
    void animTimeout();
    void itemDestroyed(QObject *);

private:
    BoundingBox *boxFor(QGraphicsObject *item) const;
    void highlightAll(bool animate);
    BoundingBox *createBoundingBox(QGraphicsObject *itemToHighlight);
    void removeBoundingBox(BoundingBox *box);
    void freeBoundingBox(BoundingBox *box);

private:
    Q_DISABLE_COPY(BoundingRectHighlighter);

    QDeclarativeViewObserver *m_view;
    QList<BoundingBox* > m_boxes;
    QList<BoundingBox* > m_freeBoxes;
    QTimer *m_animTimer;
    qreal m_animScale;
    int m_animFrame;

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
    Q_DISABLE_COPY(BoundingBox);

};

class BoundingBoxPolygonItem : public QGraphicsPolygonItem
{
public:
    explicit BoundingBoxPolygonItem(QGraphicsItem *item);
    int type() const;
};

} // namespace QmlJSDebugger

#endif // BOUNDINGRECTHIGHLIGHTER_H
