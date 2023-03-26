// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "drelation.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DRelation::IntermediatePoint::IntermediatePoint(const QPointF &pos)
    : m_pos(pos)
{
}

void DRelation::IntermediatePoint::setPos(const QPointF &pos)
{
    m_pos = pos;
}

bool operator==(const DRelation::IntermediatePoint &lhs, const DRelation::IntermediatePoint &rhs)
{
    return lhs.pos() == rhs.pos();
}

DRelation::DRelation()
    : DElement(),
      m_modelUid(Uid::invalidUid()),
      m_endAUid(Uid::invalidUid()),
      m_endBUid(Uid::invalidUid())
{
}

DRelation::~DRelation()
{
}

void DRelation::setModelUid(const Uid &uid)
{
    m_modelUid = uid;
}

void DRelation::setStereotypes(const QList<QString> &stereotypes)
{
    m_stereotypes = stereotypes;
}

void DRelation::setEndAUid(const Uid &uid)
{
    m_endAUid = uid;
}

void DRelation::setEndBUid(const Uid &uid)
{
    m_endBUid = uid;
}

void DRelation::setName(const QString &name)
{
    m_name = name;
}

void DRelation::setIntermediatePoints(const QList<DRelation::IntermediatePoint> &intermediatePoints)
{
    m_intermediatePoints = intermediatePoints;
}

void DRelation::accept(DVisitor *visitor)
{
    visitor->visitDRelation(this);
}

void DRelation::accept(DConstVisitor *visitor) const
{
    visitor->visitDRelation(this);
}

} // namespace qmt
