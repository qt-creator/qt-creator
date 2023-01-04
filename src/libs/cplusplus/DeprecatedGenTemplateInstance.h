// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
