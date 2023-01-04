// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dclass.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DClass::DClass()
{
}

DClass::~DClass()
{
}

void DClass::setUmlNamespace(const QString &umlNamespace)
{
    m_umlNamespace = umlNamespace;
}

void DClass::setTemplateParameters(const QList<QString> &templateParameters)
{
    m_templateParameters = templateParameters;
}

void DClass::setMembers(const QList<MClassMember> &members)
{
    m_members = members;
}

void DClass::setVisibleMembers(const QSet<Uid> &visibleMembers)
{
    m_visibleMembers = visibleMembers;
}

void DClass::setTemplateDisplay(DClass::TemplateDisplay templateDisplay)
{
    m_templateDisplay = templateDisplay;
}

void DClass::setShowAllMembers(bool showAllMembers)
{
    m_showAllMembers = showAllMembers;
}

void DClass::accept(DVisitor *visitor)
{
    visitor->visitDClass(this);
}

void DClass::accept(DConstVisitor *visitor) const
{
    visitor->visitDClass(this);
}

} // namespace qmt
