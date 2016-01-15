/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "dragtool.h"

#include "qmt/diagram_ui/diagram_mime_types.h"

#include <QPainter>
#include <QCursor>
#include <QDrag>
#include <QMouseEvent>
#include <QMimeData>

namespace ModelEditor {
namespace Internal {

class DragTool::DragToolPrivate {
public:
    QIcon icon;
    QSize iconSize;
    QString title;
    QString newElementId;
    QString stereotype;
    bool disableFrame = false;
    bool framePainted = false;
};

DragTool::DragTool(const QIcon &icon, const QString &title, const QString &newElementId,
                   const QString &stereotype, QWidget *parent)
    : QWidget(parent),
      d(new DragToolPrivate)
{
    d->icon = icon;
    d->iconSize = QSize(32, 32);
    d->title = title;
    d->newElementId = newElementId;
    d->stereotype = stereotype;
    QMargins margins = contentsMargins();
    if (margins.top() < 3)
        margins.setTop(3);
    if (margins.bottom() < 3)
        margins.setBottom(3);
    if (margins.left() < 3)
        margins.setLeft(3);
    if (margins.right() < 3)
        margins.setRight(3);
    setContentsMargins(margins);
}

DragTool::~DragTool()
{
    delete d;
}

QSize DragTool::sizeHint() const
{
    int width = 0;
    int height = 0;
    if (d->iconSize.width() > width)
        width = d->iconSize.width();
    height += d->iconSize.height();
    QRect textRect = fontMetrics().boundingRect(QRect(), Qt::AlignLeft | Qt::TextSingleLine, d->title);
    if (textRect.width() > width)
        width = textRect.width();
    height += textRect.height();
    QMargins margins = contentsMargins();
    width += margins.left() + margins.right();
    height += margins.top() + margins.bottom();
    return QSize(width, height);
}

void DragTool::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QMargins margins = contentsMargins();
    QPixmap pixmap = d->icon.pixmap(d->iconSize, isEnabled() ? QIcon::Normal : QIcon::Disabled, QIcon::Off);
    QPainter painter(this);
    QRect targetRect((width() - pixmap.width()) / 2,
                     margins.top() + (d->iconSize.height() - pixmap.height()) / 2,
                     pixmap.width(),
                     pixmap.height());
    QRect sourceRect(0, 0, pixmap.width(), pixmap.height());
    painter.drawPixmap(targetRect, pixmap, sourceRect);

    QRect textRect = painter.boundingRect(QRect(), Qt::AlignLeft | Qt::TextSingleLine, d->title);
    QRect boundingRect(0, d->iconSize.height(), width(), textRect.height());
    painter.drawText(boundingRect, Qt::AlignHCenter | Qt::TextSingleLine, d->title);

    // draw a weak frame if mouse is inside widget
    if (!d->disableFrame && rect().contains(QWidget::mapFromGlobal(QCursor::pos()))) {
        QRect rect(0, 0, width() - 1, height() - 1);
        QPen pen = painter.pen();
        pen.setStyle(Qt::DotLine);
        painter.setPen(pen);
        painter.drawRect(rect);
        d->framePainted = true;
    } else {
        d->framePainted = false;
    }
}

void DragTool::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    update();
}

void DragTool::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    update();
}

void DragTool::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QMargins margins = contentsMargins();
        const QRect iconRect((width() - d->iconSize.width()) / 2, margins.top(),
                       d->iconSize.width(), d->iconSize.height());
        if (iconRect.contains(event->pos())) {
            auto drag = new QDrag(this);
            auto mimeData = new QMimeData;
            QByteArray data;
            QDataStream dataStream(&data, QIODevice::WriteOnly);
            dataStream << d->newElementId << d->title << d->stereotype;
            mimeData->setData(QLatin1String(qmt::MIME_TYPE_NEW_MODEL_ELEMENTS), data);
            drag->setMimeData(mimeData);

            QPixmap pixmap = d->icon.pixmap(d->iconSize, QIcon::Normal, QIcon::Off);
            QPainter painter(&pixmap);
            painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            painter.fillRect(pixmap.rect(), QColor(0, 0, 0, 96));
            drag->setPixmap(pixmap);
            drag->setHotSpot(QPoint(drag->pixmap().width() / 2, drag->pixmap().height() / 2));

            d->disableFrame = true;
            update();
            Qt::DropAction dropAction = drag->exec();
            Q_UNUSED(dropAction);
            d->disableFrame = false;
            update();
        }
    }
}

void DragTool::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    const bool containsMouse = rect().contains(QWidget::mapFromGlobal(QCursor::pos()));
    if ((d->framePainted && !containsMouse) || (!d->framePainted && containsMouse))
        update();
}

} // namespace Internal
} // namespace ModelEditor
