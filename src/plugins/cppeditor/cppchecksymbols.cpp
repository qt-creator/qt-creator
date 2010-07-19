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

#include "cppchecksymbols.h"
#include "cpplocalsymbols.h"

#include <cplusplus/Overview.h>

#include <Names.h>
#include <Literals.h>
#include <Symbols.h>
#include <TranslationUnit.h>
#include <Scope.h>
#include <AST.h>
#include <SymbolVisitor.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QThreadPool>
#include <QtCore/QDebug>

#include <qtconcurrent/runextensions.h>

using namespace CPlusPlus;
using namespace CppEditor::Internal;

namespace {

class FriendlyThread: public QThread
{
public:
    using QThread::msleep;
};

class CollectTypes: protected SymbolVisitor
{
    Document::Ptr _doc;
    Snapshot _snapshot;
    QSet<QByteArray> _types;
    QSet<QByteArray> _members;
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

    const QSet<QByteArray> &members() const
    {
        return _members;
    }

    const QList<ScopedSymbol *> &scopes() const
    {
        return _scopes;
    }

    static Scope *findScope(unsigned tokenOffset, const QList<ScopedSymbol *> &scopes)
    {
        for (int i = scopes.size() - 1; i != -1; --i) {
            Scope *scope = scopes.at(i)->members();
            const unsigned start = scope->startOffset();
            const unsigned end = scope->endOffset();

            if (tokenOffset >= start && tokenOffset < end)
                return scope;
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
            addType(q->base());
            addType(q->name());

        } else if (name->isNameId() || name->isTemplateNameId()) {
            addType(name->identifier());

        }
    }

