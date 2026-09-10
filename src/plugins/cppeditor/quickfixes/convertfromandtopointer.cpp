// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "convertfromandtopointer.h"

#include "../cppeditortr.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"

#include <cplusplus/ASTPath.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/projectmanager.h>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#endif

using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

enum class SmartPointerKind { UniquePtr, ScopedPointer };

// Recognizes std::unique_ptr<T>/QScopedPointer<T> by exact class name.
std::optional<std::pair<SmartPointerKind, FullySpecifiedType>> smartPointerElementType(
    const Name *name)
{
    if (!name)
        return {};
    if (const QualifiedNameId *qualifiedName = name->asQualifiedNameId())
        return smartPointerElementType(qualifiedName->name());
    const TemplateNameId *templateName = name->asTemplateNameId();
    if (!templateName || templateName->templateArgumentCount() == 0)
        return {};
    const Identifier *id = templateName->identifier();
    if (!id)
        return {};
    const QString baseName = QString::fromUtf8(id->chars(), id->size());
    SmartPointerKind kind;
    if (baseName == QLatin1String("unique_ptr"))
        kind = SmartPointerKind::UniquePtr;
    else if (baseName == QLatin1String("QScopedPointer"))
        kind = SmartPointerKind::ScopedPointer;
    else
        return {};
    return std::make_pair(kind, templateName->templateArgumentAt(0).type());
}

std::optional<std::pair<SmartPointerKind, FullySpecifiedType>> smartPointerElementType(Type *type)
{
    if (!type)
        return {};
    // Peel a reference off first: an expression's deduced type can come back as e.g.
    // "unique_ptr<Foo> &&" (std::move()'s return type) or "unique_ptr<Foo> &" (an ordinary
    // reference-returning getter), and both name the same smart pointer as their non-reference
    // counterpart would.
    if (ReferenceType * const refType = type->asReferenceType())
        return smartPointerElementType(refType->elementType().type());
    NamedType * const namedType = type->asNamedType();
    if (!namedType)
        return {};
    return smartPointerElementType(namedType->name());
}

QString smartPointerTypeName(SmartPointerKind kind)
{
    switch (kind) {
    case SmartPointerKind::UniquePtr:
        return QLatin1String("std::unique_ptr");
    case SmartPointerKind::ScopedPointer:
        return QLatin1String("QScopedPointer");
    }
    return {};
}

// QScopedPointer is a Qt type; offering it in a project that doesn't have Qt available would
// suggest an addition that cannot possibly compile. std::unique_ptr has no such restriction.
// The test harness runs without a real opened project, so activeKitForCurrentProject() always
// returns null there; testMode assumes Qt is available instead of always declining under test.
bool isQScopedPointerAvailable(bool testMode)
{
    if (testMode)
        return true;
    Kit * const currentKit = activeKitForCurrentProject();
    return currentKit && !currentKit->moduleForHeader(QLatin1String("QScopedPointer")).isEmpty();
}

// The #include the newly-introduced smart pointer type needs, or an empty string if a matching
// one is already present. Only checks whether the header is literally written out already, not
// full semantic reachability (e.g. via an indirect include) - insertNewIncludeDirective() has no
// dedup of its own, so this existence check is what keeps a repeated conversion from piling up
// duplicate includes.
QString missingIncludeForSmartPointer(const Document::Ptr &doc, SmartPointerKind kind)
{
    const QString header = kind == SmartPointerKind::UniquePtr ? QLatin1String("memory")
                                                                : QLatin1String("QScopedPointer");
    for (const Document::Include &include : doc->resolvedIncludes()) {
        if (include.unresolvedFileName() == header)
            return {};
    }
    for (const Document::Include &include : doc->unresolvedIncludes()) {
        if (include.unresolvedFileName() == header)
            return {};
    }
    return QLatin1Char('<') + header + QLatin1Char('>');
}

// wrapDeclarationType()/unwrapDeclarationType() replace the declaration's specifier(s) with the
// converted type as a single run of text. That's only correct when there is exactly one
// specifier and one declarator: decl_specifier_list is shared by every declarator in the same
// declaration (so converting one would silently convert the type of all the others too), and
// any specifier beyond the plain type itself - a cv-qualifier or a storage class like "static" -
// would either get duplicated alongside the type (elementTypeName() already folds
// cv-qualification into it) or silently dropped, changing what the declaration means.
bool hasSinglePlainDeclarator(const SimpleDeclarationAST *simpleDeclaration)
{
    return simpleDeclaration->decl_specifier_list && !simpleDeclaration->decl_specifier_list->next
           && simpleDeclaration->declarator_list && !simpleDeclaration->declarator_list->next;
}

enum class SmartPointerUseKind {
    Declaration,
    PointeeAccess,  // "->" or "*": identical for a raw and a smart pointer, never rewritten.
    BareUse,        // Read as a value (bool context, function argument, ...).
    GetCall,        // ".get()"
    ResetNoArgs,    // ".reset()"
    ResetWithArg,   // ".reset(expr)"
    DeleteCall,     // "delete x;" - only ever seen while x is still a raw pointer.
    AssignExpr,     // "x = expr;" for any raw-pointer-valued expr (new, nullptr, another pointer,
                    // a function call, ...) - the rewrite is a pure text-wrap either way, so the
                    // shape of expr doesn't matter.
    AssignMakeCall, // "x = std::make_unique<Foo>(...);" / "... make_shared ..." - unlike
                    // AssignExpr, this needs unwrapping to "new Foo(...)" when converting to a
                    // raw pointer, since the RHS itself produces an owning smart pointer.
    Unsupported     // Anything else: ".release()", ".swap()", "&x", ...
};

struct ClassifiedSmartPointerUse
{
    SmartPointerUseKind kind = SmartPointerUseKind::Unsupported;
    AST *node = nullptr; // CallAST for Get/Reset, BinaryExpressionAST for the Assign* kinds.
};

