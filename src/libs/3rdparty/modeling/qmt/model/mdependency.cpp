/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "mdependency.h"

#include "mvisitor.h"
#include "mconstvisitor.h"


namespace qmt {

MDependency::MDependency()
    : MRelation(),
      m_direction(A_TO_B)
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
    return m_direction == B_TO_A ? endBUid() : endAUid();
}

void MDependency::setSource(const Uid &source)
{
    if (m_direction == B_TO_A) {
        setEndBUid(source);
    } else {
        setEndAUid(source);
    }
}

Uid MDependency::target() const
{
    return m_direction == B_TO_A ? endAUid() : endBUid();
}

void MDependency::setTarget(const Uid &target)
{
    if (m_direction == B_TO_A) {
        setEndAUid(target);
    } else {
        setEndBUid(target);
    }
}

void MDependency::accept(MVisitor *visitor)
{
    visitor->visitMDependency(this);
}

void MDependency::accept(MConstVisitor *visitor) const
{
    visitor->visitMDependency(this);
}

}
