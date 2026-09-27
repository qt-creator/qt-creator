// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debugger_global.h"

#include <utils/result.h>

#include <QString>

#include <memory>

namespace Debugger::Internal {

// As much of a Java expression as a debugger can answer: a name, a literal,
// the two ways of reaching inside a value, and a method call. What an operator
// means for an object - "+" on two strings, say - is not here, since that is
// the compiler's doing rather than the machine's.
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
        // A method of what base holds, of the class base names where it names
        // one, or of the frame itself where there is no base. This is the one
        // thing here that makes the program run.
        Call,
        // On base, and on right as well where there are two of them.
        Unary,
        Binary,
    };

    enum class Operator {
        None,
        Not, Negate,
        Multiply, Divide, Remainder,
        Add, Subtract,
        Less, LessEqual, Greater, GreaterEqual,
        Equal, NotEqual,
        And, Or,
    };

    Kind kind = Kind::Null;
    Operator op = Operator::None;
    // The identifier of a Name, a Field or a Call, and the contents of a String.
    QString text;
    // Int, Long, Char and Boolean keep their value here, Float and Double theirs
    // in number.
    qint64 integer = 0;
    double number = 0;
    // What a Field, an Index or a Call is taken from, and the one side of a
    // Unary or the left of a Binary. A Call of a method of the frame itself
    // has none.
    std::shared_ptr<JdwpExpression> base;
    std::shared_ptr<JdwpExpression> index;
    std::shared_ptr<JdwpExpression> right;
    // What a Call passes, in the order they are written.
    QList<std::shared_ptr<JdwpExpression>> arguments;

    QString typeName() const;
    // Whether anything in it has to be run in the virtual machine, which is
    // what tells an expression that can be read off a suspended thread from
    // one that makes the program go.
    bool makesTheProgramRun() const;
    // The dotted name it spells out, "com.example.Foo", or nothing where it is
    // anything but names joined by dots.
    QString dottedName() const;
};

// Parses expression, or says what is wrong with it.
DEBUGGER_EXPORT Utils::Result<JdwpExpression> parseJdwpExpression(const QString &expression);

} // namespace Debugger::Internal
