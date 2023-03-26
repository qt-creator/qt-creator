// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
