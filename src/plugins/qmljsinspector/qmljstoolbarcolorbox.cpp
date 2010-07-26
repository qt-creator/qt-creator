#include "qmljstoolbarcolorbox.h"

#include <QPixmap>
#include <QPainter>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QClipboard>
#include <QApplication>
#include <QColorDialog>
#include <QDrag>
#include <QMimeData>

#include <QDebug>

namespace QmlJSInspector {

ToolBarColorBox::ToolBarColorBox(QWidget *parent) :
    QLabel(parent)
{
    m_color = Qt::white;
    m_borderColorOuter = Qt::white;
    m_borderColorInner = QColor(143, 143 ,143);

    m_copyHexColorAction = new QAction(QIcon(":/qml/images/color-picker-small-hicontrast.png"), tr("Copy Color"), this);
    connect(m_copyHexColorAction, SIGNAL(triggered()), SLOT(copyColorToClipboard()));
    setScaledContents(false);
}

void ToolBarColorBox::setColor(const QColor &color)
{
    m_color = color;

    QPixmap pix = createDragPixmap(width());
    setPixmap(pix);
    update();
}

void ToolBarColorBox::setInnerBorderColor(const QColor &color)
{
    m_borderColorInner = color;
    setColor(m_color);
}

void ToolBarColorBox::setOuterBorderColor(const QColor &color)
 {
     m_borderColorOuter = color;
     setColor(m_color);
 }

void ToolBarColorBox::mousePressEvent(QMouseEvent *event)
{
    m_dragBeginPoint = event->pos();
    m_dragStarted = false;
}

void ToolBarColorBox::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton
        && QPoint(event->pos() - m_dragBeginPoint).manhattanLength() > QApplication::startDragDistance()
        && !m_dragStarted)
    {
        m_dragStarted = true;
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;

        mimeData->setText(m_color.name());
        drag->setMimeData(mimeData);
        drag->setPixmap(createDragPixmap());

        drag->exec();
    }
}

QPixmap ToolBarColorBox::createDragPixmap(int size) const
{
    QPixmap pix(size, size);
    QPainter p(&pix);

    p.setBrush(QBrush(m_color));
    p.setPen(QPen(QBrush(m_borderColorInner),1));

    p.fillRect(0, 0, size, size, m_borderColorOuter);
    p.drawRect(1,1, size - 3, size - 3);
    return pix;
}

void ToolBarColorBox::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu contextMenu;
    contextMenu.addAction(m_copyHexColorAction);
    contextMenu.exec(ev->globalPos());
}

void ToolBarColorBox::mouseDoubleClickEvent(QMouseEvent *)
{
    QColorDialog dialog(m_color);
    dialog.show();
}

void ToolBarColorBox::copyColorToClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_color.name());
}


} // namespace QmlJSInspector
