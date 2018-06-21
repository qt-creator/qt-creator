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

#include "customrelation.h"

namespace qmt {

void CustomRelation::End::setEndItems(const QList<QString> &endItems)
{
    m_endItems = endItems;
}

void CustomRelation::End::setRole(const QString &role)
{
    m_role = role;
}

void CustomRelation::End::setCardinality(const QString &cardinality)
{
    m_cardinality = cardinality;
}

void CustomRelation::End::setNavigable(bool navigable)
{
    m_navigable = navigable;
}

void CustomRelation::End::setRelationship(CustomRelation::Relationship relationship)
{
    m_relationship = relationship;
}

void CustomRelation::End::setHead(CustomRelation::Head head)
{
    m_head = head;
}

void CustomRelation::End::setShape(const IconShape &shape)
{
    m_shape = shape;
}


CustomRelation::CustomRelation()
{
}

CustomRelation::~CustomRelation()
{
}

bool CustomRelation::isNull() const
{
    return m_id.isEmpty();
}

void CustomRelation::setElement(CustomRelation::Element element)
{
    m_element = element;
}

void CustomRelation::setId(const QString &id)
{
    m_id = id;
}

void CustomRelation::setTitle(const QString &title)
{
    m_title = title;
}

void CustomRelation::setEndItems(const QList<QString> &endItems)
{
    m_endItems = endItems;
}

void CustomRelation::setStereotypes(const QSet<QString> &stereotypes)
{
    m_stereotypes = stereotypes;
}

void CustomRelation::setName(const QString &name)
{
    m_name = name;
}

void CustomRelation::setDirection(CustomRelation::Direction direction)
{
    m_direction = direction;
}

void CustomRelation::setEndA(const CustomRelation::End &end)
{
    m_endA = end;
}

void CustomRelation::setEndB(const CustomRelation::End &end)
{
    m_endB = end;
}

void CustomRelation::setShaftPattern(CustomRelation::ShaftPattern shaftPattern)
{
    m_shaftPattern = shaftPattern;
}

void CustomRelation::setColorType(CustomRelation::ColorType colorType)
{
    m_colorType = colorType;
}

void CustomRelation::setColor(const QColor &color)
{
    m_color = color;
}

} // namespace qmt
