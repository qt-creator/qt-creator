/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include <cplusplus/TypeVisitor.h>
#include <cplusplus/NameVisitor.h>
#include <cplusplus/FullySpecifiedType.h>

#include <QList>
#include <QPair>
#include <QSharedPointer>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT DeprecatedGenTemplateInstance
{
public:
    typedef QList< QPair<const Identifier *, FullySpecifiedType> > Substitution;

public:
    static FullySpecifiedType instantiate(const Name *className, Symbol *candidate, QSharedPointer<Control> control);

private:
    DeprecatedGenTemplateInstance(QSharedPointer<Control> control, const Substitution &substitution);
    FullySpecifiedType gen(Symbol *symbol);

private:
    QSharedPointer<Control> _control;
    const Substitution _substitution;
};

} // namespace CPlusPlus
