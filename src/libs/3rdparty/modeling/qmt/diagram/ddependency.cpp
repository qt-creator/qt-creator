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

#include "ddependency.h"

#include "dvisitor.h"
#include "dconstvisitor.h"


namespace qmt {

DDependency::DDependency()
    : _direction(MDependency::A_TO_B)
{
}

DDependency::~DDependency()
{
}

Uid DDependency::getSource() const
{
    return _direction == MDependency::B_TO_A ? getEndB() : getEndA();
}

void DDependency::setSource(const Uid &source)
{
    _direction == MDependency::B_TO_A ? setEndB(source) : setEndA(source);
}

Uid DDependency::getTarget() const
{
    return _direction == MDependency::B_TO_A ? getEndA() : getEndB();
}

void DDependency::setTarget(const Uid &target)
{
    return _direction == MDependency::B_TO_A ? setEndA(target) : setEndB(target);
}

void DDependency::setDirection(MDependency::Direction direction)
{
    _direction = direction;
}

void DDependency::accept(DVisitor *visitor)
{
    visitor->visitDDependency(this);
}

void DDependency::accept(DConstVisitor *visitor) const
{
    visitor->visitDDependency(this);
}

}
