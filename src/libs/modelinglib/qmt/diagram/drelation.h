// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    const QList<IntermediatePoint> intermediatePoints() const { return m_intermediatePoints; }
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
