// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "rstast.h"

#include <parsing/engine.h>

#include <QAnyStringView>
#include <QHash>
#include <QList>
#include <QStringList>

#include <functional>
#include <memory>
#include <optional>

namespace RstLang {

class Document;
using DocumentPtr = std::shared_ptr<const Document>;

class RSTLANG_EXPORT ParseOptions
{
public:
    // The directives whose argument names a file whose text belongs where
    // the directive stands.  CMake adds "cmake-module" to pull the
    // documentation out of the modules it ships.
    QStringList includeDirectives = {QStringLiteral("include")};

    // Called with the type of such a directive and with its argument.  What
    // it returns takes the place of the directive.  Where it is unset, the
    // directives are left standing.
    std::function<std::optional<QString>(const QString &type, const QString &argument)>
        resolveInclude;

    // How deep the files an include directive names may nest.  A file that is
    // already being read is never read again, so a cycle is reported instead
    // of being followed; this bounds what a chain of files may pull in.
    int includeLimit = 8;
};

// A parsed reStructuredText document.  It owns the engine the AST and its
// tokens point into, so the AST stays valid for as long as the document is
// held.
class RSTLANG_EXPORT Document
{
    Document(const Document &other) = delete;
    void operator=(const Document &other) = delete;

public:
    static DocumentPtr fromSource(const QString &source, const ParseOptions &options = {});

    // The parser recovers from a syntax error, so an invalid document still
    // has the blocks around the broken one.
    bool isValid() const { return !_engine.hasErrors(); }
    DocumentAST *ast() const { return _ast; }

    // The text the tokens of the AST point into, which is the source with
    // every include it names spliced in.
    QStringView source() const { return _engine.source(); }

    const QList<Diagnostic> &diagnostics() const { return _engine.diagnostics(); }
    QString errorString() const;

    // Every section of the document in source order, the nested ones
    // included.
    const QList<SectionAST *> &sections() const { return _sections; }

    // Every directive in source order, the nested ones included.
    const QList<DirectiveAST *> &directives() const { return _directives; }

    // The first directive of that type, null where the document has none.
    // An empty argument matches any.
    DirectiveAST *directive(QAnyStringView type, QAnyStringView argument = {}) const;

    // What the document spells out that a "|name|" stands for.
    const QHash<QString, SubstitutionAST *> &substitutions() const { return _substitutions; }

    // The text of every substitution, which is what rendering the document
    // needs to put in place of a reference to one.
    QHash<QString, QString> substitutionTexts() const;

    // The title of the document, which is the title of its first section.
    QStringView title() const;

private:
    Document() = default;

    void collect();

    Engine _engine;
    DocumentAST *_ast = nullptr;
    QList<SectionAST *> _sections;
    QList<DirectiveAST *> _directives;
    QHash<QString, SubstitutionAST *> _substitutions;
};

} // namespace RstLang
