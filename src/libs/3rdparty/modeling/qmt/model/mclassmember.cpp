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

#include "mclassmember.h"

namespace qmt {

MClassMember::MClassMember(MemberType member_type)
    : m_visibility(VISIBILITY_UNDEFINED),
      m_memberType(member_type)
{
}

MClassMember::MClassMember(const MClassMember &rhs)
    : m_uid(rhs.m_uid),
      m_stereotypes(rhs.m_stereotypes),
      m_group(rhs.m_group),
      m_declaration(rhs.m_declaration),
      m_visibility(rhs.m_visibility),
      m_memberType(rhs.m_memberType),
      m_properties(rhs.m_properties)
{
}

MClassMember::~MClassMember()
{
}

MClassMember &MClassMember::operator=(const MClassMember &rhs)
{
    if (this != &rhs) {
        m_uid = rhs.m_uid;
        m_stereotypes = rhs.m_stereotypes;
        m_group = rhs.m_group;
        m_declaration = rhs.m_declaration;
        m_visibility = rhs.m_visibility;
        m_memberType = rhs.m_memberType;
        m_properties = rhs.m_properties;
    }
    return *this;
}

void MClassMember::setUid(const Uid &uid)
{
    m_uid = uid;
}

void MClassMember::renewUid()
{
    m_uid.renew();
}

void MClassMember::setStereotypes(const QList<QString> &stereotypes)
{
    m_stereotypes = stereotypes;
}

void MClassMember::setGroup(const QString &group)
{
    m_group = group;
}

void MClassMember::setDeclaration(const QString &declaration)
{
    m_declaration = declaration;
}

void MClassMember::setVisibility(MClassMember::Visibility visibility)
{
    m_visibility = visibility;
}

void MClassMember::setMemberType(MClassMember::MemberType member_type)
{
    m_memberType = member_type;
}

void MClassMember::setProperties(Properties properties)
{
    m_properties = properties;
}

bool operator==(const MClassMember &lhs, const MClassMember &rhs)
{
    return lhs.getUid() == rhs.getUid();
}

}
