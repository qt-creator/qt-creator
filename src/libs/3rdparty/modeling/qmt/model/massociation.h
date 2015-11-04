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

#ifndef QMT_MASSOCIATION_H
#define QMT_MASSOCIATION_H

#include "mrelation.h"
#include "qmt/infrastructure/handle.h"

#include <QString>


namespace qmt {

class MClass;


class QMT_EXPORT MAssociationEnd
{
public:

    enum Kind {
        ASSOCIATION,
        AGGREGATION,
        COMPOSITION
    };

public:

    MAssociationEnd();

    MAssociationEnd(const MAssociationEnd &rhs);

    ~MAssociationEnd();

public:

    MAssociationEnd &operator=(const MAssociationEnd &rhs);

public:

    QString name() const { return m_name; }

    void setName(const QString &name);

    QString cardinality() const { return m_cardinality; }

    void setCardinality(const QString &cardinality);

    Kind kind() const { return m_kind; }

    void setKind(Kind kind);

    bool isNavigable() const { return m_navigable; }

    void setNavigable(bool navigable);

private:

    QString m_name;

    QString m_cardinality;

    Kind m_kind;

    bool m_navigable;
};

bool operator==(const MAssociationEnd &lhs, const MAssociationEnd &rhs);

inline bool operator!=(const MAssociationEnd &lhs, const MAssociationEnd &rhs) { return !(lhs == rhs); }


class QMT_EXPORT MAssociation :
        public MRelation
{
public:

    MAssociation();

    ~MAssociation();

public:

    MAssociationEnd endA() const { return m_endA; }

    void setEndA(const MAssociationEnd &end);

    MAssociationEnd endB() const { return m_endB; }

    void setEndB(const MAssociationEnd &end);

    Uid assoicationClassUid() const { return m_associationClassUid; }

    void setAssociationClassUid(const Uid &uid);

public:

    virtual void accept(MVisitor *visitor);

    virtual void accept(MConstVisitor *visitor) const;

private:

    MAssociationEnd m_endA;

    MAssociationEnd m_endB;

    Uid m_associationClassUid;
};

}

#endif // QMT_MASSOCIATION_H
