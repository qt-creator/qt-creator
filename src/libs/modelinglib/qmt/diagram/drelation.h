// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "delement.h"

#include <QColor>
#include <QList>
#include <QPointF>

namespace qmt {

class DObject;

class QMT_EXPORT DRelation : public DElement
{
public:
    enum VisualPrimaryRole {
        PrimaryRoleNormal,
        PrimaryRoleCustom1,
        PrimaryRoleCustom2,
        PrimaryRoleCustom3,
        PrimaryRoleCustom4,
        PrimaryRoleCustom5
    };

    enum VisualSecondaryRole {
        SecondaryRoleNone,
        SecondaryRoleWarning,
        SecondaryRoleError,
        SecondaryRoleSoften
    };

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
    VisualPrimaryRole visualPrimaryRole() const { return m_visualPrimaryRole; }
    void setVisualPrimaryRole(VisualPrimaryRole visualPrimaryRole);
    VisualSecondaryRole visualSecondaryRole() const { return m_visualSecondaryRole; }
    void setVisualSecondaryRole(VisualSecondaryRole visualSecondaryRole);
    bool isVisualEmphasized() const { return m_isVisualEmphasized; }
    void setVisualEmphasized(bool visualEmphasized);
    QColor color() const { return m_color; }
    void setColor(const QColor &color);
    qreal thickness() const { return m_thickness; }
    void setThickness(qreal thickness);

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;

private:
    Uid m_modelUid;
    QList<QString> m_stereotypes;
    Uid m_endAUid;
    Uid m_endBUid;
    QString m_name;
    QList<IntermediatePoint> m_intermediatePoints;
    VisualPrimaryRole m_visualPrimaryRole = PrimaryRoleNormal;
    VisualSecondaryRole m_visualSecondaryRole = SecondaryRoleNone;
    bool m_isVisualEmphasized = false;
    QColor m_color;
    qreal m_thickness = 0;
};

bool operator==(const DRelation::IntermediatePoint &lhs, const DRelation::IntermediatePoint &rhs);
inline bool operator!=(const DRelation::IntermediatePoint &lhs,
                       const DRelation::IntermediatePoint &rhs)
{
    return !(lhs == rhs);
}

} // namespace qmt
