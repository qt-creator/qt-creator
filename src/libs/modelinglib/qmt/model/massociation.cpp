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

#include "massociation.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MAssociationEnd::MAssociationEnd()
{
}

MAssociationEnd::MAssociationEnd(const MAssociationEnd &rhs)
    : m_name(rhs.m_name),
      m_cardinality(rhs.m_cardinality),
      m_kind(rhs.m_kind),
      m_isNavigable(rhs.m_isNavigable)
{
}

MAssociationEnd::~MAssociationEnd()
{
}

MAssociationEnd &MAssociationEnd::operator =(const MAssociationEnd &rhs)
{
    if (this != &rhs) {
        m_name = rhs.m_name;
        m_cardinality = rhs.m_cardinality;
        m_kind = rhs.m_kind;
        m_isNavigable = rhs.m_isNavigable;
    }
    return *this;
}

void MAssociationEnd::setName(const QString &name)
{
    m_name = name;
}

void MAssociationEnd::setCardinality(const QString &cardinality)
{
    m_cardinality = cardinality;
}

void MAssociationEnd::setKind(MAssociationEnd::Kind kind)
{
    m_kind = kind;
}

void MAssociationEnd::setNavigable(bool navigable)
{
    m_isNavigable = navigable;
}

bool operator==(const MAssociationEnd &lhs, const MAssociationEnd &rhs)
{
    return lhs.name() == rhs.name()
            && lhs.cardinality() == rhs.cardinality()
            && lhs.kind() == rhs.kind()
            && lhs.isNavigable() == rhs.isNavigable();
}

MAssociation::MAssociation()
    : MRelation(),
      m_associationClassUid(Uid::invalidUid())
{
}

MAssociation::~MAssociation()
{
}

void MAssociation::setEndA(const MAssociationEnd &end)
{
    m_endA = end;
}

void MAssociation::setEndB(const MAssociationEnd &end)
{
    m_endB = end;
}

void MAssociation::setAssociationClassUid(const Uid &uid)
{
    m_associationClassUid = uid;
}

void MAssociation::accept(MVisitor *visitor)
{
    visitor->visitMAssociation(this);
}

void MAssociation::accept(MConstVisitor *visitor) const
{
    visitor->visitMAssociation(this);
}

} // namespace qmt
