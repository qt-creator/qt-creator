/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "mdependency.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MDependency::MDependency()
    : MRelation(),
      m_direction(AToB)
{
}

MDependency::MDependency(const MDependency &rhs)
    : MRelation(rhs),
      m_direction(rhs.m_direction)
{
}

MDependency::~MDependency()
{
}

MDependency &MDependency::operator =(const MDependency &rhs)
{
    if (this != &rhs) {
        MRelation::operator=(rhs);
        m_direction = rhs.m_direction;
    }
    return *this;
}

void MDependency::setDirection(MDependency::Direction direction)
{
    m_direction = direction;
}

Uid MDependency::source() const
{
    return m_direction == BToA ? endBUid() : endAUid();
}

void MDependency::setSource(const Uid &source)
{
    if (m_direction == BToA)
        setEndBUid(source);
    else
        setEndAUid(source);
}

Uid MDependency::target() const
{
    return m_direction == BToA ? endAUid() : endBUid();
}

void MDependency::setTarget(const Uid &target)
{
    if (m_direction == BToA)
        setEndAUid(target);
    else
        setEndBUid(target);
}

void MDependency::accept(MVisitor *visitor)
{
    visitor->visitMDependency(this);
}

void MDependency::accept(MConstVisitor *visitor) const
{
    visitor->visitMDependency(this);
}

} // namespace qmt