    void addMember(const Name *name)
    {
        if (! name) {
            return;

        } else if (name->isNameId()) {
            const Identifier *id = name->identifier();
            _members.insert(QByteArray::fromRawData(id->chars(), id->size()));

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
        else if (! symbol->type()->isFunctionType() && symbol->enclosingSymbol()->isClass())
            addMember(symbol->name());

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

CheckSymbols::Future CheckSymbols::go(Document::Ptr doc, const LookupContext &context)
{
    Q_ASSERT(doc);

    return (new CheckSymbols(doc, context))->start();
}

CheckSymbols::CheckSymbols(Document::Ptr doc, const LookupContext &context)
    : ASTVisitor(doc->translationUnit()), _doc(doc), _context(context)
{
    _fileName = doc->fileName();
    CollectTypes collectTypes(doc, context.snapshot());
    _potentialTypes = collectTypes.types();
    _potentialMembers = collectTypes.members();
    _scopes = collectTypes.scopes();
    _flushRequested = false;
    _flushLine = 0;
}

CheckSymbols::~CheckSymbols()
{ }

void CheckSymbols::run()
{
    _diagnosticMessages.clear();

    if (! isCanceled()) {
        if (_doc->translationUnit()) {
            accept(_doc->translationUnit()->ast());
            flush();
        }
    }

    reportFinished();
}

bool CheckSymbols::warning(unsigned line, unsigned column, const QString &text, unsigned length)
{
    Document::DiagnosticMessage m(Document::DiagnosticMessage::Warning, _fileName, line, column, text, length);
    _diagnosticMessages.append(m);
    return false;
}

bool CheckSymbols::warning(AST *ast, const QString &text)
{
    const Token &firstToken = tokenAt(ast->firstToken());
    const Token &lastToken = tokenAt(ast->lastToken() - 1);

    const unsigned length = lastToken.end() - firstToken.begin();
    unsigned line = 1, column = 1;
    getTokenStartPosition(ast->firstToken(), &line, &column);

    warning(line, column, text, length);
    return false;
}

bool CheckSymbols::preVisit(AST *)
{
    if (isCanceled())
        return false;

    return true;
}

bool CheckSymbols::visit(NamespaceAST *ast)
{
    if (ast->identifier_token) {
        const Token &tok = tokenAt(ast->identifier_token);
        if (! tok.generated()) {
            unsigned line, column;
            getTokenStartPosition(ast->identifier_token, &line, &column);
            Use use(line, column, tok.length());
            addUsage(use);
        }
    }

    return true;
}

bool CheckSymbols::visit(UsingDirectiveAST *)
{
    return true;
}

bool CheckSymbols::visit(SimpleDeclarationAST *)
{
    return true;
}

bool CheckSymbols::visit(NamedTypeSpecifierAST *)
{
    return true;
}

bool CheckSymbols::visit(MemberAccessAST *ast)
{
    accept(ast->base_expression);
    return false;
}

void CheckSymbols::checkNamespace(NameAST *name)
{
    if (! name)
        return;

    unsigned line, column;
    getTokenStartPosition(name->firstToken(), &line, &column);

    Scope *enclosingScope = _doc->scopeAt(line, column);
    if (ClassOrNamespace *b = _context.lookupType(name->name, enclosingScope)) {
        foreach (Symbol *s, b->symbols()) {
            if (s->isNamespace())
                return;
        }
    }

    const unsigned length = tokenAt(name->lastToken() - 1).end() - tokenAt(name->firstToken()).begin();
    warning(line, column, QCoreApplication::translate("CheckUndefinedSymbols", "Expected a namespace-name"), length);
}

void CheckSymbols::checkName(NameAST *ast)
{
    if (ast && ast->name) {
        if (const Identifier *ident = ast->name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialTypes.contains(id)) {
                Scope *scope = findScope(ast);
                const QList<LookupItem> candidates = _context.lookup(ast->name, scope);
                addUsage(candidates, ast);
            } else if (_potentialMembers.contains(id)) {
                Scope *scope = findScope(ast);
                const QList<LookupItem> candidates = _context.lookup(ast->name, scope);
                addMemberUsage(candidates, ast);
            }
        }
    }
}

void CheckSymbols::checkMemberName(NameAST *ast)
{
    if (ast && ast->name) {
        if (const Identifier *ident = ast->name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialMembers.contains(id)) {
                Scope *scope = findScope(ast);
                const QList<LookupItem> candidates = _context.lookup(ast->name, scope);
                addMemberUsage(candidates, ast);
            } else if (_potentialMembers.contains(id)) {
                Scope *scope = findScope(ast);
                const QList<LookupItem> candidates = _context.lookup(ast->name, scope);
                addMemberUsage(candidates, ast);
            }
        }
    }
}

bool CheckSymbols::visit(SimpleNameAST *ast)
{
    checkName(ast);
    return true;
}

bool CheckSymbols::visit(TemplateIdAST *ast)
{
    checkName(ast);
    return true;
}

bool CheckSymbols::visit(DestructorNameAST *ast)
{
    checkName(ast);
    return true;
}

bool CheckSymbols::visit(QualifiedNameAST *ast)
{
    if (ast->name) {
        Scope *scope = findScope(ast);

        ClassOrNamespace *b = 0;
        if (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list) {
            NestedNameSpecifierAST *nested_name_specifier = it->value;
            if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) { // ### remove shadowing

                if (TemplateIdAST *template_id = class_or_namespace_name->asTemplateId())
                    accept(template_id->template_argument_list);

                const Name *name = class_or_namespace_name->name;
                b = _context.lookupType(name, scope);
                addUsage(b, class_or_namespace_name);

                for (it = it->next; b && it; it = it->next) {
                    NestedNameSpecifierAST *nested_name_specifier = it->value;

                    if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) {
                        if (TemplateIdAST *template_id = class_or_namespace_name->asTemplateId())
                            accept(template_id->template_argument_list);

                        b = b->findType(class_or_namespace_name->name);
                        addUsage(b, class_or_namespace_name);
                    }
                }
            }
        }

        if (b && ast->unqualified_name)
            addUsage(b->find(ast->unqualified_name->name), ast->unqualified_name);
    }

    return false;
}