// Peels off any redundant enclosing parentheses, so e.g. "std::move(other)" and
// "((std::move(other)))" classify identically. Matters because an extra pair of parens around a
// parenthesized constructor-style initializer's argument (e.g. to sidestep the "most vexing
// parse" ambiguity) is represented as a distinct NestedExpressionAST wrapper node,
// not silently discarded.
ExpressionAST *stripParens(ExpressionAST *expr)
{
    while (expr) {
        NestedExpressionAST * const nested = expr->asNestedExpression();
        if (!nested)
            break;
        expr = nested->expression;
    }
    return expr;
}

// The callee's bare (identifier) name, with any qualification peeled off - Name::identifier()
// already looks through an explicit template-argument list on its own (e.g. the identifier of
// "std::make_unique<Foo>" is "make_unique", not "make_unique<Foo>"), so a text-based comparison
// against the call's source range would wrongly fail to match whenever the call spells out an
// explicit template argument, exactly as make_unique's normally does.
const Identifier *calleeIdentifier(ExpressionAST *expr)
{
    CallAST * const call = expr ? expr->asCall() : nullptr;
    IdExpressionAST * const idExpr
        = call && call->base_expression ? call->base_expression->asIdExpression() : nullptr;
    const Name *name = idExpr && idExpr->name ? idExpr->name->name : nullptr;
    if (const QualifiedNameId * const qualifiedName = name ? name->asQualifiedNameId() : nullptr)
        name = qualifiedName->name();
    return name ? name->identifier() : nullptr;
}

bool calleeIdentifierIs(ExpressionAST *expr, const char *name)
{
    const Identifier * const id = calleeIdentifier(expr);
    return id && QString::fromUtf8(id->chars(), id->size()) == QLatin1String(name);
}

// We special-case std::move, because our code model is not able to properly deduce its
// return type from a call expression.
bool isMoveCall(ExpressionAST *expr)
{
    return calleeIdentifierIs(expr, "move");
}

// Compares the callee name's identifier against "make_unique" exactly (see calleeIdentifier()):
// a substring match would also accept an ordinary factory like "custom_make_unique()", silently
// dropping whatever it actually does once unwrapped into a bare "new Foo(...)".
bool isMakeUniqueCall(ExpressionAST *expr)
{
    return calleeIdentifierIs(expr, "make_unique");
}

// "&local" is the one common, unambiguous non-owning shape visible directly in the AST: wrapping
// it into a smart-pointer constructor argument or ".reset(...)" call would hand ownership of a
// non-owned address to the smart pointer, which deletes it once it goes out of scope. Ownership
// cannot be decided from an arbitrary expression in general, but this one can - including when
// it's buried under pointer arithmetic ("&local + 1", "1 + &local", "&local - 1"), which is
// exactly as non-owning, since offsetting an address doesn't create ownership. "integer - pointer"
// isn't valid C++, so only "+" needs to check both sides; "-" only ever has a pointer on the left.
bool isAddressOfExpression(const CppRefactoringFilePtr &file, ExpressionAST *expr)
{
    expr = stripParens(expr);
    if (!expr)
        return false;
    if (UnaryExpressionAST * const unaryExpr = expr->asUnaryExpression())
        return file->tokenAt(unaryExpr->unary_op_token).kind() == T_AMPER;
    if (BinaryExpressionAST * const binExpr = expr->asBinaryExpression()) {
        const int op = file->tokenAt(binExpr->binary_op_token).kind();
        if (op == T_PLUS) {
            return isAddressOfExpression(file, binExpr->left_expression)
                   || isAddressOfExpression(file, binExpr->right_expression);
        }
        if (op == T_MINUS)
            return isAddressOfExpression(file, binExpr->left_expression);
    }
    return false;
}

// The "delete x;\nx = ...;" rewrite (see rewriteSmartPointerUses() below) replaces one statement
// with two, which only preserves the original control flow - and only makes syntactic sense at
// all - if targetNode is not just somewhere inside a block, but is itself the *entire* expression
// of a bare ExpressionStatementAST that is a direct child of a CompoundStatementAST. Merely being
// inside a statement that is itself inside a block isn't enough: a ternary branch
// ("c ? f.reset() : g();"), a return expression ("return f = std::make_unique<Foo>(1);"), or a
// for-loop's own increment clause ("for (...; ...; f.reset())") would all satisfy that looser
// check while breaking the same way as the unbraced if/while case - splicing gibberish into the
// ternary, or (for return/for) never having an enclosing ExpressionStatementAST to split into two
// in the first place.
bool canSplitIntoTwoStatements(const QList<AST *> &path, int fromIndex, ExpressionAST *targetNode)
{
    int i = fromIndex;
    while (i >= 0 && !path.at(i)->asStatement())
        --i;
    if (i <= 0)
        return false;
    ExpressionStatementAST * const exprStatement = path.at(i)->asExpressionStatement();
    if (!exprStatement || !path.at(i - 1)->asCompoundStatement())
        return false;
    return stripParens(exprStatement->expression) == targetNode;
}

// Recognizes any expression whose own type is itself a unique_ptr/QScopedPointer - covers any
// arbitrarily-named function returning a smart pointer by value, as long as its return type is
// concretely declared (not deduced from the call, see isMoveCall() above for why that case needs
// separate handling). Such an expression names an already-existing (or otherwise foreign) object
// whose ownership is being transferred, not a fresh one being created, so (unlike a
// make_unique call) it cannot be unwrapped into a "new Foo(...)" equivalent.
bool isSmartPointerTypedExpression(
    const CppQuickFixInterface &interface, const CppRefactoringFilePtr &file, ExpressionAST *expr)
{
    if (!expr)
        return false;
    TypeOfExpression typeOfExpression;
    typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot());
    typeOfExpression.setExpandTemplates(true);
    Scope * const scope = file->scopeAt(expr->firstToken());
    const QList<LookupItem> result
        = typeOfExpression(file->textOf(expr).toUtf8(), scope, TypeOfExpression::Preprocess);
    if (result.isEmpty())
        return false;
    return smartPointerElementType(result.first().type().type()).has_value();
}

