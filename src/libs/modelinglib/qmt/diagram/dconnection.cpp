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

#include "dconnection.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DConnectionEnd::DConnectionEnd()
{
}

DConnectionEnd::~DConnectionEnd()
{
}

void DConnectionEnd::setName(const QString &name)
{
    m_name = name;
}

void DConnectionEnd::setCardinatlity(const QString &cardinality)
{
    m_cardinality = cardinality;
}

void DConnectionEnd::setNavigable(bool navigable)
{
    m_isNavigable = navigable;
}

bool operator==(const DConnectionEnd &lhs, const DConnectionEnd &rhs)
{
    if (&lhs == &rhs)
        return true;
    return lhs.name() == rhs.name()
            && lhs.cardinality() == rhs.cardinality()
            && lhs.isNavigable() == rhs.isNavigable();
}

bool operator!=(const DConnectionEnd &lhs, const DConnectionEnd &rhs)
{
    return !operator==(lhs, rhs);
}

DConnection::DConnection()
{
}

DConnection::~DConnection()
{
}

void DConnection::setCustomRelationId(const QString &customRelationId)
{
    m_customRelationId = customRelationId;
}

void DConnection::setEndA(const DConnectionEnd &endA)
{
    m_endA = endA;
}

void DConnection::setEndB(const DConnectionEnd &endB)
{
    m_endB = endB;
}

void DConnection::accept(DVisitor *visitor)
{
    visitor->visitDConnection(this);
}

void DConnection::accept(DConstVisitor *visitor) const
{
    visitor->visitDConnection(this);
}

} // namespace qmt
