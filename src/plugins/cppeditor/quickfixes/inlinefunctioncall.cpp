// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "inlinefunctioncall.h"

#include "../cppeditortr.h"
#include "../cppfindreferences.h"
#include "../cppmodelmanager.h"
#include "../cpprefactoringchanges.h"
#include "../cppworkingcopy.h"
#include "../functionutils.h"
#include "../symbolfinder.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"

#include <coreplugin/messagemanager.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/FindUsages.h>
#include <cplusplus/Matcher.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Token.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/declarationcomments.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/result.h>

#include <algorithm>
#include <optional>

#include <QHash>
#include <QSet>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

enum class InlineBodyShape { Empty, Statement, ReturnExpr };

struct ParameterReference
{
    ChangeSet::Range range;
    int parameterIndex = -1;
};

struct DefaultValueInfo
{
    ChangeSet::Range range;
    bool isPrimaryExpression = false;
};

struct InlineBodyInfo
{
    InlineBodyShape shape = InlineBodyShape::Empty;
    AST *content = nullptr;
    QList<ParameterReference> parameterReferences;
    QHash<int, DefaultValueInfo> defaultValues;
};

// An id-expression or literal can never need parentheses for precedence reasons, unlike an
// arbitrary expression; leaving them unparenthesized avoids visual noise for the common case.
bool isPrimaryExpression(ExpressionAST *expr)
{
    if (!expr)
        return false;
    if (IdExpressionAST * const idExpr = expr->asIdExpression())
        return idExpr->name && idExpr->name->asSimpleName();
    return expr->asNumericLiteral() || expr->asBoolLiteral() || expr->asStringLiteral()
           || expr->asPointerLiteral();
}

struct RawParameterReference
{
    IdExpressionAST *ast = nullptr;
    int parameterIndex = -1;
};

struct CalleeInfo
{
    // The declaration symbol is needed for checks on virtualness, template-ness etc.
    Function *declarationSymbol = nullptr;
    Function *definitionSymbol = nullptr;
    FunctionDefinitionAST *definitionAst = nullptr;
    CppRefactoringFilePtr definitionFile;
};

// Maps a Function symbol back to its AST.
FunctionDefinitionAST *functionDefinitionAst(Function *func, const CppRefactoringFilePtr &file)
{
    if (!file || !file->cppDocument() || !file->cppDocument()->translationUnit()
        || !file->cppDocument()->translationUnit()->ast()) {
        return nullptr;
    }
    const QList<AST *> path = ASTPath(file->cppDocument())(func->line(), func->column());
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        if (FunctionDefinitionAST * const funcDef = (*it)->asFunctionDefinition())
            return funcDef->symbol == func ? funcDef : nullptr;
    }
    return nullptr;
}

FunctionDeclaratorAST *functionDeclaratorAst(FunctionDefinitionAST *funcDefAst, Function *func)
{
    if (!funcDefAst->declarator)
        return nullptr;
    for (PostfixDeclaratorListAST *it = funcDefAst->declarator->postfix_declarator_list; it;
         it = it->next) {
        if (FunctionDeclaratorAST * const funcDecl = it->value->asFunctionDeclarator()) {
            if (funcDecl->symbol == func)
                return funcDecl;
        }
    }
    return nullptr;
}

ParameterDeclarationAST *parameterDeclarationAst(FunctionDeclaratorAST *funcDeclAst, int index)
{
    if (!funcDeclAst || !funcDeclAst->parameter_declaration_clause)
        return nullptr;
    int i = 0;
    for (ParameterDeclarationListAST *it
         = funcDeclAst->parameter_declaration_clause->parameter_declaration_list;
         it;
         it = it->next, ++i) {
        if (i == index)
            return it->value;
    }
    return nullptr;
}

// The argument the call supplies for the given parameter, or null if the call relies on that
// parameter's default value (or omits it outright).
ExpressionAST *callArgumentAt(CallAST *callAST, int index)
{
    int i = 0;
    for (ExpressionListAST *it = callAST->expression_list; it; it = it->next, ++i) {
        if (i == index)
            return it->value;
    }
    return nullptr;
}

