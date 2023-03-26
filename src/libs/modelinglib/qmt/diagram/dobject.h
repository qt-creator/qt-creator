// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "delement.h"

#include "qmt/infrastructure/uid.h"

#include <QList>
#include <QPointF>
#include <QRectF>

namespace qmt {

class MObject;

class QMT_EXPORT DObject : public DElement
{
public:
    enum VisualPrimaryRole {
        PrimaryRoleNormal,
        DeprecatedPrimaryRoleLighter,
        DeprecatedPrimaryRoleDarker,
        DeprecatedPrimaryRoleSoften,
        DeprecatedPrimaryRoleOutline,
        PrimaryRoleCustom1,
        PrimaryRoleCustom2,
        PrimaryRoleCustom3,
        PrimaryRoleCustom4,
        PrimaryRoleCustom5
    };

    enum VisualSecondaryRole {
        SecondaryRoleNone,
        SecondaryRoleLighter,
        SecondaryRoleDarker,
        SecondaryRoleSoften,
        SecondaryRoleOutline,
        SecondaryRoleFlat
    };

    enum StereotypeDisplay {
        StereotypeNone,
        StereotypeLabel,
        StereotypeDecoration,
        StereotypeIcon,
        StereotypeSmart
    };

    DObject();
    DObject(const DObject &);
    ~DObject() override;

    DObject &operator=(const DObject &rhs);

    Uid modelUid() const override { return m_modelUid; }
    void setModelUid(const Uid &uid);
    QList<QString> stereotypes() const { return m_stereotypes; }
    void setStereotypes(const QList<QString> &stereotypes);
    QString context() const { return m_context; }
    void setContext(const QString &context);
    QString name() const { return m_name; }
    void setName(const QString &name);
    QPointF pos() const { return m_pos; }
    void setPos(const QPointF &pos);
    QRectF rect() const { return m_rect; }
    void setRect(const QRectF &rect);
    int depth() const { return m_depth; }
    void setDepth(int depth);
    VisualPrimaryRole visualPrimaryRole() const { return m_visualPrimaryRole; }
    void setVisualPrimaryRole(VisualPrimaryRole visualPrimaryRole);
    VisualSecondaryRole visualSecondaryRole() const { return m_visualSecondaryRole; }
    void setVisualSecondaryRole(VisualSecondaryRole visualSecondaryRole);
    StereotypeDisplay stereotypeDisplay() const { return m_stereotypeDisplay; }
    void setStereotypeDisplay(StereotypeDisplay stereotypeDisplay);
    bool isAutoSized() const { return m_isAutoSized; }
    void setAutoSized(bool autoSized);
    bool isVisualEmphasized() const { return m_isVisualEmphasized; }
    void setVisualEmphasized(bool visualEmphasized);

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;

private:
    Uid m_modelUid = Uid::invalidUid();
    QList<QString> m_stereotypes;
    QString m_context;
    QString m_name;
    QPointF m_pos;
    QRectF m_rect;
    int m_depth = 0;
    VisualPrimaryRole m_visualPrimaryRole = PrimaryRoleNormal;
    VisualSecondaryRole m_visualSecondaryRole = SecondaryRoleNone;
    StereotypeDisplay m_stereotypeDisplay = StereotypeSmart;
    bool m_isAutoSized = true;
    bool m_isVisualEmphasized = false;
};

} // namespace qmt
