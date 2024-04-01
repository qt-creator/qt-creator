// Copyright (C) 2024 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "relationvisuals.h"

namespace qmt {

RelationVisuals::RelationVisuals() {}

RelationVisuals::RelationVisuals(DObject::VisualPrimaryRole visualObjectPrimaryRole,
                                 DRelation::VisualPrimaryRole visualPrimaryRole,
                                 DRelation::VisualSecondaryRole visualSecondaryRole,
                                 bool emphasized)
    : m_visualObjectPrimaryRole(visualObjectPrimaryRole)
    , m_visualPrimaryRole(visualPrimaryRole)
    , m_visualSecondaryRole(visualSecondaryRole)
    , m_isEmphasized(emphasized)
{}

RelationVisuals::~RelationVisuals() {}

void RelationVisuals::setVisualPrimaryRole(DRelation::VisualPrimaryRole VisualPrimaryRole)
{
    m_visualPrimaryRole = VisualPrimaryRole;
}

void RelationVisuals::setVisualObjectPrimaryRole(DObject::VisualPrimaryRole visualPrimaryRole)
{
    m_visualObjectPrimaryRole = visualPrimaryRole;
}

void RelationVisuals::setVisualSecondaryRole(DRelation::VisualSecondaryRole visualSecondaryRole)
{
    m_visualSecondaryRole = visualSecondaryRole;
}

void RelationVisuals::setEmphasized(bool emphasized)
{
    m_isEmphasized = emphasized;
}

bool operator==(const RelationVisuals &lhs, const RelationVisuals &rhs)
{
    return lhs.visualObjectPrimaryRole() == rhs.visualObjectPrimaryRole()
           && lhs.visualPrimaryRole() == rhs.visualPrimaryRole()
           && lhs.visualSecondaryRole() == rhs.visualSecondaryRole()
           && lhs.isEmphasized() == rhs.isEmphasized();
}

size_t qHash(const RelationVisuals &relationVisuals)
{
    return ::qHash(static_cast<int>(relationVisuals.visualObjectPrimaryRole()))
           ^ ::qHash(static_cast<int>(relationVisuals.visualPrimaryRole()))
           ^ ::qHash(static_cast<int>(relationVisuals.visualSecondaryRole()))
           ^ ::qHash(relationVisuals.isEmphasized());
}

} // namespace qmt
