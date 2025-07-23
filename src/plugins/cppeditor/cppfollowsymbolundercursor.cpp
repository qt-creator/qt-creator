// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppfollowsymbolundercursor.h"

#include "baseeditordocumentprocessor.h"
#include "cppeditordocument.h"
#include "cppeditorwidget.h"
#include "cppmodelmanager.h"
#include "cpptoolsreuse.h"
#include "cppvirtualfunctionassistprovider.h"
#include "functionutils.h"
#include "symbolfinder.h"

#include <cplusplus/ASTPath.h>
#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/ResolveExpression.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/TypeOfExpression.h>
#include <texteditor/textdocumentlayout.h>
#include <utils/algorithm.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>

#include <QHash>
#include <QList>
#include <QQueue>
#include <QSet>

#ifdef WITH_TESTS
#include "cpptoolstestcase.h"
#include <QEventLoop>
#include <QTest>
#include <QTimer>
#endif

using namespace CPlusPlus;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor {

struct Declarations
{
    Symbol *decl = nullptr;
    Symbol *funcDecl = nullptr;
    Function *funcDef = nullptr;
};
Declarations declsFromCursor(const Document::Ptr &doc, const QTextCursor &cursor)
{
    Declarations decls;
    ASTPath astPathFinder(doc);
    const QList<AST *> astPath = astPathFinder(cursor);

    for (AST *ast : astPath) { // FIXME: Shouldn't this search backwards?
        if (FunctionDefinitionAST *functionDefinitionAST = ast->asFunctionDefinition()) {
            if ((decls.funcDef = functionDefinitionAST->symbol))
                break; // Function definition found!
        } else if (SimpleDeclarationAST *simpleDeclaration = ast->asSimpleDeclaration()) {
            if (List<Symbol *> *symbols = simpleDeclaration->symbols) {
                if (Symbol *symbol = symbols->value) {
                    if (symbol->asDeclaration()) {
                        decls.decl = symbol;
                        if (symbol->type()->asFunctionType()) {
                            decls.funcDecl = symbol;
                            break; // Function declaration found!
                        }
                    }
                }
            }
        }
    }
    return decls;
}

static Symbol *functionDeclFromDef(Function *def, const Document::Ptr &doc, const Snapshot &snapshot)
{
    const LookupContext context(doc, snapshot);
    ClassOrNamespace * const binding = context.lookupType(def);
    const QList<LookupItem> declarations = context.lookup(def->name(), def->enclosingScope());
    QList<Symbol *> best;
    for (const LookupItem &r : declarations) {
        if (Symbol *decl = r.declaration()) {
            if (Function *funTy = decl->type()->asFunctionType()) {
                if (funTy->match(def)) {
                    if (decl != def && binding == r.binding())
                        best.prepend(decl);
                    else
                        best.append(decl);
                }
            }
        }
    }

    if (best.isEmpty())
        return nullptr;
    return best.first();
}

namespace {

class VirtualFunctionHelper {
public:
    VirtualFunctionHelper(TypeOfExpression &typeOfExpression,
                          Scope *scope,
                          const Document::Ptr &document,
                          const Snapshot &snapshot,
                          SymbolFinder *symbolFinder);
    VirtualFunctionHelper() = delete;

    bool canLookupVirtualFunctionOverrides(Function *function);

    /// Returns != 0 if canLookupVirtualFunctionOverrides() succeeded.
    Class *staticClassOfFunctionCallExpression() const
    { return m_staticClassOfFunctionCallExpression; }

private:
    Q_DISABLE_COPY(VirtualFunctionHelper)

    Class *staticClassOfFunctionCallExpression_internal() const;

private:
    // Provided
    const Document::Ptr m_expressionDocument;
    Scope *m_scope;
    const Document::Ptr &m_document;
    const Snapshot &m_snapshot;
    TypeOfExpression &m_typeOfExpression;
    SymbolFinder *m_finder;

    // Determined
    ExpressionAST *m_baseExpressionAST = nullptr;
    Function *m_function = nullptr;
    int m_accessTokenKind = 0;
    Class *m_staticClassOfFunctionCallExpression = nullptr; // Output
};

VirtualFunctionHelper::VirtualFunctionHelper(TypeOfExpression &typeOfExpression,
                                             Scope *scope,
                                             const Document::Ptr &document,
                                             const Snapshot &snapshot,
                                             SymbolFinder *finder)
    : m_expressionDocument(typeOfExpression.context().expressionDocument())
    , m_scope(scope)
    , m_document(document)
    , m_snapshot(snapshot)
    , m_typeOfExpression(typeOfExpression)
    , m_finder(finder)
{
    if (ExpressionAST *expressionAST = typeOfExpression.expressionAST()) {
        if (CallAST *callAST = expressionAST->asCall()) {
            if (ExpressionAST *baseExpressionAST = callAST->base_expression)
                m_baseExpressionAST = baseExpressionAST;
        }
    }
}

bool VirtualFunctionHelper::canLookupVirtualFunctionOverrides(Function *function)
{
    m_function = function;

    if (!m_document || m_snapshot.isEmpty() || !m_function || !m_scope)
        return false;

    if (m_scope->asClass() && m_function->isPureVirtual()) {
        m_staticClassOfFunctionCallExpression = m_scope->asClass();
        return true;
    }

    if (!m_baseExpressionAST || !m_expressionDocument
            || m_scope->asClass() || m_scope->asFunction()) {
        return false;
    }

    bool result = false;

    if (IdExpressionAST *idExpressionAST = m_baseExpressionAST->asIdExpression()) {
        NameAST *name = idExpressionAST->name;
        const bool nameIsQualified = name && name->asQualifiedName();
        result = !nameIsQualified && Internal::FunctionUtils::isVirtualFunction(
                    function, LookupContext(m_document, m_snapshot));
    } else if (MemberAccessAST *memberAccessAST = m_baseExpressionAST->asMemberAccess()) {
        NameAST *name = memberAccessAST->member_name;
        const bool nameIsQualified = name && name->asQualifiedName();
        if (!nameIsQualified && Internal::FunctionUtils::isVirtualFunction(
                    function, LookupContext(m_document, m_snapshot))) {
            TranslationUnit *unit = m_expressionDocument->translationUnit();
            QTC_ASSERT(unit, return false);
            m_accessTokenKind = unit->tokenKind(memberAccessAST->access_token);

            if (m_accessTokenKind == T_ARROW) {
                result = true;
            } else if (m_accessTokenKind == T_DOT) {
                const QList<LookupItem> items = m_typeOfExpression.reference(
                            memberAccessAST->base_expression, m_document, m_scope);
                if (!items.isEmpty()) {
                    const LookupItem item = items.first();
                    if (Symbol *declaration = item.declaration())
                        result = declaration->type()->asReferenceType();
                }
            }
        }
    }

    if (!result)
        return false;
    return (m_staticClassOfFunctionCallExpression = staticClassOfFunctionCallExpression_internal());
}

/// For "f()" in "class C { void g() { f(); };" return class C.
/// For "c->f()" in "{ C *c; c->f(); }" return class C.
Class *VirtualFunctionHelper::staticClassOfFunctionCallExpression_internal() const
{
    if (!m_finder)
        return nullptr;

    Class *result = nullptr;

    if (m_baseExpressionAST->asIdExpression()) {
        for (Scope *s = m_scope; s ; s = s->enclosingScope()) {
            if (Function *function = s->asFunction()) {
                result = m_finder->findMatchingClassDeclaration(function, m_snapshot);
                break;
            }
        }
    } else if (MemberAccessAST *memberAccessAST = m_baseExpressionAST->asMemberAccess()) {
        QTC_ASSERT(m_accessTokenKind == T_ARROW || m_accessTokenKind == T_DOT, return result);
        const QList<LookupItem> items = m_typeOfExpression(memberAccessAST->base_expression,
                                                           m_expressionDocument, m_scope);
        ResolveExpression resolveExpression(m_typeOfExpression.context());
        ClassOrNamespace *binding = resolveExpression.baseExpression(items, m_accessTokenKind);
        if (binding) {
            if (Class *klass = binding->rootClass()) {
                result = klass;
            } else {
                const QList<Symbol *> symbols = binding->symbols();
                if (!symbols.isEmpty()) {
                    Symbol * const first = symbols.first();
                    if (first->asForwardClassDeclaration())
                        result = m_finder->findMatchingClassDeclaration(first, m_snapshot);
                }
            }
        }
    }

    return result;
}

Link findMacroLink_helper(const QByteArray &name, Document::Ptr doc, const Snapshot &snapshot,
                          QSet<QString> *processed)
{
    if (doc && !name.startsWith('<') && Utils::insert(*processed, doc->filePath().path())) {
        for (const Macro &macro : doc->definedMacros()) {
            if (macro.name() == name) {
                Link link;
                link.targetFilePath = macro.filePath();
                link.target.line = macro.line();
                return link;
            }
        }

        const QList<Document::Include> includes = doc->resolvedIncludes();
        for (int index = includes.size() - 1; index != -1; --index) {
            const Document::Include &i = includes.at(index);
            Link link = findMacroLink_helper(name, snapshot.document(i.resolvedFileName()),
                                             snapshot, processed);
            if (link.hasValidTarget())
                return link;
        }
    }

    return Link();
}

Link findMacroLink(const QByteArray &name, const Document::Ptr &doc)
{
    if (!name.isEmpty()) {
        if (doc) {
            const Snapshot snapshot = CppModelManager::snapshot();
            QSet<QString> processed;
            return findMacroLink_helper(name, doc, snapshot, &processed);
        }
    }

    return Link();
}

/// Considers also forward declared templates.
static bool isForwardClassDeclaration(Type *type)
{
    if (!type)
        return false;

    if (type->asForwardClassDeclarationType()) {
        return true;
    } else if (Template *templ = type->asTemplateType()) {
        if (Symbol *declaration = templ->declaration()) {
            if (declaration->asForwardClassDeclaration())
                return true;
        }
    }

    return false;
}

inline LookupItem skipForwardDeclarations(const QList<LookupItem> &resolvedSymbols)
{
    QList<LookupItem> candidates = resolvedSymbols;

    LookupItem result = candidates.first();
    const FullySpecifiedType ty = result.type().simplified();

    if (isForwardClassDeclaration(ty.type())) {
        while (!candidates.isEmpty()) {
            LookupItem r = candidates.takeFirst();

            if (!isForwardClassDeclaration(r.type().type())) {
                result = r;
                break;
            }
        }
    }

    if (ty->asObjCForwardClassDeclarationType()) {
        while (!candidates.isEmpty()) {
            LookupItem r = candidates.takeFirst();

            if (!r.type()->asObjCForwardClassDeclarationType()) {
                result = r;
                break;
            }
        }
    }

    if (ty->asObjCForwardProtocolDeclarationType()) {
        while (!candidates.isEmpty()) {
            LookupItem r = candidates.takeFirst();

            if (!r.type()->asObjCForwardProtocolDeclarationType()) {
                result = r;
                break;
            }
        }
    }

    return result;
}

Link attemptDeclDef(const QTextCursor &cursor, Snapshot snapshot,
                    const Document::Ptr &document,
                    SymbolFinder *symbolFinder)
{
    Link result;
    QTC_ASSERT(document, return result);

    snapshot.insert(document);

    QList<AST *> path = ASTPath(document)(cursor);

    if (path.size() < 5)
        return result;

    NameAST *name = path.last()->asName();
    if (!name)
        return result;

    if (QualifiedNameAST *qName = path.at(path.size() - 2)->asQualifiedName()) {
        // TODO: check which part of the qualified name we're on
        if (qName->unqualified_name != name)
            return result;
    }

    for (int i = path.size() - 1; i != -1; --i) {
        AST *node = path.at(i);

        if (node->asParameterDeclaration() != nullptr)
            return result;
    }

    AST *declParent = nullptr;
    DeclaratorAST *decl = nullptr;
    for (int i = path.size() - 2; i > 0; --i) {
        if ((decl = path.at(i)->asDeclarator()) != nullptr) {
            declParent = path.at(i - 1);
            break;
        }
    }
    if (!decl || !declParent)
        return result;

    Symbol *target = nullptr;
    if (FunctionDefinitionAST *funDef = declParent->asFunctionDefinition()) {
        QList<Declaration *> candidates =
                symbolFinder->findMatchingDeclaration(LookupContext(document, snapshot),
                                                        funDef->symbol);
        if (!candidates.isEmpty()) // TODO: improve disambiguation
            target = candidates.first();
    } else if (const SimpleDeclarationAST * const simpleDecl = declParent->asSimpleDeclaration()) {
        FunctionDeclaratorAST *funcDecl = nullptr;
        if (decl->postfix_declarator_list && decl->postfix_declarator_list->value)
            funcDecl = decl->postfix_declarator_list->value->asFunctionDeclarator();
        if (funcDecl)
            target = symbolFinder->findMatchingDefinition(funcDecl->symbol, snapshot, false);
        else if (simpleDecl->symbols)
            target = symbolFinder->findMatchingVarDefinition(simpleDecl->symbols->value, snapshot);
    }

    if (target) {
        result = target->toLink();

        int startLine, startColumn, endLine, endColumn;
        document->translationUnit()->getTokenPosition(name->firstToken(), &startLine,
                                                           &startColumn);
        document->translationUnit()->getTokenEndPosition(name->lastToken() - 1, &endLine,
                                                         &endColumn);

        QTextDocument *textDocument = cursor.document();
        result.linkTextStart =
                textDocument->findBlockByNumber(startLine - 1).position() + startColumn - 1;
        result.linkTextEnd =
                textDocument->findBlockByNumber(endLine - 1).position() + endColumn - 1;
    }

    return result;
}

Symbol *findDefinition(Symbol *symbol, const Snapshot &snapshot, SymbolFinder *symbolFinder)
{
    if (symbol->asFunction())
        return nullptr; // symbol is a function definition.

    if (!symbol->type()->asFunctionType())
        return nullptr; // not a function declaration

    return symbolFinder->findMatchingDefinition(symbol, snapshot, false);
}

bool maybeAppendArgumentOrParameterList(QString *expression, const QTextCursor &textCursor)
{
    QTC_ASSERT(expression, return false);
    QTextDocument *textDocument = textCursor.document();
    QTC_ASSERT(textDocument, return false);

    // Skip white space
    QTextCursor cursor(textCursor);
    while (textDocument->characterAt(cursor.position()).isSpace()
           && cursor.movePosition(QTextCursor::NextCharacter)) {
    }

    // Find/Include "(arg1, arg2, ...)"
    if (textDocument->characterAt(cursor.position()) == QLatin1Char('(')) {
        if (TextBlockUserData::findNextClosingParenthesis(&cursor, true)) {
            expression->append(cursor.selectedText());
            return true;
        }
    }

    return false;
}

bool isCursorOnTrailingReturnType(const QList<AST *> &astPath)
{
    if (astPath.size() < 3)
        return false;
    for (auto it = astPath.cend() - 3, begin = astPath.cbegin(); it >= begin; --it) {
        if (!(*it)->asTrailingReturnType())
            continue;
        const auto nextIt = it + 1;
        const auto nextNextIt = nextIt + 1;
        return (*nextIt)->asNamedTypeSpecifier()
               && ((*nextNextIt)->asSimpleName()
                   || (*nextNextIt)->asQualifiedName()
                   || (*nextNextIt)->asTemplateId());
    }
    return false;
}

void maybeFixExpressionInTrailingReturnType(QString *expression,
                                            const QTextCursor &textCursor,
                                            const Document::Ptr documentFromSemanticInfo)
{
    QTC_ASSERT(expression, return);

    if (!documentFromSemanticInfo)
        return;

    const QString arrow = QLatin1String("->");
    const int arrowPosition = expression->lastIndexOf(arrow);
    if (arrowPosition != -1) {
        ASTPath astPathFinder(documentFromSemanticInfo);
        const QList<AST *> astPath = astPathFinder(textCursor);

        if (isCursorOnTrailingReturnType(astPath))
            *expression = expression->mid(arrowPosition + arrow.size()).trimmed();
    }
}

QString expressionUnderCursorAsString(const QTextCursor &textCursor,
                                      const Document::Ptr documentFromSemanticInfo,
                                      const LanguageFeatures &features)
{
    ExpressionUnderCursor expressionUnderCursor(features);
    QString expression = expressionUnderCursor(textCursor);

    if (!maybeAppendArgumentOrParameterList(&expression, textCursor))
        maybeFixExpressionInTrailingReturnType(&expression, textCursor, documentFromSemanticInfo);

    return expression;
}

} // anonymous namespace

FollowSymbolUnderCursor::FollowSymbolUnderCursor()
    : m_virtualFunctionAssistProvider(new VirtualFunctionAssistProvider)
{
}

static int skipMatchingParentheses(const Tokens &tokens, int idx, int initialDepth)
{
    int j = idx;
    int depth = initialDepth;

    for (; j < tokens.size(); ++j) {
        if (tokens.at(j).is(T_LPAREN)) {
            ++depth;
        } else if (tokens.at(j).is(T_RPAREN)) {
            if (!--depth)
                break;
        }
    }

    return j;
}

void FollowSymbolUnderCursor::findLink(
        const CursorInEditor &data,
        const Utils::LinkHandler &processLinkCallback,
        bool resolveTarget,
        const Snapshot &theSnapshot,
        const Document::Ptr &documentFromSemanticInfo,
        SymbolFinder *symbolFinder,
        bool inNextSplit)
{
    Link link;

    QTextCursor cursor = data.cursor();
    QTextDocument *document = cursor.document();
    if (!document)
        return processLinkCallback(link);

    int line = 0;
    int column = 0;
    Utils::Text::convertPosition(document, cursor.position(), &line, &column);

    Snapshot snapshot = theSnapshot;

    // Move to end of identifier
    QTextCursor tc = cursor;
    QChar ch = document->characterAt(tc.position());
    while (isValidIdentifierChar(ch)) {
        tc.movePosition(QTextCursor::NextCharacter);
        ch = document->characterAt(tc.position());
    }

    // Try to macth decl/def. For this we need the semantic doc with the AST.
    if (documentFromSemanticInfo
            && documentFromSemanticInfo->translationUnit()
            && documentFromSemanticInfo->translationUnit()->ast()) {
        int pos = tc.position();
        while (document->characterAt(pos).isSpace())
            ++pos;
        const QChar ch = document->characterAt(pos);
        if (ch == '(' || ch == ';') {
            link = attemptDeclDef(cursor, snapshot, documentFromSemanticInfo, symbolFinder);
            if (link.hasValidLinkText())
                return processLinkCallback(link);
        }
    }

    // Try to find a signal or slot inside SIGNAL() or SLOT()
    int beginOfToken = 0;
    int endOfToken = 0;

    const LanguageFeatures features = documentFromSemanticInfo
            ? documentFromSemanticInfo->languageFeatures()
            : LanguageFeatures::defaultFeatures();

    SimpleLexer tokenize;
    tokenize.setLanguageFeatures(features);
    const QString blockText = cursor.block().text();
    const Tokens tokens = tokenize(blockText, BackwardsScanner::previousBlockState(cursor.block()));

    bool recognizedQtMethod = false;

    for (int i = 0; i < tokens.size(); ++i) {
        const Token &tk = tokens.at(i);

        if (column >= tk.utf16charsBegin() && column < tk.utf16charsEnd()) {
            int closingParenthesisPos = tokens.size();
            if (i >= 2 && tokens.at(i).is(T_IDENTIFIER) && tokens.at(i - 1).is(T_LPAREN)
                && (tokens.at(i - 2).is(T_SIGNAL) || tokens.at(i - 2).is(T_SLOT))) {

                // token[i] == T_IDENTIFIER
                // token[i + 1] == T_LPAREN
                // token[.....] == ....
                // token[i + n] == T_RPAREN

                if (i + 1 < tokens.size() && tokens.at(i + 1).is(T_LPAREN))
                    closingParenthesisPos = skipMatchingParentheses(tokens, i - 1, 0);
            } else if ((i > 3 && tk.is(T_LPAREN) && tokens.at(i - 1).is(T_IDENTIFIER)
                        && tokens.at(i - 2).is(T_LPAREN)
                    && (tokens.at(i - 3).is(T_SIGNAL) || tokens.at(i - 3).is(T_SLOT)))) {

                // skip until the closing parentheses of the SIGNAL/SLOT macro
                closingParenthesisPos = skipMatchingParentheses(tokens, i, 1);
                --i; // point to the token before the opening parenthesis
            }

            if (closingParenthesisPos < tokens.size()) {
                QTextBlock block = cursor.block();

                beginOfToken = block.position() + tokens.at(i).utf16charsBegin();
                endOfToken = block.position() + tokens.at(i).utf16charsEnd();

                tc.setPosition(block.position() + tokens.at(closingParenthesisPos).utf16charsEnd());
                recognizedQtMethod = true;
            }
            break;
        }
    }

    // Check if we're on an operator declaration or definition.
    if (!recognizedQtMethod && documentFromSemanticInfo) {
        bool cursorRegionReached = false;
        for (int i = 0; i < tokens.size(); ++i) {
            const Token &tk = tokens.at(i);

            // In this case we want to look at one token before the current position to recognize
            // an operator if the cursor is inside the actual operator: operator[$]
            if (column >= tk.utf16charsBegin() && column <= tk.utf16charsEnd()) {
                cursorRegionReached = true;
                if (tk.is(T_OPERATOR)) {
                    link = attemptDeclDef(cursor, theSnapshot,
                                          documentFromSemanticInfo, symbolFinder);
                    if (link.hasValidLinkText())
                        return processLinkCallback(link);
                } else if (tk.isPunctuationOrOperator() && i > 0 && tokens.at(i - 1).is(T_OPERATOR)) {
                    QTextCursor c = cursor;
                    c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor,
                                   column - tokens.at(i - 1).utf16charsBegin());
                    link = attemptDeclDef(c, theSnapshot, documentFromSemanticInfo, symbolFinder);
                    if (link.hasValidLinkText())
                        return processLinkCallback(link);
                }
            } else if (cursorRegionReached) {
                break;
            }
        }
    }

    CppEditorWidget *editorWidget = data.editorWidget();
    if (!editorWidget)
        return processLinkCallback(link);

    // Now we prefer the doc from the snapshot with macros expanded.
    Document::Ptr doc = snapshot.document(data.filePath());
    if (!doc) {
        doc = documentFromSemanticInfo;
        if (!doc)
            return processLinkCallback(link);
    }

    if (!recognizedQtMethod) {
        const QTextBlock block = tc.block();
        int pos = cursor.positionInBlock();
        QChar ch = document->characterAt(cursor.position());
        if (pos > 0 && !isValidIdentifierChar(ch))
            --pos; // positionInBlock points to a delimiter character.
        const Token tk = SimpleLexer::tokenAt(block.text(), pos,
                                              BackwardsScanner::previousBlockState(block),
                                              features);

        beginOfToken = block.position() + tk.utf16charsBegin();
        endOfToken = block.position() + tk.utf16charsEnd();

        // Handle include directives
        if (tk.is(T_STRING_LITERAL) || tk.is(T_ANGLE_STRING_LITERAL)) {
            const int lineno = cursor.blockNumber() + 1;
            const QList<Document::Include> includes = doc->resolvedIncludes();
            for (const Document::Include &incl : includes) {
                if (incl.line() == lineno) {
                    link.targetFilePath = incl.resolvedFileName();
                    link.linkTextStart = beginOfToken + 1;
                    link.linkTextEnd = endOfToken - 1;
                    processLinkCallback(link);
                    return;
                }
            }
        }

        if (tk.isNot(T_IDENTIFIER) && !tk.isQtKeyword())
            return processLinkCallback(link);

        tc.setPosition(endOfToken);
    }

    // Handle macro uses
    const Macro *macro = doc->findMacroDefinitionAt(line);
    if (macro) {
        QTextCursor macroCursor = cursor;
        const QByteArray name = identifierUnderCursor(&macroCursor).toUtf8();
        if (macro->name() == name)
            return processLinkCallback(link); //already on definition!
    } else if (const Document::MacroUse *use = doc->findMacroUseAt(endOfToken - 1)) {
        const FilePath filePath = use->macro().filePath();
        if (filePath.path() == CppModelManager::editorConfigurationFileName().path()) {
            editorWidget->showPreProcessorWidget();
        } else if (filePath.path() != CppModelManager::configurationFileName().path()) {
            const Macro &macro = use->macro();
            link.targetFilePath = macro.filePath();
            link.target.line = macro.line();
            link.linkTextStart = use->utf16charsBegin();
            link.linkTextEnd = use->utf16charsEnd();
        }
        processLinkCallback(link);
        return;
    }

    // Find the last symbol up to the cursor position
    Scope *scope = doc->scopeAt(line, column);
    if (!scope)
        return processLinkCallback(link);

    // Evaluate the type of the expression under the cursor
    QTC_CHECK(document == tc.document());
    const QString expression = expressionUnderCursorAsString(tc, documentFromSemanticInfo,
                                                             features);
    const QSharedPointer<TypeOfExpression> typeOfExpression(new TypeOfExpression);
    typeOfExpression->init(doc, snapshot);
    // make possible to instantiate templates
    typeOfExpression->setExpandTemplates(true);
    const QList<LookupItem> resolvedSymbols =
            typeOfExpression->reference(expression.toUtf8(), scope, TypeOfExpression::Preprocess);

    if (!resolvedSymbols.isEmpty()) {
        LookupItem result = skipForwardDeclarations(resolvedSymbols);

        for (const LookupItem &r : resolvedSymbols) {
            if (Symbol *d = r.declaration()) {
                if (d->asDeclaration() || d->asFunction()) {
                    if (data.filePath() == d->filePath()) {
                        if (line == d->line() && column >= d->column()) {
                            // TODO: check the end
                            result = r; // take the symbol under cursor.
                            break;
                        }
                    }
                } else if (d->asUsingDeclaration()) {
                    int tokenBeginLineNumber = 0;
                    int tokenBeginColumnNumber = 0;
                    Utils::Text::convertPosition(document, beginOfToken, &tokenBeginLineNumber,
                                                 &tokenBeginColumnNumber);
                    if (tokenBeginLineNumber > d->line()
                            || (tokenBeginLineNumber == d->line()
                                && tokenBeginColumnNumber + 1 >= d->column())) {
                        result = r; // take the symbol under cursor.
                        break;
                    }
                }
            }
        }

        if (Symbol *symbol = result.declaration()) {
            Symbol *def = nullptr;

            if (resolveTarget) {
                // Consider to show a pop-up displaying overrides for the function
                Function *function = symbol->type()->asFunctionType();
                VirtualFunctionHelper helper(*typeOfExpression, scope, doc, snapshot, symbolFinder);

                if (helper.canLookupVirtualFunctionOverrides(function)) {
                    VirtualFunctionAssistProvider::Parameters params;
                    params.function = function;
                    params.staticClass = helper.staticClassOfFunctionCallExpression();
                    params.typeOfExpression = typeOfExpression;
                    params.snapshot = snapshot;
                    params.cursorPosition = cursor.position();
                    params.openInNextSplit = inNextSplit;

                    if (m_virtualFunctionAssistProvider->configure(params)) {
                        editorWidget->invokeTextEditorWidgetAssist(
                                    FollowSymbol,m_virtualFunctionAssistProvider.data());
                        m_virtualFunctionAssistProvider->clearParams();
                    }

                    // Ensure a valid link text, so the symbol name will be underlined on Ctrl+Hover.
                    Link link;
                    link.linkTextStart = beginOfToken;
                    link.linkTextEnd = endOfToken;
                    processLinkCallback(link);
                    return;
                }

                Symbol *lastVisibleSymbol = doc->lastVisibleSymbolAt(line, column);

                def = findDefinition(symbol, snapshot, symbolFinder);

                if (def == lastVisibleSymbol)
                    def = nullptr; // jump to declaration then.

                if (symbol->asForwardClassDeclaration()) {
                    def = symbolFinder->findMatchingClassDeclaration(symbol, snapshot);
                } else if (Template *templ = symbol->asTemplate()) {
                    if (Symbol *declaration = templ->declaration()) {
                        if (declaration->asForwardClassDeclaration())
                            def = symbolFinder->findMatchingClassDeclaration(declaration, snapshot);
                    }
                }

            }

            link = (def ? def : symbol)->toLink();
            link.linkTextStart = beginOfToken;
            link.linkTextEnd = endOfToken;
            processLinkCallback(link);
            return;
        }
    }

    // Handle macro uses
    QTextCursor macroCursor = cursor;
    const QByteArray name = identifierUnderCursor(&macroCursor).toUtf8();
    link = findMacroLink(name, documentFromSemanticInfo);
    if (link.hasValidTarget()) {
        link.linkTextStart = macroCursor.selectionStart();
        link.linkTextEnd = macroCursor.selectionEnd();
        processLinkCallback(link);
        return;
    }

    processLinkCallback(Link());
}