// Every parameter the body references either needs the call to supply an argument for it, or
// needs a default value analyzeBody() actually found and safety-checked. A default declared
// only on a separate declaration (the usual header/source split) is not visible from the
// definition's declarator alone, so it would not be found there - without this check, such a
// parameter would look like it's "covered" by a default that was never actually verified to
// exist, and argumentText() would trip its own QTC_ASSERT.
bool callSuppliesRequiredArguments(CallAST *callAST, const InlineBodyInfo &bodyInfo)
{
    for (const ParameterReference &ref : bodyInfo.parameterReferences) {
        if (!bodyInfo.defaultValues.contains(ref.parameterIndex)
            && !callArgumentAt(callAST, ref.parameterIndex)) {
            return false;
        }
    }
    return true;
}

bool isFunctionTemplate(Function *func)
{
    for (Scope *scope = func->enclosingScope(); scope; scope = scope->enclosingScope()) {
        if (scope->asTemplate())
            return true;
    }
    return false;
}

bool isAssignmentOperator(int tokenKind)
{
    switch (tokenKind) {
    case T_EQUAL:
    case T_PLUS_EQUAL:
    case T_MINUS_EQUAL:
    case T_STAR_EQUAL:
    case T_SLASH_EQUAL:
    case T_PERCENT_EQUAL:
    case T_AMPER_EQUAL:
    case T_PIPE_EQUAL:
    case T_CARET_EQUAL:
    case T_LESS_LESS_EQUAL:
    case T_GREATER_GREATER_EQUAL:
        return true;
    default:
        return false;
    }
}

// Walks a single statement or expression and determines whether it references anything but
// the callee's own parameters (no other identifiers at all: no calls to other functions, no
// member access, no "this", no reference to a named type), and whether it writes to one of
// them: a by-value parameter can be freely assigned to inside the callee without the caller
// ever seeing it, but substituting the argument's own text in its place would turn that into
// a real mutation of whatever the caller passed in.
class ReferenceChecker : public ASTVisitor
{
public:
    ReferenceChecker(TranslationUnit *unit, Function *func)
        : ASTVisitor(unit)
        , m_func(func)
    {}

    bool isSafe() const { return m_safe; }
    const QList<RawParameterReference> &parameterReferences() const { return m_paramRefs; }

private:
    int parameterIndexOf(ExpressionAST *expr) const
    {
        while (NestedExpressionAST * const nested = expr ? expr->asNestedExpression() : nullptr)
            expr = nested->expression;
        IdExpressionAST * const idExpr = expr ? expr->asIdExpression() : nullptr;
        SimpleNameAST * const simpleName = idExpr && idExpr->name ? idExpr->name->asSimpleName()
                                                                 : nullptr;
        if (!simpleName)
            return -1;
        const Identifier * const id = identifier(simpleName->identifier_token);
        for (int i = 0, count = m_func->argumentCount(); i < count; ++i) {
            Argument * const arg = m_func->argumentAt(i)->asArgument();
            if (arg && arg->identifier() && id && arg->identifier()->equalTo(id))
                return i;
        }
        return -1;
    }

    // Writing to a by-value parameter only ever mutates the callee's own local copy, which
    // substitution would incorrectly turn into a write to whatever the caller passed in. A
    // reference parameter is different: writing to it is meant to be visible to the caller,
    // and substitution reproduces that correctly.
    bool writesToByValueParameter(ExpressionAST *expr) const
    {
        const int paramIndex = parameterIndexOf(expr);
        if (paramIndex < 0)
            return false;
        Argument * const arg = m_func->argumentAt(paramIndex)->asArgument();
        return !arg->type()->asReferenceType();
    }

    bool visit(IdExpressionAST *ast) override
    {
        const int paramIndex = parameterIndexOf(ast);
        if (paramIndex >= 0) {
            m_paramRefs.append({ast, paramIndex});
            return false;
        }
        m_safe = false;
        return false;
    }

    bool visit(BinaryExpressionAST *ast) override
    {
        if (isAssignmentOperator(tokenKind(ast->binary_op_token))
            && writesToByValueParameter(ast->left_expression)) {
            m_safe = false;
        }
        return true;
    }

    bool visit(UnaryExpressionAST *ast) override
    {
        const int kind = tokenKind(ast->unary_op_token);
        if ((kind == T_PLUS_PLUS || kind == T_MINUS_MINUS)
            && writesToByValueParameter(ast->expression)) {
            m_safe = false;
        }
        return true;
    }

