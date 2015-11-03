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

#include "dobject.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DObject::DObject()
    : DElement(),
      m_modelUid(Uid::getInvalidUid()),
      m_depth(0),
      m_visualPrimaryRole(PRIMARY_ROLE_NORMAL),
      m_visualSecondaryRole(SECONDARY_ROLE_NONE),
      m_stereotypeDisplay(STEREOTYPE_SMART),
      m_autoSized(true),
      m_visualEmphasized(false)
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
      m_autoSized(rhs.m_autoSized),
      m_visualEmphasized(rhs.m_visualEmphasized)
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
        m_autoSized = rhs.m_autoSized;
        m_visualEmphasized = rhs.m_visualEmphasized;
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

void DObject::setVisualPrimaryRole(DObject::VisualPrimaryRole visual_primary_role)
{
    m_visualPrimaryRole = visual_primary_role;
}

void DObject::setVisualSecondaryRole(DObject::VisualSecondaryRole visual_secondary_role)
{
    m_visualSecondaryRole = visual_secondary_role;
}

void DObject::setStereotypeDisplay(DObject::StereotypeDisplay stereotype_display)
{
    m_stereotypeDisplay = stereotype_display;
}

void DObject::setAutoSize(bool auto_sized)
{
    m_autoSized = auto_sized;
}

void DObject::setVisualEmphasized(bool visual_emphasized)
{
    m_visualEmphasized = visual_emphasized;
}

void DObject::accept(DVisitor *visitor)
{
    visitor->visitDObject(this);
}

void DObject::accept(DConstVisitor *visitor) const
{
    visitor->visitDObject(this);
}

}