// Walks from a use of the declared symbol outward, classifying the syntactic context it
// appears in. Shared between the raw-pointer and smart-pointer directions: which
// classifications can actually occur depends on which type the variable currently has (e.g.
// DeleteCall can only occur while it is still a raw pointer, since smart pointers don't
// implicitly convert to one). symbolIsSmartPointer says which direction is being classified -
// see the comment at the isSmartPointer check below for why that distinction matters here.
ClassifiedSmartPointerUse classifySmartPointerUse(
    const CppRefactoringFilePtr &file,
    const QList<AST *> &path,
    const DeclaratorAST *declaratorAST,
    bool symbolIsSmartPointer)
{
    for (int i = path.count() - 2; i >= 0; --i) {
        AST * const ast = path.at(i);
        if (ast == declaratorAST)
            return {SmartPointerUseKind::Declaration, nullptr};
        if (MemberAccessAST * const memberAccessAST = ast->asMemberAccess()) {
            if (file->tokenAt(memberAccessAST->access_token).kind() == T_ARROW)
                return {SmartPointerUseKind::PointeeAccess, nullptr};
            const QString memberName = memberAccessAST->member_name
                                           ? file->textOf(memberAccessAST->member_name)
                                           : QString();
            // The enclosing call, if any, is the parent of this member access in the
            // root-to-leaf path (i.e. at the next-smaller index).
            CallAST *callAST = nullptr;
            if (i > 0)
                callAST = path.at(i - 1)->asCall();
            if (memberName == QLatin1String("get") && callAST)
                return {SmartPointerUseKind::GetCall, callAST};
            if (memberName == QLatin1String("reset") && callAST) {
                if (!canSplitIntoTwoStatements(path, i, callAST))
                    return {SmartPointerUseKind::Unsupported, nullptr};
                if (callAST->expression_list)
                    return {SmartPointerUseKind::ResetWithArg, callAST};
                return {SmartPointerUseKind::ResetNoArgs, callAST};
            }
            return {SmartPointerUseKind::Unsupported, nullptr};
        }
        if (UnaryExpressionAST * const unaryExprAST = ast->asUnaryExpression()) {
            const Token tk = file->tokenAt(unaryExprAST->unary_op_token);
            if (tk.kind() == T_STAR)
                return {SmartPointerUseKind::PointeeAccess, nullptr};
            if (tk.kind() == T_AMPER)
                return {SmartPointerUseKind::Unsupported, nullptr};
        } else if (DeleteExpressionAST * const deleteAST = ast->asDeleteExpression()) {
            return {SmartPointerUseKind::DeleteCall, deleteAST};
        } else if (BinaryExpressionAST * const binaryExprAST = ast->asBinaryExpression()) {
            const bool isAssignmentTarget = file->tokenAt(binaryExprAST->binary_op_token).kind()
                                                == T_EQUAL
                                            && i + 1 < path.count()
                                            && path.at(i + 1) == binaryExprAST->left_expression;
            if (!isAssignmentTarget)
                return {SmartPointerUseKind::BareUse, nullptr};
            ExpressionAST * const rhs = binaryExprAST->right_expression;
            if (rhs) {
                if (isMakeUniqueCall(rhs)) {
                    if (!canSplitIntoTwoStatements(path, i, binaryExprAST))
                        return {SmartPointerUseKind::Unsupported, binaryExprAST};
                    return {SmartPointerUseKind::AssignMakeCall, binaryExprAST};
                }
                // unique_ptr/QScopedPointer have no operator=(T*) - only operator=(unique_ptr&&)
                // and operator=(nullptr_t). So if this assignment compiled at all, and symbol is
                // *currently* a smart pointer, expr's type is already guaranteed by the language
                // itself to be a foreign smart-pointer rvalue (which can't be unwrapped into a
                // raw-pointer equivalent) - no expression-type detection needed, unlike the
                // constructor-style case in isSupportedSmartPointerInitializer(), which also
                // accepts a plain raw pointer via unique_ptr's explicit T* constructor and so
                // can't take this shortcut. A raw pointer's own reassignment has no such
                // restriction (any raw-pointer-valued expr is fine), hence the direction check.
                if (symbolIsSmartPointer && !rhs->asPointerLiteral())
                    return {SmartPointerUseKind::Unsupported, binaryExprAST};
                // The mirror image of isSupportedRawPointerInitializer(): a raw pointer's own
                // reassignment from "&local" has the exact same non-owning-address problem once
                // wrapped into ".reset(&local)".
                if (!symbolIsSmartPointer && isAddressOfExpression(file, rhs))
                    return {SmartPointerUseKind::Unsupported, binaryExprAST};
            }
            // The two-statement rewrite ("delete x;\nx = expr;") only happens when converting
            // *from* a smart pointer (symbolIsSmartPointer); the other direction's single-
            // statement ".reset(expr)" rewrite has no such control-flow risk.
            if (symbolIsSmartPointer && !canSplitIntoTwoStatements(path, i, binaryExprAST))
                return {SmartPointerUseKind::Unsupported, binaryExprAST};
            return {SmartPointerUseKind::AssignExpr, binaryExprAST};
        } else if (ast->asFunctionDefinition()) {
            break;
        }
    }
    return {SmartPointerUseKind::BareUse, nullptr};
}

class ConvertFromAndToPointerOp : public CppQuickFixOperation
{
public:
    enum Mode { FromPointer, FromVariable, FromReference, ToSmartPointer, FromSmartPointer };

