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

#include "cppfollowsymbolundercursor.h"
#include "cppeditor.h"
#include "cppvirtualfunctionassistprovider.h"

#include <cplusplus/ASTPath.h>
#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/ResolveExpression.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/TypeOfExpression.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/functionutils.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/symbolfinder.h>
#include <texteditor/textdocumentlayout.h>
#include <utils/qtcassert.h>

#include <QList>
#include <QSet>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace TextEditor;

typedef TextEditorWidget::Link Link;

namespace {

class VirtualFunctionHelper {
public:
    VirtualFunctionHelper(TypeOfExpression &typeOfExpression,
                          Scope *scope,
                          const Document::Ptr &document,
                          const Snapshot &snapshot,
                          SymbolFinder *symbolFinder);

    bool canLookupVirtualFunctionOverrides(Function *function);

    /// Returns != 0 if canLookupVirtualFunctionOverrides() succeeded.
    Class *staticClassOfFunctionCallExpression() const
    { return m_staticClassOfFunctionCallExpression; }

private:
    VirtualFunctionHelper();
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
    ExpressionAST *m_baseExpressionAST;
    Function *m_function;
    int m_accessTokenKind;
    Class *m_staticClassOfFunctionCallExpression; // Output
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
    , m_baseExpressionAST(0)
    , m_function(0)
    , m_accessTokenKind(0)
    , m_staticClassOfFunctionCallExpression(0)
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
    if (!m_function || !m_baseExpressionAST || !m_expressionDocument || !m_document || !m_scope
            || m_scope->isClass() || m_scope->isFunction() || m_snapshot.isEmpty()) {
        return false;
    }

    bool result = false;

