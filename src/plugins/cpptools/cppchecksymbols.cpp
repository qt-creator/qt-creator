/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppchecksymbols.h"

#include "cpplocalsymbols.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>

// This is for experimeting highlighting ctors/dtors as functions (instead of types).
// Whenever this feature is considered "accepted" the switch below should be permanently
// removed, unless we decide to actually make this a user setting - that is why it's
// currently a bool instead of a define.
static const bool highlightCtorDtorAsType = true;

using namespace CPlusPlus;
using namespace CppTools;

namespace {

class FriendlyThread: public QThread
{
public:
    using QThread::msleep;
};

class CollectSymbols: protected SymbolVisitor
{
    Document::Ptr _doc;
    Snapshot _snapshot;
    QSet<QByteArray> _types;
    QSet<QByteArray> _fields;
    QSet<QByteArray> _functions;
    QSet<QByteArray> _statics;
    bool _mainDocument;

public:
    CollectSymbols(Document::Ptr doc, const Snapshot &snapshot)
        : _doc(doc), _snapshot(snapshot), _mainDocument(false)
    {
        QSet<Namespace *> processed;
        process(doc, &processed);
    }

    const QSet<QByteArray> &types() const
    {
        return _types;
    }

    const QSet<QByteArray> &fields() const
    {
        return _fields;
    }

    const QSet<QByteArray> &functions() const
    {
        return _functions;
    }

