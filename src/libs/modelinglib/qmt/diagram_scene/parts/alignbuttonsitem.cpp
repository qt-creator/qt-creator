// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) final
    {
        Q_UNUSED(option)
        Q_UNUSED(widget)

        painter->save();
        painter->setPen(pen());
        painter->setBrush(brush());
        painter->drawRoundedRect(rect(), 3, 3);
        painter->restore();
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) final
    {
        Q_UNUSED(event)
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) final
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
    IAlignable *m_alignable = nullptr;
    QGraphicsPixmapItem *m_pixmapItem = nullptr;
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
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(widget)
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
        static const QPixmap alignLeftPixmap = QPixmap(":/modelinglib/25x25/align-left.png").scaled(NormalPixmapWidth, NormalPixmapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(alignLeftPixmap);
        item->setPos(pos - InnerBorder - 3.0, 0); // subtract additional shift of image within pixmap
        break;
    }
    case IAlignable::AlignRight:
    {
        static const QPixmap alignRightPixmap = QPixmap(":/modelinglib/25x25/align-right.png").scaled(NormalPixmapWidth, NormalPixmapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(alignRightPixmap);
        item->setPos(pos - item->boundingRect().width() + InnerBorder + 3.0, 0);
        break;
    }
    case IAlignable::AlignTop:
    {
        static const QPixmap alignTopPixmap = QPixmap(":/modelinglib/25x25/align-top.png").scaled(NormalPixmapWidth, NormalPixmapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(alignTopPixmap);
        item->setPos(0, pos - InnerBorder - 3.0);
        break;
    }
    case IAlignable::AlignBottom:
    {
        static const QPixmap alignBottomPixmap = QPixmap(":/modelinglib/25x25/align-bottom.png").scaled(NormalPixmapWidth, NormalPixmapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(alignBottomPixmap);
        item->setPos(0, pos - item->boundingRect().height() + InnerBorder + 3.0);
        break;
    }
    case IAlignable::AlignHcenter:
    {
        static const QPixmap alignHorizontalPixmap = QPixmap(":/modelinglib/25x25/align-horizontal.png").scaled(NormalPixmapWidth, NormalPixmapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(alignHorizontalPixmap);
        item->setPos(pos - item->boundingRect().center().x(), 0);
        break;
    }
    case IAlignable::AlignVcenter:
    {
        static const QPixmap alignVerticalPixmap = QPixmap(":/modelinglib/25x25/align-vertical.png").scaled(NormalPixmapWidth, NormalPixmapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        item->setPixmap(alignVerticalPixmap);
        item->setPos(0, pos - item->boundingRect().center().y());
        break;
    }
    case IAlignable::AlignWidth:
    case IAlignable::AlignHeight:
    case IAlignable::AlignSize:
    case IAlignable::AlignHCenterDistance:
    case IAlignable::AlignVCenterDistance:
    case IAlignable::AlignHBorderDistance:
    case IAlignable::AlignVBorderDistance:
        QMT_CHECK(false);
        break;
    }
    m_alignItems.append(item);
}

} // namespace qmt
