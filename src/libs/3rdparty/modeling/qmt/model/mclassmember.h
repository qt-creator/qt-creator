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
        VISIBILITY_UNDEFINED,
        VISIBILITY_PUBLIC,
        VISIBILITY_PROTECTED,
        VISIBILITY_PRIVATE,
        VISIBILITY_SIGNALS,
        VISIBILITY_PRIVATE_SLOTS,
        VISIBILITY_PROTECTED_SLOTS,
        VISIBILITY_PUBLIC_SLOTS
    };

    enum MemberType {
        MEMBER_UNDEFINED,
        MEMBER_ATTRIBUTE,
        MEMBER_METHOD,
    };

    enum Property {
        PROPERTY_VIRTUAL    = 0x1,
        PROPERTY_ABSTRACT   = 0x2,
        PROPERTY_CONST      = 0x4,
        PROPERTY_OVERRIDE   = 0x8,
        PROPERTY_FINAL      = 0x10,
        PROPERTY_QSIGNAL    = 0x100,
        PROPERTY_QSLOT      = 0x200,
        PROPERTY_QINVOKABLE = 0x400,
        PROPERTY_QPROPERTY  = 0x800
    };

    Q_DECLARE_FLAGS(Properties, Property)

public:

    MClassMember(MemberType member_type = MEMBER_UNDEFINED);

    MClassMember(const MClassMember &rhs);

    ~MClassMember();

public:

    MClassMember &operator=(const MClassMember &rhs);

public:

    Uid getUid() const { return m_uid; }

    void setUid(const Uid &uid);

    void renewUid();

    QList<QString> getStereotypes() const { return m_stereotypes; }

    void setStereotypes(const QList<QString> &stereotypes);

    QString getGroup() const { return m_group; }

    void setGroup(const QString &group);

    QString getDeclaration() const { return m_declaration; }

    void setDeclaration(const QString &declaration);

    Visibility getVisibility() const { return m_visibility; }

    void setVisibility(Visibility visibility);

    MemberType getMemberType() const { return m_memberType; }

    void setMemberType(MemberType member_type);

    Properties getProperties() const { return m_properties; }

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

}

#endif // QMT_MCLASSMEMBER_H
