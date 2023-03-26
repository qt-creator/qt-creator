// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineplaceholder.h"

#include <theme.h>

#include <QPainter>

namespace QmlDesigner {

TimelinePlaceholder::TimelinePlaceholder(TimelineItem *parent)
    : TimelineItem(parent)
{
    setPreferredHeight(TimelineConstants::sectionHeight);
    setMinimumHeight(TimelineConstants::sectionHeight);
    setMaximumHeight(TimelineConstants::sectionHeight);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

TimelinePlaceholder *TimelinePlaceholder::create(QGraphicsScene * /*parentScene*/,
                                                 TimelineItem *parent)
{
    auto item = new TimelinePlaceholder(parent);

    return item;
}

void TimelinePlaceholder::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->save();
    static const QColor penColor = Theme::getColor(Theme::BackgroundColorDark);
    static const QColor backgroundColor = Theme::getColor(Theme::DScontrolBackground);
    static const QColor backgroundColorSection = Theme::getColor(Theme::BackgroundColorDark);

    painter->fillRect(0, 0, size().width(), size().height(), backgroundColor);
    painter->fillRect(0, 0, TimelineConstants::sectionWidth, size().height(), backgroundColorSection);

    painter->setPen(penColor);

    drawLine(painter,
             TimelineConstants::sectionWidth - 1,
             0,
             TimelineConstants::sectionWidth - 1,
             size().height() - 1);

    drawLine(painter,
             TimelineConstants::sectionWidth,
             TimelineConstants::sectionHeight - 1,
             size().width(),
             TimelineConstants::sectionHeight - 1);
    painter->restore();
}

} // namespace QmlDesigner