    bool visit(PostIncrDecrAST *ast) override
    {
        if (writesToByValueParameter(ast->base_expression))
            m_safe = false;
        return true;
    }

    bool visit(MemberAccessAST *) override
    {
        m_safe = false;
        return false;
    }
    bool visit(ThisExpressionAST *) override
    {
        m_safe = false;
        return false;
    }
    bool visit(NamedTypeSpecifierAST *) override
    {
        m_safe = false;
        return false;
    }
    // A nested lambda could shadow an outer parameter name with a parameter of its own;
    // parameter references are matched by name, not by resolved symbol, so this cannot be
    // told apart from a genuine reference to the outer parameter.
    bool visit(LambdaExpressionAST *) override
    {
        m_safe = false;
        return false;
    }

    Function * const m_func;
    QList<RawParameterReference> m_paramRefs;
    bool m_safe = true;
};

// A macro used in the callee's body (or in a substituted default argument) would be
// reintroduced by name at the call site, where it may well not be in scope, or may expand
// differently - substitutedText() only copies source text, it has no way to inline the macro's
// own definition.
bool usesMacro(const CppRefactoringFilePtr &file, int start, int end)
{
    for (const Document::MacroUse &use : file->cppDocument()->macroUses()) {
        if (use.utf16charsBegin() < end && use.utf16charsEnd() > start)
            return true;
    }
    return false;
}

std::optional<InlineBodyInfo> analyzeBody(
    FunctionDefinitionAST *funcDefAst, Function *func, const CppRefactoringFilePtr &calleeFile)
{
    CompoundStatementAST * const body = funcDefAst->function_body
        ? funcDefAst->function_body->asCompoundStatement()
                                           : nullptr;
    if (!body)
        return std::nullopt;

    InlineBodyInfo info;
    if (!body->statement_list) {
        info.shape = InlineBodyShape::Empty;
        return info;
    }
    if (body->statement_list->next)
        return std::nullopt; // More than one statement: out of scope for v1.

    StatementAST * const stmt = body->statement_list->value;
    if (!stmt)
        return std::nullopt;

    AST *toCheck = stmt;
    if (ReturnStatementAST * const ret = stmt->asReturnStatement()) {
        if (!ret->expression) {
            info.shape = InlineBodyShape::Empty;
            return info;
        }
        info.shape = InlineBodyShape::ReturnExpr;
        info.content = ret->expression;
        toCheck = ret->expression;
    } else if (stmt->asExpressionStatement()) {
        info.shape = InlineBodyShape::Statement;
        info.content = stmt;
    } else {
        // Any other statement kind is out of scope: an if/return/... would splice its own
        // control flow into the caller unchanged, and a declaration would leak a name into
        // the caller's scope.
        return std::nullopt;
    }

    ReferenceChecker checker(calleeFile->cppDocument()->translationUnit(), func);
    checker.accept(toCheck);
    if (!checker.isSafe())
        return std::nullopt;

    for (const RawParameterReference &ref : checker.parameterReferences())
        info.parameterReferences.append({calleeFile->range(ref.ast), ref.parameterIndex});

    // A parameter referenced more than once would have its argument's source text
    // substituted at each occurrence; if that argument has side effects (or is merely
    // expensive), duplicating it changes behavior.
    QSet<int> seenParameters;
    for (const ParameterReference &ref : std::as_const(info.parameterReferences)) {
        if (!Utils::insert(seenParameters, ref.parameterIndex))
            return std::nullopt;
    }

    // A parameter's default value is substituted verbatim wherever the call omits that
    // argument, so it needs the exact same safety check as the body itself.
    FunctionDeclaratorAST * const funcDeclAst = functionDeclaratorAst(funcDefAst, func);
    QSet<int> checkedDefaults;
    for (const ParameterReference &ref : std::as_const(info.parameterReferences)) {
        if (!Utils::insert(checkedDefaults, ref.parameterIndex))
            continue;
        ParameterDeclarationAST * const paramDeclAst
            = parameterDeclarationAst(funcDeclAst, ref.parameterIndex);
        if (!paramDeclAst || !paramDeclAst->expression)
            continue; // No default; fine as long as the call always supplies this argument.
        ReferenceChecker defaultChecker(calleeFile->cppDocument()->translationUnit(), func);
        defaultChecker.accept(paramDeclAst->expression);
        if (!defaultChecker.isSafe())
            return std::nullopt;
        info.defaultValues.insert(
            ref.parameterIndex,
            {calleeFile->range(paramDeclAst->expression),
             isPrimaryExpression(paramDeclAst->expression)});
    }

    if (info.content
        && usesMacro(calleeFile, calleeFile->startOf(info.content), calleeFile->endOf(info.content))) {
        return std::nullopt;
    }
    for (const DefaultValueInfo &defaultValue : std::as_const(info.defaultValues)) {
        if (usesMacro(calleeFile, defaultValue.range.start, defaultValue.range.end))
            return std::nullopt;
    }

    return info;
}

