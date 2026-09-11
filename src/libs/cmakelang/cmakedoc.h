// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakedocument.h"
#include "cmakelang.h"

#include <rstlang/rstdocument.h>
#include <rstlang/rstmarkdown.h>

#include <QAnyStringView>
#include <QList>
#include <QString>

#include <optional>

namespace CMakeLang {

// What one argument of a command means, the way the documentation of the
// command spells it out.
class CMAKELANG_EXPORT ArgumentDoc
{
public:
    QString name;
    QString documentation;
};

// What the documentation of one name says.  CMake writes the documentation
// of its modules into the modules themselves, in a comment that opens with
// ".rst:", and the same works for the functions a project defines.
class CMAKELANG_EXPORT Documentation
{
public:
    enum Kind {
        Unknown,
        Module,
        Command,
        Variable,
        EnvironmentVariable,
        Property,
        Policy,
        GeneratorExpression
    };

    QString name;
    Kind kind = Unknown;

    // Where the comment that carries the documentation starts, in the CMake
    // file it was read from.
    int line = 1;

    // The parsed comment.  It owns the node, so it stays here.
    RstLang::DocumentPtr rst;

    // The part of the comment that documents the name.
    RstLang::AST *node = nullptr;

    bool isNull() const { return !node; }

    bool isNamed(QAnyStringView other) const;

    // The whole documentation, as Markdown.
    QString markdown() const;

    // What a tooltip has room for: the name, the first paragraph and the
    // first signature.
    QString brief() const;

    // The calls the documentation spells out, such as
    // "find_package_message(<PackageName> <message> <details>)".
    QStringList signatures() const;

    // What each argument means, the way the definition lists of the
    // documentation spell it out.
    QList<ArgumentDoc> arguments() const;

private:
    // What turning the documentation into Markdown takes.  Reading the
    // substitutions the comment spells out is what it costs, so the four
    // readers above share one.
    const RstLang::MarkdownOptions &markdownOptions() const;

    mutable std::optional<RstLang::MarkdownOptions> _markdownOptions;
};

// CMake reads the name of a command without regard to its case, so
// "check_cxx_source_compiles" and "CHECK_CXX_SOURCE_COMPILES" name the same
// macro.  The documentation of a command spells it the way it is meant to be
// written, and that is the spelling to show.
CMAKELANG_EXPORT bool isSameCommand(QAnyStringView name, QAnyStringView other);

// The text of the ".rst:" comments a CMake file carries, in source order.
class CMAKELANG_EXPORT DocComment
{
public:
    QString text;
    int line = 1;

    // Where the comment ends in the source, which is where what it
    // documents starts.
    int end = 0;
};

CMAKELANG_EXPORT QList<DocComment> documentationComments(const QString &source);

// Every name the ".rst:" comments of a CMake file document.  A comment that
// names nothing documents the function or macro that follows it.
CMAKELANG_EXPORT QList<Documentation> documentation(
    const DocumentPtr &cmakeDocument, const RstLang::ParseOptions &options = {});

// Every name a documentation block written in reStructuredText documents,
// the way the Help of CMake writes it.
CMAKELANG_EXPORT QList<Documentation> documentation(const RstLang::DocumentPtr &rst);

// The documentation of one name, taken from a block that is known to be
// about it.  This is how the Help files of CMake are read: the file is
// named after what it documents.
CMAKELANG_EXPORT Documentation documentationFor(const RstLang::DocumentPtr &rst,
                                                const QString &name,
                                                Documentation::Kind kind);

// The signature a function() or macro() definition spells out, such as
// "my_helper(<target> <source>)".  Empty for a command that is none.
CMAKELANG_EXPORT QString definitionSignature(CommandAST *definition);

} // namespace CMakeLang
