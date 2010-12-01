/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "toolbarcolorbox.h"
#include "qmlobserverconstants.h"

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

namespace QmlJSDebugger {

ToolBarColorBox::ToolBarColorBox(QWidget *parent) :
    QLabel(parent)
{
    m_copyHexColor = new QAction(QIcon(QLatin1String(":/qml/images/color-picker-hicontrast.png")),
                                 tr("Copy"), this);
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
            && (QPoint(event->pos() - m_dragBeginPoint).manhattanLength()
                > Constants::DragStartDistance)
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

} // namespace QmlJSDebugger