// Rejects a "return expr;" body whose expr's type does not exactly match the declared return
// type (e.g. "double f() { return getInt(); }"): the return statement would perform an
// implicit conversion that splicing the raw expression text at the call site does not
// reproduce, which is only actually observable in a type-dependent context (e.g.
// "auto x = f();" would silently deduce the wrong type after inlining). Rejecting outright is
// simpler and safer than reproducing the conversion for now.
// If the expression's type cannot be resolved at all, or is resolved ambiguously (multiple
// results that disagree with each other - TypeOfExpression() does not always fully disambiguate,
// e.g. for overload sets), we do not reject: both are cases where our model just isn't sure,
// and this can easily happen on legitimate code due to code model limitations, so erring on the
// side of offering the quickfix is preferable to silently disabling it.
bool returnExprMatchesDeclaredType(ExpressionAST *expr, Function *func,
                                   const CppRefactoringFilePtr &file, const Snapshot &snapshot,
                                   const LookupContext &context)
{
    TypeOfExpression typeOfExpression;
    typeOfExpression.init(file->cppDocument(), snapshot, context.bindings());
    const QList<LookupItem> items = typeOfExpression(
        file->textOf(expr).toUtf8(), file->scopeAt(expr->firstToken()),
        TypeOfExpression::Preprocess);
    if (items.isEmpty())
        return true;
    Type *agreedType = nullptr;
    for (const LookupItem &item : items) {
        Type * const itemType = item.type().type();
        if (!agreedType)
            agreedType = itemType;
        else if (!Matcher::match(agreedType, itemType))
            return true;
    }
    return Matcher::match(agreedType, func->returnType().type());
}

// Resolves a call's callee to a single, unambiguous Function* with a known body, mirroring
// the TypeOfExpression-based approach in assigntolocalvariable.cpp, extended to bridge from
// a declaration-only prototype to its out-of-line definition when necessary.
std::optional<CalleeInfo> resolveCallee(const CppQuickFixInterface &interface, CallAST *callAST)
{
    const CppRefactoringFilePtr &file = interface.currentFile();

    TypeOfExpression typeOfExpression;
    typeOfExpression
        .init(interface.semanticInfo().doc, interface.snapshot(), interface.context().bindings());
    typeOfExpression.setExpandTemplates(true);
    const QList<LookupItem> items = typeOfExpression(
        file->textOf(callAST->base_expression).toUtf8(),
        file->scopeAt(callAST->base_expression->firstToken()),
        TypeOfExpression::Preprocess);
    if (items.isEmpty())
        return std::nullopt;

    CppRefactoringChanges changes(interface.snapshot());
    QSet<Function *> definitions;
    CalleeInfo result;

    for (const LookupItem &item : items) {
        Symbol * const decl = item.declaration();
        if (!decl)
            continue;
        Function * const funcSymbol = decl->asFunction();
        if (!funcSymbol) {
            Declaration * const plainDecl = decl->asDeclaration();
            if (!plainDecl || !plainDecl->type()->asFunctionType())
                continue;
        }
        Symbol * const candidateSymbol = funcSymbol ? static_cast<Symbol *>(funcSymbol) : decl;

        CppRefactoringFilePtr candidateFile = candidateSymbol->filePath() == file->filePath()
            ? file
            : changes.cppFile(candidateSymbol->filePath());
        FunctionDefinitionAST *defAst = funcSymbol
                                            ? functionDefinitionAst(funcSymbol, candidateFile)
                                            : nullptr;
        Function *defSymbol = funcSymbol;
        CppRefactoringFilePtr defFile = candidateFile;
        if (!defAst) {
            Function * const found
                = SymbolFinder().findMatchingDefinition(candidateSymbol, interface.snapshot(), true);
            if (!found)
                continue;
            defFile = found->filePath() == file->filePath() ? file
                                                            : changes.cppFile(found->filePath());
            defAst = functionDefinitionAst(found, defFile);
            if (!defAst)
                continue;
            defSymbol = found;
        }
        if (definitions.contains(defSymbol))
            continue;
        definitions.insert(defSymbol);
        result.declarationSymbol = funcSymbol ? funcSymbol : defSymbol;
        result.definitionSymbol = defSymbol;
        result.definitionAst = defAst;
        result.definitionFile = defFile;
    }

    if (definitions.size() != 1)
        return std::nullopt;
    return result;
}

