#ifndef SUBCOMPONENTMASKLAYERITEM_H
#define SUBCOMPONENTMASKLAYERITEM_H

#include <QGraphicsPolygonItem>

namespace QmlViewer {

class QDeclarativeDesignView;

class SubcomponentMaskLayerItem : public QGraphicsPolygonItem
{
public:
    explicit SubcomponentMaskLayerItem(QDeclarativeDesignView *view, QGraphicsItem *parentItem = 0);
    int type() const;
    void setCurrentItem(QGraphicsItem *item);
    QGraphicsItem *currentItem() const;
private:
    QDeclarativeDesignView *m_view;
    QGraphicsItem *m_currentItem;
    QGraphicsRectItem *m_borderRect;

};

} // namespace QmlViewer

#endif // SUBCOMPONENTMASKLAYERITEM_H
