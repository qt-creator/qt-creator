/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "Overview.h"

#include <Control.h>
#include <Literals.h>
#include <Names.h>
#include <Scope.h>
#include <Symbols.h>
#include <AST.h>
#include <TranslationUnit.h>

#include <QtCore/QDir>
#include <QtCore/QDebug>

using namespace CPlusPlus;

FindUsages::FindUsages(Document::Ptr doc, const Snapshot &snapshot)
    : ASTVisitor(doc->translationUnit()),
      _doc(doc),
      _snapshot(snapshot),
      _context(doc, snapshot),
      _source(_doc->source()),
      _sem(doc->translationUnit()),
      _inSimpleDeclaration(0),
      _inQProperty(false)
{
    _snapshot.insert(_doc);
    typeofExpression.init(_doc, _snapshot, _context.bindings());
}

FindUsages::FindUsages(const LookupContext &context)
    : ASTVisitor(context.thisDocument()->translationUnit()),
      _doc(context.thisDocument()),
      _snapshot(context.snapshot()),
      _context(context),
      _source(_doc->source()),
      _sem(_doc->translationUnit()),
      _inSimpleDeclaration(0),
      _inQProperty(false)
{
    typeofExpression.init(_doc, _snapshot, _context.bindings());
}

QList<Usage> FindUsages::usages() const
{ return _usages; }

QList<int> FindUsages::references() const
{ return _references; }

