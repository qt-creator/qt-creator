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

#ifndef QMT_DASSOCIATION_H
#define QMT_DASSOCIATION_H

#include "drelation.h"

#include "qmt/model/massociation.h"


namespace qmt {

class QMT_EXPORT DAssociationEnd
{
public:
    DAssociationEnd();

    ~DAssociationEnd();

public:

    QString getName() const { return _name; }

    void setName(const QString &name);

    QString getCardinality() const { return _cardinality; }

    void setCardinatlity(const QString &cardinality);

    MAssociationEnd::Kind getKind() const { return _kind; }

    void setKind(MAssociationEnd::Kind kind);

    bool isNavigable() const { return _navigable; }

    void setNavigable(bool navigable);

private:

    QString _name;

    QString _cardinality;

    MAssociationEnd::Kind _kind;

    bool _navigable;

};

bool operator==(const DAssociationEnd &lhs, const DAssociationEnd &rhs);

bool operator!=(const DAssociationEnd &lhs, const DAssociationEnd &rhs);


class QMT_EXPORT DAssociation :
        public DRelation
{
public:
    DAssociation();

    ~DAssociation();

public:

    DAssociationEnd getA() const { return _end_a; }

    void setA(const DAssociationEnd &end_a);

    DAssociationEnd getB() const { return _end_b; }

    void setB(const DAssociationEnd &end_b);

    Uid getAssoicationClassUid() const { return _association_class_uid; }

    void setAssociationClassUid(const Uid &uid);

public:

    virtual void accept(DVisitor *visitor);

    virtual void accept(DConstVisitor *visitor) const;

private:

    DAssociationEnd _end_a;

    DAssociationEnd _end_b;

    Uid _association_class_uid;
};

}

#endif // QMT_DASSOCIATION_H
