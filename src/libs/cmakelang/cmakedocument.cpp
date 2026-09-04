// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakedocument.h"

#include "cmakeparser.h"

#include <QStringList>

using namespace CMakeLang;

DocumentPtr Document::fromSource(const QString &source)
{
    std::shared_ptr<Document> document(new Document);
    Parser parser(&document->_engine, source);
    document->_ast = parser.parse();
    document->collect(document->_ast->elements(), nullptr);
    return document;
}

QString Document::errorString() const
{
    QStringList messages;
    for (const Diagnostic &diagnostic : diagnostics()) {
        if (diagnostic.isError())
            messages.append(diagnostic.message);
    }
    return messages.join('\n');
}

QList<AST *> Document::enclosingConstructs(const AST *node) const
{
    QList<AST *> constructs;
    for (AST *it = _enclosing.value(node); it; it = _enclosing.value(it))
        constructs.prepend(it);
    return constructs;
}

bool Document::runsWith(const AST *node, const AST *other) const
{
    const QList<AST *> constructs = enclosingConstructs(node);
    const QList<AST *> otherConstructs = enclosingConstructs(other);
    if (constructs.size() > otherConstructs.size())
        return false;
    return otherConstructs.first(constructs.size()) == constructs;
}

void Document::collect(ListView<ElementAST *> elements, AST *construct)
{
    for (ElementAST *element : elements) {
        if (CommandAST *command = element->asCommand()) {
            addCommand(command, construct);
        } else if (IfAST *ifAst = element->asIf()) {
            // Every branch is a scope of its own, the if node standing for the
            // one its condition guards. A condition itself is evaluated where
            // the construct sits, not inside it.
            addConstruct(ifAst, construct);
            addCommand(ifAst->ifCommand, construct);
            collect(ifAst->elements(), ifAst);
            for (ElseIfClauseAST *clause : ifAst->elseIfClauses()) {
                addConstruct(clause, construct);
                addCommand(clause->command, construct);
                collect(clause->elements(), clause);
            }
            if (ElseClauseAST *clause = ifAst->elseClause) {
                addConstruct(clause, construct);
                addCommand(clause->command, construct);
                collect(clause->elements(), clause);
            }
            addCommand(ifAst->endIfCommand, construct);
        } else if (NestedCommandAST *nested = element->asNestedCommand()) {
            addConstruct(nested, construct);
            addCommand(nested->openCommand, construct);
            collect(nested->elements(), nested);
            addCommand(nested->closeCommand, construct);
        }
    }
}

void Document::addCommand(CommandAST *command, AST *construct)
{
    if (!command)
        return;
    _commands.append(command);
    addConstruct(command, construct);
}

void Document::addConstruct(AST *construct, AST *parent)
{
    if (parent)
        _enclosing.insert(construct, parent);
}
