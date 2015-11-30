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

#ifndef QMT_MCLASSMEMBER_H
#define QMT_MCLASSMEMBER_H

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
        PropertyQsignal    = 0x100,
        PropertyQslot      = 0x200,
        PropertyQinvokable = 0x400,
        PropertyQproperty  = 0x800
    };

    Q_DECLARE_FLAGS(Properties, Property)

    explicit MClassMember(MemberType memberType = MemberUndefined);
    MClassMember(const MClassMember &rhs);
    ~MClassMember();

    MClassMember &operator=(const MClassMember &rhs);

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
    Visibility m_visibility;
    MemberType m_memberType;
    Properties m_properties;
};

bool operator==(const MClassMember &lhs, const MClassMember &rhs);

} // namespace qmt

#endif // QMT_MCLASSMEMBER_H
