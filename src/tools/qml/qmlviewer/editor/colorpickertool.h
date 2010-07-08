#ifndef COLORPICKERTOOL_H
#define COLORPICKERTOOL_H

#include "abstractformeditortool.h"

#include <QColor>

QT_FORWARD_DECLARE_CLASS(QPoint);

namespace QmlViewer {

class ColorPickerTool : public AbstractFormEditorTool
{
    Q_OBJECT
public:
    explicit ColorPickerTool(QDeclarativeDesignView *view);

    virtual ~ColorPickerTool();

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

    void hoverMoveEvent(QMouseEvent *event);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *keyEvent);

    void wheelEvent(QWheelEvent *event);

    void itemsAboutToRemoved(const QList<QGraphicsItem*> &itemList);

    void clear();
    void graphicsObjectsChanged(const QList<QGraphicsObject*> &itemList);

signals:
    void selectedColorChanged(const QColor &color);

protected:

    void selectedItemsChanged(const QList<QGraphicsItem*> &itemList);

private:
    void pickColor(const QPoint &pos);

private:
    QColor m_selectedColor;

};

} // namespace QmlViewer

#endif // COLORPICKERTOOL_H
