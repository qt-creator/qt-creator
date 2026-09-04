// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakedocument.h"
#include "cmakelang.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QStringList>

#include <optional>

namespace CMakeLang {

// The keywords a command takes, the way its cmake_parse_arguments() call
// declares them.
class CMAKELANG_EXPORT Signature
{
public:
    enum Arity {
        Option,    // The keyword stands on its own.
        OneValue,
        MultiValue
    };

    bool isEmpty() const { return _arities.isEmpty(); }
    std::optional<Arity> arity(const QString &keyword) const;
    QStringList keywords() const;

    void add(const QStringList &keywords, Arity arity);
    void add(const Signature &other);

private:
    QHash<QString, Arity> _arities;
};

// The values one keyword of a call takes.
class CMAKELANG_EXPORT KeywordArguments
{
public:
    // Null for the arguments before the first keyword, and for those that no
    // keyword takes.
    ArgumentAST *keyword = nullptr;
    QList<ArgumentAST *> values;
};

// The arguments of the call, grouped the way cmake_parse_arguments() reads
// them: an option keyword takes nothing, a one-value keyword the argument
// after it, a multi-value keyword everything up to the next keyword.
CMAKELANG_EXPORT QList<KeywordArguments> groupArguments(CommandAST *command,
                                                        const Signature &signature);

// The signatures of the functions and macros that CMake files define. A
// command that hands its arguments on with ${ARGV} or ${ARGN} also takes the
// keywords of the command it hands them to.
class CMAKELANG_EXPORT SignatureTable
{
public:
    void addDocument(const DocumentPtr &document);

    // Empty for a command none of the documents defines, and for one whose
    // keywords are not spelled out in the source.
    Signature signature(const QString &commandName) const;

private:
    class Definition
    {
    public:
        Signature signature;
        QStringList forwardsTo;
    };

    Signature resolve(const QString &name, QSet<QString> &visited) const;

    QHash<QString, Definition> _definitions;
};

} // namespace CMakeLang
