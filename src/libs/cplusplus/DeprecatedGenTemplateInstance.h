// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/TypeVisitor.h>
#include <cplusplus/NameVisitor.h>
#include <cplusplus/FullySpecifiedType.h>

#include <QList>
#include <QPair>

#include <memory>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT DeprecatedGenTemplateInstance
{
public:
    using Substitution = QList<QPair<const Identifier *, FullySpecifiedType>>;

public:
    static FullySpecifiedType instantiate(const Name *className, Symbol *candidate,
                                          std::shared_ptr<Control> control);

private:
    DeprecatedGenTemplateInstance(std::shared_ptr<Control> control, const Substitution &substitution);
    FullySpecifiedType gen(Symbol *symbol);

private:
    std::shared_ptr<Control> _control;
    const Substitution _substitution;
};

} // namespace CPlusPlus
