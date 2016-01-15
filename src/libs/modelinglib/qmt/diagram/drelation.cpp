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
