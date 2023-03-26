// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineitem.h"

namespace QmlDesigner {

class TimelinePlaceholder : public TimelineItem
{
    Q_OBJECT

public:
    static TimelinePlaceholder *create(QGraphicsScene *parentScene, TimelineItem *parent);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

private:
    TimelinePlaceholder(TimelineItem *parent = nullptr);
};

} // namespace QmlDesigner
