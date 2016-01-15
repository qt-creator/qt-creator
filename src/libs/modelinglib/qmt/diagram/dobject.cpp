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

#include "dobject.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DObject::DObject()
    : DElement(),
      m_modelUid(Uid::invalidUid()),
      m_depth(0),
      m_visualPrimaryRole(PrimaryRoleNormal),
      m_visualSecondaryRole(SecondaryRoleNone),
      m_stereotypeDisplay(StereotypeSmart),
      m_isAutoSized(true),
      m_isVisualEmphasized(false)
{
}

DObject::DObject(const DObject &rhs)
    : DElement(rhs),
      m_modelUid(rhs.m_modelUid),
      m_stereotypes(rhs.m_stereotypes),
      m_context(rhs.m_context),
      m_name(rhs.m_name),
      m_pos(rhs.m_pos),
      m_rect(rhs.m_rect),
      m_depth(rhs.m_depth),
      m_visualPrimaryRole(rhs.m_visualPrimaryRole),
      m_visualSecondaryRole(rhs.m_visualSecondaryRole),
      m_stereotypeDisplay(rhs.m_stereotypeDisplay),
      m_isAutoSized(rhs.m_isAutoSized),
      m_isVisualEmphasized(rhs.m_isVisualEmphasized)
{
}

DObject::~DObject()
{
}

DObject &DObject::operator =(const DObject &rhs)
{
    if (this != &rhs) {
        DElement::operator=(rhs);
        m_modelUid = rhs.m_modelUid;
        m_stereotypes = rhs.m_stereotypes;
        m_context = rhs.m_context;
        m_name = rhs.m_name;
        m_pos = rhs.m_pos;
        m_rect = rhs.m_rect;
        m_depth = rhs.m_depth;
        m_visualPrimaryRole = rhs.m_visualPrimaryRole;
        m_visualSecondaryRole = rhs.m_visualSecondaryRole;
        m_stereotypeDisplay = rhs.m_stereotypeDisplay;
        m_isAutoSized = rhs.m_isAutoSized;
        m_isVisualEmphasized = rhs.m_isVisualEmphasized;
    }
    return *this;
}

void DObject::setModelUid(const Uid &uid)
{
    m_modelUid = uid;
}

void DObject::setStereotypes(const QList<QString> &stereotypes)
{
    m_stereotypes = stereotypes;
}

void DObject::setContext(const QString &context)
{
    m_context = context;
}

void DObject::setName(const QString &name)
{
    m_name = name;
}

void DObject::setPos(const QPointF &pos)
{
    m_pos = pos;
}

void DObject::setRect(const QRectF &rect)
{
    m_rect = rect;
}

void DObject::setDepth(int depth)
{
    m_depth = depth;
}

void DObject::setVisualPrimaryRole(DObject::VisualPrimaryRole visualPrimaryRole)
{
    m_visualPrimaryRole = visualPrimaryRole;
}

void DObject::setVisualSecondaryRole(DObject::VisualSecondaryRole visualSecondaryRole)
{
    m_visualSecondaryRole = visualSecondaryRole;
}

void DObject::setStereotypeDisplay(DObject::StereotypeDisplay stereotypeDisplay)
{
    m_stereotypeDisplay = stereotypeDisplay;
}

void DObject::setAutoSized(bool autoSized)
{
    m_isAutoSized = autoSized;
}

void DObject::setVisualEmphasized(bool visualEmphasized)
{
    m_isVisualEmphasized = visualEmphasized;
}

void DObject::accept(DVisitor *visitor)
{
    visitor->visitDObject(this);
}

void DObject::accept(DConstVisitor *visitor) const
{
    visitor->visitDObject(this);
}

} // namespace qmt
