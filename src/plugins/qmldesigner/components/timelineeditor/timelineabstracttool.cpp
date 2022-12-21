// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelineabstracttool.h"

#include "timelinetooldelegate.h"

#include <QPointF>

namespace QmlDesigner {

TimelineAbstractTool::TimelineAbstractTool(AbstractScrollGraphicsScene *scene)
    : m_scene(scene)
    , m_delegate(nullptr)
{}

TimelineAbstractTool::TimelineAbstractTool(AbstractScrollGraphicsScene *scene,
                                           TimelineToolDelegate *delegate)
    : m_scene(scene)
    , m_delegate(delegate)
{}

TimelineAbstractTool::~TimelineAbstractTool() = default;

AbstractScrollGraphicsScene *TimelineAbstractTool::scene() const
{
    return m_scene;
}

TimelineToolDelegate *TimelineAbstractTool::delegate() const
{
    return m_delegate;
}

QPointF TimelineAbstractTool::startPosition() const
{
    return m_delegate->startPoint();
}

TimelineMovableAbstractItem *TimelineAbstractTool::currentItem() const
{
    return m_delegate->item();
}

} // namespace QmlDesigner