    ConvertFromAndToPointerOp(
        const CppQuickFixInterface &interface,
        int priority,
        Mode mode,
        bool isAutoDeclaration,
        const SimpleDeclarationAST *simpleDeclaration,
        const DeclaratorAST *declaratorAST,
        const SimpleNameAST *identifierAST,
        Symbol *symbol,
        SmartPointerKind smartPointerKind = SmartPointerKind::UniquePtr)
        : CppQuickFixOperation(interface, priority)
        , m_mode(mode)
        , m_isAutoDeclaration(isAutoDeclaration)
        , m_simpleDeclaration(simpleDeclaration)
        , m_declaratorAST(declaratorAST)
        , m_identifierAST(identifierAST)
        , m_symbol(symbol)
        , m_smartPointerKind(smartPointerKind)
        , m_refactoring(snapshot())
        , m_file(currentFile())
        , m_document(interface.semanticInfo().doc)
    {
        switch (mode) {
        case FromPointer:
            setDescription(Tr::tr("Convert to Stack Variable"));
            break;
        case ToSmartPointer:
            setDescription(Tr::tr("Convert to %1").arg(smartPointerTypeName(smartPointerKind)));
            break;
        default:
            // FromVariable, FromReference, FromSmartPointer.
            setDescription(Tr::tr("Convert to Raw Pointer"));
            break;
        }
    }

    void perform() override
    {
        ChangeSet changes;

        switch (m_mode) {
        case FromPointer:
            removePointerOperator(changes);
            convertToStackVariable(changes);
            break;
        case FromReference:
            removeReferenceOperator(changes);
            Q_FALLTHROUGH();
        case FromVariable:
            convertToPointer(changes);
            break;
        case ToSmartPointer:
            convertToSmartPointer(changes);
            break;
        case FromSmartPointer:
            convertFromSmartPointer(changes);
            break;
        }

        m_file->apply(changes);
    }

private:
    void removePointerOperator(ChangeSet &changes) const
    {
        if (!m_declaratorAST->ptr_operator_list)
            return;
        PointerAST *ptrAST = m_declaratorAST->ptr_operator_list->value->asPointer();
        QTC_ASSERT(ptrAST, return);
        const int pos = m_file->startOf(ptrAST->star_token);
        changes.remove(pos, pos + 1);
    }

    void removeReferenceOperator(ChangeSet &changes) const
    {
        ReferenceAST *refAST = m_declaratorAST->ptr_operator_list->value->asReference();
        QTC_ASSERT(refAST, return);
        const int pos = m_file->startOf(refAST->reference_token);
        changes.remove(pos, pos + 1);
    }

    void removeNewExpression(ChangeSet &changes, NewExpressionAST *newExprAST) const
    {
        ExpressionListAST *exprlist = nullptr;
        if (newExprAST->new_initializer) {
            if (ExpressionListParenAST *ast = newExprAST->new_initializer->asExpressionListParen())
                exprlist = ast->expression_list;
            else if (BracedInitializerAST *ast = newExprAST->new_initializer->asBracedInitializer())
                exprlist = ast->expression_list;
        }

        if (exprlist) {
            // remove 'new' keyword and type before initializer
            changes.remove(m_file->startOf(newExprAST->new_token),
                           m_file->startOf(newExprAST->new_initializer));

            changes.remove(m_file->endOf(m_declaratorAST->equal_token - 1),
                           m_file->startOf(m_declaratorAST->equal_token + 1));
        } else {
            // remove the whole new expression
            changes.remove(m_file->endOf(m_identifierAST->firstToken()),
                           m_file->startOf(newExprAST->lastToken()));
        }
    }

    void removeNewKeyword(ChangeSet &changes, NewExpressionAST *newExprAST) const
    {
        // remove 'new' keyword before initializer
        changes.remove(m_file->startOf(newExprAST->new_token),
                       m_file->startOf(newExprAST->new_type_id));
    }

    void convertToStackVariable(ChangeSet &changes) const
    {
        // Handle the initializer.
        if (m_declaratorAST->initializer) {
            if (NewExpressionAST *newExpression = m_declaratorAST->initializer->asNewExpression()) {
                if (m_isAutoDeclaration) {
                    if (!newExpression->new_initializer)
                        changes.insert(m_file->endOf(newExpression), QStringLiteral("()"));
                    removeNewKeyword(changes, newExpression);
                } else {
                    removeNewExpression(changes, newExpression);
                }
            }
        }

        // Fix all occurrences of the identifier in this function.
        ASTPath astPath(m_document);
        const QList<SemanticInfo::Use> uses = semanticInfo().localUses.value(m_symbol);
        for (const SemanticInfo::Use &use : uses) {
            const QList<AST *> path = astPath(use.line, use.column);
            AST *idAST = path.last();
            bool declarationFound = false;
            bool starFound = false;
            int ampersandPos = 0;
            bool memberAccess = false;
            bool deleteCall = false;

            for (int i = path.count() - 2; i >= 0; --i) {
                if (path.at(i) == m_declaratorAST) {
                    declarationFound = true;
                    break;
                }
                if (MemberAccessAST *memberAccessAST = path.at(i)->asMemberAccess()) {
                    if (m_file->tokenAt(memberAccessAST->access_token).kind() != T_ARROW)
                        continue;
                    int pos = m_file->startOf(memberAccessAST->access_token);
                    changes.replace(pos, pos + 2, QLatin1String("."));
                    memberAccess = true;
                    break;
                } else if (DeleteExpressionAST *deleteAST = path.at(i)->asDeleteExpression()) {
                    const int pos = m_file->startOf(deleteAST->delete_token);
                    changes.insert(pos, QLatin1String("// "));
                    deleteCall = true;
                    break;
                } else if (UnaryExpressionAST *unaryExprAST = path.at(i)->asUnaryExpression()) {
                    const Token tk = m_file->tokenAt(unaryExprAST->unary_op_token);
                    if (tk.kind() == T_STAR) {
                        if (!starFound) {
                            int pos = m_file->startOf(unaryExprAST->unary_op_token);
                            changes.remove(pos, pos + 1);
                        }
                        starFound = true;
                    } else if (tk.kind() == T_AMPER) {
                        ampersandPos = m_file->startOf(unaryExprAST->unary_op_token);
                    }
                } else if (PointerAST *ptrAST = path.at(i)->asPointer()) {
                    if (!starFound) {
                        const int pos = m_file->startOf(ptrAST->star_token);
                        changes.remove(pos, pos);
                    }
                    starFound = true;
                } else if (path.at(i)->asFunctionDefinition()) {
                    break;
                }
            }
            if (!declarationFound && !starFound && !memberAccess && !deleteCall) {
                if (ampersandPos) {
                    changes.insert(ampersandPos, QLatin1String("&("));
                    changes.insert(m_file->endOf(idAST->firstToken()), QLatin1String(")"));
                } else {
                    changes.insert(m_file->startOf(idAST), QLatin1String("&"));
                }
            }
        }
    }

