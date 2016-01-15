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

#include "melement.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

void MExpansion::assign(MElement *lhs, const MElement &rhs)
{
    if (lhs->m_expansion)
        lhs->m_expansion->destroy(lhs);
    lhs->m_expansion = clone(rhs);
}

void MExpansion::destroy(MElement *element)
{
    Q_UNUSED(element);

    delete this;
}

MElement::MElement()
{
}

MElement::MElement(const MElement &rhs)
    : m_uid(rhs.m_uid),
      m_expansion(rhs.m_expansion ? rhs.m_expansion->clone(rhs) : 0),
      m_stereotypes(rhs.m_stereotypes)
{
}

MElement::~MElement()
{
    if (m_expansion)
        m_expansion->destroy(this);
}

MElement &MElement::operator=(const MElement &rhs)
{
    if (this != &rhs) {
        m_uid = rhs.m_uid;
        // owner is intentionally left unchanged
        if (rhs.m_expansion)
            rhs.m_expansion->assign(this, rhs);
        m_stereotypes = rhs.m_stereotypes;
    }
    return *this;
}

void MElement::setUid(const Uid &uid)
{
    m_uid = uid;
}

void MElement::renewUid()
{
    m_uid.renew();
}

void MElement::setOwner(MObject *owner)
{
    m_owner = owner;
}

void MElement::setExpansion(MExpansion *expansion)
{
    if (m_expansion)
        m_expansion->destroy(this);
    m_expansion = expansion;
}

void MElement::setStereotypes(const QList<QString> &stereotypes)
{
    m_stereotypes = stereotypes;
}

void MElement::setFlags(const Flags &flags)
{
    m_flags = flags;
}

void MElement::accept(MVisitor *visitor)
{
    visitor->visitMElement(this);
}

void MElement::accept(MConstVisitor *visitor) const
{
    visitor->visitMElement(this);
}

} // namespace qmt
