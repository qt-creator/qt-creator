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
    : m_visualPrimaryRole(DObject::PRIMARY_ROLE_NORMAL),
      m_visualSecondaryRole(DObject::SECONDARY_ROLE_NONE),
      m_emphasized(false),
      m_depth(0)
{
}

ObjectVisuals::ObjectVisuals(DObject::VisualPrimaryRole visual_primary_role,
                             DObject::VisualSecondaryRole visual_secondary_role,
                             bool emphasized, const QColor &base_color, int depth)
    : m_visualPrimaryRole(visual_primary_role),
      m_visualSecondaryRole(visual_secondary_role),
      m_emphasized(emphasized),
      m_baseColor(base_color),
      m_depth(depth)
{
}

ObjectVisuals::~ObjectVisuals()
{
}

void ObjectVisuals::setVisualPrimaryRole(DObject::VisualPrimaryRole visual_primary_role)
{
    m_visualPrimaryRole = visual_primary_role;
}

void ObjectVisuals::setVisualSecondaryRole(DObject::VisualSecondaryRole visual_secondary_role)
{
    m_visualSecondaryRole = visual_secondary_role;
}

void ObjectVisuals::setEmphasized(bool emphasized)
{
    m_emphasized = emphasized;
}

void ObjectVisuals::setBaseColor(const QColor &base_color)
{
    m_baseColor = base_color;
}

void ObjectVisuals::setDepth(int depth)
{
    m_depth = depth;
}

bool operator==(const ObjectVisuals &lhs, const ObjectVisuals &rhs)
{
    return lhs.getVisualPrimaryRole() == rhs.getVisualPrimaryRole()
            && lhs.getVisualSecondaryRole() == rhs.getVisualSecondaryRole()
            && lhs.isEmphasized() == rhs.isEmphasized()
            && lhs.getBaseColor() == rhs.getBaseColor()
            && lhs.getDepth() == rhs.getDepth();
}

uint qHash(const ObjectVisuals &object_visuals)
{
    return ::qHash((int) object_visuals.getVisualPrimaryRole())
            ^ ::qHash((int) object_visuals.getVisualSecondaryRole())
            ^ ::qHash(object_visuals.isEmphasized())
            ^ ::qHash(object_visuals.getBaseColor().rgb())
            ^ ::qHash(object_visuals.getDepth());
}

} // namespace qmt
