// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mclass.h"

#include "mvisitor.h"
#include "mconstvisitor.h"
#include "minheritance.h"
#include "mclassmember.h"

namespace qmt {

MClass::MClass()
    : MObject()
{
}

MClass::MClass(const MClass &rhs)
    : MObject(rhs),
      m_umlNamespace(rhs.m_umlNamespace),
      m_templateParameters(rhs.m_templateParameters),
      m_members(rhs.m_members)
{
}

MClass::~MClass()
{
}

MClass &MClass::operator=(const MClass &rhs)
{
    if (this != &rhs) {
        MObject::operator =(rhs);
        m_umlNamespace = rhs.m_umlNamespace;
        m_templateParameters = rhs.m_templateParameters;
        m_members = rhs.m_members;
    }
    return *this;
}

void MClass::setUmlNamespace(const QString &umlNamespace)
{
    m_umlNamespace = umlNamespace;
}

void MClass::setTemplateParameters(const QList<QString> &templateParameters)
{
    m_templateParameters = templateParameters;
}

void MClass::setMembers(const QList<MClassMember> &members)
{
    m_members = members;
}

void MClass::addMember(const MClassMember &member)
{
    m_members.append(member);
}

void MClass::insertMember(int beforeIndex, const MClassMember &member)
{
    m_members.insert(beforeIndex, member);
}

void MClass::removeMember(const Uid &uid)
{
    QMT_CHECK(uid.isValid());
    for (int i = 0; i < m_members.count(); ++i) {
        if (m_members.at(i).uid() == uid) {
            m_members.removeAt(i);
            return;
        }
    }
    QMT_CHECK(false);
}

void MClass::removeMember(const MClassMember &member)
{
    removeMember(member.uid());
}

void MClass::accept(MVisitor *visitor)
{
    visitor->visitMClass(this);
}

void MClass::accept(MConstVisitor *visitor) const
{
    visitor->visitMClass(this);
}

} // namespace qmt
