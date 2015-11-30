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

#ifndef QMT_DOBJECT_H
#define QMT_DOBJECT_H

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
        SecondaryRoleOutline
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
    Uid m_modelUid;
    QList<QString> m_stereotypes;
    QString m_context;
    QString m_name;
    QPointF m_pos;
    QRectF m_rect;
    int m_depth;
    VisualPrimaryRole m_visualPrimaryRole;
    VisualSecondaryRole m_visualSecondaryRole;
    StereotypeDisplay m_stereotypeDisplay;
    bool m_isAutoSized;
    bool m_isVisualEmphasized;
};

} // namespace qmt

#endif // QMT_DOBJECT_H
