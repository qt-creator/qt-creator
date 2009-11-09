/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "FindUsages.h"
#include "TypeOfExpression.h"

#include <Control.h>
#include <Literals.h>
#include <Names.h>
#include <Symbols.h>
#include <AST.h>

#include <QtCore/QDir>

using namespace CPlusPlus;

FindUsages::FindUsages(Document::Ptr doc, const Snapshot &snapshot, QFutureInterface<Usage> *future)
    : ASTVisitor(doc->control()),
      _future(future),
      _doc(doc),
      _snapshot(snapshot),
      _source(_doc->source()),
      _sem(doc->control()),
      _inSimpleDeclaration(0)
{
    _snapshot.insert(_doc);
}

void FindUsages::setGlobalNamespaceBinding(NamespaceBindingPtr globalNamespaceBinding)
{
    _globalNamespaceBinding = globalNamespaceBinding;
}

QList<int> FindUsages::operator()(Symbol *symbol, Identifier *id, AST *ast)
{
    _processed.clear();
    _references.clear();
    _declSymbol = symbol;
    _id = id;
    if (_declSymbol && _id) {
        _exprDoc = Document::create("<references>");
        accept(ast);
    }
    return _references;
}

QString FindUsages::matchingLine(const Token &tk) const
{
    const char *beg = _source.constData();
    const char *cp = beg + tk.offset;
    for (; cp != beg - 1; --cp) {
        if (*cp == '\n')
            break;
    }

    ++cp;

    const char *lineEnd = cp + 1;
    for (; *lineEnd; ++lineEnd) {
        if (*lineEnd == '\n')
            break;
    }

    const QString matchingLine = QString::fromUtf8(cp, lineEnd - cp);
    return matchingLine;
}

void FindUsages::reportResult(unsigned tokenIndex, const QList<Symbol *> &candidates)
{
    if (_processed.contains(tokenIndex))
        return;

    const bool isStrongResult = checkCandidates(candidates);

    if (isStrongResult)
        reportResult(tokenIndex);
}

void FindUsages::reportResult(unsigned tokenIndex)
{
    if (_processed.contains(tokenIndex))
        return;

    _processed.insert(tokenIndex);

    const Token &tk = tokenAt(tokenIndex);
    const QString lineText = matchingLine(tk);

    unsigned line, col;
    getTokenStartPosition(tokenIndex, &line, &col);

    if (col)
        --col;  // adjust the column position.

    const int len = tk.f.length;

    if (_future) {
        _future->reportResult(Usage(_doc->fileName(), line, lineText, col, len));
    }

    _references.append(tokenIndex);
}

bool FindUsages::checkCandidates(const QList<Symbol *> &candidates) const
{
    if (Symbol *canonicalSymbol = LookupContext::canonicalSymbol(candidates, _globalNamespaceBinding.data())) {

#if 0
        Symbol *c = candidates.first();
        qDebug() << "*** canonical symbol:" << canonicalSymbol->fileName()
                << canonicalSymbol->line() << canonicalSymbol->column()
                << "candidates:" << candidates.size()
                << c->fileName() << c->line() << c->column();
#endif

        return checkSymbol(canonicalSymbol);
    }

    return false;
}

bool FindUsages::checkScope(Symbol *symbol, Symbol *otherSymbol) const
{
    if (! (symbol && otherSymbol))
        return false;

    else if (symbol->scope() == otherSymbol->scope())
        return true;

    else if (symbol->name() && otherSymbol->name()) {

        if (! symbol->name()->isEqualTo(otherSymbol->name()))
            return false;

    } else if (symbol->name() != otherSymbol->name()) {
        return false;
    }

    return checkScope(symbol->enclosingSymbol(), otherSymbol->enclosingSymbol());
}

bool FindUsages::checkSymbol(Symbol *symbol) const
{
    if (! symbol) {
        return false;

    } else if (symbol == _declSymbol) {
        return true;

    } else if (symbol->line() == _declSymbol->line() && symbol->column() == _declSymbol->column()) {
        if (! qstrcmp(symbol->fileName(), _declSymbol->fileName()))
            return true;

    } else if (symbol->isForwardClassDeclaration() && (_declSymbol->isClass() ||
                                                       _declSymbol->isForwardClassDeclaration())) {
        return checkScope(symbol, _declSymbol);

    } else if (_declSymbol->isForwardClassDeclaration() && (symbol->isClass() ||
                                                            symbol->isForwardClassDeclaration())) {
        return checkScope(symbol, _declSymbol);
    }

    return false;
}