// Identifies the token used to name the callee at the call site (the function name itself,
// not the whole base expression), so it can be recognized and excluded from the usage scan.
int calleeNameToken(CallAST *callAST)
{
    if (MemberAccessAST * const memberAccess = callAST->base_expression->asMemberAccess()) {
        if (memberAccess->member_name)
            return memberAccess->member_name->firstToken();
    } else if (IdExpressionAST * const idExpr = callAST->base_expression->asIdExpression()) {
        if (idExpr->name)
            return idExpr->name->firstToken();
    }
    return callAST->base_expression->firstToken();
}

// A synchronous, whole-snapshot scan for the first usage of a symbol matching the given
// predicate, in the style of the (unexported) symbolUsages() helper in mcpsupport.cpp (one
// CPlusPlus::FindUsages per candidate file, pre-filtered via Control::findIdentifier()), but
// stopping as soon as a match is found rather than collecting every usage - the synchronous
// analogue of how CppFindReferences::checkUnused() cancels its QFuture on the first match.
bool hasMatchingUsage(
    Symbol *symbol,
    const CppRefactoringFilePtr &symbolFile,
    const Snapshot &snapshot,
    const std::function<bool(const Usage &)> &matches)
{
    const Identifier * const id = symbol->identifier();
    if (!id)
        return false;

    const WorkingCopy workingCopy = CppModelManager::workingCopy();
    for (auto it = snapshot.begin(), end = snapshot.end(); it != end; ++it) {
        const FilePath filePath = it.key();
        if (!it.value()->control()->findIdentifier(id->chars(), id->size()))
            continue;

        Document::Ptr doc;
        if (filePath == symbolFile->filePath() && symbolFile->cppDocument()->translationUnit()
            && symbolFile->cppDocument()->translationUnit()->ast()) {
            doc = symbolFile->cppDocument();
        } else {
            QByteArray source;
            if (const std::optional<QByteArray> wcSource = workingCopy.source(filePath))
                source = *wcSource;
            else if (const Result<QByteArray> contents = filePath.fileContents())
                source = *contents;
            doc = snapshot.preprocessedDocument(source, filePath);
            doc->tokenize();
            if (!doc->control()->findIdentifier(id->chars(), id->size()))
                continue;
            doc->check();
        }

        FindUsages findUsages(doc->utf8Source(), doc, snapshot, /*categorize=*/true);
        findUsages(symbol);
        for (const Usage &usage : findUsages.usages()) {
            if (matches(usage))
                return true;
        }
    }
    return false;
}

// Removes the symbol's declaration/definition, widening the range to also remove a leading
// doc comment if there is one. Returns that comment's original text (or an empty string), so
// the caller can preserve it elsewhere instead of letting it silently disappear.
QString scheduleSymbolRemoval(
    const CppRefactoringFilePtr &file, AST *ast, Symbol *symbol, ChangeSet &changeSet)
{
    ChangeSet::Range range = file->range(ast);
    const QList<Token> comments
        = commentsForDeclaration(symbol, ast, *file->document(), file->cppDocument());
    QString commentText;
    if (!comments.isEmpty()) {
        range.start = file->cppDocument()
                          ->translationUnit()
                          ->getTokenPositionInDocument(comments.first(), file->document());
        commentText = file->textOf(
            range.start,
            file->cppDocument()
                ->translationUnit()
                ->getTokenEndPositionInDocument(comments.last(), file->document()));
        // textOf() goes through QTextCursor::selectedText(), which represents embedded line
        // breaks as QChar::ParagraphSeparator rather than '\n'.
        commentText.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
    }
    removeRangeAndSurroundingBlankLine(file.data(), range, changeSet);
    return commentText;
}