bool CheckSymbols::visit(TypenameTypeParameterAST *ast)
{
    if (ast->name && ast->name->name) {
        if (const Identifier *templId = ast->name->name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(templId->chars(), templId->size());
            if (_potentialTypes.contains(id)) {
                Scope *scope = findScope(_templateDeclarationStack.back());
                const QList<LookupItem> candidates = _context.lookup(ast->name->name, scope);
                addUsage(candidates, ast->name);
            }
        }
    }
    return true;
}

bool CheckSymbols::visit(TemplateTypeParameterAST *ast)
{
    checkName(ast->name);
    return true;
}

bool CheckSymbols::visit(TemplateDeclarationAST *ast)
{
    _templateDeclarationStack.append(ast);
    return true;
}

void CheckSymbols::endVisit(TemplateDeclarationAST *)
{
    _templateDeclarationStack.takeFirst();
}

bool CheckSymbols::visit(FunctionDefinitionAST *ast)
{
    _functionDefinitionStack.append(ast);

    accept(ast->decl_specifier_list);
    accept(ast->declarator);
    accept(ast->ctor_initializer);
    accept(ast->function_body);

    const LocalSymbols locals(_doc, ast);
    QList<SemanticInfo::Use> uses;
    foreach (uses, locals.uses) {
        foreach (const SemanticInfo::Use &u, uses)
            addUsage(u);
    }

    _functionDefinitionStack.removeLast();

    flush();
    return false;
}

void CheckSymbols::addUsage(const Use &use)
{
    if (_functionDefinitionStack.isEmpty()) {
        if (_usages.size() >= 50) {
            if (_flushRequested && use.line != _flushLine)
                flush();
            else if (! _flushRequested) {
                _flushRequested = true;
                _flushLine = use.line;
            }
        }
    }

    _usages.append(use);
}

void CheckSymbols::addUsage(ClassOrNamespace *b, NameAST *ast)
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
    const Use use(line, column, length);
    addUsage(use);
    //qDebug() << "added use" << oo(ast->name) << line << column << length;
}

void CheckSymbols::addUsage(const QList<LookupItem> &candidates, NameAST *ast)
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

    foreach (const LookupItem &r, candidates) {
        Symbol *c = r.declaration();
        if (c->isUsingDeclaration()) // skip using declarations...
            continue;
        else if (c->isUsingNamespaceDirective()) // ... and using namespace directives.
            continue;
        else if (c->isTypedef() || c->isNamespace() ||
                 c->isClass() || c->isEnum() ||
                 c->isForwardClassDeclaration() || c->isTypenameArgument()) {
            const Use use(line, column, length);
            addUsage(use);
            //qDebug() << "added use" << oo(ast->name) << line << column << length;
            break;
        }
    }
}

void CheckSymbols::addMemberUsage(const QList<LookupItem> &candidates, NameAST *ast)
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

    foreach (const LookupItem &r, candidates) {
        Symbol *c = r.declaration();
        if (! c)
            continue;
        else if (! c->isDeclaration())
            continue;
        else if (c->isTypedef())
            continue;
        else if (c->type()->isFunctionType())
            continue;
        else if (! c->enclosingSymbol()->isClass())
            continue;

        const Use use(line, column, length, Use::Field);
        addUsage(use);
        //qDebug() << "added use" << oo(ast->name) << line << column << length;
    }
}

unsigned CheckSymbols::startOfTemplateDeclaration(TemplateDeclarationAST *ast) const
{
    if (ast->declaration) {
        if (TemplateDeclarationAST *templ = ast->declaration->asTemplateDeclaration())
            return startOfTemplateDeclaration(templ);

        return ast->declaration->firstToken();
    }

    return ast->firstToken();
}

Scope *CheckSymbols::findScope(AST *ast) const
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
        scope = _doc->globalSymbols();

    return scope;
}

void CheckSymbols::flush()
{
    _flushRequested = false;
    _flushLine = 0;

    if (_usages.isEmpty())
        return;

    FriendlyThread::msleep(10); // release some cpu
    reportResults(_usages);
    _usages.clear();
}
