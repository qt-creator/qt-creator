/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "dinheritance.h"

#include "dclass.h"
#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DInheritance::DInheritance()
    : DRelation()
{
}

DInheritance::~DInheritance()
{
}

Uid DInheritance::derived() const
{
    return endAUid();
}

void DInheritance::setDerived(const Uid &derived)
{
    setEndAUid(derived);
}

Uid DInheritance::base() const
{
    return endBUid();
}

void DInheritance::setBase(const Uid &base)
{
    setEndBUid(base);
}

void DInheritance::accept(DVisitor *visitor)
{
    visitor->visitDInheritance(this);
}

void DInheritance::accept(DConstVisitor *visitor) const
{
    visitor->visitDInheritance(this);
}

} // namespace qmt
