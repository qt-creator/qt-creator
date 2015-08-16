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

#ifndef QMT_MOBJECT_H
#define QMT_MOBJECT_H

#include "melement.h"
#include "qmt/infrastructure/handles.h"

#include <QString>


namespace qmt {

class MRelation;


class QMT_EXPORT MObject :
        public MElement
{
public:

    MObject();

    MObject(const MObject &rhs);

    ~MObject();

public:

    MObject &operator=(const MObject &rhs);

public:

    QString getName() const { return _name; }

    void setName(const QString &name);

    const Handles<MObject> &getChildren() const { return _children; }

    void setChildren(const Handles<MObject> &children);

    void addChild(const Uid &uid);

    void addChild(MObject *child);

    void insertChild(int before_index, const Uid &uid);

    void insertChild(int before_index, MObject *child);

    void removeChild(const Uid &uid);

    void removeChild(MObject *child);

    void decontrolChild(const Uid &uid);

    void decontrolChild(MObject *child);

    const Handles<MRelation> &getRelations() const { return _relations; }

    void setRelations(const Handles<MRelation> &relations);

    void addRelation(const Uid &uid);

    void addRelation(MRelation *relation);

    void insertRelation(int before_index, MRelation *relation);

    void removeRelation(MRelation *relation);

    void decontrolRelation(MRelation *relation);

public:

    virtual void accept(MVisitor *visitor);

    virtual void accept(MConstVisitor *visitor) const;

private:

    QString _name;

    Handles<MObject> _children;

    Handles<MRelation> _relations;

};

}

#endif // QMT_MOBJECT_H
