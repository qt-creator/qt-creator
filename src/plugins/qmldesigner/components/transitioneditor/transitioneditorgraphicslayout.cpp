// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "transitioneditorgraphicslayout.h"

#include "timelinegraphicsscene.h"
#include "timelineplaceholder.h"
#include "timelinesectionitem.h"
#include "timelineview.h"
#include "transitioneditorsectionitem.h"

#include <auxiliarydataproperties.h>

#include <QGraphicsLinearLayout>

#include <cmath>

namespace QmlDesigner {

TransitionEditorGraphicsLayout::TransitionEditorGraphicsLayout(QGraphicsScene *scene,
                                                               TimelineItem *parent)
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
            &TransitionEditorGraphicsLayout::rulerClicked);
}

TransitionEditorGraphicsLayout::~TransitionEditorGraphicsLayout() = default;

int TransitionEditorGraphicsLayout::zoom() const
{
    return m_rulerItem->zoom();
}

double TransitionEditorGraphicsLayout::rulerWidth() const
{
    return m_rulerItem->preferredWidth();
}

double TransitionEditorGraphicsLayout::rulerScaling() const
{
    return m_rulerItem->rulerScaling();
}

double TransitionEditorGraphicsLayout::rulerDuration() const
{
    return m_rulerItem->rulerDuration();
}

double TransitionEditorGraphicsLayout::endFrame() const
{
    return m_rulerItem->endFrame();
}

void TransitionEditorGraphicsLayout::setWidth(int width)
{
    m_rulerItem->setSizeHints(width);
    m_placeholder1->setMinimumWidth(width);
    m_placeholder2->setMinimumWidth(width);
    setPreferredWidth(width);
    setMaximumWidth(width);
}

void TransitionEditorGraphicsLayout::setTransition(const ModelNode &transition)
{
    m_layout->removeItem(m_rulerItem);
    m_layout->removeItem(m_placeholder1);
    m_layout->removeItem(m_placeholder2);

    m_rulerItem->setParentItem(nullptr);
    m_placeholder1->setParentItem(nullptr);
    m_placeholder2->setParentItem(nullptr);

    qDeleteAll(this->childItems());

    m_rulerItem->setParentItem(this);

    qreal duration = 2000;
    if (auto data = transition.auxiliaryData(transitionDurationProperty))
        duration = data->toDouble();

    setDuration(duration);
    m_layout->addItem(m_rulerItem);

    m_placeholder1->setParentItem(this);
    m_layout->addItem(m_placeholder1);

    m_layout->invalidate();

    for (const ModelNode &parallel : transition.directSubModelNodes()) {
        auto item = TransitionEditorSectionItem::create(parallel, this);
        m_layout->addItem(item);
    }

    m_placeholder2->setParentItem(this);
    m_layout->addItem(m_placeholder2);

    if (auto *scene = timelineScene())
        if (auto *view = scene->timelineView())
            if (!transition.isValid() && view->isAttached())
                emit zoomChanged(0);
}

void TransitionEditorGraphicsLayout::setDuration(qreal duration)
{
    m_rulerItem->invalidateRulerSize(duration);
}

void TransitionEditorGraphicsLayout::setZoom(int factor)
{
    m_rulerItem->setZoom(factor);
}

void TransitionEditorGraphicsLayout::invalidate()
{
    m_layout->invalidate();
}

int TransitionEditorGraphicsLayout::maximumScrollValue() const
{
    const qreal w = this->geometry().width() - qreal(TimelineConstants::sectionWidth);
    const qreal duration = m_rulerItem->rulerDuration() + m_rulerItem->rulerDuration() * 0.1;
    const qreal maxr = m_rulerItem->rulerScaling() * duration - w;
    return std::round(qMax(maxr, 0.0));
}

void TransitionEditorGraphicsLayout::activate()
{
    m_layout->activate();
}

TimelineRulerSectionItem *TransitionEditorGraphicsLayout::ruler() const
{
    return m_rulerItem;
}

} // End namespace QmlDesigner.
