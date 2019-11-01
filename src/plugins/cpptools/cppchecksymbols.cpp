/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cppchecksymbols.h"

#include "cpplocalsymbols.h"

#include <utils/algorithm.h>
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
        if (!doc)
            return;
        if (!processed->contains(doc->globalNamespace())) {
            processed->insert(doc->globalNamespace());

            foreach (const Document::Include &i, doc->resolvedIncludes())
                process(_snapshot.document(i.resolvedFileName()), processed);

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
        if (!name) {
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
        if (!name) {
            return;

        } else if (name->isNameId()) {
            const Identifier *id = name->identifier();
            _fields.insert(QByteArray::fromRawData(id->chars(), id->size()));

        }
    }

    void addFunction(const Name *name)
    {
        if (!name) {
            return;

        } else if (name->isNameId()) {
            const Identifier *id = name->identifier();
            _functions.insert(QByteArray::fromRawData(id->chars(), id->size()));
        }
    }

    void addStatic(const Name *name)
    {
        if (!name) {
            return;

        } else if (name->isNameId() || name->isTemplateNameId()) {
            const Identifier *id = name->identifier();
            _statics.insert(QByteArray::fromRawData(id->chars(), id->size()));

        }
    }

    // nothing to do
    bool visit(UsingNamespaceDirective *) override { return true; }
    bool visit(UsingDeclaration *) override { return true; }
    bool visit(Argument *) override { return true; }
    bool visit(BaseClass *) override { return true; }

    bool visit(Function *symbol) override
    {
        addFunction(symbol->name());
        return true;
    }

    bool visit(Block *) override
    {
        return true;
    }

    bool visit(NamespaceAlias *symbol) override
    {
        addType(symbol->name());
        return true;
    }

    bool visit(Declaration *symbol) override
    {
        if (symbol->enclosingEnum() != nullptr)
            addStatic(symbol->name());

        if (symbol->type()->isFunctionType())
            addFunction(symbol->name());

        if (symbol->isTypedef())
            addType(symbol->name());
        else if (!symbol->type()->isFunctionType() && symbol->enclosingScope()->isClass())
            addField(symbol->name());

        return true;
    }

    bool visit(TypenameArgument *symbol) override
    {
        addType(symbol->name());
        return true;
    }

    bool visit(Enum *symbol) override
    {
        addType(symbol->name());
        return true;
    }

    bool visit(Namespace *symbol) override
    {
        addType(symbol->name());
        return true;
    }

    bool visit(Template *) override
    {
        return true;
    }

    bool visit(Class *symbol) override
    {
        addType(symbol->name());
        return true;
    }

    bool visit(ForwardClassDeclaration *symbol) override
    {
        addType(symbol->name());
        return true;
    }

    // Objective-C
    bool visit(ObjCBaseClass *) override { return true; }
    bool visit(ObjCBaseProtocol *) override { return true; }
    bool visit(ObjCPropertyDeclaration *) override { return true; }
    bool visit(ObjCMethod *) override { return true; }

    bool visit(ObjCClass *symbol) override
    {
        addType(symbol->name());
        return true;
    }

    bool visit(ObjCForwardClassDeclaration *symbol) override
    {
        addType(symbol->name());
        return true;
    }

    bool visit(ObjCProtocol *symbol) override
    {
        addType(symbol->name());
        return true;
    }

    bool visit(ObjCForwardProtocolDeclaration *symbol) override
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
    QTC_ASSERT(doc->translationUnit(), return Future());
    QTC_ASSERT(doc->translationUnit()->ast(), return Future());

    return (new CheckSymbols(doc, context, macroUses))->start();
}

CheckSymbols * CheckSymbols::create(Document::Ptr doc, const LookupContext &context,
                                    const QList<CheckSymbols::Result> &macroUses)
{
    QTC_ASSERT(doc, return nullptr);
    QTC_ASSERT(doc->translationUnit(), return nullptr);
    QTC_ASSERT(doc->translationUnit()->ast(), return nullptr);

    return new CheckSymbols(doc, context, macroUses);
}

