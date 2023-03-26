// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "objectvisuals.h"

#include <qhash.h>

namespace qmt {

ObjectVisuals::ObjectVisuals()
{
}

ObjectVisuals::ObjectVisuals(DObject::VisualPrimaryRole visualPrimaryRole,
                             DObject::VisualSecondaryRole visualSecondaryRole,
                             bool emphasized, const QColor &baseColor, int depth)
    : m_visualPrimaryRole(visualPrimaryRole),
      m_visualSecondaryRole(visualSecondaryRole),
      m_isEmphasized(emphasized),
      m_baseColor(baseColor),
      m_depth(depth)
{
}

ObjectVisuals::~ObjectVisuals()
{
}

void ObjectVisuals::setVisualPrimaryRole(DObject::VisualPrimaryRole visualPrimaryRole)
{
    m_visualPrimaryRole = visualPrimaryRole;
}

void ObjectVisuals::setVisualSecondaryRole(DObject::VisualSecondaryRole visualSecondaryRole)
{
    m_visualSecondaryRole = visualSecondaryRole;
}

void ObjectVisuals::setEmphasized(bool emphasized)
{
    m_isEmphasized = emphasized;
}

void ObjectVisuals::setBaseColor(const QColor &baseColor)
{
    m_baseColor = baseColor;
}

void ObjectVisuals::setDepth(int depth)
{
    m_depth = depth;
}

bool operator==(const ObjectVisuals &lhs, const ObjectVisuals &rhs)
{
    return lhs.visualPrimaryRole() == rhs.visualPrimaryRole()
            && lhs.visualSecondaryRole() == rhs.visualSecondaryRole()
            && lhs.isEmphasized() == rhs.isEmphasized()
            && lhs.baseColor() == rhs.baseColor()
            && lhs.depth() == rhs.depth();
}

size_t qHash(const ObjectVisuals &objectVisuals)
{
    return ::qHash(static_cast<int>(objectVisuals.visualPrimaryRole()))
            ^ ::qHash(static_cast<int>(objectVisuals.visualSecondaryRole()))
            ^ ::qHash(objectVisuals.isEmphasized())
            ^ ::qHash(objectVisuals.baseColor().rgb())
            ^ ::qHash(objectVisuals.depth());
}

} // namespace qmt
