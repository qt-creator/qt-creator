// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/diagram/dobject.h"

#include <QColor>

namespace qmt {

class QMT_EXPORT ObjectVisuals
{
public:
    ObjectVisuals();
    ObjectVisuals(DObject::VisualPrimaryRole visualPrimaryRole,
                  DObject::VisualSecondaryRole visualSecondaryRole,
                  bool emphasized, const QColor &baseColor, int depth);
    ~ObjectVisuals();

    DObject::VisualPrimaryRole visualPrimaryRole() const { return m_visualPrimaryRole; }
    void setVisualPrimaryRole(DObject::VisualPrimaryRole visualPrimaryRole);
    DObject::VisualSecondaryRole visualSecondaryRole() const { return m_visualSecondaryRole; }
    void setVisualSecondaryRole(DObject::VisualSecondaryRole visualSecondaryRole);
    bool isEmphasized() const { return m_isEmphasized; }
    void setEmphasized(bool emphasized);
    QColor baseColor() const { return m_baseColor; }
    void setBaseColor(const QColor &baseColor);
    int depth() const { return m_depth; }
    void setDepth(int depth);

private:
    friend bool operator==(const ObjectVisuals &lhs, const ObjectVisuals &rhs);

    DObject::VisualPrimaryRole m_visualPrimaryRole = DObject::PrimaryRoleNormal;
    DObject::VisualSecondaryRole m_visualSecondaryRole = DObject::SecondaryRoleNone;
    bool m_isEmphasized = false;
    QColor m_baseColor;
    int m_depth = 0;
};

size_t qHash(const ObjectVisuals &objectVisuals, size_t seed = 0);

} // namespace qmt