    if (IdExpressionAST *idExpressionAST = m_baseExpressionAST->asIdExpression()) {
        NameAST *name = idExpressionAST->name;
        const bool nameIsQualified = name && name->asQualifiedName();
        result = !nameIsQualified && FunctionUtils::isVirtualFunction(
                    function, LookupContext(m_document, m_snapshot));
    } else if (MemberAccessAST *memberAccessAST = m_baseExpressionAST->asMemberAccess()) {
        NameAST *name = memberAccessAST->member_name;
        const bool nameIsQualified = name && name->asQualifiedName();
        if (!nameIsQualified && FunctionUtils::isVirtualFunction(
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
                        result = declaration->type()->isReferenceType();
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
        return 0;

    Class *result = 0;

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
                    if (first->isForwardClassDeclaration())
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
    if (doc && !name.startsWith('<') && !processed->contains(doc->fileName())) {
        processed->insert(doc->fileName());

        foreach (const Macro &macro, doc->definedMacros()) {
            if (macro.name() == name) {
                Link link;
                link.targetFileName = macro.fileName();
                link.targetLine = macro.line();
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
            const Snapshot snapshot = CppModelManager::instance()->snapshot();
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

    if (type->isForwardClassDeclarationType()) {
        return true;
    } else if (Template *templ = type->asTemplateType()) {
        if (Symbol *declaration = templ->declaration()) {
            if (declaration->isForwardClassDeclaration())
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

    if (ty->isObjCForwardClassDeclarationType()) {
        while (!candidates.isEmpty()) {
            LookupItem r = candidates.takeFirst();

            if (!r.type()->isObjCForwardClassDeclarationType()) {
                result = r;
                break;
            }
        }
    }

    if (ty->isObjCForwardProtocolDeclarationType()) {
        while (!candidates.isEmpty()) {
            LookupItem r = candidates.takeFirst();

            if (!r.type()->isObjCForwardProtocolDeclarationType()) {
                result = r;
                break;
            }
        }
    }

    return result;
}

CppEditorWidget::Link attemptFuncDeclDef(const QTextCursor &cursor,
    CppEditorWidget *, Snapshot snapshot, const Document::Ptr &document,
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

        if (node->asParameterDeclaration() != 0)
            return result;
    }

    AST *declParent = 0;
    DeclaratorAST *decl = 0;
    for (int i = path.size() - 2; i > 0; --i) {
        if ((decl = path.at(i)->asDeclarator()) != 0) {
            declParent = path.at(i - 1);
            break;
        }
    }
    if (!decl || !declParent)
        return result;
    if (!decl->postfix_declarator_list || !decl->postfix_declarator_list->value)
        return result;
    FunctionDeclaratorAST *funcDecl = decl->postfix_declarator_list->value->asFunctionDeclarator();
    if (!funcDecl)
        return result;

    Symbol *target = 0;
    if (FunctionDefinitionAST *funDef = declParent->asFunctionDefinition()) {
        QList<Declaration *> candidates =
                symbolFinder->findMatchingDeclaration(LookupContext(document, snapshot),
                                                        funDef->symbol);
        if (!candidates.isEmpty()) // TODO: improve disambiguation
            target = candidates.first();
    } else if (declParent->asSimpleDeclaration()) {
        target = symbolFinder->findMatchingDefinition(funcDecl->symbol, snapshot);
    }

    if (target) {
        result = CppTools::linkToSymbol(target);

        unsigned startLine, startColumn, endLine, endColumn;
        document->translationUnit()->getTokenStartPosition(name->firstToken(), &startLine,
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
    if (symbol->isFunction())
        return 0; // symbol is a function definition.

    else if (!symbol->type()->isFunctionType())
        return 0; // not a function declaration

    return symbolFinder->findMatchingDefinition(symbol, snapshot);
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
    for (auto it = astPath.cend() - 1, begin = astPath.cbegin(); it >= begin; --it) {
        const auto nextIt = it + 1;
        const auto nextNextIt = nextIt + 1;
        if (nextNextIt != astPath.cend() && (*it)->asTrailingReturnType()) {
            return (*nextIt)->asNamedTypeSpecifier()
                    && ((*nextNextIt)->asSimpleName()
                        || (*nextNextIt)->asQualifiedName()
                        || (*nextNextIt)->asTemplateId());
        }
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

FollowSymbolUnderCursor::FollowSymbolUnderCursor(CppEditorWidget *widget)
    : m_widget(widget)
    , m_virtualFunctionAssistProvider(new VirtualFunctionAssistProvider)
{
}

FollowSymbolUnderCursor::~FollowSymbolUnderCursor()
{
    delete m_virtualFunctionAssistProvider;
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

TextEditorWidget::Link FollowSymbolUnderCursor::findLink(const QTextCursor &cursor,
    bool resolveTarget, const Snapshot &theSnapshot, const Document::Ptr &documentFromSemanticInfo,
    SymbolFinder *symbolFinder, bool inNextSplit)
{
    Link link;

    Snapshot snapshot = theSnapshot;

    // Move to end of identifier
    QTextCursor tc = cursor;
    QTextDocument *document = m_widget->document();
    QChar ch = document->characterAt(tc.position());
    while (CppTools::isValidIdentifierChar(ch)) {
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
        if (document->characterAt(pos) == QLatin1Char('(')) {
            link = attemptFuncDeclDef(cursor, m_widget, snapshot, documentFromSemanticInfo,
                                      symbolFinder);
            if (link.hasValidLinkText())
                return link;
        }
    }

    int lineNumber = 0, positionInBlock = 0;
    m_widget->convertPosition(cursor.position(), &lineNumber, &positionInBlock);
    const unsigned line = lineNumber;
    const unsigned column = positionInBlock + 1;

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

        if (((unsigned) positionInBlock) >= tk.utf16charsBegin()
                && ((unsigned) positionInBlock) < tk.utf16charsEnd()) {
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
            if (unsigned(positionInBlock) >= tk.utf16charsBegin()
                    && unsigned(positionInBlock) <= tk.utf16charsEnd()) {
                cursorRegionReached = true;
                if (tk.is(T_OPERATOR)) {
                    link = attemptFuncDeclDef(cursor, m_widget, theSnapshot,
                                              documentFromSemanticInfo, symbolFinder);
                    if (link.hasValidLinkText())
                        return link;
                } else if (tk.isOperator() && i > 0 && tokens.at(i - 1).is(T_OPERATOR)) {
                    QTextCursor c = cursor;
                    c.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor,
                                   positionInBlock - tokens.at(i - 1).utf16charsBegin());
                    link = attemptFuncDeclDef(c, m_widget, theSnapshot, documentFromSemanticInfo,
                                              symbolFinder);
                    if (link.hasValidLinkText())
                        return link;
                }
            } else if (cursorRegionReached) {
                break;
            }
        }
    }

    // Now we prefer the doc from the snapshot with macros expanded.
    Document::Ptr doc = snapshot.document(m_widget->textDocument()->filePath());
    if (!doc) {
        doc = documentFromSemanticInfo;
        if (!doc)
            return link;
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
            const unsigned lineno = cursor.blockNumber() + 1;
            foreach (const Document::Include &incl, doc->resolvedIncludes()) {
                if (incl.line() == lineno) {
                    link.targetFileName = incl.resolvedFileName();
                    link.linkTextStart = beginOfToken + 1;
                    link.linkTextEnd = endOfToken - 1;
                    return link;
                }
            }
        }

        if (tk.isNot(T_IDENTIFIER) && !tk.isQtKeyword())
            return link;

        tc.setPosition(endOfToken);
    }

    // Handle macro uses
    const Macro *macro = doc->findMacroDefinitionAt(line);
    if (macro) {
        QTextCursor macroCursor = cursor;
        const QByteArray name = CppTools::identifierUnderCursor(&macroCursor).toUtf8();
        if (macro->name() == name)
            return link;    //already on definition!
    } else if (const Document::MacroUse *use = doc->findMacroUseAt(endOfToken - 1)) {
        const QString fileName = use->macro().fileName();
        if (fileName == CppModelManager::editorConfigurationFileName()) {
            m_widget->showPreProcessorWidget();
        } else if (fileName != CppModelManager::configurationFileName()) {
            const Macro &macro = use->macro();
            link.targetFileName = macro.fileName();
            link.targetLine = macro.line();
            link.linkTextStart = use->utf16charsBegin();
            link.linkTextEnd = use->utf16charsEnd();
        }
        return link;
    }

    // Find the last symbol up to the cursor position
    Scope *scope = doc->scopeAt(line, column);
    if (!scope)
        return link;

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

        foreach (const LookupItem &r, resolvedSymbols) {
            if (Symbol *d = r.declaration()) {
                if (d->isDeclaration() || d->isFunction()) {
                    const QString fileName = QString::fromUtf8(d->fileName(), d->fileNameLength());
                    if (m_widget->textDocument()->filePath().toString() == fileName) {
                        if (unsigned(lineNumber) == d->line()
                            && unsigned(positionInBlock) >= d->column()) { // TODO: check the end
                            result = r; // take the symbol under cursor.
                            break;
                        }
                    }
                } else if (d->isUsingDeclaration()) {
                    int tokenBeginLineNumber = 0, tokenBeginColumnNumber = 0;
                    m_widget->convertPosition(beginOfToken, &tokenBeginLineNumber,
                                              &tokenBeginColumnNumber);
                    if (unsigned(tokenBeginLineNumber) > d->line()
                            || (unsigned(tokenBeginLineNumber) == d->line()
                                && unsigned(tokenBeginColumnNumber) > d->column())) {
                        result = r; // take the symbol under cursor.
                        break;
                    }
                }
            }
        }

        if (Symbol *symbol = result.declaration()) {
            Symbol *def = 0;

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
                        m_widget->invokeAssist(FollowSymbol, m_virtualFunctionAssistProvider);
                        m_virtualFunctionAssistProvider->clearParams();
                    }

                    // Ensure a valid link text, so the symbol name will be underlined on Ctrl+Hover.
                    Link link;
                    link.linkTextStart = beginOfToken;
                    link.linkTextEnd = endOfToken;
                    return link;
                }

                Symbol *lastVisibleSymbol = doc->lastVisibleSymbolAt(line, column);

                def = findDefinition(symbol, snapshot, symbolFinder);

                if (def == lastVisibleSymbol)
                    def = 0; // jump to declaration then.

                if (symbol->isForwardClassDeclaration()) {
                    def = symbolFinder->findMatchingClassDeclaration(symbol, snapshot);
                } else if (Template *templ = symbol->asTemplate()) {
                    if (Symbol *declaration = templ->declaration()) {
                        if (declaration->isForwardClassDeclaration())
                            def = symbolFinder->findMatchingClassDeclaration(declaration, snapshot);
                    }
                }

            }

            link = CppTools::linkToSymbol(def ? def : symbol);
            link.linkTextStart = beginOfToken;
            link.linkTextEnd = endOfToken;
            return link;
        }
    }

    // Handle macro uses
    QTextCursor macroCursor = cursor;
    const QByteArray name = CppTools::identifierUnderCursor(&macroCursor).toUtf8();
    link = findMacroLink(name, documentFromSemanticInfo);
    if (link.hasValidTarget()) {
        link.linkTextStart = macroCursor.selectionStart();
        link.linkTextEnd = macroCursor.selectionEnd();
        return link;
    }

    return Link();
}

VirtualFunctionAssistProvider *FollowSymbolUnderCursor::virtualFunctionAssistProvider()
{
    return m_virtualFunctionAssistProvider;
}

void FollowSymbolUnderCursor::setVirtualFunctionAssistProvider(VirtualFunctionAssistProvider *provider)
{
    m_virtualFunctionAssistProvider = provider;
}
