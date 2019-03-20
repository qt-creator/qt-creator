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

#include "timelinegraphicslayout.h"

#include "timelinegraphicsscene.h"
#include "timelineplaceholder.h"
#include "timelinesectionitem.h"
#include "timelineview.h"

#include <QGraphicsLinearLayout>

#include <cmath>

namespace QmlDesigner {

TimelineGraphicsLayout::TimelineGraphicsLayout(TimelineGraphicsScene *scene, TimelineItem *parent)
    : TimelineItem(parent)
    , m_layout(new QGraphicsLinearLayout)
    , m_rulerItem(TimelineRulerSectionItem::create(scene, this))
    , m_placeholder1(TimelinePlaceholder::create(scene, this))
    , m_placeholder2(TimelinePlaceholder::create(scene, this))
{
    m_layout->setOrientation(Qt::Vertical);
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_layout->addItem(m_rulerItem);
    m_layout->addItem(m_placeholder1);
    m_layout->addItem(m_placeholder2);

    setLayout(m_layout);

    setPos(QPointF(0, 0));

    connect(m_rulerItem,
            &TimelineRulerSectionItem::rulerClicked,
            this,
            &TimelineGraphicsLayout::rulerClicked);
}

TimelineGraphicsLayout::~TimelineGraphicsLayout() = default;

double TimelineGraphicsLayout::rulerWidth() const
{
    return m_rulerItem->preferredWidth();
}

double TimelineGraphicsLayout::rulerScaling() const
{
    return m_rulerItem->rulerScaling();
}

double TimelineGraphicsLayout::rulerDuration() const
{
    return m_rulerItem->rulerDuration();
}

double TimelineGraphicsLayout::startFrame() const
{
    return m_rulerItem->startFrame();
}

double TimelineGraphicsLayout::endFrame() const
{
    return m_rulerItem->endFrame();
}

void TimelineGraphicsLayout::setWidth(int width)
{
    m_rulerItem->setSizeHints(width);
    m_placeholder1->setMinimumWidth(width);
    m_placeholder2->setMinimumWidth(width);
    setPreferredWidth(width);
    setMaximumWidth(width);
}

void TimelineGraphicsLayout::setTimeline(const QmlTimeline &timeline)
{
    m_layout->removeItem(m_rulerItem);
    m_layout->removeItem(m_placeholder1);
    m_layout->removeItem(m_placeholder2);

    m_rulerItem->setParentItem(nullptr);
    m_placeholder1->setParentItem(nullptr);
    m_placeholder2->setParentItem(nullptr);

    qDeleteAll(this->childItems());

    m_rulerItem->setParentItem(this);
    m_rulerItem->invalidateRulerSize(timeline);
    m_layout->addItem(m_rulerItem);

    m_placeholder1->setParentItem(this);
    m_layout->addItem(m_placeholder1);

    m_layout->invalidate();

    if (timeline.isValid()) {
        for (const ModelNode &target : timeline.allTargets()) {
            auto item = TimelineSectionItem::create(timeline, target, this);
            m_layout->addItem(item);
        }
    }

    m_placeholder2->setParentItem(this);
    m_layout->addItem(m_placeholder2);

    if (auto *scene = timelineScene())
        if (auto *view = scene->timelineView())
            if (!timeline.isValid() && view->isAttached())
                emit scaleFactorChanged(0);
}

void TimelineGraphicsLayout::setRulerScaleFactor(int factor)
{
    m_rulerItem->setRulerScaleFactor(factor);
}

void TimelineGraphicsLayout::invalidate()
{
    m_layout->invalidate();
}

int TimelineGraphicsLayout::maximumScrollValue() const
{
    const qreal w = this->geometry().width() - qreal(TimelineConstants::sectionWidth);
    const qreal duration = m_rulerItem->rulerDuration() + m_rulerItem->rulerDuration() * 0.1;
    const qreal maxr = m_rulerItem->rulerScaling() * duration - w;
    return std::round(qMax(maxr, 0.0));
}

void TimelineGraphicsLayout::activate()
{
    m_layout->activate();
}

} // End namespace QmlDesigner.