void FollowSymbolUnderCursor::findParentImpl(
    const CursorInEditor &data,
    const Utils::LinkHandler &processLinkCallback,
    const CPlusPlus::Snapshot &snapshot,
    const CPlusPlus::Document::Ptr &documentFromSemanticInfo,
    SymbolFinder *symbolFinder)
{
    using namespace Internal;

    const auto noOp = [&processLinkCallback] { return processLinkCallback({}); };

    if (!documentFromSemanticInfo)
        return noOp();

    // Locate the function declaration whose "parent" we want to find.
    const Declarations decls = declsFromCursor(documentFromSemanticInfo, data.cursor());
    Symbol *funcDecl = decls.funcDecl;
    if (!funcDecl && decls.funcDef)
        funcDecl = functionDeclFromDef(decls.funcDef, documentFromSemanticInfo, snapshot);
    if (!funcDecl)
        return noOp();

    LookupContext context(documentFromSemanticInfo, snapshot);
    if (!FunctionUtils::isVirtualFunction(funcDecl->type()->asFunctionType(), context))
        return noOp();

    QHash<FilePath, LookupContext> contexts;
    contexts.insert(documentFromSemanticInfo->filePath(), context);
    const auto contextForPath = [&](const FilePath &fp) -> LookupContext & {
        LookupContext &lc = contexts[fp];
        if (lc.thisDocument().isNull())
            lc = LookupContext(snapshot.document(fp), snapshot);
        return lc;
    };
    const auto classForBaseClass = [&](const Class *c, const BaseClass *bc) -> const Class * {
        const QList<LookupItem> items
            = contextForPath(c->filePath()).lookup(bc->name(), c->enclosingScope());
        for (const LookupItem &item : items) {
            if (!item.declaration())
                continue;
            if (const Class * const klass = item.declaration()->asClass())
                return klass;

            // TODO: Typedefs, templates
        }
        return nullptr;
    };

    QTC_ASSERT(funcDecl->enclosingScope(), return noOp());
    const Class * const theClass = funcDecl->enclosingScope()->asClass();
    QTC_ASSERT(theClass, return noOp());

    // Go through the base classes and look for a matching virtual function.
    QQueue<const Class *> classesWhoseBaseClassesToCheck;
    classesWhoseBaseClassesToCheck.enqueue(theClass);
    while (!classesWhoseBaseClassesToCheck.isEmpty()) {
        const Class * const c = classesWhoseBaseClassesToCheck.dequeue();
        for (int i = 0; i < c->baseClassCount(); ++i) {
            BaseClass * const bc = c->baseClassAt(i);
            const Class * const actualBaseClass = classForBaseClass(c, bc);
            if (!actualBaseClass)
                continue;
            for (auto s = actualBaseClass->memberBegin(); s != actualBaseClass->memberEnd(); ++s) {
                Function * const func = (*s)->type()->asFunctionType();
                if (!func)
                    continue;
                if (!FunctionUtils::isVirtualFunction(
                        func, contextForPath(actualBaseClass->filePath()))) {
                    continue;
                }

                // Match!
                // TODO: Operators
                if (func->identifier() && func->identifier()->match(funcDecl->identifier())
                    && func->type() && func->type()->match(funcDecl->type().type())) {

                    // Prefer implementation.
                    if (const Function * const def
                        = symbolFinder->findMatchingDefinition(func, snapshot, true)) {
                        return processLinkCallback(def->toLink());
                    }
                    return processLinkCallback(func->toLink());
                }
            }
            classesWhoseBaseClassesToCheck << actualBaseClass;
        }
    }

    noOp();
}

