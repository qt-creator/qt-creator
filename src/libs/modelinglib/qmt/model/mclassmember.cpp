// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mclassmember.h"

namespace qmt {

MClassMember::MClassMember(MemberType memberType)
    : m_memberType(memberType)
{
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

void MClassMember::setMemberType(MClassMember::MemberType memberType)
{
    m_memberType = memberType;
}

void MClassMember::setProperties(Properties properties)
{
    m_properties = properties;
}

bool operator==(const MClassMember &lhs, const MClassMember &rhs)
{
    return lhs.uid() == rhs.uid();
}

} // namespace qmt
