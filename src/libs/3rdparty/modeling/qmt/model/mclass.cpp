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

#include "mclass.h"

#include "mvisitor.h"
#include "mconstvisitor.h"
#include "minheritance.h"
#include "mclassmember.h"

namespace qmt {

MClass::MClass()
    : MObject()
{
}

MClass::MClass(const MClass &rhs)
    : MObject(rhs),
      _namespace(rhs._namespace),
      _template_parameters(rhs._template_parameters),
      _members(rhs._members)
{
}

MClass::~MClass()
{
}

MClass &MClass::operator=(const MClass &rhs)
{
    if (this != &rhs) {
        MObject::operator =(rhs);
        _namespace = rhs._namespace;
        _template_parameters = rhs._template_parameters;
        _members = rhs._members;
    }
    return *this;
}

void MClass::setNamespace(const QString &name_space)
{
    _namespace = name_space;
}

void MClass::setTemplateParameters(const QList<QString> &template_parameters)
{
    _template_parameters = template_parameters;
}

void MClass::setMembers(const QList<MClassMember> &members)
{
    _members = members;
}

void MClass::addMember(const MClassMember &member)
{
    _members.append(member);
}

void MClass::insertMember(int before_index, const MClassMember &member)
{
    _members.insert(before_index, member);
}

void MClass::removeMember(const Uid &uid)
{
    QMT_CHECK(uid.isValid());
    for (int i = 0; i < _members.count(); ++i) {
        if (_members.at(i).getUid() == uid) {
            _members.removeAt(i);
            return;
        }
    }
    QMT_CHECK(false);
}

void MClass::removeMember(const MClassMember &member)
{
    removeMember(member.getUid());
}

void MClass::accept(MVisitor *visitor)
{
    visitor->visitMClass(this);
}

void MClass::accept(MConstVisitor *visitor) const
{
    visitor->visitMClass(this);
}

}
