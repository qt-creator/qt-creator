#ifndef SUBCOMPONENTMASKLAYERITEM_H
#define SUBCOMPONENTMASKLAYERITEM_H

#include <QGraphicsPolygonItem>

namespace QmlViewer {

class QDeclarativeViewObserver;

class SubcomponentMaskLayerItem : public QGraphicsPolygonItem
{
public:
    explicit SubcomponentMaskLayerItem(QDeclarativeViewObserver *observer, QGraphicsItem *parentItem = 0);
    int type() const;
    void setCurrentItem(QGraphicsItem *item);
    void setBoundingBox(const QRectF &boundingBox);
    QGraphicsItem *currentItem() const;
    QRectF itemRect() const;

private:
    QDeclarativeViewObserver *m_observer;
    QGraphicsItem *m_currentItem;
    QGraphicsRectItem *m_borderRect;
    QRectF m_itemPolyRect;
};

} // namespace QmlViewer

#endif // SUBCOMPONENTMASKLAYERITEM_H