void FindUsages::operator()(Symbol *symbol)
{
    if (! symbol)
        return;

    _id = symbol->identifier();

    if (! _id)
        return;

    _processed.clear();
    _references.clear();
    _usages.clear();
    _declSymbolFullyQualifiedName = LookupContext::fullyQualifiedName(symbol);
    _inSimpleDeclaration = 0;
    _inQProperty = false;

    // get the canonical id
    _id = _doc->control()->findOrInsertIdentifier(_id->chars(), _id->size());

    accept(_doc->translationUnit()->ast());
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

Scope *FindUsages::scopeAt(unsigned tokenIndex) const
{
    TranslationUnit *unit = _doc->translationUnit();
    unsigned line, column;
    unit->getTokenPosition(tokenIndex, &line, &column);
    return _doc->scopeAt(line, column);
}

void FindUsages::reportResult(unsigned tokenIndex, const QList<LookupItem> &candidates)
{
    if (_processed.contains(tokenIndex))
        return;

    const bool isStrongResult = checkCandidates(candidates);

    if (isStrongResult)
        reportResult(tokenIndex);
}

void FindUsages::reportResult(unsigned tokenIndex)
{
    const Token &tk = tokenAt(tokenIndex);
    if (tk.generated())
        return;
    else if (_processed.contains(tokenIndex))
        return;

    _processed.insert(tokenIndex);

    const QString lineText = matchingLine(tk);

    unsigned line, col;
    getTokenStartPosition(tokenIndex, &line, &col);

    if (col)
        --col;  // adjust the column position.

    const int len = tk.f.length;

    const Usage u(_doc->fileName(), lineText, line, col, len);
    _usages.append(u);
    _references.append(tokenIndex);
}

bool FindUsages::compareFullyQualifiedName(const QList<const Name *> &path, const QList<const Name *> &other)
{
    if (path.length() != other.length())
        return false;

    for (int i = 0; i < path.length(); ++i) {
        if (! path.at(i)->isEqualTo(other.at(i)))
            return false;
    }

    return true;
}

bool FindUsages::checkCandidates(const QList<LookupItem> &candidates) const
{
    for (int i = candidates.size() - 1; i != -1; --i) {
        const LookupItem &r = candidates.at(i);

        if (Symbol *s = r.declaration()) {
            if (compareFullyQualifiedName(LookupContext::fullyQualifiedName(s), _declSymbolFullyQualifiedName))
                return true;
        }
    }

    return false;
}

void FindUsages::ensureNameIsValid(NameAST *ast)
{
    if (ast && ! ast->name)
        ast->name = _sem.check(ast, /*scope = */ 0);
}

bool FindUsages::visit(NamespaceAST *ast)
{
    const Identifier *id = identifier(ast->identifier_token);
    if (id == _id && ast->symbol) {
        const QList<LookupItem> candidates = _context.lookup(ast->symbol->name(), scopeAt(ast->identifier_token));
        reportResult(ast->identifier_token, candidates);
    }
    return true;
}

bool FindUsages::visit(MemInitializerAST *ast)
{
    if (ast->name && ast->name->asSimpleName() != 0) {
        ensureNameIsValid(ast->name);

        SimpleNameAST *simple = ast->name->asSimpleName();
        if (identifier(simple->identifier_token) == _id) {
            const QList<LookupItem> candidates = _context.lookup(simple->name, scopeAt(simple->identifier_token));
            reportResult(simple->identifier_token, candidates);
        }
    }
    accept(ast->expression_list);
    return false;
}

bool FindUsages::visit(MemberAccessAST *ast)
{
    if (ast->member_name) {
        if (SimpleNameAST *simple = ast->member_name->asSimpleName()) {
            if (identifier(simple->identifier_token) == _id) {
                checkExpression(ast->firstToken(), simple->identifier_token);
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

    unsigned line, column;
    getTokenStartPosition(startToken, &line, &column);
    Scope *scope = _doc->scopeAt(line, column);

    const QList<LookupItem> results = typeofExpression(expression, scope,
                                                       TypeOfExpression::Preprocess);

    reportResult(endToken, results);
}

bool FindUsages::visit(QualifiedNameAST *ast)
{
    for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next) {
        NestedNameSpecifierAST *nested_name_specifier = it->value;

        if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) {
            SimpleNameAST *simple_name = class_or_namespace_name->asSimpleName();

            TemplateIdAST *template_id = 0;
            if (! simple_name) {
                template_id = class_or_namespace_name->asTemplateId();

                if (template_id) {
                    for (TemplateArgumentListAST *arg_it = template_id->template_argument_list; arg_it; arg_it = arg_it->next) {
                        accept(arg_it->value);
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

                for (TemplateArgumentListAST *template_arguments = template_id->template_argument_list;
                     template_arguments; template_arguments = template_arguments->next) {
                    accept(template_arguments->value);
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
    const Identifier *id = identifier(ast->identifier_token);
    if (id == _id) {
        const QList<LookupItem> candidates = _context.lookup(control()->nameId(id), scopeAt(ast->identifier_token));
        reportResult(ast->identifier_token, candidates);
    }

    accept(ast->expression);

    return false;
}

bool FindUsages::visit(SimpleNameAST *ast)
{
    const Identifier *id = identifier(ast->identifier_token);
    if (id == _id) {
        const QList<LookupItem> candidates = _context.lookup(ast->name, scopeAt(ast->identifier_token));
        reportResult(ast->identifier_token, candidates);
    }

    return false;
}

bool FindUsages::visit(DestructorNameAST *ast)
{
    const Identifier *id = identifier(ast->identifier_token);
    if (id == _id) {
        const QList<LookupItem> candidates = _context.lookup(ast->name, scopeAt(ast->identifier_token));
        reportResult(ast->identifier_token, candidates);
    }

    return false;
}

bool FindUsages::visit(TemplateIdAST *ast)
{
    if (_id == identifier(ast->identifier_token)) {
        const QList<LookupItem> candidates = _context.lookup(ast->name, scopeAt(ast->identifier_token));
        reportResult(ast->identifier_token, candidates);
    }

    for (TemplateArgumentListAST *template_arguments = ast->template_argument_list;
         template_arguments; template_arguments = template_arguments->next) {
        accept(template_arguments->value);
    }

    return false;
}

bool FindUsages::visit(ParameterDeclarationAST *ast)
{
    for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next)
        accept(it->value);

    if (DeclaratorAST *declarator = ast->declarator) {
        for (SpecifierListAST *it = declarator->attribute_list; it; it = it->next)
            accept(it->value);

        for (PtrOperatorListAST *it = declarator->ptr_operator_list; it; it = it->next)
            accept(it->value);

        if (! _inSimpleDeclaration) // visit the core declarator only if we are not in simple-declaration.
            accept(declarator->core_declarator);

        for (PostfixDeclaratorListAST *it = declarator->postfix_declarator_list; it; it = it->next)
            accept(it->value);

        for (SpecifierListAST *it = declarator->post_attribute_list; it; it = it->next)
            accept(it->value);

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

    for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next)
        accept(it->value);

    accept(ast->exception_specification);

    return false;
}

bool FindUsages::visit(SimpleDeclarationAST *ast)
{
    for  (SpecifierListAST *it = ast->decl_specifier_list; it; it = it->next)
        accept(it->value);

    ++_inSimpleDeclaration;
    for (DeclaratorListAST *it = ast->declarator_list; it; it = it->next)
        accept(it->value);
    --_inSimpleDeclaration;
    return false;
}

bool FindUsages::visit(ObjCSelectorAST *ast)
{
    if (ast->name) {
        const Identifier *id = ast->name->identifier();
        if (id == _id) {
            const QList<LookupItem> candidates = _context.lookup(ast->name, scopeAt(ast->firstToken()));
            reportResult(ast->firstToken(), candidates);
        }
    }

    return false;
}

bool FindUsages::visit(QtPropertyDeclarationAST *)
{
    _inQProperty = true;
    return true;
}

void FindUsages::endVisit(QtPropertyDeclarationAST *)
{ _inQProperty = false; }

bool FindUsages::visit(TemplateDeclarationAST *ast)
{
    _templateDeclarationStack.append(ast);
    return true;
}

void FindUsages::endVisit(TemplateDeclarationAST *)
{
    _templateDeclarationStack.takeFirst();
}

bool FindUsages::visit(TypenameTypeParameterAST *ast)
{
    if (NameAST *name = ast->name) {
        const Identifier *id = name->name->identifier();
        if (id == _id) {
            unsigned start = startOfTemplateDeclaration(_templateDeclarationStack.back());
            const QList<LookupItem> candidates = _context.lookup(name->name, scopeAt(start));
            reportResult(ast->name->firstToken(), candidates);
        }
    }
    accept(ast->type_id);
    return false;
}

bool FindUsages::visit(TemplateTypeParameterAST *ast)
{
    if (NameAST *name = ast->name) {
        const Identifier *id = name->name->identifier();
        if (id == _id) {
            unsigned start = startOfTemplateDeclaration(_templateDeclarationStack.back());
            const QList<LookupItem> candidates = _context.lookup(name->name, scopeAt(start));
            reportResult(ast->name->firstToken(), candidates);
        }
    }
    accept(ast->type_id);
    return false;
}

unsigned FindUsages::startOfTemplateDeclaration(TemplateDeclarationAST *ast) const
{
    if (ast->declaration) {
        if (TemplateDeclarationAST *templ = ast->declaration->asTemplateDeclaration())
            return startOfTemplateDeclaration(templ);

        return ast->declaration->firstToken();
    }

    return ast->firstToken();
}
