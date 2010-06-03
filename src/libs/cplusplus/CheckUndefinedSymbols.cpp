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

#include "CheckUndefinedSymbols.h"
#include "Overview.h"

#include <Names.h>
#include <Literals.h>
#include <Symbols.h>
#include <TranslationUnit.h>
#include <Scope.h>
#include <AST.h>
#include <SymbolVisitor.h>

#include <QCoreApplication>
#include <QDebug>

using namespace CPlusPlus;

namespace {

class CollectTypes: protected SymbolVisitor
{
    Document::Ptr _doc;
    Snapshot _snapshot;
    QSet<QByteArray> _types;
    QList<ScopedSymbol *> _scopes;
    QList<NameAST *> _names;
    bool _mainDocument;

public:
    CollectTypes(Document::Ptr doc, const Snapshot &snapshot)
        : _doc(doc), _snapshot(snapshot), _mainDocument(false)
    {
        QSet<Namespace *> processed;
        process(doc, &processed);
    }

    const QSet<QByteArray> &types() const
    {
        return _types;
    }

    const QList<ScopedSymbol *> &scopes() const
    {
        return _scopes;
    }

    static Scope *findScope(unsigned tokenOffset, const QList<ScopedSymbol *> &scopes)
    {
        for (int i = scopes.size() - 1; i != -1; --i) {
            ScopedSymbol *symbol = scopes.at(i);
            const unsigned start = symbol->startOffset();
            const unsigned end = symbol->endOffset();

            if (tokenOffset >= start && tokenOffset < end)
                return symbol->members();
        }

        return 0;
    }

protected:
    void process(Document::Ptr doc, QSet<Namespace *> *processed)
    {
        if (! doc)
            return;
        else if (! processed->contains(doc->globalNamespace())) {
            processed->insert(doc->globalNamespace());

            foreach (const Document::Include &i, doc->includes())
                process(_snapshot.document(i.fileName()), processed);

            _mainDocument = (doc == _doc); // ### improve
            accept(doc->globalNamespace());
        }
    }

    void addType(const Identifier *id)
    {
        if (id)
            _types.insert(QByteArray::fromRawData(id->chars(), id->size()));
    }

    void addType(const Name *name)
    {
        if (! name) {
            return;

        } else if (const QualifiedNameId *q = name->asQualifiedNameId()) {
            for (unsigned i = 0; i < q->nameCount(); ++i)
                addType(q->nameAt(i));

        } else if (name->isNameId() || name->isTemplateNameId()) {
            addType(name->identifier());

        }
    }

    void addScope(ScopedSymbol *symbol)
    {
        if (_mainDocument)
            _scopes.append(symbol);
    }

    // nothing to do
    virtual bool visit(UsingNamespaceDirective *) { return true; }
    virtual bool visit(UsingDeclaration *) { return true; }
    virtual bool visit(Argument *) { return true; }
    virtual bool visit(BaseClass *) { return true; }

    virtual bool visit(Function *symbol)
    {
        for (TemplateParameters *p = symbol->templateParameters(); p; p = p->previous()) {
            Scope *scope = p->scope();
            for (unsigned i = 0; i < scope->symbolCount(); ++i)
                accept(scope->symbolAt(i));
        }

        addScope(symbol);
        return true;
    }

    virtual bool visit(Block *symbol)
    {
        addScope(symbol);
        return true;
    }

