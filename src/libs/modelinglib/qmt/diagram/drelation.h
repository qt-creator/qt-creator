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

#pragma once

#include "delement.h"

#include <QList>
#include <QPointF>

namespace qmt {

class DObject;

class QMT_EXPORT DRelation : public DElement
{
public:
    class IntermediatePoint
    {
    public:
        explicit IntermediatePoint(const QPointF &pos = QPointF());

        QPointF pos() const { return m_pos; }
        void setPos(const QPointF &pos);

    private:
        QPointF m_pos;
    };

    DRelation();
    ~DRelation() override;

    Uid modelUid() const override { return m_modelUid; }
    void setModelUid(const Uid &uid);
    QList<QString> stereotypes() const { return m_stereotypes; }
    void setStereotypes(const QList<QString> &stereotypes);
    Uid endAUid() const { return m_endAUid; }
    void setEndAUid(const Uid &uid);
    Uid endBUid() const { return m_endBUid; }
    void setEndBUid(const Uid &uid);
    QString name() const { return m_name; }
    void setName(const QString &name);
    QList<IntermediatePoint> intermediatePoints() const { return m_intermediatePoints; }
    void setIntermediatePoints(const QList<IntermediatePoint> &intermediatePoints);

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;

private:
    Uid m_modelUid;
    QList<QString> m_stereotypes;
    Uid m_endAUid;
    Uid m_endBUid;
    QString m_name;
    QList<IntermediatePoint> m_intermediatePoints;
};

bool operator==(const DRelation::IntermediatePoint &lhs, const DRelation::IntermediatePoint &rhs);
inline bool operator!=(const DRelation::IntermediatePoint &lhs,
                       const DRelation::IntermediatePoint &rhs)
{
    return !(lhs == rhs);
}

} // namespace qmt
