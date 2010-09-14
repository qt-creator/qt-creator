#include "colorpickertool.h"
#include "qdeclarativeviewobserver.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QRectF>
#include <QRgb>
#include <QImage>
#include <QApplication>
#include <QPalette>

namespace QmlViewer {

ColorPickerTool::ColorPickerTool(QDeclarativeViewObserver *view) :
        AbstractFormEditorTool(view)
{
    m_selectedColor.setRgb(0,0,0);
}

ColorPickerTool::~ColorPickerTool()
{

}

void ColorPickerTool::mousePressEvent(QMouseEvent * /*event*/)
{
}

void ColorPickerTool::mouseMoveEvent(QMouseEvent *event)
{
    pickColor(event->pos());
}

void ColorPickerTool::mouseReleaseEvent(QMouseEvent *event)
{
    pickColor(event->pos());
}

void ColorPickerTool::mouseDoubleClickEvent(QMouseEvent * /*event*/)
{
}


void ColorPickerTool::hoverMoveEvent(QMouseEvent * /*event*/)
{
}

void ColorPickerTool::keyPressEvent(QKeyEvent * /*event*/)
{
}

void ColorPickerTool::keyReleaseEvent(QKeyEvent * /*keyEvent*/)
{
}
void ColorPickerTool::wheelEvent(QWheelEvent * /*event*/)
{
}

void ColorPickerTool::itemsAboutToRemoved(const QList<QGraphicsItem*> &/*itemList*/)
{
}

void ColorPickerTool::clear()
{
    view()->setCursor(Qt::CrossCursor);
}

void ColorPickerTool::selectedItemsChanged(const QList<QGraphicsItem*> &/*itemList*/)
{
}

void ColorPickerTool::pickColor(const QPoint &pos)
{
    QRgb fillColor = view()->backgroundBrush().color().rgb();
    if (view()->backgroundBrush().style() == Qt::NoBrush)
        fillColor = view()->palette().color(QPalette::Base).rgb();

    QRectF target(0,0, 1, 1);
    QRect source(pos.x(), pos.y(), 1, 1);
    QImage img(1, 1, QImage::Format_ARGB32);
    img.fill(fillColor);
    QPainter painter(&img);
    view()->render(&painter, target, source);
    m_selectedColor = QColor::fromRgb(img.pixel(0, 0));

    emit selectedColorChanged(m_selectedColor);
}

} // namespace QmlViewer
