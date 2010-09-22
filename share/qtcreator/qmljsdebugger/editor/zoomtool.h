#ifndef ZOOMTOOL_H
#define ZOOMTOOL_H

#include "abstractformeditortool.h"
#include "rubberbandselectionmanipulator.h"

QT_FORWARD_DECLARE_CLASS(QAction);

namespace QmlJSDebugger {

class ZoomTool : public AbstractFormEditorTool
{
    Q_OBJECT
public:
    enum ZoomDirection {
        ZoomIn,
        ZoomOut
    };

    explicit ZoomTool(QDeclarativeViewObserver *view);

    virtual ~ZoomTool();

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

    void hoverMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *keyEvent);
    void itemsAboutToRemoved(const QList<QGraphicsItem*> &itemList);

    void clear();
protected:
    void selectedItemsChanged(const QList<QGraphicsItem*> &itemList);

private slots:
    void zoomTo100();
    void zoomIn();
    void zoomOut();

private:
    qreal nextZoomScale(ZoomDirection direction) const;
    void scaleView(const QPointF &centerPos);

private:
    bool m_dragStarted;
    QPoint m_mousePos; // in view coords
    QPointF m_dragBeginPos;
    QAction *m_zoomTo100Action;
    QAction *m_zoomInAction;
    QAction *m_zoomOutAction;
    RubberBandSelectionManipulator m_rubberbandManipulator;

    qreal m_smoothZoomMultiplier;
    qreal m_currentScale;

};

} // namespace QmlJSDebugger

#endif // ZOOMTOOL_H
