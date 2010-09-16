#include "toolbarcolorbox.h"
#include "qmlviewerconstants.h"

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

namespace QmlObserver {

ToolBarColorBox::ToolBarColorBox(QWidget *parent) :
    QLabel(parent)
{
    m_copyHexColor = new QAction(QIcon(QLatin1String(":/qml/images/color-picker-hicontrast.png")), tr("Copy"), this);
    connect(m_copyHexColor, SIGNAL(triggered()), SLOT(copyColorToClipboard()));
    setScaledContents(false);
}

void ToolBarColorBox::setColor(const QColor &color)
{
    m_color = color;

    QPixmap pix = createDragPixmap(width());
    setPixmap(pix);
    update();
}

void ToolBarColorBox::mousePressEvent(QMouseEvent *event)
{
    m_dragBeginPoint = event->pos();
    m_dragStarted = false;
}

void ToolBarColorBox::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton
        && QPoint(event->pos() - m_dragBeginPoint).manhattanLength() > Constants::DragStartDistance
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

    QColor borderColor1 = QColor(143, 143 ,143);
    QColor borderColor2 = QColor(43, 43, 43);

    p.setBrush(QBrush(m_color));
    p.setPen(QPen(QBrush(borderColor2),1));

    p.fillRect(0, 0, size, size, borderColor1);
    p.drawRect(1,1, size - 3, size - 3);
    return pix;
}

void ToolBarColorBox::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu contextMenu;
    contextMenu.addAction(m_copyHexColor);
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


} // namespace QmlObserver