    QString typeNameOfDeclaration() const
    {
        if (!m_simpleDeclaration
            || !m_simpleDeclaration->decl_specifier_list
            || !m_simpleDeclaration->decl_specifier_list->value) {
            return QString();
        }
        NamedTypeSpecifierAST *namedType
            = m_simpleDeclaration->decl_specifier_list->value->asNamedTypeSpecifier();
        if (!namedType)
            return QString();

        Overview overview;
        return overview.prettyName(namedType->name->name);
    }

    void insertNewExpression(ChangeSet &changes, ExpressionAST *ast) const
    {
        const QString typeName = typeNameOfDeclaration();
        if (CallAST *callAST = ast->asCall()) {
            if (typeName.isEmpty()) {
                changes.insert(m_file->startOf(callAST), QLatin1String("new "));
            } else {
                changes.insert(m_file->startOf(callAST),
                               QLatin1String("new ") + typeName + QLatin1Char('('));
                changes.insert(m_file->startOf(callAST->lastToken()), QLatin1String(")"));
            }
        } else {
            if (typeName.isEmpty())
                return;
            changes.insert(m_file->startOf(ast), QLatin1String(" = new ") + typeName);
        }
    }

    void insertNewExpression(ChangeSet &changes) const
    {
        const QString typeName = typeNameOfDeclaration();
        if (typeName.isEmpty())
            return;
        changes.insert(m_file->endOf(m_identifierAST->firstToken()),
                       QLatin1String(" = new ") + typeName);
    }

    void convertToPointer(ChangeSet &changes) const
    {
        // Handle initializer.
        if (m_declaratorAST->initializer) {
            if (IdExpressionAST *idExprAST = m_declaratorAST->initializer->asIdExpression()) {
                changes.insert(m_file->startOf(idExprAST), QLatin1String("&"));
            } else if (CallAST *callAST = m_declaratorAST->initializer->asCall()) {
                insertNewExpression(changes, callAST);
            } else if (ExpressionListParenAST *exprListAST = m_declaratorAST->initializer
                                                                 ->asExpressionListParen()) {
                insertNewExpression(changes, exprListAST);
            } else if (BracedInitializerAST *bracedInitializerAST = m_declaratorAST->initializer
                                                                        ->asBracedInitializer()) {
                insertNewExpression(changes, bracedInitializerAST);
            }
        } else {
            insertNewExpression(changes);
        }

        // Fix all occurrences of the identifier in this function.
        ASTPath astPath(m_document);
        const QList<SemanticInfo::Use> uses = semanticInfo().localUses.value(m_symbol);
        for (const SemanticInfo::Use &use : uses) {
            const QList<AST *> path = astPath(use.line, use.column);
            AST *idAST = path.last();
            bool insertStar = true;
            for (int i = path.count() - 2; i >= 0; --i) {
                if (m_isAutoDeclaration && path.at(i) == m_declaratorAST) {
                    insertStar = false;
                    break;
                }
                if (MemberAccessAST *memberAccessAST = path.at(i)->asMemberAccess()) {
                    const int pos = m_file->startOf(memberAccessAST->access_token);
                    changes.replace(pos, pos + 1, QLatin1String("->"));
                    insertStar = false;
                    break;
                } else if (UnaryExpressionAST *unaryExprAST = path.at(i)->asUnaryExpression()) {
                    if (m_file->tokenAt(unaryExprAST->unary_op_token).kind() == T_AMPER) {
                        const int pos = m_file->startOf(unaryExprAST->unary_op_token);
                        changes.remove(pos, pos + 1);
                        insertStar = false;
                        break;
                    }
                } else if (path.at(i)->asFunctionDefinition()) {
                    break;
                }
            }
            if (insertStar)
                changes.insert(m_file->startOf(idAST), QLatin1String("*"));
        }
    }

    // The pointee type's name: "Foo" for both "Foo *f" and "std::unique_ptr<Foo> f" - computed
    // from the symbol's current (pre-conversion) type rather than threaded through from
    // doMatch(), since it is derivable either way and this keeps the constructor unchanged.
    QString elementTypeName() const
    {
        const FullySpecifiedType type = m_symbol->type();
        if (PointerType * const ptrType = type.type()->asPointerType())
            return Overview().prettyType(ptrType->elementType());
        if (const auto smart = smartPointerElementType(type.type()))
            return Overview().prettyType(smart->second);
        return {};
    }

    static QString argumentListText(const CppRefactoringFilePtr &file, ExpressionListAST *list)
    {
        QStringList args;
        for (ExpressionListAST *it = list; it; it = it->next) {
            if (it->value)
                args << file->textOf(it->value);
        }
        return args.join(QLatin1String(", "));
    }

    // The type argument spelled out in a "make_unique<T>(...)" call, e.g. "Derived" for
    // "std::make_unique<Derived>(x)", or an empty string if none is written out (in practice,
    // make_unique always needs one, since its return type cannot otherwise be deduced).
    static QString makeUniqueTemplateArgumentText(
        const CppRefactoringFilePtr &file, CallAST *makeCallAST)
    {
        IdExpressionAST * const idExpr = makeCallAST->base_expression
                                              ? makeCallAST->base_expression->asIdExpression()
                                              : nullptr;
        NameAST *nameAst = idExpr ? idExpr->name : nullptr;
        if (QualifiedNameAST * const qualifiedName = nameAst ? nameAst->asQualifiedName() : nullptr)
            nameAst = qualifiedName->unqualified_name;
        TemplateIdAST * const templateId = nameAst ? nameAst->asTemplateId() : nullptr;
        if (!templateId || !templateId->template_argument_list
            || !templateId->template_argument_list->value) {
            return {};
        }
        return file->textOf(templateId->template_argument_list->value);
    }

