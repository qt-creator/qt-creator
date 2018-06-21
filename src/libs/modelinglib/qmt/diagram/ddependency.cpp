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

#include "ddependency.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DDependency::DDependency()
{
}

DDependency::~DDependency()
{
}

Uid DDependency::source() const
{
    return m_direction == MDependency::BToA ? endBUid() : endAUid();
}

void DDependency::setSource(const Uid &source)
{
    m_direction == MDependency::BToA ? setEndBUid(source) : setEndAUid(source);
}

Uid DDependency::target() const
{
    return m_direction == MDependency::BToA ? endAUid() : endBUid();
}

void DDependency::setTarget(const Uid &target)
{
    return m_direction == MDependency::BToA ? setEndAUid(target) : setEndBUid(target);
}

void DDependency::setDirection(MDependency::Direction direction)
{
    m_direction = direction;
}

void DDependency::accept(DVisitor *visitor)
{
    visitor->visitDDependency(this);
}

void DDependency::accept(DConstVisitor *visitor) const
{
    visitor->visitDDependency(this);
}

} // namespace qmt
