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

namespace QmlViewer {

class QDeclarativeDesignView;
class BoundingBox;

class BoundingRectHighlighter : public LayerItem
{
    Q_OBJECT
public:
    explicit BoundingRectHighlighter(QDeclarativeDesignView *view);
    void clear();
    void highlight(QList<QGraphicsObject*> items);
    void highlight(QGraphicsObject* item);
    void removeHighlight(QGraphicsObject *item);

private slots:
    void refresh();
    void animTimeout();

private:
    BoundingBox *boxFor(QGraphicsObject *item) const;
    void highlightAll(bool animate);

private:
    Q_DISABLE_COPY(BoundingRectHighlighter);

    QDeclarativeDesignView *m_view;
    QList<BoundingBox*> m_boxes;
//    QList<QWeakPointer<QGraphicsObject> > m_highlightedObjects;
//    QGraphicsPolygonItem *m_highlightPolygon;
//    QGraphicsPolygonItem *m_highlightPolygonEdge;

    QTimer *m_animTimer;
    qreal m_animScale;
    int m_animFrame;

};

} // namespace QmlViewer

#endif // BOUNDINGRECTHIGHLIGHTER_H