void FollowSymbolUnderCursor::switchDeclDef(
        const CursorInEditor &data,
        const Utils::LinkHandler &processLinkCallback,
        const CPlusPlus::Snapshot &snapshot,
        const CPlusPlus::Document::Ptr &documentFromSemanticInfo,
        SymbolFinder *symbolFinder)
{
    if (!documentFromSemanticInfo) {
        processLinkCallback({});
        return;
    }

    const Declarations decls = declsFromCursor(documentFromSemanticInfo, data.cursor());

    Utils::Link symbolLink;
    if (decls.funcDecl) {
        if (Symbol *symbol = symbolFinder->findMatchingDefinition(decls.funcDecl, snapshot, false))
            symbolLink = symbol->toLink();
    } else if (decls.decl) {
        if (Symbol *symbol = symbolFinder->findMatchingVarDefinition(decls.decl, snapshot))
            symbolLink = symbol->toLink();
    } else if (decls.funcDef) {
        if (const Symbol * const decl
            = functionDeclFromDef(decls.funcDef, documentFromSemanticInfo, snapshot)) {
            symbolLink = decl->toLink();
        }
    }
    processLinkCallback(symbolLink);
}

QSharedPointer<VirtualFunctionAssistProvider> FollowSymbolUnderCursor::virtualFunctionAssistProvider()
{
    return m_virtualFunctionAssistProvider;
}