// The existing indentation immediately before the given position, or an empty string if that
// position isn't preceded solely by whitespace back to the start of its line.
QString indentationBefore(const CppRefactoringFilePtr &file, int position)
{
    int start = position;
    while (start > 0) {
        const QChar ch = file->charAt(start - 1);
        if (ch == QChar::ParagraphSeparator)
            break;
        if (!ch.isSpace())
            return {};
        --start;
    }
    return file->textOf(start, position);
}

// A copied doc comment's own indentation is almost certainly wrong at the new location (the
// call site is rarely at the same nesting level as the removed function), so continuation
// lines are realigned to the indentation already present at the insertion point.
QString reindentedComment(const QString &comment, const QString &indent)
{
    QStringList lines = comment.split(QLatin1Char('\n'));
    for (int i = 1; i < lines.size(); ++i)
        lines[i] = indent + lines[i].trimmed();
    return lines.join(QLatin1Char('\n'));
}

QString joinedCommentText(const QStringList &comments, const QString &indent)
{
    QString text;
    for (int i = 0; i < comments.size(); ++i) {
        if (i > 0) {
            text += QLatin1Char('\n');
            text += indent;
        }
        text += reindentedComment(comments.at(i), indent);
    }
    return text;
}

class InlineFunctionCallOp : public CppQuickFixOperation
{
public:
    InlineFunctionCallOp(
        const CppQuickFixInterface &interface,
        CallAST *callAST,
        ExpressionStatementAST *callSiteStatement,
        StatementAST *enclosingStatement,
        const CalleeInfo &callee,
        const InlineBodyInfo &bodyInfo,
        bool removeFunction)
        : CppQuickFixOperation(interface)
        , m_callAST(callAST)
        , m_callSiteStatement(callSiteStatement)
        , m_enclosingStatement(enclosingStatement)
        , m_callee(callee)
        , m_bodyInfo(bodyInfo)
        , m_removeFunction(removeFunction)
    {
        setDescription(
            removeFunction ? Tr::tr("Inline Function Call and Remove Function If Unused")
                           : Tr::tr("Inline Function Call"));
    }

private:
    void perform() override
    {
        QHash<FilePath, ChangeSet> changesByPath;
        QHash<FilePath, CppRefactoringFilePtr> filesByPath;
        filesByPath.insert(currentFile()->filePath(), currentFile());

        const QStringList copiedComments = m_removeFunction
            ? scheduleRemovalIfUnused(changesByPath, filesByPath)
                                               : QStringList();

        // The removed function's doc comment (if any) is spliced in as a leading comment on
        // whichever statement contains the call, so it isn't silently lost along with the
        // function - it may no longer be fully applicable (e.g. it may still refer to
        // parameter names that no longer exist at this position), but that is for the user to
        // notice and adapt, rather than the comment disappearing unnoticed.
        QString commentBlock;
        if (m_enclosingStatement && !copiedComments.isEmpty()) {
            commentBlock = joinedCommentText(
                copiedComments,
                indentationBefore(currentFile(), currentFile()->startOf(m_enclosingStatement)));
        }

        ChangeSet &callSiteChanges = changesByPath[currentFile()->filePath()];
        switch (m_bodyInfo.shape) {
        case InlineBodyShape::Empty:
            if (!commentBlock.isEmpty())
                callSiteChanges.replace(currentFile()->range(m_callSiteStatement), commentBlock);
            else
                removeRangeAndSurroundingBlankLine(
                    currentFile().data(),
                    currentFile()->range(m_callSiteStatement),
                    callSiteChanges);
            break;
        case InlineBodyShape::Statement: {
            QString text = commentBlock;
            if (!text.isEmpty()) {
                text += QLatin1Char('\n');
                text += indentationBefore(currentFile(), currentFile()->startOf(m_callSiteStatement));
            }
            text += substitutedText(m_bodyInfo.content);
            callSiteChanges.replace(currentFile()->range(m_callSiteStatement), text);
            break;
        }
        case InlineBodyShape::ReturnExpr: {
            if (!commentBlock.isEmpty()) {
                QString text = commentBlock;
                text += QLatin1Char('\n');
                text
                    += indentationBefore(currentFile(), currentFile()->startOf(m_enclosingStatement));
                callSiteChanges.insert(currentFile()->startOf(m_enclosingStatement), text);
            }
            QString text = "(";
            text += substitutedText(m_bodyInfo.content);
            text += ")";
            callSiteChanges.replace(currentFile()->range(m_callAST), text);
            break;
        }
        }

        for (auto it = changesByPath.begin(); it != changesByPath.end(); ++it) {
            const CppRefactoringFilePtr file = filesByPath.value(it.key());
            QTC_ASSERT(file, continue);
            file->apply(it.value());
        }
    }

