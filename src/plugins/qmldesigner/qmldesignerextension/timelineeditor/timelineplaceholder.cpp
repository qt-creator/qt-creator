/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
    static const QColor penColor = Theme::instance()->qmlDesignerBackgroundColorDarker();
    static const QColor backgroundColor = Theme::instance()
                                              ->qmlDesignerBackgroundColorDarkAlternate();
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