    virtual bool visit(NamespaceAlias *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(Declaration *symbol)
    {
        if (symbol->isTypedef())
            addType(symbol->name());

        return true;
    }

    virtual bool visit(TypenameArgument *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(Enum *symbol)
    {
        addScope(symbol);
        addType(symbol->name());
        return true;
    }

    virtual bool visit(Namespace *symbol)
    {
        addScope(symbol);
        addType(symbol->name());
        return true;
    }

    virtual bool visit(Class *symbol)
    {
        for (TemplateParameters *p = symbol->templateParameters(); p; p = p->previous()) {
            Scope *scope = p->scope();
            for (unsigned i = 0; i < scope->symbolCount(); ++i)
                accept(scope->symbolAt(i));
        }

        addScope(symbol);
        addType(symbol->name());
        return true;
    }

    virtual bool visit(ForwardClassDeclaration *symbol)
    {
        for (TemplateParameters *p = symbol->templateParameters(); p; p = p->previous()) {
            Scope *scope = p->scope();
            for (unsigned i = 0; i < scope->symbolCount(); ++i)
                accept(scope->symbolAt(i));
        }

        addType(symbol->name());
        return true;
    }

    // Objective-C
    virtual bool visit(ObjCBaseClass *) { return true; }
    virtual bool visit(ObjCBaseProtocol *) { return true; }
    virtual bool visit(ObjCPropertyDeclaration *) { return true; }

    virtual bool visit(ObjCMethod *symbol)
    {
        addScope(symbol);
        return true;
    }

    virtual bool visit(ObjCClass *symbol)
    {
        addScope(symbol);
        addType(symbol->name());
        return true;
    }

    virtual bool visit(ObjCForwardClassDeclaration *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(ObjCProtocol *symbol)
    {
        addScope(symbol);
        addType(symbol->name());
        return true;
    }

    virtual bool visit(ObjCForwardProtocolDeclaration *symbol)
    {
        addType(symbol->name());
        return true;
    }
};

} // end of anonymous namespace

CheckUndefinedSymbols::CheckUndefinedSymbols(TranslationUnit *unit, const LookupContext &context)
    : ASTVisitor(unit), _context(context)
{
    _fileName = context.thisDocument()->fileName();
    CollectTypes collectTypes(context.thisDocument(), context.snapshot());
    _potentialTypes = collectTypes.types();
    _scopes = collectTypes.scopes();
}

CheckUndefinedSymbols::~CheckUndefinedSymbols()
{ }

QList<Document::DiagnosticMessage> CheckUndefinedSymbols::operator()(AST *ast)
{
    _diagnosticMessages.clear();
    accept(ast);
    return _diagnosticMessages;
}

bool CheckUndefinedSymbols::warning(unsigned line, unsigned column, const QString &text, unsigned length)
{
    Document::DiagnosticMessage m(Document::DiagnosticMessage::Warning, _fileName, line, column, text, length);
    _diagnosticMessages.append(m);
    return false;
}

bool CheckUndefinedSymbols::warning(AST *ast, const QString &text)
{
    const Token &firstToken = tokenAt(ast->firstToken());
    const Token &lastToken = tokenAt(ast->lastToken() - 1);

    const unsigned length = lastToken.end() - firstToken.begin();
    unsigned line = 1, column = 1;
    getTokenStartPosition(ast->firstToken(), &line, &column);

    warning(line, column, text, length);
    return false;
}

bool CheckUndefinedSymbols::visit(NamespaceAST *ast)
{
    if (ast->identifier_token) {
        const Token &tok = tokenAt(ast->identifier_token);
        if (! tok.generated()) {
            unsigned line, column;
            getTokenStartPosition(ast->identifier_token, &line, &column);
            Use use(line, column, tok.length());
            _typeUsages.append(use);
        }
    }

    return true;
}

bool CheckUndefinedSymbols::visit(UsingDirectiveAST *)
{
    return true;
}

bool CheckUndefinedSymbols::visit(SimpleDeclarationAST *)
{
    return true;
}

bool CheckUndefinedSymbols::visit(NamedTypeSpecifierAST *)
{
    return true;
}

void CheckUndefinedSymbols::checkNamespace(NameAST *name)
{
    if (! name)
        return;

    unsigned line, column;
    getTokenStartPosition(name->firstToken(), &line, &column);

    Scope *enclosingScope = _context.thisDocument()->scopeAt(line, column);
    if (ClassOrNamespace *b = _context.lookupType(name->name, enclosingScope)) {
        foreach (Symbol *s, b->symbols()) {
            if (s->isNamespace())
                return;
        }
    }

    const unsigned length = tokenAt(name->lastToken() - 1).end() - tokenAt(name->firstToken()).begin();
    warning(line, column, QCoreApplication::translate("CheckUndefinedSymbols", "Expected a namespace-name"), length);
}

void CheckUndefinedSymbols::checkName(NameAST *ast)
{
    if (ast->name) {
        if (const Identifier *ident = ast->name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialTypes.contains(id)) {
                Scope *scope = findScope(ast);
                const QList<Symbol *> candidates = _context.lookup(ast->name, scope);
                addTypeUsage(candidates, ast);
            }
        }
    }
}

bool CheckUndefinedSymbols::visit(SimpleNameAST *ast)
{
    checkName(ast);
    return true;
}

bool CheckUndefinedSymbols::visit(TemplateIdAST *ast)
{
    checkName(ast);
    return true;
}

bool CheckUndefinedSymbols::visit(DestructorNameAST *ast)
{
    checkName(ast);
    return true;
}

bool CheckUndefinedSymbols::visit(QualifiedNameAST *ast)
{
    if (ast->name) {
        Scope *scope = findScope(ast);

        ClassOrNamespace *b = 0;
        if (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list) {
            NestedNameSpecifierAST *nested_name_specifier = it->value;
            NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name; // ### remove shadowing

            if (class_or_namespace_name)
                if (TemplateIdAST *template_id = class_or_namespace_name->asTemplateId())
                    accept(template_id->template_argument_list);

            const Name *name = class_or_namespace_name->name;
            b = _context.lookupType(name, scope);
            addTypeUsage(b, class_or_namespace_name);

            for (it = it->next; b && it; it = it->next) {
                NestedNameSpecifierAST *nested_name_specifier = it->value;

                if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) {
                    if (TemplateIdAST *template_id = class_or_namespace_name->asTemplateId())
                        accept(template_id->template_argument_list);

                    b = b->findType(class_or_namespace_name->name);
                    addTypeUsage(b, class_or_namespace_name);
                }
            }
        }

        if (b && ast->unqualified_name)
            addTypeUsage(b->find(ast->unqualified_name->name), ast->unqualified_name);
    }

