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
    QSet<QByteArray> _virtualMethods;
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

    const QSet<QByteArray> &virtualMethods() const
    {
        return _virtualMethods;
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

    void addVirtualMethod(const Name *name)
    {
        if (! name) {
            return;

        } else if (name->isNameId()) {
            const Identifier *id = name->identifier();
            _virtualMethods.insert(QByteArray::fromRawData(id->chars(), id->size()));

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
        if (symbol->isVirtual())
            addVirtualMethod(symbol->name());

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
        if (Function *funTy = symbol->type()->asFunctionType()) {
            if (funTy->isVirtual())
                addVirtualMethod(symbol->name());
        }

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
    _potentialVirtualMethods = collectTypes.virtualMethods();
    _scopes = collectTypes.scopes();
    _flushRequested = false;
    _flushLine = 0;

    typeOfExpression.init(_doc, _context.snapshot(), _context.bindings());
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

bool CheckSymbols::visit(SimpleDeclarationAST *ast)
{
    if (ast->declarator_list && !ast->declarator_list->next) {
        if (ast->symbols && ! ast->symbols->next && !ast->symbols->value->isGenerated()) {
            Symbol *decl = ast->symbols->value;
            if (NameAST *declId = declaratorId(ast->declarator_list->value)) {
                if (Function *funTy = decl->type()->asFunctionType()) {
                    if (funTy->isVirtual()) {
                        addVirtualMethodUsage(declId);
                    } else if (maybeVirtualMethod(decl->name())) {
                        addVirtualMethodUsage(_context.lookup(decl->name(), decl->scope()), declId, funTy->argumentCount());
                    }
                }
            }
        }
    }

    return true;
}

bool CheckSymbols::visit(NamedTypeSpecifierAST *)
{
    return true;
}

bool CheckSymbols::visit(MemberAccessAST *ast)
{
    accept(ast->base_expression);
    if (! ast->member_name)
        return false;

    if (const Name *name = ast->member_name->name) {
        if (const Identifier *ident = name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialMembers.contains(id)) {
                Scope *scope = findScope(ast);

                const Token start = tokenAt(ast->firstToken());
                const Token end = tokenAt(ast->lastToken() - 1);
                const QByteArray expression = _doc->source().mid(start.begin(), end.end() - start.begin());

                const QList<LookupItem> candidates = typeOfExpression(expression, scope, TypeOfExpression::Preprocess);
                addMemberUsage(candidates, ast->member_name);
            }
        }
    }

    return false;
}

bool CheckSymbols::visit(CallAST *ast)
{
    if (ast->base_expression) {
        accept(ast->base_expression);

        unsigned argumentCount = 0;

        for (ExpressionListAST *it = ast->expression_list; it; it = it->next)
            ++argumentCount;

        if (MemberAccessAST *access = ast->base_expression->asMemberAccess()) {
            if (access->member_name && access->member_name->name) {
                if (maybeVirtualMethod(access->member_name->name)) {
                    Scope *scope = findScope(access);
                    const QByteArray expression = textOf(access);

                    const QList<LookupItem> candidates = typeOfExpression(expression, scope, TypeOfExpression::Preprocess);

                    NameAST *memberName = access->member_name;
                    if (QualifiedNameAST *q = memberName->asQualifiedName())
                        memberName = q->unqualified_name;

                    addVirtualMethodUsage(candidates, memberName, argumentCount);
                }
            }
        } else if (IdExpressionAST *idExpr = ast->base_expression->asIdExpression()) {
            if (const Name *name = idExpr->name->name) {
                if (maybeVirtualMethod(name)) {
                    NameAST *exprName = idExpr->name;
                    if (QualifiedNameAST *q = exprName->asQualifiedName())
                        exprName = q->unqualified_name;

                    Scope *scope = findScope(idExpr);
                    const QByteArray expression = textOf(idExpr);

                    const QList<LookupItem> candidates = typeOfExpression(expression, scope, TypeOfExpression::Preprocess);
                    addVirtualMethodUsage(candidates, exprName, argumentCount);
                }
            }
        }

        accept(ast->expression_list);
    }

    return false;
}

QByteArray CheckSymbols::textOf(AST *ast) const
{
    const Token start = tokenAt(ast->firstToken());
    const Token end = tokenAt(ast->lastToken() - 1);
    const QByteArray text = _doc->source().mid(start.begin(), end.end() - start.begin());
    return text;
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

void CheckSymbols::checkName(NameAST *ast, Scope *scope)
{
    if (ast && ast->name) {
        if (! scope)
            scope = findScope(ast);

        if (const Identifier *ident = ast->name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialTypes.contains(id)) {
                const QList<LookupItem> candidates = _context.lookup(ast->name, scope);
                addUsage(candidates, ast);
            } else if (_potentialMembers.contains(id)) {
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

bool CheckSymbols::visit(MemInitializerAST *ast)
{
    if (_functionDefinitionStack.isEmpty())
        return false;

    if (ast->name) {
        FunctionDefinitionAST *enclosingFunction = _functionDefinitionStack.back();
        if (ClassOrNamespace *binding = _context.lookupType(enclosingFunction->symbol)) {
            foreach (Symbol *s, binding->symbols()) {
                if (Class *klass = s->asClass()){
                    checkName(ast->name, klass->members());
                    break;
                }
            }
        }
    }

    accept(ast->expression_list);

    return false;
}

bool CheckSymbols::visit(FunctionDefinitionAST *ast)
{
    _functionDefinitionStack.append(ast);

    accept(ast->decl_specifier_list);

    if (ast->declarator && ast->symbol && ! ast->symbol->isGenerated()) {
        Function *fun = ast->symbol;
        if (NameAST *declId = declaratorId(ast->declarator)) {
            if (QualifiedNameAST *q = declId->asQualifiedName())
                declId = q->unqualified_name;

            if (fun->isVirtual()) {
                addVirtualMethodUsage(declId);
            } else if (maybeVirtualMethod(fun->name())) {
                addVirtualMethodUsage(_context.lookup(fun->name(), fun->scope()), declId, fun->argumentCount());
            }
        }
    }

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
            return;
        else if (! (c->enclosingSymbol() && c->enclosingSymbol()->isClass()))
            return; // shadowed
        else if (c->isTypedef() || c->type()->isFunctionType())
            return; // shadowed

        const Use use(line, column, length, Use::Field);
        addUsage(use);
        break;
    }
}

void CheckSymbols::addVirtualMethodUsage(NameAST *ast)
{
    if (! ast)
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

    const Use use(line, column, length, Use::VirtualMethod);
    addUsage(use);
}

void CheckSymbols::addVirtualMethodUsage(const QList<LookupItem> &candidates, NameAST *ast, unsigned argumentCount)
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

        Function *funTy = r.type()->asFunctionType();
        if (! funTy)
            continue;
        if (! funTy->isVirtual())
            continue;
        else if (argumentCount < funTy->minimumArgumentCount())
            continue;

        const Use use(line, column, length, Use::VirtualMethod);
        addUsage(use);
        break;
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

NameAST *CheckSymbols::declaratorId(DeclaratorAST *ast) const
{
    if (ast && ast->core_declarator) {
        if (NestedDeclaratorAST *nested = ast->core_declarator->asNestedDeclarator())
            return declaratorId(nested->declarator);
        else if (DeclaratorIdAST *declId = ast->core_declarator->asDeclaratorId()) {
            return declId->name;
        }
    }

    return 0;
}

bool CheckSymbols::maybeVirtualMethod(const Name *name) const
{
    if (const Identifier *ident = name->identifier()) {
        const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
        if (_potentialVirtualMethods.contains(id))
            return true;
    }

    return false;
}

void CheckSymbols::flush()
{
    _flushRequested = false;
    _flushLine = 0;

    if (_usages.isEmpty())
        return;

    reportResults(_usages);
    _usages.clear();
}
