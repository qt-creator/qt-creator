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

#include "mrelation.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MRelation::MRelation()
    : MElement(),
      m_endAUid(Uid::invalidUid()),
      m_endBUid(Uid::invalidUid())
{
}

MRelation::MRelation(const MRelation &rhs)
    : MElement(rhs),
      m_name(rhs.m_name),
      m_endAUid(rhs.m_endAUid),
      m_endBUid(rhs.m_endBUid)
{
}

MRelation::~MRelation()
{
}

MRelation &MRelation::operator =(const MRelation &rhs)
{
    if (this != &rhs) {
        MElement::operator=(rhs);
        m_name = rhs.m_name;
        m_endAUid = rhs.m_endAUid;
        m_endBUid = rhs.m_endBUid;
    }
    return *this;
}

void MRelation::setName(const QString &name)
{
    m_name = name;
}

void MRelation::setEndAUid(const Uid &uid)
{
    m_endAUid = uid;
}

void MRelation::setEndBUid(const Uid &uid)
{
    m_endBUid = uid;
}

void MRelation::accept(MVisitor *visitor)
{
    visitor->visitMRelation(this);
}

void MRelation::accept(MConstVisitor *visitor) const
{
    visitor->visitMRelation(this);
}

} // namespace qmt
