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
    : _kind(ASSOCIATION),
      _navigable(false)
{
}

MAssociationEnd::MAssociationEnd(const MAssociationEnd &rhs)
    : _name(rhs._name),
      _cardinality(rhs._cardinality),
      _kind(rhs._kind),
      _navigable(rhs._navigable)
{
}

MAssociationEnd::~MAssociationEnd()
{
}

MAssociationEnd &MAssociationEnd::operator =(const MAssociationEnd &rhs)
{
    if (this != &rhs) {
        _name = rhs._name;
        _cardinality = rhs._cardinality;
        _kind = rhs._kind;
        _navigable = rhs._navigable;
    }
    return *this;
}

void MAssociationEnd::setName(const QString &name)
{
    _name = name;
}

void MAssociationEnd::setCardinality(const QString &cardinality)
{
    _cardinality = cardinality;
}

void MAssociationEnd::setKind(MAssociationEnd::Kind kind)
{
    _kind = kind;
}

void MAssociationEnd::setNavigable(bool navigable)
{
    _navigable = navigable;
}

bool operator==(const MAssociationEnd &lhs, const MAssociationEnd &rhs)
{
    return lhs.getName() == rhs.getName()
            && lhs.getCardinality() == rhs.getCardinality()
            && lhs.getKind() == rhs.getKind()
            && lhs.isNavigable() == rhs.isNavigable();
}


MAssociation::MAssociation()
    : MRelation(),
      _association_class_uid(Uid::getInvalidUid())
{
}

MAssociation::~MAssociation()
{
}

void MAssociation::setA(const MAssociationEnd &end)
{
    _a = end;
}

void MAssociation::setB(const MAssociationEnd &end)
{
    _b = end;
}

void MAssociation::setAssociationClassUid(const Uid &uid)
{
    _association_class_uid = uid;
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