void FollowSymbolUnderCursor::setVirtualFunctionAssistProvider(
        const QSharedPointer<VirtualFunctionAssistProvider> &provider)
{
    m_virtualFunctionAssistProvider = provider;
}

#ifdef WITH_TESTS
namespace Internal {
class FindParentImplTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("source");

        QTest::newRow("not a virtual function") << QByteArray("class C { void @foo(); };");

        const QByteArray complex = R"cpp(
namespace A {
class Base {
virtual void @0foo(int) {}
virtual void foo(double) {}
};
}
namespace B { class Derived1 : public A::Base { void foo(int) override; }; }
void B::Derived1::@1foo(int) {}
namespace C { class Derived2 : public B::Derived1 { void foo(int) override; }; }
void C::Derived2::@2foo(int) {}
class Derived3 : public C::Derived2 { void foo(double) override; }
class Derived4 : public Derived3 { void @3foo(int) override; }
)cpp";

        QByteArray source = complex;
        source.replace("@3", "@").replace("@2", "$").replace("@1", "").replace("@0", "");
        QTest::newRow("derived decl to base impl") << source;

        source = complex;
        source.replace("@3", "").replace("@2", "@").replace("@1", "$").replace("@0", "");
        QTest::newRow("derived impl to base impl") << source;

        source = complex;
        source.replace("@3", "").replace("@2", "").replace("@1", "@").replace("@0", "$");
        QTest::newRow("derived impl to base decl") << source;

        source = complex;
        source.replace("@3", "").replace("@2", "").replace("@1", "").replace("@0", "@");
        QTest::newRow("base decl to nothing") << source;
    }

    void test()
    {
        using namespace Tests;
        using namespace CppEditor::Tests;

        QFETCH(QByteArray, source);

        TemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        TestDocumentPtr doc = CppTestDocument::create(source, "test.cpp");
        QVERIFY(doc->hasCursorMarker());
        doc->setBaseDirectory(tempDir.path());
        QVERIFY(doc->writeToDisk());
        TestCase testCase;
        QVERIFY(testCase.succeededSoFar());
        QVERIFY(testCase.openCppEditor(doc->filePath(), &doc->m_editor, &doc->m_editorWidget));
        testCase.closeEditorAtEndOfTestCase(doc->m_editor);
        TextDocument * const textDoc = doc->m_editorWidget->cppEditorDocument();
        BaseEditorDocumentProcessor * const processor = CppModelManager::cppEditorDocumentProcessor(
            doc->filePath());
        QVERIFY(processor);
        QVERIFY(TestCase::waitForProcessedEditorDocument(doc->filePath()));
        TestCase::waitForRehighlightedSemanticDocument(doc->m_editorWidget);
        Snapshot snapshot = processor->snapshot();
        QCOMPARE(snapshot.size(), 2); // Configuration file included

        Link expectedTarget;
        if (doc->hasTargetCursorMarker()) {
            expectedTarget.targetFilePath = doc->filePath();
            expectedTarget.target = Text::Position::fromPositionInDocument(
                textDoc->document(), doc->m_targetCursorPosition);
        }
        Link actualTarget;

        QTimer timer;
        QEventLoop loop;
        const auto handler = [&](const Link &link) {
            actualTarget = link;
            timer.stop();
            loop.quit();
        };
        QTextCursor cursor(textDoc->document());
        cursor.setPosition(doc->m_cursorPosition);
        QTimer::singleShot(0, [&] {
            CppModelManager::followFunctionToParentImpl(
                {cursor, doc->filePath(), doc->m_editorWidget, textDoc}, handler);
        });
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, [&loop] { loop.exit(1); });
        timer.start(TestCase::defaultTimeOutInMs);
        QCOMPARE(loop.exec(), 0);
        QCOMPARE(actualTarget.hasValidTarget(), expectedTarget.hasValidTarget());
        if (expectedTarget.hasValidTarget())
            QCOMPARE(actualTarget, expectedTarget);

    }
};

QObject *createFindParentImplTest() { return new FindParentImplTest; }

} // namespace Internal
#endif

} // namespace CppEditor


#ifdef WITH_TESTS
#include <cppfollowsymbolundercursor.moc>
#endif
