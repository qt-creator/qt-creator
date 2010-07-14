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

class BoundingBoxPolygonItem : public QGraphicsPolygonItem
{
public:
    explicit BoundingBoxPolygonItem(QGraphicsItem *item);
    int type() const;
};

class BoundingBox
{
public:
    explicit BoundingBox(QGraphicsObject *itemToHighlight, QGraphicsItem *parentItem);
    ~BoundingBox();
    QWeakPointer<QGraphicsObject> highlightedObject;
    QGraphicsPolygonItem *highlightPolygon;
    QGraphicsPolygonItem *highlightPolygonEdge;

private:
    Q_DISABLE_COPY(BoundingBox);

};

BoundingBox::BoundingBox(QGraphicsObject *itemToHighlight, QGraphicsItem *parentItem)
    : highlightedObject(itemToHighlight),
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
    delete highlightPolygon;
    delete highlightPolygonEdge;
    highlightPolygon = 0;
    highlightPolygonEdge = 0;
    highlightedObject.clear();
}

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

    qreal alpha = m_animFrame / float(AnimFrames);

    foreach(BoundingBox *box, m_boxes) {
        box->highlightPolygonEdge->setOpacity(alpha);
    }
}

void BoundingRectHighlighter::clear()
{
    if (m_boxes.length()) {
        m_animTimer->stop();

        qDeleteAll(m_boxes);
        m_boxes.clear();

//        disconnect(m_highlightedObject.data(), SIGNAL(xChanged()), this, SLOT(refresh()));
//        disconnect(m_highlightedObject.data(), SIGNAL(yChanged()), this, SLOT(refresh()));
//        disconnect(m_highlightedObject.data(), SIGNAL(widthChanged()), this, SLOT(refresh()));
//        disconnect(m_highlightedObject.data(), SIGNAL(heightChanged()), this, SLOT(refresh()));
//        disconnect(m_highlightedObject.data(), SIGNAL(rotationChanged()), this, SLOT(refresh()));
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

    QList<BoundingBox*> newBoxes;

    foreach(QGraphicsObject *itemToHighlight, items) {
        BoundingBox *box = boxFor(itemToHighlight);
        if (!box) {
            box = new BoundingBox(itemToHighlight, this);
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
        box = new BoundingBox(itemToHighlight, this);
        m_boxes << box;
        animate = true;
        qSort(m_boxes);
    }

    highlightAll(animate);
}

void BoundingRectHighlighter::highlightAll(bool animate)
{
    foreach(BoundingBox *box, m_boxes) {
        QGraphicsObject *item = box->highlightedObject.data();
        if (!item) {
            m_boxes.removeOne(box);
            continue;
        }

        QRectF itemAndChildRect = item->boundingRect() | item->childrenBoundingRect();

        QPolygonF boundingRectInSceneSpace(item->mapToScene(itemAndChildRect));
        QPolygonF boundingRectInLayerItemSpace = mapFromScene(boundingRectInSceneSpace);
        QRectF bboxRect = boundingRectInLayerItemSpace.boundingRect();
        QRectF edgeRect = boundingRectInLayerItemSpace.boundingRect();
        edgeRect.adjust(-1, -1, 1, 1);

        box->highlightPolygon->setPolygon(QPolygonF(bboxRect));
        box->highlightPolygonEdge->setPolygon(QPolygonF(edgeRect));

//        if (XXX) {
//            connect(item, SIGNAL(xChanged()), this, SLOT(refresh()));
//            connect(item, SIGNAL(yChanged()), this, SLOT(refresh()));
//            connect(item, SIGNAL(widthChanged()), this, SLOT(refresh()));
//            connect(item, SIGNAL(heightChanged()), this, SLOT(refresh()));
//            connect(item, SIGNAL(rotationChanged()), this, SLOT(refresh()));
//        }

        if (animate)
            box->highlightPolygonEdge->setOpacity(0);
    }

    if (animate) {
        m_animFrame = 0;
        m_animTimer->start();
    }
}

void BoundingRectHighlighter::removeHighlight(QGraphicsObject *item)
{
    if (!item)
        return;

    BoundingBox *box = boxFor(item);
    if (box) {
        m_boxes.removeOne(box);
        delete box;
        box = 0;
    }
}

void BoundingRectHighlighter::refresh()
{
    if (!m_boxes.isEmpty())
        highlightAll(true);
}


} // namespace QmlViewer
