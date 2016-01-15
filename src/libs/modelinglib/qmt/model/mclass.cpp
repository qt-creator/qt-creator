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
