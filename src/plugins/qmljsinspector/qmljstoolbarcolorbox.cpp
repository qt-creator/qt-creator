/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "qmljstoolbarcolorbox.h"

#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QClipboard>
#include <QtGui/QApplication>
#include <QtGui/QColorDialog>
#include <QtGui/QDrag>

#include <QtCore/QMimeData>
#include <QtCore/QDebug>

namespace QmlJSInspector {

ToolBarColorBox::ToolBarColorBox(QWidget *parent) :
    QLabel(parent)
{
    m_color = Qt::white;
    m_borderColorOuter = Qt::white;
    m_borderColorInner = QColor(143, 143 ,143);

    m_copyHexColorAction = new QAction(QIcon(QLatin1String(":/qml/images/color-picker-small-hicontrast.png")), tr("Copy Color"), this);
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

void ToolBarColorBox::copyColorToClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_color.name());
}

} // namespace QmlJSInspector