CheckSymbols::CheckSymbols(Document::Ptr doc, const LookupContext &context, const QList<CheckSymbols::Result> &macroUses)
    : ASTVisitor(doc->translationUnit()), _doc(doc), _context(context)
    , _lineOfLastUsage(0), _macroUses(macroUses)
{
    int line = 0;
    getTokenEndPosition(translationUnit()->ast()->lastToken(), &line, nullptr);
    _chunkSize = qMax(50, line / 200);
    _usages.reserve(_chunkSize);

    _astStack.reserve(200);

    typeOfExpression.init(_doc, _context.snapshot(), _context.bindings());
    // make possible to instantiate templates
    typeOfExpression.setExpandTemplates(true);
}

CheckSymbols::~CheckSymbols() = default;

void CheckSymbols::run()
{
    CollectSymbols collectTypes(_doc, _context.snapshot());

    _fileName = _doc->fileName();
    _potentialTypes = collectTypes.types();
    _potentialFields = collectTypes.fields();
    _potentialFunctions = collectTypes.functions();
    _potentialStatics = collectTypes.statics();

    Utils::sort(_macroUses, sortByLinePredicate);
    if (!isCanceled()) {
        if (_doc->translationUnit()) {
            accept(_doc->translationUnit()->ast());
            _usages << QVector<Result>::fromList(_macroUses);
            flush();
        }

        emit codeWarningsUpdated(_doc, _diagMsgs);
    }

    reportFinished();
}

bool CheckSymbols::warning(unsigned line, unsigned column, const QString &text, unsigned length)
{
    Document::DiagnosticMessage m(Document::DiagnosticMessage::Warning, _fileName, line, column, text, length);
    _diagMsgs.append(m);
    return false;
}

bool CheckSymbols::warning(AST *ast, const QString &text)
{
    const Token &firstToken = tokenAt(ast->firstToken());
    const Token &lastToken = tokenAt(ast->lastToken() - 1);

    const unsigned length = lastToken.utf16charsEnd() - firstToken.utf16charsBegin();
    int line = 1, column = 1;
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

    return nullptr;
}

TemplateDeclarationAST *CheckSymbols::enclosingTemplateDeclaration() const
{
    for (int index = _astStack.size() - 1; index != -1; --index) {
        AST *ast = _astStack.at(index);

        if (TemplateDeclarationAST *funDef = ast->asTemplateDeclaration())
            return funDef;
    }

    return nullptr;
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

        } else if (TemplateDeclarationAST *templateDeclaration = ast->asTemplateDeclaration()) {
            if (templateDeclaration->symbol)
                return templateDeclaration->symbol;

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
    return !isCanceled();
}

void CheckSymbols::postVisit(AST *)
{
    _astStack.takeLast();
}