    const QSet<QByteArray> &statics() const
    {
        return _statics;
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

    void addField(const Name *name)
    {
        if (! name) {
            return;

        } else if (name->isNameId()) {
            const Identifier *id = name->identifier();
            _fields.insert(QByteArray::fromRawData(id->chars(), id->size()));

        }
    }

    void addFunction(const Name *name)
    {
        if (! name) {
            return;

        } else if (name->isNameId()) {
            const Identifier *id = name->identifier();
            _functions.insert(QByteArray::fromRawData(id->chars(), id->size()));
        }
    }

    void addStatic(const Name *name)
    {
        if (! name) {
            return;

        } else if (name->isNameId() || name->isTemplateNameId()) {
            const Identifier *id = name->identifier();
            _statics.insert(QByteArray::fromRawData(id->chars(), id->size()));

        }
    }

    // nothing to do
    virtual bool visit(UsingNamespaceDirective *) { return true; }
    virtual bool visit(UsingDeclaration *) { return true; }
    virtual bool visit(Argument *) { return true; }
    virtual bool visit(BaseClass *) { return true; }

    virtual bool visit(Function *symbol)
    {
        addFunction(symbol->name());
        return true;
    }

    virtual bool visit(Block *)
    {
        return true;
    }

    virtual bool visit(NamespaceAlias *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(Declaration *symbol)
    {
        if (symbol->enclosingEnum() != 0)
            addStatic(symbol->name());

        if (symbol->type()->isFunctionType())
            addFunction(symbol->name());

        if (symbol->isTypedef())
            addType(symbol->name());
        else if (! symbol->type()->isFunctionType() && symbol->enclosingScope()->isClass())
            addField(symbol->name());

        return true;
    }

    virtual bool visit(TypenameArgument *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(Enum *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(Namespace *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(Template *)
    {
        return true;
    }

    virtual bool visit(Class *symbol)
    {
        addType(symbol->name());
        return true;
    }

    virtual bool visit(ForwardClassDeclaration *symbol)
    {
        addType(symbol->name());
        return true;
    }

    // Objective-C
    virtual bool visit(ObjCBaseClass *) { return true; }
    virtual bool visit(ObjCBaseProtocol *) { return true; }
    virtual bool visit(ObjCPropertyDeclaration *) { return true; }
    virtual bool visit(ObjCMethod *) { return true; }

    virtual bool visit(ObjCClass *symbol)
    {
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

static bool sortByLinePredicate(const CheckSymbols::Result &lhs, const CheckSymbols::Result &rhs)
{
    if (lhs.line == rhs.line)
        return lhs.column < rhs.column;
    else
        return lhs.line < rhs.line;
}


static bool acceptName(NameAST *ast, unsigned *referenceToken)
{
    *referenceToken = ast->firstToken();
    DestructorNameAST *dtor = ast->asDestructorName();
    if (dtor)
        *referenceToken = dtor->unqualified_name->firstToken();

    if (highlightCtorDtorAsType)
        return true;

    return !dtor
            && !ast->asConversionFunctionId()
            && !ast->asOperatorFunctionId();
}

CheckSymbols::Future CheckSymbols::go(Document::Ptr doc, const LookupContext &context, const QList<CheckSymbols::Result> &macroUses)
{
    QTC_ASSERT(doc, return Future());

    return (new CheckSymbols(doc, context, macroUses))->start();
}

CheckSymbols::CheckSymbols(Document::Ptr doc, const LookupContext &context, const QList<CheckSymbols::Result> &macroUses)
    : ASTVisitor(doc->translationUnit()), _doc(doc), _context(context)
    , _lineOfLastUsage(0), _macroUses(macroUses)
{
    unsigned line = 0;
    getTokenEndPosition(translationUnit()->ast()->lastToken(), &line, 0);
    _chunkSize = qMax(50U, line / 200);
    _usages.reserve(_chunkSize);

    _astStack.reserve(200);

    typeOfExpression.init(_doc, _context.snapshot(), _context.bindings());
    // make possible to instantiate templates
    typeOfExpression.setExpandTemplates(true);
}

CheckSymbols::~CheckSymbols()
{ }

void CheckSymbols::run()
{
    CollectSymbols collectTypes(_doc, _context.snapshot());

    _fileName = _doc->fileName();
    _potentialTypes = collectTypes.types();
    _potentialFields = collectTypes.fields();
    _potentialFunctions = collectTypes.functions();
    _potentialStatics = collectTypes.statics();

    qSort(_macroUses.begin(), _macroUses.end(), sortByLinePredicate);
    _doc->clearDiagnosticMessages();

    if (! isCanceled()) {
        if (_doc->translationUnit()) {
            accept(_doc->translationUnit()->ast());
            _usages << QVector<Result>::fromList(_macroUses);
            flush();
        }
    }

    reportFinished();
}

bool CheckSymbols::warning(unsigned line, unsigned column, const QString &text, unsigned length)
{
    Document::DiagnosticMessage m(Document::DiagnosticMessage::Warning, _fileName, line, column, text, length);
    _doc->addDiagnosticMessage(m);
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

FunctionDefinitionAST *CheckSymbols::enclosingFunctionDefinition(bool skipTopOfStack) const
{
    int index = _astStack.size() - 1;
    if (skipTopOfStack && !_astStack.isEmpty())
        --index;
    for (; index != -1; --index) {
        AST *ast = _astStack.at(index);

        if (FunctionDefinitionAST *funDef = ast->asFunctionDefinition())
            return funDef;
    }

    return 0;
}

TemplateDeclarationAST *CheckSymbols::enclosingTemplateDeclaration() const
{
    for (int index = _astStack.size() - 1; index != -1; --index) {
        AST *ast = _astStack.at(index);

        if (TemplateDeclarationAST *funDef = ast->asTemplateDeclaration())
            return funDef;
    }

    return 0;
}

Scope *CheckSymbols::enclosingScope() const
{
    for (int index = _astStack.size() - 1; index != -1; --index) {
        AST *ast = _astStack.at(index);

        if (NamespaceAST *ns = ast->asNamespace()) {
            if (ns->symbol)
                return ns->symbol;

        } else if (ClassSpecifierAST *classSpec = ast->asClassSpecifier()) {
            if (classSpec->symbol)
                return classSpec->symbol;

        } else if (FunctionDefinitionAST *funDef = ast->asFunctionDefinition()) {
            if (funDef->symbol)
                return funDef->symbol;

        } else if (CompoundStatementAST *blockStmt = ast->asCompoundStatement()) {
            if (blockStmt->symbol)
                return blockStmt->symbol;

        } else if (IfStatementAST *ifStmt = ast->asIfStatement()) {
            if (ifStmt->symbol)
                return ifStmt->symbol;

        } else if (WhileStatementAST *whileStmt = ast->asWhileStatement()) {
            if (whileStmt->symbol)
                return whileStmt->symbol;

        } else if (ForStatementAST *forStmt = ast->asForStatement()) {
            if (forStmt->symbol)
                return forStmt->symbol;

        } else if (ForeachStatementAST *foreachStmt = ast->asForeachStatement()) {
            if (foreachStmt->symbol)
                return foreachStmt->symbol;

        } else if (RangeBasedForStatementAST *rangeBasedForStmt = ast->asRangeBasedForStatement()) {
            if (rangeBasedForStmt->symbol)
                return rangeBasedForStmt->symbol;

        } else if (SwitchStatementAST *switchStmt = ast->asSwitchStatement()) {
            if (switchStmt->symbol)
                return switchStmt->symbol;

        } else if (CatchClauseAST *catchClause = ast->asCatchClause()) {
            if (catchClause->symbol)
                return catchClause->symbol;

        }
    }

    return _doc->globalNamespace();
}

bool CheckSymbols::preVisit(AST *ast)
{
    _astStack.append(ast);

    if (isCanceled())
        return false;

    return true;
}

void CheckSymbols::postVisit(AST *)
{
    _astStack.takeLast();
}

bool CheckSymbols::visit(NamespaceAST *ast)
{
    if (ast->identifier_token) {
        const Token &tok = tokenAt(ast->identifier_token);
        if (! tok.generated()) {
            unsigned line, column;
            getTokenStartPosition(ast->identifier_token, &line, &column);
            Result use(line, column, tok.length(), CppHighlightingSupport::TypeUse);
            addUse(use);
        }
    }

    return true;
}

bool CheckSymbols::visit(UsingDirectiveAST *)
{
    return true;
}

bool CheckSymbols::visit(EnumeratorAST *ast)
{
    addUse(ast->identifier_token, CppHighlightingSupport::EnumerationUse);
    return true;
}

bool CheckSymbols::visit(SimpleDeclarationAST *ast)
{
    NameAST *declrIdNameAST = 0;
    if (ast->declarator_list && !ast->declarator_list->next) {
        if (ast->symbols && ! ast->symbols->next && !ast->symbols->value->isGenerated()) {
            Symbol *decl = ast->symbols->value;
            if (NameAST *nameAST = declaratorId(ast->declarator_list->value)) {
                if (Function *funTy = decl->type()->asFunctionType()) {
                    if (funTy->isVirtual()
                            || (nameAST->asDestructorName()
                                && hasVirtualDestructor(_context.lookupType(funTy->enclosingScope())))) {
                        addUse(nameAST, CppHighlightingSupport::VirtualMethodUse);
                        declrIdNameAST = nameAST;
                    } else if (maybeAddFunction(_context.lookup(decl->name(),
                                                                decl->enclosingScope()),
                                                nameAST, funTy->argumentCount())) {
                        declrIdNameAST = nameAST;

                        // Add a diagnostic message if non-virtual function has override/final marker
                        if ((_usages.back().kind != CppHighlightingSupport::VirtualMethodUse)) {
                            if (funTy->isOverride())
                                warning(declrIdNameAST, QCoreApplication::translate(
                                            "CPlusplus::CheckSymbols", "Only virtual methods can be marked 'override'"));
                            else if (funTy->isFinal())
                                warning(declrIdNameAST, QCoreApplication::translate(
                                            "CPlusPlus::CheckSymbols", "Only virtual methods can be marked 'final'"));
                        }
                    }
                }
            }
        }
    }

    accept(ast->decl_specifier_list);

    for (DeclaratorListAST *it = ast->declarator_list; it ; it = it->next) {
        DeclaratorAST *declr = it->value;
        if (declrIdNameAST
                && declr->core_declarator
                && declr->core_declarator->asDeclaratorId()
                && declr->core_declarator->asDeclaratorId()->name == declrIdNameAST) {
            accept(declr->attribute_list);
            accept(declr->postfix_declarator_list);
            accept(declr->post_attribute_list);
            accept(declr->initializer);
        } else {
            accept(declr);
        }
    }

    return false;
}

bool CheckSymbols::visit(ElaboratedTypeSpecifierAST *ast)
{
    accept(ast->attribute_list);
    accept(ast->name);
    addUse(ast->name, CppHighlightingSupport::TypeUse);
    return false;
}

bool CheckSymbols::visit(MemberAccessAST *ast)
{
    accept(ast->base_expression);
    if (! ast->member_name)
        return false;

    if (const Name *name = ast->member_name->name) {
        if (const Identifier *ident = name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialFields.contains(id)) {
                const Token start = tokenAt(ast->firstToken());
                const Token end = tokenAt(ast->lastToken() - 1);
                const QByteArray expression = _doc->utf8Source().mid(start.begin(), end.end() - start.begin());

                const QList<LookupItem> candidates =
                    typeOfExpression(expression, enclosingScope(), TypeOfExpression::Preprocess);
                maybeAddField(candidates, ast->member_name);
            }
        }
    }

    return false;
}

bool CheckSymbols::visit(CallAST *ast)
{
    if (ast->base_expression) {
        unsigned argumentCount = 0;
        for (ExpressionListAST *it = ast->expression_list; it; it = it->next)
            ++argumentCount;

        ExpressionAST *expr = ast->base_expression;
        if (MemberAccessAST *access = ast->base_expression->asMemberAccess()) {
            if (access->member_name && access->member_name->name) {
                if (maybeFunction(access->member_name->name)) {
                    expr = access->base_expression;

                    const QByteArray expression = textOf(access);
                    const QList<LookupItem> candidates =
                        typeOfExpression(expression, enclosingScope(),
                                         TypeOfExpression::Preprocess);

                    NameAST *memberName = access->member_name;
                    if (QualifiedNameAST *q = memberName->asQualifiedName()) {
                        checkNestedName(q);
                        memberName = q->unqualified_name;
                    } else if (TemplateIdAST *tId = memberName->asTemplateId()) {
                        accept(tId->template_argument_list);
                    }

                    if (!maybeAddFunction(candidates, memberName, argumentCount)
                            && highlightCtorDtorAsType) {
                        expr = ast->base_expression;
                    }
                }
            }
        } else if (IdExpressionAST *idExpr = ast->base_expression->asIdExpression()) {
            if (const Name *name = idExpr->name->name) {
                if (maybeFunction(name)) {
                    expr = 0;

                    NameAST *exprName = idExpr->name;
                    if (QualifiedNameAST *q = exprName->asQualifiedName()) {
                        checkNestedName(q);
                        exprName = q->unqualified_name;
                    } else if (TemplateIdAST *tId = exprName->asTemplateId()) {
                        accept(tId->template_argument_list);
                    }

                    const QList<LookupItem> candidates =
                        typeOfExpression(textOf(idExpr), enclosingScope(),
                                         TypeOfExpression::Preprocess);

                    if (!maybeAddFunction(candidates, exprName, argumentCount)
                            && highlightCtorDtorAsType) {
                        expr = ast->base_expression;
                    }
                }
            }
        }

        accept(expr);
        accept(ast->expression_list);
    }

    return false;
}

bool CheckSymbols::visit(NewExpressionAST *ast)
{
    accept(ast->new_placement);
    accept(ast->type_id);

    if (highlightCtorDtorAsType) {
        accept(ast->new_type_id);
    } else {
        ClassOrNamespace *binding = 0;
        NameAST *nameAST = 0;
        if (ast->new_type_id) {
            for (SpecifierListAST *it = ast->new_type_id->type_specifier_list; it; it = it->next) {
                if (NamedTypeSpecifierAST *spec = it->value->asNamedTypeSpecifier()) {
                    nameAST = spec->name;
                    if (QualifiedNameAST *qNameAST = nameAST->asQualifiedName()) {
                        binding = checkNestedName(qNameAST);
                        if (binding)
                            binding = binding->findType(qNameAST->unqualified_name->name);
                        nameAST = qNameAST->unqualified_name;
                    } else if (maybeType(nameAST->name)) {
                        binding = _context.lookupType(nameAST->name, enclosingScope());
                    }

                    break;
                }
            }
        }

        if (binding && nameAST) {
            int arguments = 0;
            if (ast->new_initializer) {
                ExpressionListAST *list = 0;
                if (ExpressionListParenAST *exprListParen = ast->new_initializer->asExpressionListParen())
                    list = exprListParen->expression_list;
                else if (BracedInitializerAST *braceInit = ast->new_initializer->asBracedInitializer())
                    list = braceInit->expression_list;
                for (ExpressionListAST *it = list; it; it = it->next)
                    ++arguments;
            }

            Scope *scope = enclosingScope();
            foreach (Symbol *s, binding->symbols()) {
                if (Class *klass = s->asClass()) {
                    scope = klass;
                    break;
                }
            }

            maybeAddFunction(_context.lookup(nameAST->name, scope), nameAST, arguments);
        }
    }

    accept(ast->new_initializer);

    return false;
}

QByteArray CheckSymbols::textOf(AST *ast) const
{
    const Token start = tokenAt(ast->firstToken());
    const Token end = tokenAt(ast->lastToken() - 1);
    const QByteArray text = _doc->utf8Source().mid(start.begin(), end.end() - start.begin());
    return text;
}

void CheckSymbols::checkNamespace(NameAST *name)
{
    if (! name)
        return;

    unsigned line, column;
    getTokenStartPosition(name->firstToken(), &line, &column);

    if (ClassOrNamespace *b = _context.lookupType(name->name, enclosingScope())) {
        foreach (Symbol *s, b->symbols()) {
            if (s->isNamespace())
                return;
        }
    }

    const unsigned length = tokenAt(name->lastToken() - 1).end() - tokenAt(name->firstToken()).begin();
    warning(line, column, QCoreApplication::translate("CPlusPlus::CheckSymbols", "Expected a namespace-name"), length);
}

bool CheckSymbols::hasVirtualDestructor(Class *klass) const
{
    if (! klass)
        return false;
    const Identifier *id = klass->identifier();
    if (! id)
        return false;
    for (Symbol *s = klass->find(id); s; s = s->next()) {
        if (! s->name())
            continue;
        else if (s->name()->isDestructorNameId()) {
            if (Function *funTy = s->type()->asFunctionType()) {
                if (funTy->isVirtual() && id->isEqualTo(s->identifier()))
                    return true;
            }
        }
    }
    return false;
}

bool CheckSymbols::hasVirtualDestructor(ClassOrNamespace *binding) const
{
    QSet<ClassOrNamespace *> processed;
    QList<ClassOrNamespace *> todo;
    todo.append(binding);

    while (! todo.isEmpty()) {
        ClassOrNamespace *b = todo.takeFirst();
        if (b && ! processed.contains(b)) {
            processed.insert(b);
            foreach (Symbol *s, b->symbols()) {
                if (Class *k = s->asClass()) {
                    if (hasVirtualDestructor(k))
                        return true;
                }
            }

            todo += b->usings();
        }
    }

    return false;
}

void CheckSymbols::checkName(NameAST *ast, Scope *scope)
{
    if (ast && ast->name) {
        if (! scope)
            scope = enclosingScope();

        if (ast->asDestructorName() != 0) {
            Class *klass = scope->asClass();
            if (!klass && scope->asFunction())
                klass = scope->asFunction()->enclosingScope()->asClass();

            if (klass) {
                if (hasVirtualDestructor(_context.lookupType(klass))) {
                    addUse(ast, CppHighlightingSupport::VirtualMethodUse);
                } else {
                    bool added = false;
                    if (highlightCtorDtorAsType && maybeType(ast->name))
                        added = maybeAddTypeOrStatic(_context.lookup(ast->name, klass), ast);

                    if (!added)
                        addUse(ast, CppHighlightingSupport::FunctionUse);
                }
            }
        } else if (maybeType(ast->name) || maybeStatic(ast->name)) {
            if (! maybeAddTypeOrStatic(_context.lookup(ast->name, scope), ast)) {
                // it can be a local variable
                if (maybeField(ast->name))
                    maybeAddField(_context.lookup(ast->name, scope), ast);
            }
        } else if (maybeField(ast->name)) {
            maybeAddField(_context.lookup(ast->name, scope), ast);
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

bool CheckSymbols::visit(ParameterDeclarationAST *ast)
{
    accept(ast->type_specifier_list);
    // Skip parameter name, it does not need to be colored
    accept(ast->expression);
    return false;
}

bool CheckSymbols::visit(QualifiedNameAST *ast)
{
    if (ast->name) {

        ClassOrNamespace *binding = checkNestedName(ast);

        if (binding && ast->unqualified_name) {
            if (ast->unqualified_name->asDestructorName() != 0) {
                if (hasVirtualDestructor(binding)) {
                    addUse(ast->unqualified_name, CppHighlightingSupport::VirtualMethodUse);
                } else {
                    bool added = false;
                    if (highlightCtorDtorAsType && maybeType(ast->name))
                        added = maybeAddTypeOrStatic(binding->find(ast->unqualified_name->name),
                                                     ast->unqualified_name);
                    if (!added)
                        addUse(ast->unqualified_name, CppHighlightingSupport::FunctionUse);
                }
            } else {
                maybeAddTypeOrStatic(binding->find(ast->unqualified_name->name), ast->unqualified_name);
            }

            if (TemplateIdAST *template_id = ast->unqualified_name->asTemplateId())
                accept(template_id->template_argument_list);
        }
    }

    return false;
}

ClassOrNamespace *CheckSymbols::checkNestedName(QualifiedNameAST *ast)
{
    ClassOrNamespace *binding = 0;

    if (ast->name) {
        if (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list) {
            NestedNameSpecifierAST *nested_name_specifier = it->value;
            if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) { // ### remove shadowing

                if (TemplateIdAST *template_id = class_or_namespace_name->asTemplateId())
                    accept(template_id->template_argument_list);

                const Name *name = class_or_namespace_name->name;
                binding = _context.lookupType(name, enclosingScope());
                addType(binding, class_or_namespace_name);

                for (it = it->next; it; it = it->next) {
                    NestedNameSpecifierAST *nested_name_specifier = it->value;

                    if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) {
                        if (TemplateIdAST *template_id = class_or_namespace_name->asTemplateId()) {
                            if (template_id->template_token) {
                                addUse(template_id, CppHighlightingSupport::TypeUse);
                                binding = 0; // there's no way we can find a binding.
                            }

                            accept(template_id->template_argument_list);
                            if (! binding)
                                continue;
                        }

                        if (binding) {
                            binding = binding->findType(class_or_namespace_name->name);
                            addType(binding, class_or_namespace_name);
                        }
                    }
                }
            }
        }
    }

    return binding;
}

bool CheckSymbols::visit(TypenameTypeParameterAST *ast)
{
    addUse(ast->name, CppHighlightingSupport::TypeUse);
    accept(ast->type_id);
    return false;
}

bool CheckSymbols::visit(TemplateTypeParameterAST *ast)
{
    accept(ast->template_parameter_list);
    addUse(ast->name, CppHighlightingSupport::TypeUse);
    accept(ast->type_id);
    return false;
}

bool CheckSymbols::visit(MemInitializerAST *ast)
{
    if (FunctionDefinitionAST *enclosingFunction = enclosingFunctionDefinition()) {
        if (ast->name && enclosingFunction->symbol) {
            if (ClassOrNamespace *binding = _context.lookupType(enclosingFunction->symbol)) {
                foreach (Symbol *s, binding->symbols()) {
                    if (Class *klass = s->asClass()) {
                        NameAST *nameAST = ast->name;
                        if (QualifiedNameAST *q = nameAST->asQualifiedName()) {
                            checkNestedName(q);
                            nameAST = q->unqualified_name;
                        }

                        if (highlightCtorDtorAsType && maybeType(nameAST->name)) {
                            checkName(nameAST, klass);
                        } else if (maybeField(nameAST->name)) {
                            maybeAddField(_context.lookup(nameAST->name, klass), nameAST);
                        } else {
                            // It's a constructor, count the number of arguments
                            unsigned arguments = 0;
                            if (ast->expression) {
                                ExpressionListAST *expr_list = 0;
                                if (ExpressionListParenAST *parenExprList = ast->expression->asExpressionListParen())
                                    expr_list = parenExprList->expression_list;
                                else if (BracedInitializerAST *bracedInitList = ast->expression->asBracedInitializer())
                                    expr_list = bracedInitList->expression_list;
                                for (ExpressionListAST *it = expr_list; it; it = it->next)
                                    ++arguments;
                            }
                            maybeAddFunction(_context.lookup(nameAST->name, klass), nameAST, arguments);
                        }

                        break;
                    }
                }
            }
        }

        accept(ast->expression);
    }

    return false;
}

bool CheckSymbols::visit(GotoStatementAST *ast)
{
    if (ast->identifier_token)
        addUse(ast->identifier_token, CppHighlightingSupport::LabelUse);

    return false;
}

bool CheckSymbols::visit(LabeledStatementAST *ast)
{
    if (ast->label_token && !tokenAt(ast->label_token).isKeyword())
        addUse(ast->label_token, CppHighlightingSupport::LabelUse);

    accept(ast->statement);
    return false;
}


/**
 * \brief Highlights "override" and "final" pseudokeywords like true keywords
 */
bool CheckSymbols::visit(SimpleSpecifierAST *ast)
{
    if (ast->specifier_token)
    {
        const Token &tk = tokenAt(ast->specifier_token);
        if (tk.is(T_IDENTIFIER))
        {
            const Identifier &id = *(tk.identifier);
            if (id.equalTo(_doc->control()->cpp11Override())
                    || id.equalTo(_doc->control()->cpp11Final()))
            {
                addUse(ast->specifier_token, CppHighlightingSupport::PseudoKeywordUse);
            }
        }
    }

    return false;
}

bool CheckSymbols::visit(ClassSpecifierAST *ast)
{
    if (ast->final_token)
        addUse(ast->final_token, CppHighlightingSupport::PseudoKeywordUse);

    return true;
}

bool CheckSymbols::visit(FunctionDefinitionAST *ast)
{
    AST *thisFunction = _astStack.takeLast();
    accept(ast->decl_specifier_list);
    _astStack.append(thisFunction);

    bool processEntireDeclr = true;
    if (ast->declarator && ast->symbol && ! ast->symbol->isGenerated()) {
        Function *fun = ast->symbol;
        if (NameAST *declId = declaratorId(ast->declarator)) {
            processEntireDeclr = false;

            if (QualifiedNameAST *q = declId->asQualifiedName()) {
                checkNestedName(q);
                declId = q->unqualified_name;
            }

            if (fun->isVirtual()
                    || (declId->asDestructorName()
                        && hasVirtualDestructor(_context.lookupType(fun->enclosingScope())))) {
                addUse(declId, CppHighlightingSupport::VirtualMethodUse);
            } else if (!maybeAddFunction(_context.lookup(fun->name(),
                                                         fun->enclosingScope()),
                                         declId, fun->argumentCount())) {
                processEntireDeclr = true;
            }
        }
    }

    if (ast->declarator) {
        if (processEntireDeclr) {
            accept(ast->declarator);
        } else {
            accept(ast->declarator->attribute_list);
            accept(ast->declarator->postfix_declarator_list);
            accept(ast->declarator->post_attribute_list);
            accept(ast->declarator->initializer);
        }
    }

    accept(ast->ctor_initializer);
    accept(ast->function_body);

    const LocalSymbols locals(_doc, ast);
    foreach (const QList<Result> &uses, locals.uses) {
        foreach (const Result &u, uses)
            addUse(u);
    }

    if (!enclosingFunctionDefinition(true))
        if (_usages.size() >= _chunkSize)
            flush();

    return false;
}

void CheckSymbols::addUse(NameAST *ast, Kind kind)
{
    if (! ast)
        return;

    if (QualifiedNameAST *q = ast->asQualifiedName())
        ast = q->unqualified_name;
    if (DestructorNameAST *dtor = ast->asDestructorName())
        ast = dtor->unqualified_name;

    if (! ast)
        return; // nothing to do
    else if (ast->asOperatorFunctionId() != 0 || ast->asConversionFunctionId() != 0)
        return; // nothing to do

    unsigned startToken = ast->firstToken();

    if (TemplateIdAST *templ = ast->asTemplateId())
        startToken = templ->identifier_token;

    addUse(startToken, kind);
}

void CheckSymbols::addUse(unsigned tokenIndex, Kind kind)
{
    if (! tokenIndex)
        return;

    const Token &tok = tokenAt(tokenIndex);
    if (tok.generated())
        return;

    unsigned line, column;
    getTokenStartPosition(tokenIndex, &line, &column);
    const unsigned length = tok.length();

    const Result use(line, column, length, kind);
    addUse(use);
}

void CheckSymbols::addUse(const Result &use)
{
    if (use.isInvalid())
        return;

    if (! enclosingFunctionDefinition()) {
        if (_usages.size() >= _chunkSize) {
            if (use.line > _lineOfLastUsage)
                flush();
        }
    }

    while (!_macroUses.isEmpty() && _macroUses.first().line <= use.line)
        _usages.append(_macroUses.takeFirst());

    _lineOfLastUsage = qMax(_lineOfLastUsage, use.line);
    _usages.append(use);
}

void CheckSymbols::addType(ClassOrNamespace *b, NameAST *ast)
{
    unsigned startToken;
    if (! b || !acceptName(ast, &startToken))
        return;

    const Token &tok = tokenAt(startToken);
    if (tok.generated())
        return;

    unsigned line, column;
    getTokenStartPosition(startToken, &line, &column);
    const unsigned length = tok.length();
    const Result use(line, column, length, CppHighlightingSupport::TypeUse);
    addUse(use);
}

bool CheckSymbols::isTemplateClass(Symbol *symbol) const
{
    if (symbol) {
        if (Template *templ = symbol->asTemplate()) {
            if (Symbol *declaration = templ->declaration()) {
                if (declaration->isClass() || declaration->isForwardClassDeclaration())
                    return true;
            }
        }
    }
    return false;
}

bool CheckSymbols::maybeAddTypeOrStatic(const QList<LookupItem> &candidates, NameAST *ast)
{
    unsigned startToken;
    if (!acceptName(ast, &startToken))
        return false;

    const Token &tok = tokenAt(startToken);
    if (tok.generated())
        return false;

    foreach (const LookupItem &r, candidates) {
        Symbol *c = r.declaration();
        if (c->isUsingDeclaration()) // skip using declarations...
            continue;
        else if (c->isUsingNamespaceDirective()) // ... and using namespace directives.
            continue;
        else if (c->isTypedef() || c->isNamespace() ||
                 c->isStatic() || //consider also static variable
                 c->isClass() || c->isEnum() || isTemplateClass(c) ||
                 c->isForwardClassDeclaration() || c->isTypenameArgument() || c->enclosingEnum() != 0) {

            unsigned line, column;
            getTokenStartPosition(startToken, &line, &column);
            const unsigned length = tok.length();

            Kind kind = CppHighlightingSupport::TypeUse;
            if (c->enclosingEnum() != 0)
                kind = CppHighlightingSupport::EnumerationUse;
            else if (c->isStatic())
                // treat static variable as a field(highlighting)
                kind = CppHighlightingSupport::FieldUse;

            const Result use(line, column, length, kind);
            addUse(use);

            return true;
        }
    }

    return false;
}

bool CheckSymbols::maybeAddField(const QList<LookupItem> &candidates, NameAST *ast)
{
    unsigned startToken;
    if (!acceptName(ast, &startToken))
        return false;

    const Token &tok = tokenAt(startToken);
    if (tok.generated())
        return false;

    foreach (const LookupItem &r, candidates) {
        Symbol *c = r.declaration();
        if (! c)
            continue;
        else if (! c->isDeclaration())
            return false;
        else if (! (c->enclosingScope() && c->enclosingScope()->isClass()))
            return false; // shadowed
        else if (c->isTypedef() || (c->type() && c->type()->isFunctionType()))
            return false; // shadowed

        unsigned line, column;
        getTokenStartPosition(startToken, &line, &column);
        const unsigned length = tok.length();

        const Result use(line, column, length, CppHighlightingSupport::FieldUse);
        addUse(use);

        return true;
    }

    return false;
}

bool CheckSymbols::maybeAddFunction(const QList<LookupItem> &candidates, NameAST *ast, unsigned argumentCount)
{
    unsigned startToken = ast->firstToken();
    bool isDestructor = false;
    bool isConstructor = false;
    if (DestructorNameAST *dtor = ast->asDestructorName()) {
        isDestructor = true;
        if (dtor->unqualified_name)
            startToken = dtor->unqualified_name->firstToken();
    }

    const Token &tok = tokenAt(startToken);
    if (tok.generated())
        return false;

    enum { Match_None, Match_TooManyArgs, Match_TooFewArgs, Match_Ok } matchType = Match_None;
    Kind kind = CppHighlightingSupport::FunctionUse;
    foreach (const LookupItem &r, candidates) {
        Symbol *c = r.declaration();

        // Skip current if there's no declaration or name.
        if (! c || !c->name())
            continue;

        // In addition check for destructors, since the leading ~ is not taken into consideration.
        // We don't want to compare destructors with something else or the other way around.
        if (isDestructor != c->name()->isDestructorNameId())
            continue;

        isConstructor = isConstructorDeclaration(c);

        Function *funTy = c->type()->asFunctionType();
        if (! funTy) {
            //Try to find a template function
            if (Template * t = r.type()->asTemplateType())
                if ((c = t->declaration()))
                    funTy = c->type()->asFunctionType();
        }
        if (! funTy)
            continue; // TODO: add diagnostic messages and color call-operators calls too?

        if (argumentCount < funTy->minimumArgumentCount()) {
            if (matchType != Match_Ok) {
                kind = funTy->isVirtual() ? CppHighlightingSupport::VirtualMethodUse : CppHighlightingSupport::FunctionUse;
                matchType = Match_TooFewArgs;
            }
        } else if (argumentCount > funTy->argumentCount() && ! funTy->isVariadic()) {
            if (matchType != Match_Ok) {
                matchType = Match_TooManyArgs;
                kind = funTy->isVirtual() ? CppHighlightingSupport::VirtualMethodUse : CppHighlightingSupport::FunctionUse;
            }
        } else if (!funTy->isVirtual()) {
            matchType = Match_Ok;
            kind = CppHighlightingSupport::FunctionUse;
            //continue, to check if there is a matching candidate which is virtual
        } else {
            matchType = Match_Ok;
            kind = CppHighlightingSupport::VirtualMethodUse;
            break;
        }
    }

    if (matchType != Match_None) {
        // decide how constructor and destructor should be highlighted
        if (highlightCtorDtorAsType
                && (isConstructor || isDestructor)
                && maybeType(ast->name)
                && kind == CppHighlightingSupport::FunctionUse) {
            return false;
        }

        unsigned line, column;
        getTokenStartPosition(startToken, &line, &column);
        const unsigned length = tok.length();

        // Add a diagnostic message if argument count does not match
        if (matchType == Match_TooFewArgs)
            warning(line, column, QCoreApplication::translate("CplusPlus::CheckSymbols", "Too few arguments"), length);
        else if (matchType == Match_TooManyArgs)
            warning(line, column, QCoreApplication::translate("CPlusPlus::CheckSymbols", "Too many arguments"), length);

        const Result use(line, column, length, kind);
        addUse(use);

        return true;
    }

    return false;
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

bool CheckSymbols::maybeType(const Name *name) const
{
    if (name) {
        if (const Identifier *ident = name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialTypes.contains(id))
                return true;
        }
    }

    return false;
}

bool CheckSymbols::maybeField(const Name *name) const
{
    if (name) {
        if (const Identifier *ident = name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialFields.contains(id))
                return true;
        }
    }

    return false;
}

bool CheckSymbols::maybeStatic(const Name *name) const
{
    if (name) {
        if (const Identifier *ident = name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialStatics.contains(id))
                return true;
        }
    }

    return false;
}

bool CheckSymbols::maybeFunction(const Name *name) const
{
    if (name) {
        if (const Identifier *ident = name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialFunctions.contains(id))
                return true;
        }
    }

    return false;
}

void CheckSymbols::flush()
{
    _lineOfLastUsage = 0;

    if (_usages.isEmpty())
        return;

    qSort(_usages.begin(), _usages.end(), sortByLinePredicate);
    reportResults(_usages);
    int cap = _usages.capacity();
    _usages.clear();
    _usages.reserve(cap);
}

bool CheckSymbols::isConstructorDeclaration(Symbol *declaration)
{
    Class *clazz = declaration->enclosingClass();
    if (clazz && clazz->name())
        return declaration->name()->isEqualTo(clazz->name());

    return false;
}
