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

void SubcomponentMaskLayerItem::setCurrentItem(QGraphicsItem *item)
{
    m_currentItem = item;

    if (!m_currentItem)
        return;

    QPolygonF viewPoly(QRectF(m_view->rect()));
    viewPoly = m_view->mapToScene(viewPoly.toPolygon());

    QRectF itemRect = item->boundingRect() | item->childrenBoundingRect();
    QPolygonF itemPoly(itemRect);
    itemPoly = item->mapToScene(itemPoly);

    QRectF borderRect = itemPoly.boundingRect();
    borderRect.adjust(-1, -1, 1, 1);
    m_borderRect->setRect(borderRect);

    itemPoly = viewPoly.subtracted(QPolygonF(itemPoly.boundingRect()));
    setPolygon(itemPoly);
}

QGraphicsItem *SubcomponentMaskLayerItem::currentItem() const
{
    return m_currentItem;
}

} // namespace QmlViewer