LookupContext FindUsages::currentContext(AST *ast)
{
    unsigned line, column;
    getTokenStartPosition(ast->firstToken(), &line, &column);
    Symbol *lastVisibleSymbol = _doc->findSymbolAt(line, column);

    if (lastVisibleSymbol && lastVisibleSymbol == _previousContext.symbol())
        return _previousContext;

    LookupContext ctx(lastVisibleSymbol, _exprDoc, _doc, _snapshot);
    _previousContext = ctx;
    return ctx;
}

void FindUsages::ensureNameIsValid(NameAST *ast)
{
    if (ast && ! ast->name)
        ast->name = _sem.check(ast, /*scope = */ 0);
}

bool FindUsages::visit(MemInitializerAST *ast)
{
    if (ast->name && ast->name->asSimpleName() != 0) {
        ensureNameIsValid(ast->name);

        SimpleNameAST *simple = ast->name->asSimpleName();
        if (identifier(simple->identifier_token) == _id) {
            LookupContext context = currentContext(ast);
            const QList<Symbol *> candidates = context.resolve(simple->name);
            reportResult(simple->identifier_token, candidates);
        }
    }
    accept(ast->expression);
    return false;
}

bool FindUsages::visit(PostfixExpressionAST *ast)
{
    _postfixExpressionStack.append(ast);
    return true;
}

void FindUsages::endVisit(PostfixExpressionAST *)
{
    _postfixExpressionStack.removeLast();
}

bool FindUsages::visit(MemberAccessAST *ast)
{
    if (ast->member_name) {
        if (SimpleNameAST *simple = ast->member_name->asSimpleName()) {
            if (identifier(simple->identifier_token) == _id) {
                Q_ASSERT(! _postfixExpressionStack.isEmpty());

                checkExpression(_postfixExpressionStack.last()->firstToken(),
                                simple->identifier_token);

                return false;
            }
        }
    }

    return true;
}

void FindUsages::checkExpression(unsigned startToken, unsigned endToken)
{
    const unsigned begin = tokenAt(startToken).begin();
    const unsigned end = tokenAt(endToken).end();

    const QString expression = _source.mid(begin, end - begin);
    // qDebug() << "*** check expression:" << expression;

    TypeOfExpression typeofExpression;
    typeofExpression.setSnapshot(_snapshot);

    unsigned line, column;
    getTokenStartPosition(startToken, &line, &column);
    Symbol *lastVisibleSymbol = _doc->findSymbolAt(line, column);

    const QList<TypeOfExpression::Result> results =
            typeofExpression(expression, _doc, lastVisibleSymbol,
                             TypeOfExpression::Preprocess);

    QList<Symbol *> candidates;

    foreach (TypeOfExpression::Result r, results) {
        FullySpecifiedType ty = r.first;
        Symbol *lastVisibleSymbol = r.second;

        candidates.append(lastVisibleSymbol);
    }

    reportResult(endToken, candidates);
}