    // Returns the doc comment(s) removed along with the definition and (if applicable)
    // declaration, so perform() can splice them into the call site instead of losing them.
    QStringList scheduleRemovalIfUnused(
        QHash<FilePath, ChangeSet> &changesByPath,
        QHash<FilePath, CppRefactoringFilePtr> &filesByPath) const
    {
        const int nameToken = calleeNameToken(m_callAST);
        int callLine = 0;
        int callColumn = 0;
        currentFile()
            ->cppDocument()
            ->translationUnit()
            ->getTokenPosition(nameToken, &callLine, &callColumn);
        const int callUsageColumn = callColumn - 1;
        const FilePath callSitePath = filePath();

        const Function * const calleeFunc = m_callee.definitionSymbol;
        const auto isCallSiteBeingInlined =
            [callSitePath, callLine, callUsageColumn](const Usage &usage) {
                return usage.path == callSitePath && usage.line == callLine
                       && usage.col == callUsageColumn;
            };
        const auto isProperUsage = [calleeFunc, isCallSiteBeingInlined](const Usage &usage) {
            if (isCallSiteBeingInlined(usage))
                return false;
            return CppEditor::Internal::isProperUsage(usage, calleeFunc);
        };
        if (hasMatchingUsage(m_callee.definitionSymbol, m_callee.definitionFile, snapshot(),
                             isProperUsage)) {
            Core::MessageManager::writeFlashing(
                Tr::tr("\"%1\" was not removed because it is still used elsewhere.")
                    .arg(Overview().prettyName(m_callee.definitionSymbol->name())));
            return {};
        }

        QStringList comments;
        const QString defComment = scheduleSymbolRemoval(
            m_callee.definitionFile, m_callee.definitionAst, m_callee.definitionSymbol,
            changesByPath[m_callee.definitionFile->filePath()]);
        if (!defComment.isEmpty())
            comments << defComment;
        filesByPath.insert(m_callee.definitionFile->filePath(), m_callee.definitionFile);

        const LookupContext context(m_callee.definitionFile->cppDocument(), snapshot());
        const QList<Declaration *> declCandidates
            = SymbolFinder().findMatchingDeclaration(context, m_callee.definitionSymbol);
        if (declCandidates.isEmpty())
            return comments;

        Declaration * const declSymbol = declCandidates.first();
        CppRefactoringFilePtr declFile = filesByPath.value(declSymbol->filePath());
        if (!declFile) {
            CppRefactoringChanges refactoring(snapshot());
            declFile = refactoring.cppFile(declSymbol->filePath());
        }
        const QList<AST *> declPath = ASTPath(
            declFile->cppDocument())(declSymbol->line(), declSymbol->column());
        SimpleDeclarationAST *declAst = nullptr;
        for (auto it = declPath.rbegin(); it != declPath.rend() && !declAst; ++it)
            declAst = (*it)->asSimpleDeclaration();
        if (!declAst)
            return comments;

        const QString declComment = scheduleSymbolRemoval(
            declFile, declAst, declSymbol, changesByPath[declFile->filePath()]);
        filesByPath.insert(declFile->filePath(), declFile);
        // The declaration's doc comment is usually the public-facing one; show it first.
        if (!declComment.isEmpty())
            comments.prepend(declComment);
        return comments;
    }

    // Parenthesized because the parameter occurrence being replaced may sit in a
    // precedence-sensitive position in the body (e.g. "*ptr"), while the argument being
    // substituted in may be an arbitrary, non-atomic expression (e.g. "arr + 1").
    QString argumentText(int parameterIndex) const
    {
        if (ExpressionAST * const arg = callArgumentAt(m_callAST, parameterIndex)) {
            const QString text = currentFile()->textOf(arg);
            return isPrimaryExpression(arg) ? text : '(' + text + ')';
        }
        // The call relies on the parameter's default value.
        const auto it = m_bodyInfo.defaultValues.constFind(parameterIndex);
        QTC_ASSERT(it != m_bodyInfo.defaultValues.constEnd(), return {});
        const QString text
            = m_callee.definitionFile->textOf(it.value().range.start, it.value().range.end);
        return it.value().isPrimaryExpression ? text : '(' + text + ')';
    }

