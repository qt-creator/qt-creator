// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "delement.h"

#include <QPointF>
#include <QRectF>

namespace qmt {

class QMT_EXPORT DAnnotation : public DElement
{
public:
    enum VisualRole {
        RoleNormal,
        RoleTitle,
        RoleSubtitle,
        RoleEmphasized,
        RoleSoften,
        RoleFootnote
    };

    DAnnotation();
    DAnnotation(const DAnnotation &);
    ~DAnnotation() override;

    DAnnotation &operator=(const DAnnotation &);

    Uid modelUid() const override { return Uid::invalidUid(); }
    QString text() const { return m_text; }
    void setText(const QString &text);
    QPointF pos() const { return m_pos; }
    void setPos(const QPointF &pos);
    QRectF rect() const { return m_rect; }
    void setRect(const QRectF &rect);
    VisualRole visualRole() const { return m_visualRole; }
    void setVisualRole(VisualRole visualRole);
    bool isAutoSized() const { return m_isAutoSized; }
    void setAutoSized(bool autoSized);

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;

private:
    QString m_text;
    QPointF m_pos;
    QRectF m_rect;
    VisualRole m_visualRole = RoleNormal;
    bool m_isAutoSized = true;
};

} // namespace qmt
