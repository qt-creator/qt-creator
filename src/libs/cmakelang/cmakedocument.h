// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakeast.h"
#include "cmakeengine.h"

#include <QHash>
#include <QList>

#include <memory>

namespace CMakeLang {

class Document;
using DocumentPtr = std::shared_ptr<const Document>;

// A parsed CMake file. It owns the engine the AST and its tokens point into,
// so the AST stays valid for as long as the document is held.
class CMAKELANG_EXPORT Document
{
    Document(const Document &other) = delete;
    void operator=(const Document &other) = delete;

public:
    static DocumentPtr fromSource(const QString &source);

    // The parser recovers from a syntax error, so an invalid document still
    // has the commands around the broken ones.
    bool isValid() const { return !_engine.hasErrors(); }
    SourceFileAST *ast() const { return _ast; }
    const QList<Diagnostic> &diagnostics() const { return _engine.diagnostics(); }
    QString errorString() const;

    // Every command of the file in source order, the opening and closing
    // commands of the nested constructs included.
    const QList<CommandAST *> &commands() const { return _commands; }

    // What has to be entered to reach the node, outermost first: an if,
    // elseif or else branch, a foreach, while, function, macro or block.
    // Empty for what the file does unconditionally.
    QList<AST *> enclosingConstructs(const AST *node) const;

    // Whether both nodes run together: everything that governs the first also
    // governs the second.
    bool runsWith(const AST *node, const AST *other) const;

private:
    Document() = default;

    void collect(ListView<ElementAST *> elements, AST *construct);
    void addCommand(CommandAST *command, AST *construct);
    void addConstruct(AST *construct, AST *parent);

    Engine _engine;
    SourceFileAST *_ast = nullptr;
    QList<CommandAST *> _commands;
    QHash<const AST *, AST *> _enclosing;
};

} // namespace CMakeLang