bool CheckSymbols::visit(NamespaceAST *ast)
{
    if (ast->identifier_token) {
        const Token &tok = tokenAt(ast->identifier_token);
        if (!tok.generated()) {
            int line, column;
            getTokenStartPosition(ast->identifier_token, &line, &column);
            Result use(line, column, tok.utf16chars(), SemanticHighlighter::TypeUse);
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
    addUse(ast->identifier_token, SemanticHighlighter::EnumerationUse);
    return true;
}

bool CheckSymbols::visit(DotDesignatorAST *ast)
{
    addUse(ast->identifier_token, SemanticHighlighter::FieldUse);
    return true;
}

bool CheckSymbols::visit(SimpleDeclarationAST *ast)
{
    NameAST *declrIdNameAST = nullptr;
    if (ast->declarator_list && !ast->declarator_list->next) {
        if (ast->symbols && !ast->symbols->next && !ast->symbols->value->isGenerated()) {
            Symbol *decl = ast->symbols->value;
            if (NameAST *nameAST = declaratorId(ast->declarator_list->value)) {
                if (Function *funTy = decl->type()->asFunctionType()) {
                    if (funTy->isVirtual()
                            || (nameAST->asDestructorName()
                                && hasVirtualDestructor(_context.lookupType(funTy->enclosingScope())))) {
                        addUse(nameAST, SemanticHighlighter::VirtualFunctionDeclarationUse);
                        declrIdNameAST = nameAST;
                    } else if (maybeAddFunction(_context.lookup(decl->name(),
                                                                decl->enclosingScope()),
                                                nameAST, funTy->argumentCount(),
                                                FunctionDeclaration)) {
                        declrIdNameAST = nameAST;

                        // Add a diagnostic message if non-virtual function has override/final marker
                        if ((_usages.back().kind != SemanticHighlighter::VirtualFunctionDeclarationUse)) {
                            if (funTy->isOverride())
                                warning(declrIdNameAST, QCoreApplication::translate(
                                            "CPlusplus::CheckSymbols", "Only virtual functions can be marked 'override'"));
                            else if (funTy->isFinal())
                                warning(declrIdNameAST, QCoreApplication::translate(
                                            "CPlusPlus::CheckSymbols", "Only virtual functions can be marked 'final'"));
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
    addUse(ast->name, SemanticHighlighter::TypeUse);
    return false;
}

bool CheckSymbols::visit(ObjCProtocolDeclarationAST *ast)
{
    accept(ast->attribute_list);
    accept(ast->name);
    accept(ast->protocol_refs);
    accept(ast->member_declaration_list);
    addUse(ast->name, SemanticHighlighter::TypeUse);
    return false;
}

bool CheckSymbols::visit(ObjCProtocolForwardDeclarationAST *ast)
{
    accept(ast->attribute_list);
    accept(ast->identifier_list);
    for (NameListAST *i = ast->identifier_list; i; i = i->next)
        addUse(i->value, SemanticHighlighter::TypeUse);
    return false;
}

bool CheckSymbols::visit(ObjCClassDeclarationAST *ast)
{
    accept(ast->attribute_list);
    accept(ast->class_name);
    accept(ast->category_name);
    accept(ast->superclass);
    accept(ast->protocol_refs);
    accept(ast->inst_vars_decl);
    accept(ast->member_declaration_list);
    addUse(ast->class_name, SemanticHighlighter::TypeUse);
    if (ast->superclass && maybeType(ast->superclass->name))
        addUse(ast->superclass, SemanticHighlighter::TypeUse);
    return false;
}

bool CheckSymbols::visit(ObjCClassForwardDeclarationAST *ast)
{
    accept(ast->attribute_list);
    accept(ast->identifier_list);
    for (NameListAST *i = ast->identifier_list; i; i = i->next)
        addUse(i->value, SemanticHighlighter::TypeUse);
    return false;
}

bool CheckSymbols::visit(ObjCProtocolRefsAST *ast)
{
    accept(ast->identifier_list);
    for (NameListAST *i = ast->identifier_list; i; i = i->next)
        if (maybeType(i->value->name))
            addUse(i->value, SemanticHighlighter::TypeUse);
    return false;
}

bool CheckSymbols::visit(MemberAccessAST *ast)
{
    accept(ast->base_expression);
    if (!ast->member_name)
        return false;

    if (const Name *name = ast->member_name->name) {
        if (const Identifier *ident = name->identifier()) {
            const QByteArray id = QByteArray::fromRawData(ident->chars(), ident->size());
            if (_potentialFields.contains(id)) {
                const Token start = tokenAt(ast->firstToken());
                const Token end = tokenAt(ast->lastToken() - 1);
                const QByteArray expression = _doc->utf8Source()
                        .mid(start.bytesBegin(), end.bytesEnd() - start.bytesBegin());

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

                    if (!maybeAddFunction(candidates, memberName, argumentCount, FunctionCall)
                            && highlightCtorDtorAsType) {
                        expr = ast->base_expression;
                    }
                }
            }
        } else if (IdExpressionAST *idExpr = ast->base_expression->asIdExpression()) {
            if (const Name *name = idExpr->name->name) {
                if (maybeFunction(name)) {
                    expr = nullptr;

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

                    if (!maybeAddFunction(candidates, exprName, argumentCount, FunctionCall)
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

bool CheckSymbols::visit(ObjCSelectorArgumentAST *ast)
{
    addUse(ast->firstToken(), SemanticHighlighter::FunctionUse);
    return true;
}

bool CheckSymbols::visit(NewExpressionAST *ast)
{
    accept(ast->new_placement);
    accept(ast->type_id);

    if (highlightCtorDtorAsType) {
        accept(ast->new_type_id);
    } else {
        ClassOrNamespace *binding = nullptr;
        NameAST *nameAST = nullptr;
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
                ExpressionListAST *list = nullptr;
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

            maybeAddFunction(_context.lookup(nameAST->name, scope), nameAST, arguments,
                             FunctionCall);
        }
    }

    accept(ast->new_initializer);

    return false;
}

QByteArray CheckSymbols::textOf(AST *ast) const
{
    const Token start = tokenAt(ast->firstToken());
    const Token end = tokenAt(ast->lastToken() - 1);
    const QByteArray text = _doc->utf8Source().mid(start.bytesBegin(),
                                                   end.bytesEnd() - start.bytesBegin());
    return text;
}

void CheckSymbols::checkNamespace(NameAST *name)
{
    if (!name)
        return;

    int line, column;
    getTokenStartPosition(name->firstToken(), &line, &column);

    if (ClassOrNamespace *b = _context.lookupType(name->name, enclosingScope())) {
        foreach (Symbol *s, b->symbols()) {
            if (s->isNamespace())
                return;
        }
    }

    const unsigned length = tokenAt(name->lastToken() - 1).utf16charsEnd()
            - tokenAt(name->firstToken()).utf16charsBegin();
    warning(line, column, QCoreApplication::translate("CPlusPlus::CheckSymbols",
                                                      "Expected a namespace-name"), length);
}

bool CheckSymbols::hasVirtualDestructor(Class *klass) const
{
    if (!klass)
        return false;
    const Identifier *id = klass->identifier();
    if (!id)
        return false;
    for (Symbol *s = klass->find(id); s; s = s->next()) {
        if (!s->name())
            continue;
        if (s->name()->isDestructorNameId()) {
            if (Function *funTy = s->type()->asFunctionType()) {
                if (funTy->isVirtual() && id->match(s->identifier()))
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

    while (!todo.isEmpty()) {
        ClassOrNamespace *b = todo.takeFirst();
        if (b && !processed.contains(b)) {
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
        if (!scope)
            scope = enclosingScope();

        if (ast->asDestructorName() != nullptr) {
            Class *klass = scope->asClass();
            if (!klass && scope->asFunction())
                klass = scope->asFunction()->enclosingScope()->asClass();

            if (klass) {
                if (hasVirtualDestructor(_context.lookupType(klass))) {
                    addUse(ast, SemanticHighlighter::VirtualFunctionDeclarationUse);
                } else {
                    bool added = false;
                    if (highlightCtorDtorAsType && maybeType(ast->name))
                        added = maybeAddTypeOrStatic(_context.lookup(ast->name, klass), ast);

                    if (!added)
                        addUse(ast, SemanticHighlighter::FunctionUse);
                }
            }
        } else if (maybeType(ast->name) || maybeStatic(ast->name)) {
            if (!maybeAddTypeOrStatic(_context.lookup(ast->name, scope), ast)) {
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
            if (ast->unqualified_name->asDestructorName() != nullptr) {
                if (hasVirtualDestructor(binding)) {
                    addUse(ast->unqualified_name, SemanticHighlighter::VirtualFunctionDeclarationUse);
                } else {
                    bool added = false;
                    if (highlightCtorDtorAsType && maybeType(ast->name))
                        added = maybeAddTypeOrStatic(binding->find(ast->unqualified_name->name),
                                                     ast->unqualified_name);
                    if (!added)
                        addUse(ast->unqualified_name, SemanticHighlighter::FunctionUse);
                }
            } else {
                QList<LookupItem> items = binding->find(ast->unqualified_name->name);
                if (items.empty())
                    items = _context.lookup(ast->name, enclosingScope());
                maybeAddTypeOrStatic(items, ast->unqualified_name);
            }

            if (TemplateIdAST *template_id = ast->unqualified_name->asTemplateId())
                accept(template_id->template_argument_list);
        }
    }

    return false;
}

ClassOrNamespace *CheckSymbols::checkNestedName(QualifiedNameAST *ast)
{
    ClassOrNamespace *binding = nullptr;

    if (ast->name) {
        if (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list) {
            NestedNameSpecifierAST *nested_name_specifier = it->value;
            if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) { // ### remove shadowing

                if (TemplateIdAST *template_id = class_or_namespace_name->asTemplateId())
                    accept(template_id->template_argument_list);

                const Name *name = class_or_namespace_name->name;
                binding = _context.lookupType(name, enclosingScope());
                if (binding)
                    addType(binding, class_or_namespace_name);
                else
                    // for the case when we use template parameter as qualifier
                    // e.g.: template <typename T> void fun() { T::type type; }
                    accept(nested_name_specifier->class_or_namespace_name);

                for (it = it->next; it; it = it->next) {
                    NestedNameSpecifierAST *nested_name_specifier = it->value;

                    if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) {
                        if (TemplateIdAST *template_id = class_or_namespace_name->asTemplateId()) {
                            if (template_id->template_token) {
                                addUse(template_id, SemanticHighlighter::TypeUse);
                                binding = nullptr; // there's no way we can find a binding.
                            }

                            accept(template_id->template_argument_list);
                            if (!binding)
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
    addUse(ast->name, SemanticHighlighter::TypeUse);
    accept(ast->type_id);
    return false;
}

bool CheckSymbols::visit(TemplateTypeParameterAST *ast)
{
    accept(ast->template_parameter_list);
    addUse(ast->name, SemanticHighlighter::TypeUse);
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
                                ExpressionListAST *expr_list = nullptr;
                                if (ExpressionListParenAST *parenExprList = ast->expression->asExpressionListParen())
                                    expr_list = parenExprList->expression_list;
                                else if (BracedInitializerAST *bracedInitList = ast->expression->asBracedInitializer())
                                    expr_list = bracedInitList->expression_list;
                                for (ExpressionListAST *it = expr_list; it; it = it->next)
                                    ++arguments;
                            }
                            maybeAddFunction(_context.lookup(nameAST->name, klass),
                                             nameAST, arguments, FunctionCall);
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
        addUse(ast->identifier_token, SemanticHighlighter::LabelUse);

    return false;
}

bool CheckSymbols::visit(LabeledStatementAST *ast)
{
    if (ast->label_token && !tokenAt(ast->label_token).isKeyword())
        addUse(ast->label_token, SemanticHighlighter::LabelUse);

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
                addUse(ast->specifier_token, SemanticHighlighter::PseudoKeywordUse);
            }
        }
    }

    return false;
}

bool CheckSymbols::visit(ClassSpecifierAST *ast)
{
    if (ast->final_token)
        addUse(ast->final_token, SemanticHighlighter::PseudoKeywordUse);

    return true;
}

bool CheckSymbols::visit(FunctionDefinitionAST *ast)
{
    AST *thisFunction = _astStack.takeLast();
    accept(ast->decl_specifier_list);
    _astStack.append(thisFunction);

    bool processEntireDeclr = true;
    if (ast->declarator && ast->symbol && !ast->symbol->isGenerated()) {
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
                addUse(declId, SemanticHighlighter::VirtualFunctionDeclarationUse);
            } else if (!maybeAddFunction(_context.lookup(fun->name(), fun->enclosingScope()),
                                         declId, fun->argumentCount(), FunctionDeclaration)) {
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
    if (!ast)
        return;

    if (QualifiedNameAST *q = ast->asQualifiedName())
        ast = q->unqualified_name;
    if (DestructorNameAST *dtor = ast->asDestructorName())
        ast = dtor->unqualified_name;

    if (!ast)
        return; // nothing to do
    else if (ast->asOperatorFunctionId() != nullptr || ast->asConversionFunctionId() != nullptr)
        return; // nothing to do

    unsigned startToken = ast->firstToken();

    if (TemplateIdAST *templ = ast->asTemplateId())
        startToken = templ->identifier_token;

    addUse(startToken, kind);
}

void CheckSymbols::addUse(unsigned tokenIndex, Kind kind)
{
    if (!tokenIndex)
        return;

    const Token &tok = tokenAt(tokenIndex);
    if (tok.generated())
        return;

    int line, column;
    getTokenStartPosition(tokenIndex, &line, &column);
    const unsigned length = tok.utf16chars();

    const Result use(line, column, length, kind);
    addUse(use);
}

void CheckSymbols::addUse(const Result &use)
{
    if (use.isInvalid())
        return;

    if (!enclosingFunctionDefinition()) {
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
    if (!b || !acceptName(ast, &startToken))
        return;

    const Token &tok = tokenAt(startToken);
    if (tok.generated())
        return;

    int line, column;
    getTokenStartPosition(startToken, &line, &column);
    const unsigned length = tok.utf16chars();
    const Result use(line, column, length, SemanticHighlighter::TypeUse);
    addUse(use);
}

bool CheckSymbols::isTemplateClass(Symbol *symbol) const
{
    if (symbol) {
        if (Template *templ = symbol->asTemplate()) {
            if (Symbol *declaration = templ->declaration()) {
                return declaration->isClass()
                        || declaration->isForwardClassDeclaration()
                        || declaration->isTypedef();
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
        if (c->isUsingNamespaceDirective()) // ... and using namespace directives.
            continue;
        if (c->isTypedef() || c->isNamespace() ||
                c->isStatic() || //consider also static variable
                c->isClass() || c->isEnum() || isTemplateClass(c) ||
                c->isForwardClassDeclaration() || c->isTypenameArgument() || c->enclosingEnum()) {
            int line, column;
            getTokenStartPosition(startToken, &line, &column);
            const unsigned length = tok.utf16chars();

            Kind kind = SemanticHighlighter::TypeUse;
            if (c->enclosingEnum() != nullptr)
                kind = SemanticHighlighter::EnumerationUse;
            else if (c->isStatic())
                // treat static variable as a field(highlighting)
                kind = SemanticHighlighter::FieldUse;

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
        if (!c)
            continue;
        if (!c->isDeclaration())
            return false;
        if (!(c->enclosingScope() && c->enclosingScope()->isClass()))
            return false; // shadowed
        if (c->isTypedef() || (c->type() && c->type()->isFunctionType()))
            return false; // shadowed

        int line, column;
        getTokenStartPosition(startToken, &line, &column);
        const unsigned length = tok.utf16chars();

        const Result use(line, column, length, SemanticHighlighter::FieldUse);
        addUse(use);

        return true;
    }

    return false;
}

bool CheckSymbols::maybeAddFunction(const QList<LookupItem> &candidates, NameAST *ast,
                                    int argumentCount, FunctionKind functionKind)
{
    int startToken = ast->firstToken();
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

    Kind kind = functionKind == FunctionDeclaration ? SemanticHighlighter::FunctionDeclarationUse
                                                    : SemanticHighlighter::FunctionUse;
    foreach (const LookupItem &r, candidates) {
        Symbol *c = r.declaration();

        // Skip current if there's no declaration or name.
        if (!c || !c->name())
            continue;

        // In addition check for destructors, since the leading ~ is not taken into consideration.
        // We don't want to compare destructors with something else or the other way around.
        if (isDestructor != c->name()->isDestructorNameId())
            continue;

        isConstructor = isConstructorDeclaration(c);

        Function *funTy = c->type()->asFunctionType();
        if (!funTy) {
            //Try to find a template function
            if (Template * t = r.type()->asTemplateType())
                if ((c = t->declaration()))
                    funTy = c->type()->asFunctionType();
        }
        if (!funTy || funTy->isAmbiguous())
            continue; // TODO: add diagnostic messages and color call-operators calls too?

        const bool isVirtual = funTy->isVirtual();
        Kind matchingKind;
        if (functionKind == FunctionDeclaration) {
            matchingKind = isVirtual ? SemanticHighlighter::VirtualFunctionDeclarationUse
                                     : SemanticHighlighter::FunctionDeclarationUse;
        } else {
            matchingKind = isVirtual ? SemanticHighlighter::VirtualMethodUse
                                     : SemanticHighlighter::FunctionUse;
        }
        if (argumentCount < funTy->minimumArgumentCount()) {
            if (matchType != Match_Ok) {
                kind = matchingKind;
                matchType = Match_TooFewArgs;
            }
        } else if (argumentCount > funTy->argumentCount() && !funTy->isVariadic()) {
            if (matchType != Match_Ok) {
                matchType = Match_TooManyArgs;
                kind = matchingKind;
            }
        } else {
            matchType = Match_Ok;
            kind = matchingKind;
            if (isVirtual)
                break;
            // else continue, to check if there is a matching candidate which is virtual
        }
    }

    if (matchType != Match_None) {
        // decide how constructor and destructor should be highlighted
        if (highlightCtorDtorAsType
                && (isConstructor || isDestructor)
                && maybeType(ast->name)
                && kind == SemanticHighlighter::FunctionDeclarationUse) {
            return false;
        }

        int line, column;
        getTokenStartPosition(startToken, &line, &column);
        const unsigned length = tok.utf16chars();

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
        if (DeclaratorIdAST *declId = ast->core_declarator->asDeclaratorId())
            return declId->name;
    }

    return nullptr;
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

    Utils::sort(_usages, sortByLinePredicate);
    reportResults(_usages);
    int cap = _usages.capacity();
    _usages.clear();
    _usages.reserve(cap);
}

bool CheckSymbols::isConstructorDeclaration(Symbol *declaration)
{
    Class *clazz = declaration->enclosingClass();
    if (clazz && clazz->name())
        return declaration->name()->match(clazz->name());

    return false;
}
