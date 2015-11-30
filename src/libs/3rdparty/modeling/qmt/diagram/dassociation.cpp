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

#include "dassociation.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DAssociationEnd::DAssociationEnd()
    : m_kind(MAssociationEnd::Association),
      m_isNavigable(false)
{
}

DAssociationEnd::~DAssociationEnd()
{
}

void DAssociationEnd::setName(const QString &name)
{
    m_name = name;
}

void DAssociationEnd::setCardinatlity(const QString &cardinality)
{
    m_cardinality = cardinality;
}

void DAssociationEnd::setNavigable(bool navigable)
{
    m_isNavigable = navigable;
}

void DAssociationEnd::setKind(MAssociationEnd::Kind kind)
{
    m_kind = kind;
}

bool operator==(const DAssociationEnd &lhs, const DAssociationEnd &rhs)
{
    if (&lhs == &rhs)
        return true;
    return lhs.name() == rhs.name()
            && lhs.cardinality() == rhs.cardinality()
            && lhs.kind() == rhs.kind()
            && lhs.isNavigable() == rhs.isNavigable();
}

bool operator!=(const DAssociationEnd &lhs, const DAssociationEnd &rhs)
{
    return !operator==(lhs, rhs);
}

DAssociation::DAssociation()
    : m_associationClassUid(Uid::invalidUid())
{
}

DAssociation::~DAssociation()
{
}

void DAssociation::setEndA(const DAssociationEnd &endA)
{
    m_endA = endA;
}

void DAssociation::setEndB(const DAssociationEnd &endB)
{
    m_endB = endB;
}

void DAssociation::setAssociationClassUid(const Uid &uid)
{
    m_associationClassUid = uid;
}

void DAssociation::accept(DVisitor *visitor)
{
    visitor->visitDAssociation(this);
}

void DAssociation::accept(DConstVisitor *visitor) const
{
    visitor->visitDAssociation(this);
}

} // namespace qmt