bool FindUsages::visit(QualifiedNameAST *ast)
{
    for (NestedNameSpecifierAST *nested_name_specifier = ast->nested_name_specifier;
         nested_name_specifier; nested_name_specifier = nested_name_specifier->next) {

        if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) {
            SimpleNameAST *simple_name = class_or_namespace_name->asSimpleName();

            TemplateIdAST *template_id = 0;
            if (! simple_name) {
                template_id = class_or_namespace_name->asTemplateId();

                if (template_id) {
                    for (TemplateArgumentListAST *template_arguments = template_id->template_arguments;
                         template_arguments; template_arguments = template_arguments->next) {
                        accept(template_arguments->template_argument);
                    }
                }
            }

            if (simple_name || template_id) {
                const unsigned identifier_token = simple_name
                           ? simple_name->identifier_token
                           : template_id->identifier_token;

                if (identifier(identifier_token) == _id)
                    checkExpression(ast->firstToken(), identifier_token);
            }
        }
    }

    if (NameAST *unqualified_name = ast->unqualified_name) {
        unsigned identifier_token = 0;

        if (SimpleNameAST *simple_name = unqualified_name->asSimpleName())
            identifier_token = simple_name->identifier_token;

        else if (DestructorNameAST *dtor_name = unqualified_name->asDestructorName())
            identifier_token = dtor_name->identifier_token;

        TemplateIdAST *template_id = 0;
        if (! identifier_token) {
            template_id = unqualified_name->asTemplateId();

            if (template_id) {
                identifier_token = template_id->identifier_token;

                for (TemplateArgumentListAST *template_arguments = template_id->template_arguments;
                     template_arguments; template_arguments = template_arguments->next) {
                    accept(template_arguments->template_argument);
                }
            }
        }

        if (identifier_token && identifier(identifier_token) == _id)
            checkExpression(ast->firstToken(), identifier_token);
    }

    return false;
}

bool FindUsages::visit(EnumeratorAST *ast)
{
    Identifier *id = identifier(ast->identifier_token);
    if (id == _id) {
        LookupContext context = currentContext(ast);
        const QList<Symbol *> candidates = context.resolve(control()->nameId(id));
        reportResult(ast->identifier_token, candidates);
    }

    accept(ast->expression);

    return false;
}

bool FindUsages::visit(SimpleNameAST *ast)
{
    Identifier *id = identifier(ast->identifier_token);
    if (id == _id) {
        LookupContext context = currentContext(ast);
        const QList<Symbol *> candidates = context.resolve(ast->name);
        reportResult(ast->identifier_token, candidates);
    }

    return false;
}

bool FindUsages::visit(DestructorNameAST *ast)
{
    Identifier *id = identifier(ast->identifier_token);
    if (id == _id) {
        LookupContext context = currentContext(ast);
        const QList<Symbol *> candidates = context.resolve(ast->name);
        reportResult(ast->identifier_token, candidates);
    }

    return false;
}

bool FindUsages::visit(TemplateIdAST *ast)
{
    if (_id == identifier(ast->identifier_token)) {
        LookupContext context = currentContext(ast);
        const QList<Symbol *> candidates = context.resolve(ast->name);
        reportResult(ast->identifier_token, candidates);
    }

    for (TemplateArgumentListAST *template_arguments = ast->template_arguments;
         template_arguments; template_arguments = template_arguments->next) {
        accept(template_arguments->template_argument);
    }

    return false;
}

bool FindUsages::visit(ParameterDeclarationAST *ast)
{
    for (SpecifierAST *spec = ast->type_specifier; spec; spec = spec->next)
        accept(spec);

    if (DeclaratorAST *declarator = ast->declarator) {
        for (SpecifierAST *attr = declarator->attributes; attr; attr = attr->next)
            accept(attr);

        for (PtrOperatorAST *ptr_op = declarator->ptr_operators; ptr_op; ptr_op = ptr_op->next)
            accept(ptr_op);

        if (! _inSimpleDeclaration) // visit the core declarator only if we are not in simple-declaration.
            accept(declarator->core_declarator);

        for (PostfixDeclaratorAST *fx_op = declarator->postfix_declarators; fx_op; fx_op = fx_op->next)
            accept(fx_op);

        for (SpecifierAST *spec = declarator->post_attributes; spec; spec = spec->next)
            accept(spec);

        accept(declarator->initializer);
    }

    accept(ast->expression);
    return false;
}

bool FindUsages::visit(ExpressionOrDeclarationStatementAST *ast)
{
    accept(ast->declaration);
    return false;
}

bool FindUsages::visit(FunctionDeclaratorAST *ast)
{
    accept(ast->parameters);

    for (SpecifierAST *spec = ast->cv_qualifier_seq; spec; spec = spec->next)
        accept(spec);

    accept(ast->exception_specification);

    return false;
}

bool FindUsages::visit(SimpleDeclarationAST *)
{
    ++_inSimpleDeclaration;
    return true;
}

void FindUsages::endVisit(SimpleDeclarationAST *)
{ --_inSimpleDeclaration; }
