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

#include "massociation.h"

#include "mvisitor.h"
#include "mconstvisitor.h"


namespace qmt {

MAssociationEnd::MAssociationEnd()
    : m_kind(ASSOCIATION),
      m_navigable(false)
{
}

MAssociationEnd::MAssociationEnd(const MAssociationEnd &rhs)
    : m_name(rhs.m_name),
      m_cardinality(rhs.m_cardinality),
      m_kind(rhs.m_kind),
      m_navigable(rhs.m_navigable)
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
        m_navigable = rhs.m_navigable;
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
    m_navigable = navigable;
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

}