    QString substitutedText(AST *content) const
    {
        QList<ParameterReference> sorted = m_bodyInfo.parameterReferences;
        std::sort(
            sorted.begin(),
            sorted.end(),
            [](const ParameterReference &a, const ParameterReference &b) {
                return a.range.start < b.range.start;
            });
        QString result;
        int cursor = m_callee.definitionFile->startOf(content);
        const int contentEnd = m_callee.definitionFile->endOf(content);
        for (const ParameterReference &ref : std::as_const(sorted)) {
            result += m_callee.definitionFile->textOf(cursor, ref.range.start);
            result += argumentText(ref.parameterIndex);
            cursor = ref.range.end;
        }
        result += m_callee.definitionFile->textOf(cursor, contentEnd);
        return result;
    }

    CallAST * const m_callAST;
    ExpressionStatementAST * const m_callSiteStatement;
    StatementAST * const m_enclosingStatement;
    const CalleeInfo m_callee;
    const InlineBodyInfo m_bodyInfo;
    const bool m_removeFunction;
};

//! Replaces a function call with the callee's body, and optionally removes the callee
//! if the call site was its only usage.
class InlineFunctionCall : public CppQuickFixFactory
{
private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        CallAST *callAST = nullptr;
        int callIndex = -1;
        for (int i = path.size() - 1; i >= 0; --i) {
            if (CallAST * const call = path.at(i)->asCall()) {
                if (!interface.isCursorOn(call))
                    return;
                callAST = call;
                callIndex = i;
                break;
            }
        }
        if (!callAST || !callAST->base_expression)
            return;

        const std::optional<CalleeInfo> callee = resolveCallee(interface, callAST);
        if (!callee)
            return;

        if (FunctionUtils::isVirtualFunction(callee->declarationSymbol, interface.context()))
            return;
        if (isFunctionTemplate(callee->declarationSymbol))
            return;

        const std::optional<InlineBodyInfo> bodyInfo
            = analyzeBody(callee->definitionAst, callee->definitionSymbol, callee->definitionFile);
        if (!bodyInfo)
            return;
        if (!callSuppliesRequiredArguments(callAST, *bodyInfo))
            return;

        if (bodyInfo->shape == InlineBodyShape::ReturnExpr
            && !returnExprMatchesDeclaredType(
                bodyInfo->content->asExpression(), callee->definitionSymbol,
                callee->definitionFile, interface.snapshot(), interface.context())) {
            return;
        }

        ExpressionStatementAST *callSiteStatement = nullptr;
        if (bodyInfo->shape != InlineBodyShape::ReturnExpr) {
            if (callIndex <= 0)
                return;
            callSiteStatement = path.at(callIndex - 1)->asExpressionStatement();
            if (!callSiteStatement || callSiteStatement->expression != callAST)
                return;
        }

        // Best-effort anchor for splicing a removed function's doc comment back in (see
        // scheduleRemovalIfUnused()); not every call site has one (e.g. a global-scope
        // initializer isn't a statement), in which case the comment is simply dropped.
        StatementAST *enclosingStatement = nullptr;
        for (int i = callIndex - 1; i >= 0; --i) {
            if (StatementAST * const stmt = path.at(i)->asStatement()) {
                enclosingStatement = stmt;
                break;
            }
        }

        for (const bool removeFunction : {false, true}) {
            result << new InlineFunctionCallOp(
                interface,
                callAST,
                callSiteStatement,
                enclosingStatement,
                *callee,
                *bodyInfo,
                removeFunction);
        }
    }
};

#ifdef WITH_TESTS
class InlineFunctionCallTest : public Tests::CppQuickFixTestObject
{
    Q_OBJECT
public:
    using CppQuickFixTestObject::CppQuickFixTestObject;
};
#endif

} // namespace

void registerInlineFunctionCallQuickfix()
{
    REGISTER_QUICKFIX_FACTORY_WITH_STANDARD_TEST(InlineFunctionCall);
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <inlinefunctioncall.moc>
#endif
