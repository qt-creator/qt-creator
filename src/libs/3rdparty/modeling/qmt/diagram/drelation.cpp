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

#include "drelation.h"

#include "dvisitor.h"
#include "dconstvisitor.h"


namespace qmt {

DRelation::IntermediatePoint::IntermediatePoint(const QPointF &pos)
    : _pos(pos)
{
}

void DRelation::IntermediatePoint::setPos(const QPointF &pos)
{
    _pos = pos;
}

bool operator==(const DRelation::IntermediatePoint &lhs, const DRelation::IntermediatePoint &rhs)
{
    return lhs.getPos() == rhs.getPos();
}


DRelation::DRelation()
    : DElement(),
      _model_uid(Uid::getInvalidUid()),
      _end_a_uid(Uid::getInvalidUid()),
      _end_b_uid(Uid::getInvalidUid())
{
}

DRelation::~DRelation()
{
}

void DRelation::setModelUid(const Uid &uid)
{
    _model_uid = uid;
}

void DRelation::setStereotypes(const QList<QString> &stereotypes)
{
    _stereotypes = stereotypes;
}

void DRelation::setEndA(const Uid &uid)
{
    _end_a_uid = uid;
}

void DRelation::setEndB(const Uid &uid)
{
    _end_b_uid = uid;
}

void DRelation::setName(const QString &name)
{
    _name = name;
}

void DRelation::setIntermediatePoints(const QList<DRelation::IntermediatePoint> &intermediate_points)
{
    _intermediate_points = intermediate_points;
}

}
