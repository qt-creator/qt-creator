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

#include "melement.h"

#include "mvisitor.h"
#include "mconstvisitor.h"


namespace qmt {

void MExpansion::assign(MElement *lhs, const MElement &rhs)
{
    if (lhs->_expansion) {
        lhs->_expansion->destroy(lhs);
    }
    lhs->_expansion = clone(rhs);
}

void MExpansion::destroy(MElement *element)
{
    Q_UNUSED(element);

    delete this;
}


MElement::MElement()
    : _owner(0),
      _expansion(0)
{
}

MElement::MElement(const MElement &rhs)
    : _uid(rhs._uid),
      _owner(0),
      _expansion(rhs._expansion ? rhs._expansion->clone(rhs) : 0),
      _stereotypes(rhs._stereotypes)
{
}

MElement::~MElement()
{
    if (_expansion) {
        _expansion->destroy(this);
    }
}

MElement &MElement::operator=(const MElement &rhs)
{
    if (this != &rhs) {
        _uid = rhs._uid;
        // owner is intentionally left unchanged
        if (rhs._expansion) {
            rhs._expansion->assign(this, rhs);
        }
        _stereotypes = rhs._stereotypes;
    }
    return *this;
}

void MElement::setUid(const Uid &uid)
{
    _uid = uid;
}

void MElement::renewUid()
{
    _uid.renew();
}

void MElement::setOwner(MObject *owner)
{
    _owner = owner;
}

void MElement::setExpansion(MExpansion *expansion)
{
    if (_expansion) {
        _expansion->destroy(this);
    }
    _expansion = expansion;
}

void MElement::setStereotypes(const QList<QString> &stereotypes)
{
    _stereotypes = stereotypes;
}

void MElement::setFlags(const Flags &flags)
{
    _flags = flags;
}

void MElement::accept(MVisitor *visitor)
{
    visitor->visitMElement(this);
}

void MElement::accept(MConstVisitor *visitor) const
{
    visitor->visitMElement(this);
}

}
