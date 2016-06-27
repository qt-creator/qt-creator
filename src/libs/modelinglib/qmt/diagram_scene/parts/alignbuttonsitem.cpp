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

#include "alignbuttonsitem.h"

#include "qmt/diagram_scene/capabilities/alignable.h"
#include "qmt/infrastructure/qmtassert.h"

#include <QPen>
#include <QBrush>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

namespace qmt {

class AlignButtonsItem::AlignButtonItem : public QGraphicsRectItem
{
public:
    AlignButtonItem(IAlignable::AlignType alignType, const QString &identifier, IAlignable *alignable, QGraphicsItem *parent)
        : QGraphicsRectItem(parent),
          m_alignType(alignType),
          m_identifier(identifier),
          m_alignable(alignable),
          m_pixmapItem(new QGraphicsPixmapItem(this))
    {
        setBrush(QBrush(QColor(192, 192, 192)));
        setPen(QPen(QColor(64, 64, 64)));
    }

    QString identifier() const { return m_identifier; }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);

        painter->save();
        painter->setPen(pen());
        painter->setBrush(brush());
        painter->drawRoundedRect(rect(), 3, 3);
        painter->restore();
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event)
    {
        Q_UNUSED(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
    {
        if (contains(event->pos()))
            m_alignable->align(m_alignType, m_identifier);
    }

    void setPixmap(const QPixmap &pixmap)
    {
        setRect(0.0, 0.0, pixmap.width() + 2 * InnerBorder, pixmap.height() + 2 * InnerBorder);
        m_pixmapItem->setPos(InnerBorder, InnerBorder);
        m_pixmapItem->setPixmap(pixmap);
    }

private:
    IAlignable::AlignType m_alignType = IAlignable::AlignLeft;
    QString m_identifier;
    IAlignable *m_alignable = 0;
    QGraphicsPixmapItem *m_pixmapItem = 0;
};

AlignButtonsItem::AlignButtonsItem(IAlignable *alignable, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_alignable(alignable)
{
}

AlignButtonsItem::~AlignButtonsItem()
{
    qDeleteAll(m_alignItems);
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
    qDeleteAll(m_alignItems);
    m_alignItems.clear();
}

void AlignButtonsItem::addButton(IAlignable::AlignType alignType, const QString &identifier, qreal pos)
{
    auto item = new AlignButtonItem(alignType, identifier, m_alignable, this);
    switch (alignType) {
    case IAlignable::AlignLeft:
    {
        static const QPixmap alignLeftPixmap = QPixmap(QStringLiteral(":/modelinglib/25x25/align-left.png")).scaled(NormalPixmapWidth, NormalPixmapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(alignLeftPixmap);
        item->setPos(pos - InnerBorder - 3.0, 0); // subtract additional shift of image within pixmap
        break;
    }
    case IAlignable::AlignRight:
    {
        static const QPixmap alignRightPixmap = QPixmap(QStringLiteral(":/modelinglib/25x25/align-right.png")).scaled(NormalPixmapWidth, NormalPixmapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(alignRightPixmap);
        item->setPos(pos - item->boundingRect().width() + InnerBorder + 3.0, 0);
        break;
    }
    case IAlignable::AlignTop:
    {
        static const QPixmap alignTopPixmap = QPixmap(QStringLiteral(":/modelinglib/25x25/align-top.png")).scaled(NormalPixmapWidth, NormalPixmapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(alignTopPixmap);
        item->setPos(0, pos - InnerBorder - 3.0);
        break;
    }
    case IAlignable::AlignBottom:
    {
        static const QPixmap alignBottomPixmap = QPixmap(QStringLiteral(":/modelinglib/25x25/align-bottom.png")).scaled(NormalPixmapWidth, NormalPixmapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(alignBottomPixmap);
        item->setPos(0, pos - item->boundingRect().height() + InnerBorder + 3.0);
        break;
    }
    case IAlignable::AlignHcenter:
    {
        static const QPixmap alignHorizontalPixmap = QPixmap(QStringLiteral(":/modelinglib/25x25/align-horizontal.png")).scaled(NormalPixmapWidth, NormalPixmapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(alignHorizontalPixmap);
        item->setPos(pos - item->boundingRect().center().x(), 0);
        break;
    }
    case IAlignable::AlignVcenter:
    {
        static const QPixmap alignVerticalPixmap = QPixmap(QStringLiteral(":/modelinglib/25x25/align-vertical.png")).scaled(NormalPixmapWidth, NormalPixmapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(alignVerticalPixmap);
        item->setPos(0, pos - item->boundingRect().center().y());
        break;
    }
    case IAlignable::AlignWidth:
    case IAlignable::AlignHeight:
    case IAlignable::AlignSize:
        QMT_CHECK(false);
        break;
    }
    m_alignItems.append(item);
}

} // namespace qmt
