/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "keyframe.h"

namespace DesignTools {

Keyframe::Keyframe()
    : m_position()
    , m_leftHandle()
    , m_rightHandle()
{}

Keyframe::Keyframe(const QPointF &position)
    : m_position(position)
    , m_leftHandle()
    , m_rightHandle()
{}

Keyframe::Keyframe(const QPointF &position, const QPointF &leftHandle, const QPointF &rightHandle)
    : m_position(position)
    , m_leftHandle(leftHandle)
    , m_rightHandle(rightHandle)
{}

bool Keyframe::hasLeftHandle() const
{
    return !m_leftHandle.isNull();
}

bool Keyframe::hasRightHandle() const
{
    return !m_rightHandle.isNull();
}

QPointF Keyframe::position() const
{
    return m_position;
}

QPointF Keyframe::leftHandle() const
{
    return m_leftHandle;
}

QPointF Keyframe::rightHandle() const
{
    return m_rightHandle;
}

void Keyframe::setPosition(const QPointF &pos)
{
    m_position = pos;
}

void Keyframe::setLeftHandle(const QPointF &pos)
{
    m_leftHandle = pos;
}

void Keyframe::setRightHandle(const QPointF &pos)
{
    m_rightHandle = pos;
}

} // End namespace DesignTools.