    // Turns "std::make_unique<Foo>(1, 2)" into "new Foo(1, 2)" - using make_unique's own
    // spelled-out type argument rather than the declared pointee type, since they can legitimately
    // differ (e.g. std::unique_ptr<Base> constructed from std::make_unique<Derived>(...)), and
    // only the actual constructed type reproduces the original behavior correctly.
    QString rewriteMakeCallToNew(CallAST *makeCallAST) const
    {
        const QString typeArgText = makeUniqueTemplateArgumentText(m_file, makeCallAST);
        return QLatin1String("new ") + (typeArgText.isEmpty() ? elementTypeName() : typeArgText)
               + QLatin1Char('(') + argumentListText(m_file, makeCallAST->expression_list)
               + QLatin1Char(')');
    }

    void wrapDeclarationType(ChangeSet &changes) const
    {
        if (!m_simpleDeclaration || !m_simpleDeclaration->decl_specifier_list
            || !m_simpleDeclaration->decl_specifier_list->value) {
            return;
        }
        const ChangeSet::Range range = m_file->range(
            m_simpleDeclaration->decl_specifier_list->value);
        changes.replace(
            range,
            smartPointerTypeName(m_smartPointerKind) + QLatin1Char('<') + elementTypeName()
                + QLatin1Char('>'));
    }

    void unwrapDeclarationType(ChangeSet &changes) const
    {
        if (!m_simpleDeclaration || !m_simpleDeclaration->decl_specifier_list
            || !m_simpleDeclaration->decl_specifier_list->value) {
            return;
        }
        // Unlike wrapDeclarationType() (where the star ends up adjacent to the identifier
        // naturally, once the caller separately removes it from the declarator), here the
        // replaced range must extend through the original whitespace up to the identifier
        // itself, or that whitespace would be preserved in addition to our own, leaving
        // "Foo * f" instead of "Foo *f".
        changes.replace(
            m_file->startOf(m_simpleDeclaration->decl_specifier_list->value),
            m_file->startOf(m_identifierAST),
            elementTypeName() + QLatin1String(" *"));
    }

    // Shared per-use rewriting for both smart-pointer directions; which ClassifiedSmartPointerUse
    // kinds are actually reachable depends on which type the variable currently has (see
    // classifySmartPointerUse()).
    void rewriteSmartPointerUses(ChangeSet &changes, bool toSmartPointer) const
    {
        ASTPath astPath(m_document);
        const QList<SemanticInfo::Use> uses = semanticInfo().localUses.value(m_symbol);
        for (const SemanticInfo::Use &use : uses) {
            const QList<AST *> path = astPath(use.line, use.column);
            AST * const idAST = path.last();
            // m_symbol's declared type at this point (before the changeset is applied) is
            // whatever it was *before* this conversion, i.e. the opposite of toSmartPointer.
            const ClassifiedSmartPointerUse classified
                = classifySmartPointerUse(m_file, path, m_declaratorAST, !toSmartPointer);
            const QString idText = m_file->textOf(idAST);
            switch (classified.kind) {
            case SmartPointerUseKind::Declaration: // Handled elsewhere.
            case SmartPointerUseKind::PointeeAccess:
            case SmartPointerUseKind::Unsupported:
                break;
            case SmartPointerUseKind::BareUse:
                if (toSmartPointer)
                    changes.insert(m_file->endOf(idAST), QLatin1String(".get()"));
                break;
            case SmartPointerUseKind::GetCall:
                if (!toSmartPointer) {
                    if (CallAST * const callAST = classified.node->asCall())
                        changes.remove(m_file->endOf(idAST), m_file->endOf(callAST));
                }
                break;
            case SmartPointerUseKind::ResetNoArgs:
                if (!toSmartPointer) {
                    if (CallAST * const callAST = classified.node->asCall()) {
                        changes.replace(
                            m_file->startOf(idAST),
                            m_file->endOf(callAST),
                            QLatin1String("delete ") + idText + QLatin1String(";\n") + idText
                                + QLatin1String(" = nullptr"));
                    }
                }
                break;
            case SmartPointerUseKind::ResetWithArg:
                if (!toSmartPointer) {
                    if (CallAST * const callAST = classified.node->asCall()) {
                        const QString argText = argumentListText(m_file, callAST->expression_list);
                        changes.replace(
                            m_file->startOf(idAST),
                            m_file->endOf(callAST),
                            QLatin1String("delete ") + idText + QLatin1String(";\n") + idText
                                + QLatin1String(" = ") + argText);
                    }
                }
                break;
            case SmartPointerUseKind::DeleteCall:
                if (toSmartPointer) {
                    if (DeleteExpressionAST * const deleteAST
                        = classified.node->asDeleteExpression()) {
                        changes.replace(m_file->range(deleteAST), idText + QLatin1String(".reset()"));
                    }
                }
                break;
            case SmartPointerUseKind::AssignExpr:
                if (BinaryExpressionAST * const binExpr = classified.node->asBinaryExpression()) {
                    const QString rhsText = m_file->textOf(binExpr->right_expression);
                    if (toSmartPointer) {
                        changes.replace(
                            m_file->range(binExpr),
                            idText + QLatin1String(".reset(") + rhsText + QLatin1Char(')'));
                    } else {
                        changes.replace(
                            m_file->startOf(idAST),
                            m_file->endOf(binExpr),
                            QLatin1String("delete ") + idText + QLatin1String(";\n") + idText
                                + QLatin1String(" = ") + rhsText);
                    }
                }
                break;
            case SmartPointerUseKind::AssignMakeCall:
                if (!toSmartPointer) {
                    if (BinaryExpressionAST * const binExpr = classified.node->asBinaryExpression()) {
                        if (CallAST * const rhsCall = binExpr->right_expression->asCall()) {
                            changes.replace(
                                m_file->startOf(idAST),
                                m_file->endOf(binExpr),
                                QLatin1String("delete ") + idText + QLatin1String(";\n") + idText
                                    + QLatin1String(" = ") + rewriteMakeCallToNew(rhsCall));
                        }
                    }
                }
                break;
            }
        }
    }

