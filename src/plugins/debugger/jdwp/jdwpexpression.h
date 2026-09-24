// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debugger_global.h"

#include <utils/result.h>

#include <QString>

#include <memory>

namespace Debugger::Internal {

// As much of a Java expression as a debugger can answer by reading the virtual
// machine, which is what JDWP offers: a name, a literal, and the two ways of
// reaching inside a value. Anything that would have to run code in the machine
// to produce its result - a method call, an operator on objects - is not here.
class DEBUGGER_EXPORT JdwpExpression
{
public:
    enum class Kind {
        Int, Long, Float, Double, Boolean, Char, String, Null,
        // A local variable of the frame, a field of its object, or a static
        // field of its class, in that order.
        Name,
        This,
        // Of what base holds: a field by name, an element by index. The length
        // of an array reads as a field, as it does in Java.
        Field,
        Index,
    };

    Kind kind = Kind::Null;
    // The identifier of a Name or a Field, and the contents of a String.
    QString text;
    // Int, Long, Char and Boolean keep their value here, Float and Double theirs
    // in number.
    qint64 integer = 0;
    double number = 0;
    std::shared_ptr<JdwpExpression> base;
    std::shared_ptr<JdwpExpression> index;

    QString typeName() const;
};

// Parses expression, or says what is wrong with it.
DEBUGGER_EXPORT Utils::Result<JdwpExpression> parseJdwpExpression(const QString &expression);

} // namespace Debugger::Internal
