// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/uid.h"

#include <QList>
#include <QString>

namespace qmt {

class QMT_EXPORT MClassMember
{
public:
    enum Visibility {
        VisibilityUndefined,
        VisibilityPublic,
        VisibilityProtected,
        VisibilityPrivate,
        VisibilitySignals,
        VisibilityPrivateSlots,
        VisibilityProtectedSlots,
        VisibilityPublicSlots
    };

    enum MemberType {
        MemberUndefined,
        MemberAttribute,
        MemberMethod,
    };

    enum Property {
        PropertyVirtual    = 0x1,
        PropertyAbstract   = 0x2,
        PropertyConst      = 0x4,
        PropertyOverride   = 0x8,
        PropertyFinal      = 0x10,
        PropertyConstexpr  = 0x20,
        PropertyStatic     = 0x40,
        PropertyQsignal    = 0x100,
        PropertyQslot      = 0x200,
        PropertyQinvokable = 0x400,
        PropertyQproperty  = 0x800
    };

    Q_DECLARE_FLAGS(Properties, Property)

    explicit MClassMember(MemberType memberType = MemberUndefined);

    Uid uid() const { return m_uid; }
    void setUid(const Uid &uid);
    void renewUid();
    QList<QString> stereotypes() const { return m_stereotypes; }
    void setStereotypes(const QList<QString> &stereotypes);
    QString group() const { return m_group; }
    void setGroup(const QString &group);
    QString declaration() const { return m_declaration; }
    void setDeclaration(const QString &declaration);
    Visibility visibility() const { return m_visibility; }
    void setVisibility(Visibility visibility);
    MemberType memberType() const { return m_memberType; }
    void setMemberType(MemberType memberType);
    Properties properties() const { return m_properties; }
    void setProperties(Properties properties);

private:
    Uid m_uid;
    QList<QString> m_stereotypes;
    QString m_group;
    QString m_declaration;
    Visibility m_visibility = VisibilityUndefined;
    MemberType m_memberType = MemberUndefined;
    Properties m_properties;
};

bool operator==(const MClassMember &lhs, const MClassMember &rhs);

} // namespace qmt