    void convertToSmartPointer(ChangeSet &changes) const
    {
        wrapDeclarationType(changes);
        removePointerOperator(changes);

        if (ExpressionAST * const init = m_declaratorAST->initializer) {
            // "Foo *f = expr;" -> "std::unique_ptr<Foo> f(expr);" for any expr (new, another
            // pointer, a function call, ...) - replace everything between the identifier and
            // expr (not just the "=" token) so no stray space is left behind, mirroring how
            // removeNewExpression() above already takes care to do this for the opposite
            // (pointer-to-stack) direction.
            changes.replace(
                m_file->endOf(m_identifierAST->firstToken()),
                m_file->startOf(init),
                QLatin1String("("));
            changes.insert(m_file->endOf(init), QLatin1String(")"));
        }

        rewriteSmartPointerUses(changes, /*toSmartPointer=*/true);

        const QString include = missingIncludeForSmartPointer(m_document, m_smartPointerKind);
        if (!include.isEmpty())
            insertNewIncludeDirective(include, m_file, m_document, changes);
    }

    void convertFromSmartPointer(ChangeSet &changes) const
    {
        unwrapDeclarationType(changes);

        if (m_declaratorAST->initializer) {
            if (ExpressionListParenAST * const parenInit
                = m_declaratorAST->initializer->asExpressionListParen()) {
                if (parenInit->expression_list && parenInit->expression_list->value) {
                    ExpressionAST * const inner = parenInit->expression_list->value;
                    // "Type<Foo> f(new Foo(...));" -> "Foo *f = new Foo(...);"
                    changes.replace(
                        m_file->startOf(parenInit), m_file->startOf(inner), QLatin1String(" = "));
                    changes.remove(m_file->endOf(inner), m_file->endOf(parenInit));
                }
            } else if (CallAST * const callInit = m_declaratorAST->initializer->asCall()) {
                // "= std::make_unique<Foo>(...);" -> "= new Foo(...);"
                changes.replace(m_file->range(callInit), rewriteMakeCallToNew(callInit));
            }
        } else {
            changes.insert(m_file->endOf(m_identifierAST->firstToken()), QLatin1String(" = nullptr"));
        }

        rewriteSmartPointerUses(changes, /*toSmartPointer=*/false);
    }

    const Mode m_mode;
    const bool m_isAutoDeclaration;
    const SimpleDeclarationAST * const m_simpleDeclaration;
    const DeclaratorAST * const m_declaratorAST;
    const SimpleNameAST * const m_identifierAST;
    Symbol * const m_symbol;
    const SmartPointerKind m_smartPointerKind;
    const CppRefactoringChanges m_refactoring;
    const CppRefactoringFilePtr m_file;
    const Document::Ptr m_document;
};

// A raw pointer's own "= expr;" initializer generally needs no restriction: wrapping any single
// expression as "(expr)" to turn it into a smart pointer's constructor argument works regardless
// of what expr is (new, another pointer, a function call, ...) - except for isAddressOfExpression()
// above.
bool isSupportedRawPointerInitializer(const CppRefactoringFilePtr &file, ExpressionAST *initializer)
{
    if (!initializer)
        return true;
    return !isAddressOfExpression(file, initializer);
}

bool isSupportedSmartPointerInitializer(
    const CppQuickFixInterface &interface,
    const CppRefactoringFilePtr &file,
    ExpressionAST *initializer)
{
    if (!initializer)
        return true;
    if (ExpressionListParenAST * const parenInit = initializer->asExpressionListParen()) {
        // Constructor-style "Type<Foo> f(expr);" - same reasoning as above: expr's shape
        // doesn't matter, it just needs to be exactly one argument - except one that is itself
        // smart-pointer-typed (e.g. "std::move(other)", or any other function returning a
        // unique_ptr by value), which cannot be unwrapped into a raw-pointer
        // equivalent. stripParens() so a redundantly-parenthesized argument (e.g. "((expr))",
        // see isMoveCall() for why that shape matters) classifies the same as an unparenthesized
        // one.
        if (!parenInit->expression_list || !parenInit->expression_list->value
            || parenInit->expression_list->next) {
            return false;
        }
        ExpressionAST * const arg = stripParens(parenInit->expression_list->value);
        return !isMoveCall(arg) && !isSmartPointerTypedExpression(interface, file, arg);
    }
    // Assignment-style "Type<Foo> f = someCall();" - only a make_unique call qualifies: the
    // declared type here is unique_ptr/QScopedPointer, and neither can be initialized from a
    // make_shared call, so that shape would never have compiled in the first place.
    if (initializer->asCall())
        return isMakeUniqueCall(initializer);
    // Plain "Type<Foo> f = nullptr;": needs no rewriting at all, since nullptr is a valid
    // initializer for both a smart and a raw pointer.
    if (initializer->asPointerLiteral())
        return true;
    return false;
}

