// Copyright (C) 2024 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/diagram/drelation.h"
#include "qmt/diagram/dobject.h"

#include <QColor>

namespace qmt {

class QMT_EXPORT RelationVisuals
{
public:
    RelationVisuals();
    RelationVisuals(DObject::VisualPrimaryRole visualObjectPrimaryRole,
                    DRelation::VisualPrimaryRole visualPrimaryRole,
                    DRelation::VisualSecondaryRole visualSecondaryRole,
                    bool emphasized);
    ~RelationVisuals();

    DObject::VisualPrimaryRole visualObjectPrimaryRole() const { return m_visualObjectPrimaryRole; }
    void setVisualObjectPrimaryRole(DObject::VisualPrimaryRole visualPrimaryRole);
    DRelation::VisualPrimaryRole visualPrimaryRole() const { return m_visualPrimaryRole; }
    void setVisualPrimaryRole(DRelation::VisualPrimaryRole visualPrimaryRole);
    DRelation::VisualSecondaryRole visualSecondaryRole() const { return m_visualSecondaryRole; }
    void setVisualSecondaryRole(DRelation::VisualSecondaryRole visualSecondaryRole);
    bool isEmphasized() const { return m_isEmphasized; }
    void setEmphasized(bool emphasized);

private:
    DObject::VisualPrimaryRole m_visualObjectPrimaryRole = DObject::PrimaryRoleNormal;
    DRelation::VisualPrimaryRole m_visualPrimaryRole = DRelation::PrimaryRoleNormal;
    DRelation::VisualSecondaryRole m_visualSecondaryRole = DRelation::SecondaryRoleNone;
    bool m_isEmphasized = false;
};

bool operator==(const RelationVisuals &lhs, const RelationVisuals &rhs);
size_t qHash(const RelationVisuals &relationVisuals);

} // namespace qmt
