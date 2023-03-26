// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    Q_UNUSED(element)

    delete this;
}

MElement::MElement()
{
}

MElement::MElement(const MElement &rhs)
    : m_uid(rhs.m_uid),
      m_expansion(rhs.m_expansion ? rhs.m_expansion->clone(rhs) : nullptr),
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
