/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "alignbuttonsitem.h"

#include "qmt/diagram_scene/capabilities/alignable.h"
#include "qmt/infrastructure/qmtassert.h"

#include <QPen>
#include <QBrush>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

namespace qmt {

class AlignButtonsItem::AlignButtonItem :
        public QGraphicsRectItem
{
public:
    AlignButtonItem(IAlignable::AlignType align_type, const QString &identifier, IAlignable *alignable, QGraphicsItem *parent)
        : QGraphicsRectItem(parent),
          _align_type(align_type),
          _identifier(identifier),
          _alignable(alignable),
          _pixmap_item(new QGraphicsPixmapItem(this))
    {
        setBrush(QBrush(QColor(192, 192, 192)));
        setPen(QPen(QColor(64, 64, 64)));
    }

public:

    QString getIdentifier() const { return _identifier; }

public:

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(pen());
        painter->setBrush(brush());
        painter->drawRoundedRect(rect(), 3, 3);
        painter->restore();
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event)
    {
        Q_UNUSED(event;)
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
    {
        if (contains(event->pos())) {
            _alignable->align(_align_type, _identifier);
        }
    }

public:

    void setPixmap(const QPixmap &pixmap)
    {
        setRect(0.0, 0.0, pixmap.width() + 2 * INNER_BORDER, pixmap.height() + 2 * INNER_BORDER);
        _pixmap_item->setPos(INNER_BORDER, INNER_BORDER);
        _pixmap_item->setPixmap(pixmap);
    }

private:

    IAlignable::AlignType _align_type;

    QString _identifier;

    IAlignable *_alignable;

    QGraphicsPixmapItem *_pixmap_item;

};


AlignButtonsItem::AlignButtonsItem(IAlignable *alignable, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      _alignable(alignable)
{
}

AlignButtonsItem::~AlignButtonsItem()
{
    qDeleteAll(_align_items);
}

QRectF AlignButtonsItem::boundingRect() const
{
    return childrenBoundingRect();
}

void AlignButtonsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void AlignButtonsItem::clear()
{
    qDeleteAll(_align_items);
    _align_items.clear();
}

void AlignButtonsItem::addButton(IAlignable::AlignType align_type, const QString &identifier, qreal pos)
{
    AlignButtonItem *item = new AlignButtonItem(align_type, identifier, _alignable, this);
    switch (align_type) {
    case IAlignable::ALIGN_LEFT:
    {
        static const QPixmap align_left_pixmap = QPixmap(QStringLiteral(":/modelinglib/25x25/align-left.png")).scaled(NORMAL_PIXMAP_WIDTH, NORMAL_PIXMAP_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(align_left_pixmap);
        item->setPos(pos - INNER_BORDER - 3.0, 0); // subtract additional shift of image within pixmap
        break;
    }
    case IAlignable::ALIGN_RIGHT:
    {
        static const QPixmap align_right_pixmap = QPixmap(QStringLiteral(":/modelinglib/25x25/align-right.png")).scaled(NORMAL_PIXMAP_WIDTH, NORMAL_PIXMAP_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(align_right_pixmap);
        item->setPos(pos - item->boundingRect().width() + INNER_BORDER + 3.0, 0);
        break;
    }
    case IAlignable::ALIGN_TOP:
    {
        static const QPixmap align_top_pixmap = QPixmap(QStringLiteral(":/modelinglib/25x25/align-top.png")).scaled(NORMAL_PIXMAP_WIDTH, NORMAL_PIXMAP_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(align_top_pixmap);
        item->setPos(0, pos - INNER_BORDER - 3.0);
        break;
    }
    case IAlignable::ALIGN_BOTTOM:
    {
        static const QPixmap align_bottom_pixmap = QPixmap(QStringLiteral(":/modelinglib/25x25/align-bottom.png")).scaled(NORMAL_PIXMAP_WIDTH, NORMAL_PIXMAP_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(align_bottom_pixmap);
        item->setPos(0, pos - item->boundingRect().height() + INNER_BORDER + 3.0);
        break;
    }
    case IAlignable::ALIGN_HCENTER:
    {
        static const QPixmap align_horizontal_pixmap = QPixmap(QStringLiteral(":/modelinglib/25x25/align-horizontal.png")).scaled(NORMAL_PIXMAP_WIDTH, NORMAL_PIXMAP_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(align_horizontal_pixmap);
        item->setPos(pos - item->boundingRect().center().x(), 0);
        break;
    }
    case IAlignable::ALIGN_VCENTER:
    {
        static const QPixmap align_vertical_pixmap = QPixmap(QStringLiteral(":/modelinglib/25x25/align-vertical.png")).scaled(NORMAL_PIXMAP_WIDTH, NORMAL_PIXMAP_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(align_vertical_pixmap);
        item->setPos(0, pos - item->boundingRect().center().y());
        break;
    }
    case IAlignable::ALIGN_WIDTH:
    case IAlignable::ALIGN_HEIGHT:
    case IAlignable::ALIGN_SIZE:
        QMT_CHECK(false);
        break;
    }
    _align_items.append(item);
}

}
