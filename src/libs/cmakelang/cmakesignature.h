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

// What a command writes in the scope of its caller. Nothing where the values
// did not come out of the source: whoever reads them then knows only that
// whatever the variable held before is gone.
class CMAKELANG_EXPORT HandedBack
{
public:
    bool isEmpty() const;
    bool operator==(const HandedBack &other) const;
    bool operator!=(const HandedBack &other) const { return !(*this == other); }

    // The values by the position the call gives the parameter that names the
    // variable they go to.
    QHash<int, std::optional<QStringList>> positions;

    // The values by the name the body spells out itself, which is a variable
    // of the caller of that very name.
    QHash<QString, std::optional<QStringList>> names;

    // Whether the body writes a variable of its caller it cannot tell the name
    // of, which any variable the call names may be.
    bool anyVariable = false;
};

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

    // The commands a command hands its arguments on to, the ones those hand
    // them on to in turn included.  Whatever is said about the arguments of
    // one of them is said about the arguments of this command.
    QStringList forwardsTo(const QString &commandName) const;

private:
    class Definition
    {
    public:
        Signature signature;
        QStringList forwardsTo;
    };

    // The body of one function or macro definition.  The document holds the
    // AST alive, and the body is kept for as long as the table is: a document
    // that comes later may say what a command it calls hands back, and it is
    // read anew then.
    class Body
    {
    public:
        DocumentPtr document;
        NestedCommandAST *node = nullptr;
        QList<CommandAST *> commands;
    };

    void forgetUnread(const DocumentPtr &document);
    void readDocument(const DocumentPtr &document);
    QStringList readingOrder(const QSet<QString> &added) const;
    void addToOrder(const QString &name, const QSet<QString> &names, QSet<QString> &visited,
                    QStringList *order) const;
    bool read(const QString &name);

    Signature resolve(const QString &name, QSet<QString> &visited) const;
    void collectForwarded(const QString &name, QSet<QString> &visited,
                          QStringList *result) const;

    QHash<QString, Definition> _definitions;

    // The bodies that define a command, by its name: more than one document
    // may define the same one.  The names of them all, which is what tells a
    // call to a command of the documents from one to a command of somewhere
    // else: that one may write any variable the call names.
    QHash<QString, QList<Body>> _bodies;
    QSet<QString> _defined;

    // The documents that were looked at but not read, by the name of each
    // command they define, and the ones that were read, by their address.  A
    // document that was read is held on to whether it says anything or not:
    // another one would otherwise be given its address and pass for read.
    QHash<QString, QList<DocumentPtr>> _unread;
    QHash<const Document *, DocumentPtr> _documentsRead;

    // The commands the body of a definition calls, and the definitions whose
    // body calls a command, by the name of each.
    QHash<QString, QSet<QString>> _calls;
    QHash<QString, QSet<QString>> _callers;

    // What a command writes in the scope of its caller, by its name.  A
    // command whose keyword lists come out of such a call knows them only once
    // the command it got them from has been read.
    QHash<QString, HandedBack> _handedBack;
};

} // namespace CMakeLang