    return false;
}

bool CheckUndefinedSymbols::visit(TypenameTypeParameterAST *ast)
{
    if (ast->name && ast->name->name) {
        if (const Identifier *templId = ast->name->name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(templId->chars(), templId->size());
            if (_potentialTypes.contains(id)) {
                Scope *scope = findScope(_templateDeclarationStack.back());
                const QList<Symbol *> candidates = _context.lookup(ast->name->name, scope);
                addTypeUsage(candidates, ast->name);
            }
        }
    }
    return true;
}

bool CheckUndefinedSymbols::visit(TemplateTypeParameterAST *ast)
{
    checkName(ast->name);
    return true;
}

bool CheckUndefinedSymbols::visit(TemplateDeclarationAST *ast)
{
    _templateDeclarationStack.append(ast);
    return true;
}

void CheckUndefinedSymbols::endVisit(TemplateDeclarationAST *)
{
    _templateDeclarationStack.takeFirst();
}

void CheckUndefinedSymbols::addTypeUsage(ClassOrNamespace *b, NameAST *ast)
{
    if (! b)
        return;

    unsigned startToken = ast->firstToken();
    if (DestructorNameAST *dtor = ast->asDestructorName())
        startToken = dtor->identifier_token;

    const Token &tok = tokenAt(startToken);
    if (tok.generated())
        return;

    unsigned line, column;
    getTokenStartPosition(startToken, &line, &column);
    const unsigned length = tok.length();
    Use use(line, column, length);
    _typeUsages.append(use);
    //qDebug() << "added use" << oo(ast->name) << line << column << length;
}

void CheckUndefinedSymbols::addTypeUsage(const QList<Symbol *> &candidates, NameAST *ast)
{
    unsigned startToken = ast->firstToken();
    if (DestructorNameAST *dtor = ast->asDestructorName())
        startToken = dtor->identifier_token;

    const Token &tok = tokenAt(startToken);
    if (tok.generated())
        return;

    unsigned line, column;
    getTokenStartPosition(startToken, &line, &column);
    const unsigned length = tok.length();

    foreach (Symbol *c, candidates) {
        if (c->isUsingDeclaration()) // skip using declarations...
            continue;
        else if (c->isUsingNamespaceDirective()) // ... and using namespace directives.
            continue;
        else if (c->isTypedef() || c->isNamespace() ||
                 c->isClass() || c->isEnum() ||
                 c->isForwardClassDeclaration() || c->isTypenameArgument()) {
            Use use(line, column, length);
            _typeUsages.append(use);
            //qDebug() << "added use" << oo(ast->name) << line << column << length;
            break;
        }
    }
}

QList<CheckUndefinedSymbols::Use> CheckUndefinedSymbols::typeUsages() const
{
    return _typeUsages;
}

unsigned CheckUndefinedSymbols::startOfTemplateDeclaration(TemplateDeclarationAST *ast) const
{
    if (ast->declaration) {
        if (TemplateDeclarationAST *templ = ast->declaration->asTemplateDeclaration())
            return startOfTemplateDeclaration(templ);

        return ast->declaration->firstToken();
    }

    return ast->firstToken();
}

Scope *CheckUndefinedSymbols::findScope(AST *ast) const
{
    Scope *scope = 0;

    if (ast) {
        unsigned startToken = ast->firstToken();
        if (TemplateDeclarationAST *templ = ast->asTemplateDeclaration())
            startToken = startOfTemplateDeclaration(templ);

        const unsigned tokenOffset = tokenAt(startToken).offset;
        scope = CollectTypes::findScope(tokenOffset, _scopes);
    }

    if (! scope)
        scope = _context.thisDocument()->globalSymbols();

    return scope;
}
