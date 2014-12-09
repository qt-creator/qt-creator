/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "timelinerenderstate_p.h"

namespace Timeline {

TimelineRenderState::TimelineRenderState(qint64 start, qint64 end, qreal scale, int numPasses) :
    d_ptr(new TimelineRenderStatePrivate)
{
    Q_D(TimelineRenderState);
    d->expandedRowRoot = new QSGNode;
    d->collapsedRowRoot = new QSGNode;
    d->expandedOverlayRoot = new QSGNode;
    d->collapsedOverlayRoot = new QSGNode;
    d->start = start;
    d->end = end;
    d->scale = scale;
    d->passes.resize(numPasses);

    d->expandedRowRoot->setFlag(QSGNode::OwnedByParent, false);
    d->collapsedRowRoot->setFlag(QSGNode::OwnedByParent, false);
    d->expandedOverlayRoot->setFlag(QSGNode::OwnedByParent, false);
    d->collapsedOverlayRoot->setFlag(QSGNode::OwnedByParent, false);
}

TimelineRenderState::~TimelineRenderState()
{
    Q_D(TimelineRenderState);
    delete d->expandedRowRoot;
    delete d->collapsedRowRoot;
    delete d->expandedOverlayRoot;
    delete d->collapsedOverlayRoot;
    delete d;
}

qint64 TimelineRenderState::start() const
{
    Q_D(const TimelineRenderState);
    return d->start;
}

qint64 TimelineRenderState::end() const
{
    Q_D(const TimelineRenderState);
    return d->end;
}

qreal TimelineRenderState::scale() const
{
    Q_D(const TimelineRenderState);
    return d->scale;
}

const QSGNode *TimelineRenderState::expandedRowRoot() const
{
    Q_D(const TimelineRenderState);
    return d->expandedRowRoot;
}

const QSGNode *TimelineRenderState::collapsedRowRoot() const
{
    Q_D(const TimelineRenderState);
    return d->collapsedRowRoot;
}

const QSGNode *TimelineRenderState::expandedOverlayRoot() const
{
    Q_D(const TimelineRenderState);
    return d->expandedOverlayRoot;
}

const QSGNode *TimelineRenderState::collapsedOverlayRoot() const
{
    Q_D(const TimelineRenderState);
    return d->collapsedOverlayRoot;
}

QSGNode *TimelineRenderState::expandedRowRoot()
{
    Q_D(TimelineRenderState);
    return d->expandedRowRoot;
}

QSGNode *TimelineRenderState::collapsedRowRoot()
{
    Q_D(TimelineRenderState);
    return d->collapsedRowRoot;
}

QSGNode *TimelineRenderState::expandedOverlayRoot()
{
    Q_D(TimelineRenderState);
    return d->expandedOverlayRoot;
}

QSGNode *TimelineRenderState::collapsedOverlayRoot()
{
    Q_D(TimelineRenderState);
    return d->collapsedOverlayRoot;
}

bool TimelineRenderState::isEmpty() const
{
    Q_D(const TimelineRenderState);
    return d->collapsedRowRoot->childCount() == 0 && d->expandedRowRoot->childCount() == 0 &&
            d->collapsedOverlayRoot->childCount() == 0 && d->expandedOverlayRoot->childCount() == 0;
}

TimelineRenderPass::State *TimelineRenderState::passState(int i)
{
    Q_D(TimelineRenderState);
    return d->passes[i];
}

const TimelineRenderPass::State *TimelineRenderState::passState(int i) const
{
    Q_D(const TimelineRenderState);
    return d->passes[i];
}

void TimelineRenderState::setPassState(int i, TimelineRenderPass::State *state)
{
    Q_D(TimelineRenderState);
    d->passes[i] = state;
}

} // namespace Timeline
