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

#include "objectvisuals.h"

#include <qhash.h>

namespace qmt {

ObjectVisuals::ObjectVisuals()
    : m_visualPrimaryRole(DObject::PrimaryRoleNormal),
      m_visualSecondaryRole(DObject::SecondaryRoleNone),
      m_emphasized(false),
      m_depth(0)
{
}

ObjectVisuals::ObjectVisuals(DObject::VisualPrimaryRole visualPrimaryRole,
                             DObject::VisualSecondaryRole visualSecondaryRole,
                             bool emphasized, const QColor &baseColor, int depth)
    : m_visualPrimaryRole(visualPrimaryRole),
      m_visualSecondaryRole(visualSecondaryRole),
      m_emphasized(emphasized),
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
    m_emphasized = emphasized;
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

uint qHash(const ObjectVisuals &objectVisuals)
{
    return ::qHash(static_cast<int>(objectVisuals.visualPrimaryRole()))
            ^ ::qHash(static_cast<int>(objectVisuals.visualSecondaryRole()))
            ^ ::qHash(objectVisuals.isEmphasized())
            ^ ::qHash(objectVisuals.baseColor().rgb())
            ^ ::qHash(objectVisuals.depth());
}

} // namespace qmt
