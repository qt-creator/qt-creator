#ifndef COLORPICKERTOOL_H
#define COLORPICKERTOOL_H

#include "abstractformeditortool.h"

#include <QColor>

QT_FORWARD_DECLARE_CLASS(QPoint);

namespace QmlJSDebugger {

class ColorPickerTool : public AbstractFormEditorTool
{
    Q_OBJECT
public:
    explicit ColorPickerTool(QDeclarativeViewObserver *view);

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

signals:
    void selectedColorChanged(const QColor &color);

protected:

    void selectedItemsChanged(const QList<QGraphicsItem*> &itemList);

private:
    void pickColor(const QPoint &pos);

private:
    QColor m_selectedColor;

};

} // namespace QmlJSDebugger

#endif // COLORPICKERTOOL_H
