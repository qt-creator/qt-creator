#include "subcomponentmasklayeritem.h"
#include "qmlviewerconstants.h"
#include "qdeclarativedesignview.h"
#include <QPolygonF>

namespace QmlViewer {

SubcomponentMaskLayerItem::SubcomponentMaskLayerItem(QDeclarativeDesignView *view, QGraphicsItem *parentItem) :
    QGraphicsPolygonItem(parentItem),
    m_view(view),
    m_currentItem(0),
    m_borderRect(new QGraphicsRectItem(this))
{
    m_borderRect->setRect(0,0,0,0);
    m_borderRect->setPen(QPen(QColor(60, 60, 60), 1));
    m_borderRect->setData(Constants::EditorItemDataKey, QVariant(true));

    setBrush(QBrush(QColor(160,160,160)));
    setPen(Qt::NoPen);
}

int SubcomponentMaskLayerItem::type() const
{
    return Constants::EditorItemType;
}

static QRectF resizeRect(const QRectF &newRect, const QRectF &oldRect)
{
    QRectF result = newRect;
    if (oldRect.left() < newRect.left())
        result.setLeft(oldRect.left());

    if (oldRect.top() < newRect.top())
        result.setTop(oldRect.top());

    if (oldRect.right() > newRect.right())
        result.setRight(oldRect.right());

    if (oldRect.bottom() > newRect.bottom())
        result.setBottom(oldRect.bottom());

    return result;
}


void SubcomponentMaskLayerItem::setCurrentItem(QGraphicsItem *item)
{
    QGraphicsItem *prevItem = m_currentItem;
    m_currentItem = item;

    if (!m_currentItem)
        return;

    QPolygonF viewPoly(QRectF(m_view->rect()));
    viewPoly = m_view->mapToScene(viewPoly.toPolygon());

    QRectF itemRect = item->boundingRect() | item->childrenBoundingRect();
    QPolygonF itemPoly(itemRect);
    itemPoly = item->mapToScene(itemPoly);

    // if updating the same item as before, resize the rectangle only bigger, not smaller.
    if (prevItem == item && prevItem != 0) {
        m_itemPolyRect = resizeRect(itemPoly.boundingRect(), m_itemPolyRect);
    } else {
        m_itemPolyRect = itemPoly.boundingRect();
    }
    QRectF borderRect = m_itemPolyRect;
    borderRect.adjust(-1, -1, 1, 1);
    m_borderRect->setRect(borderRect);

    itemPoly = viewPoly.subtracted(QPolygonF(m_itemPolyRect));
    setPolygon(itemPoly);
}

QGraphicsItem *SubcomponentMaskLayerItem::currentItem() const
{
    return m_currentItem;
}

} // namespace QmlViewer
