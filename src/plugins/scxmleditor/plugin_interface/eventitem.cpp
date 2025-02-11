// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "eventitem.h"

#include <QColor>
#include <QFont>
#include <QGraphicsItem>
#include <QList>
#include <QString>

namespace ScxmlEditor {

namespace PluginInterface {

EventItem::EventItem(const QPointF &pos, BaseItem *parent)
    : BaseItem(parent)
{
    m_eventNameItem = new TextItem(this);
    m_eventNameItem->setParentItem(this);
    QFont serifFont("Times", 10, QFont::Normal);
    m_eventNameItem->setFont(serifFont);

    QString color = editorInfo("fontColor");
    m_eventNameItem->setDefaultTextColor(color.isEmpty() ? QColor(Qt::black) : QColor(color));

    setPos(pos);
    m_eventNameItem->setTextInteractionFlags(Qt::NoTextInteraction);
    setItemBoundingRect(m_eventNameItem->boundingRect());
}

void EventItem::updateAttributes()
{
    QString text = "  " + tag()->tagName();
    if (tag()->attributeNames().size() > 0) {
        for (int i = 0; i < tag()->attributeNames().size(); ++i)
            if (tag()->attributeNames().at(i) == "event") {
                if (tag()->attributeValues().size() > i)
                    text += " / " + tag()->attributeValues().at(i);
                break;
            }
    }
    m_eventNameItem->setText(text);
    setItemBoundingRect(m_eventNameItem->boundingRect());
}

OnEntryExitItem::OnEntryExitItem(BaseItem *parent)
    : BaseItem(parent)
{
    m_eventNameItem = new TextItem(this);
    m_eventNameItem->setParentItem(this);
    QFont serifFont("Times", 10, QFont::Normal);
    m_eventNameItem->setFont(serifFont);
    m_eventNameItem->setDefaultTextColor(Qt::black);
    m_eventNameItem->setTextInteractionFlags(Qt::NoTextInteraction);
}

void OnEntryExitItem::updateAttributes()
{
    QString text = tag()->tagName();

    m_eventNameItem->setText(text);
    setItemBoundingRect(childBoundingRect());
    checkParentBoundingRect();
}

void OnEntryExitItem::finalizeCreation()
{
    auto children = tag()->allChildren();
    auto pos = m_eventNameItem->boundingRect().bottomLeft();
    for (auto child : children) {
        EventItem *item = new EventItem(pos, this);
        item->setTag(child);
        item->updateAttributes();
        pos = item->pos() + item->boundingRect().bottomLeft();
    }

    setItemBoundingRect(childBoundingRect());
}

void OnEntryExitItem::addChild(ScxmlTag *tag)
{
    auto pos = childBoundingRect().bottomLeft();
    EventItem *item = new EventItem(pos, this);
    item->setTag(tag);
    item->updateAttributes();

    setItemBoundingRect(childBoundingRect());
    checkParentBoundingRect();
}

QRectF OnEntryExitItem::childBoundingRect() const
{
    QRectF r = m_eventNameItem->boundingRect();

    const QList<QGraphicsItem *> children = childItems();

    for (const QGraphicsItem *child : children) {
        QRectF br = child->boundingRect();
        QPointF p = child->pos() + br.topLeft();
        br.moveTopLeft(p);
        r = r.united(br);
    }
    return r;
}

} // namespace PluginInterface
} // namespace ScxmlEditor
