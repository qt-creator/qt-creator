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

#include "timelinerenderstate.h"

namespace QmlProfiler {
namespace Internal {

TimelineRenderState::TimelineRenderState(qint64 start, qint64 end, qreal scale, int numPasses) :
    m_expandedRowRoot(new QSGNode), m_collapsedRowRoot(new QSGNode),
    m_expandedOverlayRoot(new QSGNode), m_collapsedOverlayRoot(new QSGNode),
    m_start(start), m_end(end), m_scale(scale), m_passes(numPasses)
{
    m_expandedRowRoot->setFlag(QSGNode::OwnedByParent, false);
    m_collapsedRowRoot->setFlag(QSGNode::OwnedByParent, false);
    m_expandedOverlayRoot->setFlag(QSGNode::OwnedByParent, false);
    m_collapsedOverlayRoot->setFlag(QSGNode::OwnedByParent, false);
}

TimelineRenderState::~TimelineRenderState()
{
    delete m_expandedRowRoot;
    delete m_collapsedRowRoot;
    delete m_expandedOverlayRoot;
    delete m_collapsedOverlayRoot;
}

qint64 TimelineRenderState::start() const
{
    return m_start;
}

qint64 TimelineRenderState::end() const
{
    return m_end;
}

qreal TimelineRenderState::scale() const
{
    return m_scale;
}

QSGNode *TimelineRenderState::expandedRowRoot() const
{
    return m_expandedRowRoot;
}

QSGNode *TimelineRenderState::collapsedRowRoot() const
{
    return m_collapsedRowRoot;
}

QSGNode *TimelineRenderState::expandedOverlayRoot() const
{
    return m_expandedOverlayRoot;
}

QSGNode *TimelineRenderState::collapsedOverlayRoot() const
{
    return m_collapsedOverlayRoot;
}

bool TimelineRenderState::isEmpty() const
{
    return m_collapsedRowRoot->childCount() == 0 && m_expandedRowRoot->childCount() == 0 &&
            m_collapsedOverlayRoot->childCount() == 0 && m_expandedOverlayRoot->childCount() == 0;
}

TimelineRenderPass::State *TimelineRenderState::passState(int i)
{
    return m_passes[i];
}

const TimelineRenderPass::State *TimelineRenderState::passState(int i) const
{
    return m_passes[i];
}

void TimelineRenderState::setPassState(int i, TimelineRenderPass::State *state)
{
    m_passes[i] = state;
}

}
}
