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

#include "boundingrecthighlighter.h"
#include "qdeclarativeviewobserver.h"
#include "qmlobserverconstants.h"

#include <QGraphicsPolygonItem>
#include <QTimer>
#include <QObject>

#include <QDebug>

namespace QmlJSDebugger {

const qreal AnimDelta = 0.025f;
const int AnimInterval = 30;
const int AnimFrames = 10;

BoundingBox::BoundingBox(QGraphicsObject *itemToHighlight, QGraphicsItem *parentItem,
                         QObject *parent)
    : QObject(parent),
      highlightedObject(itemToHighlight),
      highlightPolygon(0),
      highlightPolygonEdge(0)
{
    highlightPolygon = new BoundingBoxPolygonItem(parentItem);
    highlightPolygonEdge = new BoundingBoxPolygonItem(parentItem);

    highlightPolygon->setPen(QPen(QColor(0, 22, 159)));
    highlightPolygonEdge->setPen(QPen(QColor(158, 199, 255)));

    highlightPolygon->setFlag(QGraphicsItem::ItemIsSelectable, false);
    highlightPolygonEdge->setFlag(QGraphicsItem::ItemIsSelectable, false);
}

BoundingBox::~BoundingBox()
{
    highlightedObject.clear();
}

BoundingBoxPolygonItem::BoundingBoxPolygonItem(QGraphicsItem *item) : QGraphicsPolygonItem(item)
{
    QPen pen;
    pen.setColor(QColor(108, 141, 221));
    pen.setWidth(1);
    setPen(pen);
}

int BoundingBoxPolygonItem::type() const
{
    return Constants::EditorItemType;
}

BoundingRectHighlighter::BoundingRectHighlighter(QDeclarativeViewObserver *view) :
    LayerItem(view->declarativeView()->scene()),
    m_view(view),
    m_animFrame(0)
{
    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(AnimInterval);
    connect(m_animTimer, SIGNAL(timeout()), SLOT(animTimeout()));
}

BoundingRectHighlighter::~BoundingRectHighlighter()
{

}

void BoundingRectHighlighter::animTimeout()
{
    ++m_animFrame;
    if (m_animFrame == AnimFrames) {
        m_animTimer->stop();
    }

    qreal alpha = m_animFrame / float(AnimFrames);

    foreach(BoundingBox *box, m_boxes) {
        box->highlightPolygonEdge->setOpacity(alpha);
    }
}

void BoundingRectHighlighter::clear()
{
    if (m_boxes.length()) {
        m_animTimer->stop();

        foreach(BoundingBox *box, m_boxes) {
            freeBoundingBox(box);
        }
    }
}

BoundingBox *BoundingRectHighlighter::boxFor(QGraphicsObject *item) const
{
    foreach(BoundingBox *box, m_boxes) {
        if (box->highlightedObject.data() == item) {
            return box;
        }
    }
    return 0;
}

void BoundingRectHighlighter::highlight(QList<QGraphicsObject*> items)
{
    if (items.isEmpty())
        return;

    bool animate = false;

    QList<BoundingBox *> newBoxes;
    foreach(QGraphicsObject *itemToHighlight, items) {
        BoundingBox *box = boxFor(itemToHighlight);
        if (!box) {
            box = createBoundingBox(itemToHighlight);
            animate = true;
        }

        newBoxes << box;
    }
    qSort(newBoxes);

    if (newBoxes != m_boxes) {
        clear();
        m_boxes << newBoxes;
    }

    highlightAll(animate);
}

void BoundingRectHighlighter::highlight(QGraphicsObject* itemToHighlight)
{
    if (!itemToHighlight)
        return;

    bool animate = false;

    BoundingBox *box = boxFor(itemToHighlight);
    if (!box) {
        box = createBoundingBox(itemToHighlight);
        m_boxes << box;
        animate = true;
        qSort(m_boxes);
    }

    highlightAll(animate);
}

BoundingBox *BoundingRectHighlighter::createBoundingBox(QGraphicsObject *itemToHighlight)
{
    if (!m_freeBoxes.isEmpty()) {
        BoundingBox *box = m_freeBoxes.last();
        if (box->highlightedObject.isNull()) {
            box->highlightedObject = itemToHighlight;
            box->highlightPolygon->show();
            box->highlightPolygonEdge->show();
            m_freeBoxes.removeLast();
            return box;
        }
    }

    BoundingBox *box = new BoundingBox(itemToHighlight, this, this);

    connect(itemToHighlight, SIGNAL(xChanged()), this, SLOT(refresh()));
    connect(itemToHighlight, SIGNAL(yChanged()), this, SLOT(refresh()));
    connect(itemToHighlight, SIGNAL(widthChanged()), this, SLOT(refresh()));
    connect(itemToHighlight, SIGNAL(heightChanged()), this, SLOT(refresh()));
    connect(itemToHighlight, SIGNAL(rotationChanged()), this, SLOT(refresh()));
    connect(itemToHighlight, SIGNAL(destroyed(QObject*)), this, SLOT(itemDestroyed(QObject*)));

    return box;
}

void BoundingRectHighlighter::removeBoundingBox(BoundingBox *box)
{
    delete box;
    box = 0;
}

void BoundingRectHighlighter::freeBoundingBox(BoundingBox *box)
{
    if (!box->highlightedObject.isNull()) {
        disconnect(box->highlightedObject.data(), SIGNAL(xChanged()), this, SLOT(refresh()));
        disconnect(box->highlightedObject.data(), SIGNAL(yChanged()), this, SLOT(refresh()));
        disconnect(box->highlightedObject.data(), SIGNAL(widthChanged()), this, SLOT(refresh()));
        disconnect(box->highlightedObject.data(), SIGNAL(heightChanged()), this, SLOT(refresh()));
        disconnect(box->highlightedObject.data(), SIGNAL(rotationChanged()), this, SLOT(refresh()));
    }

    box->highlightedObject.clear();
    box->highlightPolygon->hide();
    box->highlightPolygonEdge->hide();
    m_boxes.removeOne(box);
    m_freeBoxes << box;
}

void BoundingRectHighlighter::itemDestroyed(QObject *obj)
{
    foreach(BoundingBox *box, m_boxes) {
        if (box->highlightedObject.data() == obj) {
            freeBoundingBox(box);
            break;
        }
    }
}

void BoundingRectHighlighter::highlightAll(bool animate)
{
    foreach(BoundingBox *box, m_boxes) {
        if (box && box->highlightedObject.isNull()) {
            // clear all highlights
            clear();
            return;
        }
        QGraphicsObject *item = box->highlightedObject.data();
        QRectF itemAndChildRect = item->boundingRect() | item->childrenBoundingRect();

        QPolygonF boundingRectInSceneSpace(item->mapToScene(itemAndChildRect));
        QPolygonF boundingRectInLayerItemSpace = mapFromScene(boundingRectInSceneSpace);
        QRectF bboxRect
                = m_view->adjustToScreenBoundaries(boundingRectInLayerItemSpace.boundingRect());
        QRectF edgeRect = bboxRect;
        edgeRect.adjust(-1, -1, 1, 1);

        box->highlightPolygon->setPolygon(QPolygonF(bboxRect));
        box->highlightPolygonEdge->setPolygon(QPolygonF(edgeRect));

        if (animate)
            box->highlightPolygonEdge->setOpacity(0);
    }

    if (animate) {
        m_animFrame = 0;
        m_animTimer->start();
    }
}

void BoundingRectHighlighter::refresh()
{
    if (!m_boxes.isEmpty())
        highlightAll(true);
}


} // namespace QmlJSDebugger