// Pre-validates every local use of the symbol, so the quickfix can simply not be offered if any
// use doesn't fit a recognized shape, rather than emitting a best-effort (and possibly wrong)
// rewrite for it - see classifySmartPointerUse() for why this matters more here than it does
// for the raw-pointer<->stack/reference directions.
bool allSmartPointerUsesSupported(
    const CppQuickFixInterface &interface, Symbol *symbol, const DeclaratorAST *declarator)
{
    ASTPath astPath(interface.semanticInfo().doc);
    const CppRefactoringFilePtr file = interface.currentFile();
    const QList<SemanticInfo::Use> uses = interface.semanticInfo().localUses.value(symbol);
    const bool symbolIsSmartPointer = smartPointerElementType(symbol->type().type()).has_value();
    for (const SemanticInfo::Use &use : uses) {
        const QList<AST *> path = astPath(use.line, use.column);
        if (path.isEmpty())
            continue;
        if (classifySmartPointerUse(file, path, declarator, symbolIsSmartPointer).kind
            == SmartPointerUseKind::Unsupported) {
            return false;
        }
    }
    return true;
}

/*!
  Converts the selected variable to a pointer if it is a stack variable or reference, or vice versa.
  Activates on variable declarations.
 */
class ConvertFromAndToPointer : public CppQuickFixFactory
{
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        if (path.count() < 2)
            return;
        SimpleNameAST *identifier = path.last()->asSimpleName();
        if (!identifier)
            return;
        SimpleDeclarationAST *simpleDeclaration = nullptr;
        DeclaratorAST *declarator = nullptr;
        bool isFunctionLocal = false;
        bool isClassLocal = false;
        ConvertFromAndToPointerOp::Mode mode = ConvertFromAndToPointerOp::FromVariable;
        for (int i = path.count() - 2; i >= 0; --i) {
            AST *ast = path.at(i);
            if (!declarator && (declarator = ast->asDeclarator()))
                continue;
            if (!simpleDeclaration && (simpleDeclaration = ast->asSimpleDeclaration()))
                continue;
            if (declarator && simpleDeclaration) {
                if (ast->asClassSpecifier()) {
                    isClassLocal = true;
                } else if (ast->asFunctionDefinition() && !isClassLocal) {
                    isFunctionLocal = true;
                    break;
                }
            }
        }
        if (!isFunctionLocal || !simpleDeclaration || !declarator)
            return;

        Symbol *symbol = nullptr;
        for (List<Symbol *> *lst = simpleDeclaration->symbols; lst; lst = lst->next) {
            if (lst->value->name() == identifier->name) {
                symbol = lst->value;
                break;
            }
        }
        if (!symbol)
            return;

        bool isAutoDeclaration = false;
        if (symbol->storage() == Symbol::Auto) {
            // For auto variables we must deduce the type from the initializer.
            if (!declarator->initializer)
                return;

            isAutoDeclaration = true;
            TypeOfExpression typeOfExpression;
            typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot());
            typeOfExpression.setExpandTemplates(true);
            CppRefactoringFilePtr file = interface.currentFile();
            Scope *scope = file->scopeAt(declarator->firstToken());
            QList<LookupItem> result = typeOfExpression(file->textOf(declarator->initializer).toUtf8(),
                                                        scope, TypeOfExpression::Preprocess);
            if (!result.isEmpty() && result.first().type()->asPointerType())
                mode = ConvertFromAndToPointerOp::FromPointer;
        } else if (declarator->ptr_operator_list) {
            for (PtrOperatorListAST *ops = declarator->ptr_operator_list; ops; ops = ops->next) {
                if (ops != declarator->ptr_operator_list) {
                    // Bail out on more complex pointer types (e.g. pointer of pointer,
                    // or reference of pointer).
                    return;
                }
                if (ops->value->asPointer())
                    mode = ConvertFromAndToPointerOp::FromPointer;
                else if (ops->value->asReference())
                    mode = ConvertFromAndToPointerOp::FromReference;
            }
        } else if (!isAutoDeclaration) {
            // std::unique_ptr<T>/QScopedPointer<T> (function-local, spelled-out type only -
            // auto-declared smart pointers are out of scope for now).
            if (const auto smart = smartPointerElementType(symbol->type().type())) {
                if (!hasSinglePlainDeclarator(simpleDeclaration)
                    || !isSupportedSmartPointerInitializer(
                        interface, interface.currentFile(), declarator->initializer)
                    || !allSmartPointerUsesSupported(interface, symbol, declarator)) {
                    return;
                }
                const int priority = path.size() - 1;
                result << new ConvertFromAndToPointerOp(
                    interface,
                    priority,
                    ConvertFromAndToPointerOp::FromSmartPointer,
                    isAutoDeclaration,
                    simpleDeclaration,
                    declarator,
                    identifier,
                    symbol,
                    smart->first);
                return;
            }
        }

        const int priority = path.size() - 1;
        result << new ConvertFromAndToPointerOp(interface, priority, mode, isAutoDeclaration,
                                                simpleDeclaration, declarator, identifier, symbol);

        if (mode == ConvertFromAndToPointerOp::FromPointer
            && hasSinglePlainDeclarator(simpleDeclaration)
            && isSupportedRawPointerInitializer(interface.currentFile(), declarator->initializer)
            && allSmartPointerUsesSupported(interface, symbol, declarator)) {
            QList<SmartPointerKind> offeredKinds{SmartPointerKind::UniquePtr};
            if (isQScopedPointerAvailable(testMode()))
                offeredKinds << SmartPointerKind::ScopedPointer;
            for (const SmartPointerKind kind : offeredKinds) {
                result << new ConvertFromAndToPointerOp(
                    interface,
                    priority,
                    ConvertFromAndToPointerOp::ToSmartPointer,
                    isAutoDeclaration,
                    simpleDeclaration,
                    declarator,
                    identifier,
                    symbol,
                    kind);
            }
        }
    }
};

#ifdef WITH_TESTS
class ConvertFromAndToPointerTest : public Tests::CppQuickFixTestObject
{
    Q_OBJECT
public:
    using CppQuickFixTestObject::CppQuickFixTestObject;
};
#endif

} // namespace

void registerConvertFromAndToPointerQuickfix()
{
    REGISTER_QUICKFIX_FACTORY_WITH_STANDARD_TEST(ConvertFromAndToPointer);
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <convertfromandtopointer.moc>
#endif
