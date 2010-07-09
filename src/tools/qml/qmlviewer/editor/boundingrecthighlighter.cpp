#include "boundingrecthighlighter.h"
#include "qdeclarativedesignview.h"
#include "qmlviewerconstants.h"

#include <QGraphicsPolygonItem>
#include <QTimer>

#include <QDebug>

namespace QmlViewer {

const qreal AnimDelta = 0.025f;
const int AnimInterval = 30;
const int AnimFrames = 10;

BoundingBoxPolygonItem::BoundingBoxPolygonItem(QGraphicsItem *item) : QGraphicsPolygonItem(item)
{
    QPen pen;
    pen.setColor(QColor(108, 141, 221));
    setPen(pen);
}

int BoundingBoxPolygonItem::type() const
{
    return Constants::EditorItemType;
}

BoundingRectHighlighter::BoundingRectHighlighter(QDeclarativeDesignView *view) :
    LayerItem(view->scene()),
    m_view(view),
    m_highlightPolygon(0),
    m_highlightPolygonEdge(0),
    m_animFrame(0)
{
    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(AnimInterval);
    connect(m_animTimer, SIGNAL(timeout()), SLOT(animTimeout()));
}

void BoundingRectHighlighter::animTimeout()
{
    ++m_animFrame;
    if (m_animFrame == AnimFrames) {
        m_animTimer->stop();
    }

    m_highlightPolygon->setPen(QPen(QColor(0, 22, 159)));
    m_highlightPolygonEdge->setPen(QPen(QColor(158, 199, 255)));
    qreal alpha = m_animFrame / float(AnimFrames);
    m_highlightPolygonEdge->setOpacity(alpha);
}

void BoundingRectHighlighter::clear()
{
    if (m_highlightPolygon) {
        m_view->scene()->removeItem(m_highlightPolygon);
        delete m_highlightPolygon;
        m_highlightPolygon = 0;
        delete m_highlightPolygonEdge;
        m_highlightPolygonEdge = 0;
        m_animTimer->stop();

        disconnect(m_highlightedObject.data(), SIGNAL(xChanged()), this, SLOT(refresh()));
        disconnect(m_highlightedObject.data(), SIGNAL(yChanged()), this, SLOT(refresh()));
        disconnect(m_highlightedObject.data(), SIGNAL(widthChanged()), this, SLOT(refresh()));
        disconnect(m_highlightedObject.data(), SIGNAL(heightChanged()), this, SLOT(refresh()));
        disconnect(m_highlightedObject.data(), SIGNAL(rotationChanged()), this, SLOT(refresh()));

        m_highlightedObject.clear();
    }
}

void BoundingRectHighlighter::highlight(QGraphicsObject* item)
{
    if (!item)
        return;

    bool animate = false;
    QGraphicsPolygonItem *polygonItem = 0;
    QGraphicsPolygonItem *polygonItemEdge = 0;
    if (item != m_highlightedObject.data() || !m_highlightPolygon) {
        animate = true;
        polygonItem = new BoundingBoxPolygonItem(this);
        polygonItemEdge = new BoundingBoxPolygonItem(this);
    } else {
        polygonItem = m_highlightPolygon;
        polygonItemEdge = m_highlightPolygonEdge;
    }

    QRectF itemAndChildRect = item->boundingRect() | item->childrenBoundingRect();

    QPolygonF boundingRectInSceneSpace(item->mapToScene(itemAndChildRect));
    QPolygonF boundingRectInLayerItemSpace = mapFromScene(boundingRectInSceneSpace);
    QRectF bboxRect = boundingRectInLayerItemSpace.boundingRect();
    QRectF edgeRect = boundingRectInLayerItemSpace.boundingRect();
    edgeRect.adjust(-1, -1, 1, 1);

    polygonItem->setPolygon(QPolygonF(bboxRect));
    polygonItem->setFlag(QGraphicsItem::ItemIsSelectable, false);

    polygonItemEdge->setPolygon(QPolygonF(edgeRect));
    polygonItemEdge->setFlag(QGraphicsItem::ItemIsSelectable, false);

    if (item != m_highlightedObject.data())
        clear();

    m_highlightPolygon = polygonItem;
    m_highlightPolygonEdge = polygonItemEdge;
    m_highlightedObject = item;

    if (item != m_highlightedObject.data()) {
        connect(m_highlightedObject.data(), SIGNAL(xChanged()), this, SLOT(refresh()));
        connect(m_highlightedObject.data(), SIGNAL(yChanged()), this, SLOT(refresh()));
        connect(m_highlightedObject.data(), SIGNAL(widthChanged()), this, SLOT(refresh()));
        connect(m_highlightedObject.data(), SIGNAL(heightChanged()), this, SLOT(refresh()));
        connect(m_highlightedObject.data(), SIGNAL(rotationChanged()), this, SLOT(refresh()));
    }

    if (animate) {
        m_highlightPolygonEdge->setOpacity(0);
        m_animFrame = 0;
        m_animTimer->start();
    }
}

void BoundingRectHighlighter::refresh()
{
    if (!m_highlightedObject.isNull())
        highlight(m_highlightedObject.data());
}


} // namespace QmlViewer
