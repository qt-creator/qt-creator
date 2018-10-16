----------------------------------------------------------------------------
--
-- Copyright (C) 2016 The Qt Company Ltd.
-- Contact: http://www.qt.io/licensing/
--
-- This file is part of the QtQml module of the Qt Toolkit.
--
-- $QT_BEGIN_LICENSE:LGPL$
-- Commercial License Usage
-- Licensees holding valid commercial Qt licenses may use this file in
-- accordance with the commercial license agreement provided with the
-- Software or, alternatively, in accordance with the terms contained in
-- a written agreement between you and The Qt Company. For licensing terms
-- and conditions see https://www.qt.io/terms-conditions. For further
-- information use the contact form at https://www.qt.io/contact-us.
--
-- GNU Lesser General Public License Usage
-- Alternatively, this file may be used under the terms of the GNU Lesser
-- General Public License version 3 as published by the Free Software
-- Foundation and appearing in the file LICENSE.LGPL3 included in the
-- packaging of this file. Please review the following information to
-- ensure the GNU Lesser General Public License version 3 requirements
-- will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
--
-- GNU General Public License Usage
-- Alternatively, this file may be used under the terms of the GNU
-- General Public License version 2.0 or (at your option) the GNU General
-- Public license version 3 or any later version approved by the KDE Free
-- Qt Foundation. The licenses are as published by the Free Software
-- Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
-- included in the packaging of this file. Please review the following
-- information to ensure the GNU General Public License requirements will
-- be met: https://www.gnu.org/licenses/gpl-2.0.html and
-- https://www.gnu.org/licenses/gpl-3.0.html.
--
-- $QT_END_LICENSE$
--
----------------------------------------------------------------------------

%parser         QmlJSGrammar
%decl           qmljsparser_p.h
%impl           qmljsparser.cpp
%expect         1

%token T_AND "&"                T_AND_AND "&&"              T_AND_EQ "&="
%token T_BREAK "break"          T_CASE "case"               T_CATCH "catch"
%token T_COLON ":"              T_COMMA ","                 T_CONTINUE "continue"
%token T_DEFAULT "default"      T_DELETE "delete"           T_DIVIDE_ "/"
%token T_DIVIDE_EQ "/="         T_DO "do"                   T_DOT "."
%token T_ELSE "else"            T_EQ "="                    T_EQ_EQ "=="
%token T_EQ_EQ_EQ "==="         T_FINALLY "finally"         T_FOR "for"
%token T_FUNCTION "function"    T_GE ">="                   T_GT ">"
%token T_GT_GT ">>"             T_GT_GT_EQ ">>="            T_GT_GT_GT ">>>"
%token T_GT_GT_GT_EQ ">>>="     T_IDENTIFIER "identifier"   T_IF "if"
%token T_IN "in"                T_INSTANCEOF "instanceof"   T_LBRACE "{"
%token T_LBRACKET "["           T_LE "<="                   T_LPAREN "("
%token T_LT "<"                 T_LT_LT "<<"                T_LT_LT_EQ "<<="
%token T_MINUS "-"              T_MINUS_EQ "-="             T_MINUS_MINUS "--"
%token T_NEW "new"              T_NOT "!"                   T_NOT_EQ "!="
%token T_NOT_EQ_EQ "!=="        T_NUMERIC_LITERAL "numeric literal"     T_OR "|"
%token T_OR_EQ "|="             T_OR_OR "||"                T_PLUS "+"
%token T_PLUS_EQ "+="           T_PLUS_PLUS "++"            T_QUESTION "?"
%token T_RBRACE "}"             T_RBRACKET "]"              T_REMAINDER "%"
%token T_REMAINDER_EQ "%="      T_RETURN "return"           T_RPAREN ")"
%token T_SEMICOLON ";"          T_AUTOMATIC_SEMICOLON       T_STAR "*"
%token T_STAR_STAR "**"         T_STAR_STAR_EQ "**="        T_STAR_EQ "*="
%token T_STRING_LITERAL "string literal"
%token T_PROPERTY "property"    T_SIGNAL "signal"           T_READONLY "readonly"
%token T_SWITCH "switch"        T_THIS "this"               T_THROW "throw"
%token T_TILDE "~"              T_TRY "try"                 T_TYPEOF "typeof"
%token T_VAR "var"              T_VOID "void"               T_WHILE "while"
%token T_WITH "with"            T_XOR "^"                   T_XOR_EQ "^="
%token T_NULL "null"            T_TRUE "true"               T_FALSE "false"
%token T_CONST "const"          T_LET "let"
%token T_DEBUGGER "debugger"
%token T_RESERVED_WORD "reserved word"
%token T_MULTILINE_STRING_LITERAL "multiline string literal"
%token T_COMMENT "comment"
%token T_COMPATIBILITY_SEMICOLON
%token T_ARROW "=>"
%token T_ENUM "enum"
%token T_ELLIPSIS "..."
%token T_YIELD "yield"
%token T_SUPER "super"
%token T_CLASS "class"
%token T_EXTENDS "extends"
%token T_STATIC "static"
%token T_EXPORT "export"
%token T_FROM "from"

--- template strings
%token T_NO_SUBSTITUTION_TEMPLATE"(no subst template)"
%token T_TEMPLATE_HEAD "(template head)"
%token T_TEMPLATE_MIDDLE "(template middle)"
%token T_TEMPLATE_TAIL "(template tail)"

--- context keywords.
%token T_PUBLIC "public"
%token T_IMPORT "import"
%token T_PRAGMA "pragma"
%token T_AS "as"
%token T_OF "of"
%token T_GET "get"
%token T_SET "set"

%token T_ERROR

--- feed tokens
%token T_FEED_UI_PROGRAM
%token T_FEED_UI_OBJECT_MEMBER
%token T_FEED_JS_STATEMENT
%token T_FEED_JS_EXPRESSION
%token T_FEED_JS_SCRIPT
%token T_FEED_JS_MODULE

--- Lookahead handling
%token T_FORCE_DECLARATION "(force decl)"
%token T_FORCE_BLOCK "(force block)"
%token T_FOR_LOOKAHEAD_OK "(for lookahead ok)"

--%left T_PLUS T_MINUS
%nonassoc T_IDENTIFIER T_COLON T_SIGNAL T_PROPERTY T_READONLY T_ON T_SET T_GET T_OF T_STATIC T_FROM T_AS
%nonassoc REDUCE_HERE

%start TopLevel

/./****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmljsengine_p.h"
#include "qmljslexer_p.h"
#include "qmljsast_p.h"
#include "qmljsmemorypool_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qcoreapplication.h>

#include <string.h>

./

/:/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

//
//  W A R N I N G
//  -------------
//
// This file is automatically generated from qmljs.g.
// Changes should be made to that file, not here. Any change to this file will
// be lost!
//
// To regenerate this file, run:
//    qlalr --no-debug --no-lines --qt qmljs.g
//

#ifndef QMLJSPARSER_P_H
#define QMLJSPARSER_P_H

#include "qmljsglobal_p.h"
#include "qmljsgrammar_p.h"
#include "qmljsast_p.h"
#include "qmljsengine_p.h"

#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

QT_QML_BEGIN_NAMESPACE

namespace QmlJS {

class Engine;

class QML_PARSER_EXPORT Parser: protected $table
{
public:
    union Value {
      int ival;
      double dval;
      AST::VariableScope scope;
      AST::ForEachType forEachType;
      AST::ArgumentList *ArgumentList;
      AST::CaseBlock *CaseBlock;
      AST::CaseClause *CaseClause;
      AST::CaseClauses *CaseClauses;
      AST::Catch *Catch;
      AST::DefaultClause *DefaultClause;
      AST::Elision *Elision;
      AST::ExpressionNode *Expression;
      AST::TemplateLiteral *Template;
      AST::Finally *Finally;
      AST::FormalParameterList *FormalParameterList;
      AST::FunctionDeclaration *FunctionDeclaration;
      AST::Node *Node;
      AST::PropertyName *PropertyName;
      AST::Statement *Statement;
      AST::StatementList *StatementList;
      AST::Block *Block;
      AST::VariableDeclarationList *VariableDeclarationList;
      AST::Pattern *Pattern;
      AST::PatternElement *PatternElement;
      AST::PatternElementList *PatternElementList;
      AST::PatternProperty *PatternProperty;
      AST::PatternPropertyList *PatternPropertyList;
      AST::ClassElementList *ClassElementList;
      AST::ImportClause *ImportClause;
      AST::FromClause *FromClause;
      AST::NameSpaceImport *NameSpaceImport;
      AST::ImportsList *ImportsList;
      AST::NamedImports *NamedImports;
      AST::ImportSpecifier *ImportSpecifier;
      AST::ExportSpecifier *ExportSpecifier;
      AST::ExportsList *ExportsList;
      AST::ExportClause *ExportClause;
      AST::ExportDeclaration *ExportDeclaration;

      AST::UiProgram *UiProgram;
      AST::UiHeaderItemList *UiHeaderItemList;
      AST::UiPragma *UiPragma;
      AST::UiImport *UiImport;
      AST::UiParameterList *UiParameterList;
      AST::UiPublicMember *UiPublicMember;
      AST::UiObjectDefinition *UiObjectDefinition;
      AST::UiObjectInitializer *UiObjectInitializer;
      AST::UiObjectBinding *UiObjectBinding;
      AST::UiScriptBinding *UiScriptBinding;
      AST::UiArrayBinding *UiArrayBinding;
      AST::UiObjectMember *UiObjectMember;
      AST::UiObjectMemberList *UiObjectMemberList;
      AST::UiArrayMemberList *UiArrayMemberList;
      AST::UiQualifiedId *UiQualifiedId;
      AST::UiEnumMemberList *UiEnumMemberList;
    };

public:
    Parser(Engine *engine);
    ~Parser();

    // parse a UI program
    bool parse() { ++functionNestingLevel; bool r = parse(T_FEED_UI_PROGRAM); --functionNestingLevel; return r; }
    bool parseStatement() { return parse(T_FEED_JS_STATEMENT); }
    bool parseExpression() { return parse(T_FEED_JS_EXPRESSION); }
    bool parseUiObjectMember() { ++functionNestingLevel; bool r = parse(T_FEED_UI_OBJECT_MEMBER); --functionNestingLevel; return r; }
    bool parseProgram() { return parse(T_FEED_JS_SCRIPT); }
    bool parseScript() { return parse(T_FEED_JS_SCRIPT); }
    bool parseModule() { return parse(T_FEED_JS_MODULE); }

    AST::UiProgram *ast() const
    { return AST::cast<AST::UiProgram *>(program); }

    AST::Statement *statement() const
    {
        if (! program)
            return 0;

        return program->statementCast();
    }

    AST::ExpressionNode *expression() const
    {
        if (! program)
            return 0;

        return program->expressionCast();
    }

    AST::UiObjectMember *uiObjectMember() const
    {
        if (! program)
            return 0;

        return program->uiObjectMemberCast();
    }

    AST::Node *rootNode() const
    { return program; }

    QList<DiagnosticMessage> diagnosticMessages() const
    { return diagnostic_messages; }

    inline DiagnosticMessage diagnosticMessage() const
    {
        for (const DiagnosticMessage &d : diagnostic_messages) {
            if (d.kind != Severity::Warning)
                return d;
        }

        return DiagnosticMessage();
    }

    inline QString errorMessage() const
    { return diagnosticMessage().message; }

    inline int errorLineNumber() const
    { return diagnosticMessage().loc.startLine; }

    inline int errorColumnNumber() const
    { return diagnosticMessage().loc.startColumn; }

protected:
    bool parse(int startToken);

    void reallocateStack();

    inline Value &sym(int index)
    { return sym_stack [tos + index - 1]; }

    inline QStringRef &stringRef(int index)
    { return string_stack [tos + index - 1]; }

    inline AST::SourceLocation &loc(int index)
    { return location_stack [tos + index - 1]; }

    AST::UiQualifiedId *reparseAsQualifiedId(AST::ExpressionNode *expr);

    void pushToken(int token);
    int lookaheadToken(Lexer *lexer);

    void syntaxError(const AST::SourceLocation &location, const char *message) {
        diagnostic_messages.append(DiagnosticMessage(Severity::Error, location, QLatin1String(message)));
     }
     void syntaxError(const AST::SourceLocation &location, const QString &message) {
         diagnostic_messages.append(DiagnosticMessage(Severity::Error, location, message));
      }

protected:
    Engine *driver;
    MemoryPool *pool;
    int tos = 0;
    int stack_size = 0;
    Value *sym_stack = nullptr;
    int *state_stack = nullptr;
    AST::SourceLocation *location_stack = nullptr;
    QVector<QStringRef> string_stack;

    AST::Node *program = nullptr;

    // error recovery and lookahead handling
    enum { TOKEN_BUFFER_SIZE = 5 };

    struct SavedToken {
       int token;
       double dval;
       AST::SourceLocation loc;
       QStringRef spell;
    };

    int yytoken = -1;
    double yylval = 0.;
    QStringRef yytokenspell;
    AST::SourceLocation yylloc;
    AST::SourceLocation yyprevlloc;

    SavedToken token_buffer[TOKEN_BUFFER_SIZE];
    SavedToken *first_token = nullptr;
    SavedToken *last_token = nullptr;

    int functionNestingLevel = 0;

    enum CoverExpressionType {
        CE_Invalid,
        CE_ParenthesizedExpression,
        CE_FormalParameterList
    };
    AST::SourceLocation coverExpressionErrorLocation;
    CoverExpressionType coverExpressionType = CE_Invalid;

    QList<DiagnosticMessage> diagnostic_messages;
};

} // end of namespace QmlJS


:/


/.

#include "qmljsparser_p.h"

#include <QtCore/qvarlengtharray.h>

//
//  W A R N I N G
//  -------------
//
// This file is automatically generated from qmljs.g.
// Changes should be made to that file, not here. Any change to this file will
// be lost!
//
// To regenerate this file, run:
//    qlalr --no-debug --no-lines --qt qmljs.g
//

#define UNIMPLEMENTED syntaxError(loc(1), "Unimplemented"); return false

using namespace QmlJS;

QT_QML_BEGIN_NAMESPACE

void Parser::reallocateStack()
{
    if (! stack_size)
        stack_size = 128;
    else
        stack_size <<= 1;

    sym_stack = reinterpret_cast<Value*> (realloc(sym_stack, stack_size * sizeof(Value)));
    state_stack = reinterpret_cast<int*> (realloc(state_stack, stack_size * sizeof(int)));
    location_stack = reinterpret_cast<AST::SourceLocation*> (realloc(location_stack, stack_size * sizeof(AST::SourceLocation)));
    string_stack.resize(stack_size);
}

Parser::Parser(Engine *engine):
    driver(engine),
    pool(engine->pool())
{
}

Parser::~Parser()
{
    if (stack_size) {
        free(sym_stack);
        free(state_stack);
        free(location_stack);
    }
}

static inline AST::SourceLocation location(Lexer *lexer)
{
    AST::SourceLocation loc;
    loc.offset = lexer->tokenOffset();
    loc.length = lexer->tokenLength();
    loc.startLine = lexer->tokenStartLine();
    loc.startColumn = lexer->tokenStartColumn();
    return loc;
}

AST::UiQualifiedId *Parser::reparseAsQualifiedId(AST::ExpressionNode *expr)
{
    QVarLengthArray<QStringRef, 4> nameIds;
    QVarLengthArray<AST::SourceLocation, 4> locations;

    AST::ExpressionNode *it = expr;
    while (AST::FieldMemberExpression *m = AST::cast<AST::FieldMemberExpression *>(it)) {
        nameIds.append(m->name);
        locations.append(m->identifierToken);
        it = m->base;
    }

    if (AST::IdentifierExpression *idExpr = AST::cast<AST::IdentifierExpression *>(it)) {
        AST::UiQualifiedId *q = new (pool) AST::UiQualifiedId(idExpr->name);
        q->identifierToken = idExpr->identifierToken;

        AST::UiQualifiedId *currentId = q;
        for (int i = nameIds.size() - 1; i != -1; --i) {
            currentId = new (pool) AST::UiQualifiedId(currentId, nameIds[i]);
            currentId->identifierToken = locations[i];
        }

        return currentId->finish();
    }

    return 0;
}

void Parser::pushToken(int token)
{
    last_token->token = yytoken;
    last_token->dval = yylval;
    last_token->spell = yytokenspell;
    last_token->loc = yylloc;
    ++last_token;
    yytoken = token;
}

int Parser::lookaheadToken(Lexer *lexer)
{
    if (yytoken < 0) {
        yytoken = lexer->lex();
        yylval = lexer->tokenValue();
        yytokenspell = lexer->tokenSpell();
        yylloc = location(lexer);
    }
    return yytoken;
}

//#define PARSER_DEBUG

bool Parser::parse(int startToken)
{
    Lexer *lexer = driver->lexer();
    bool hadErrors = false;
    yytoken = -1;
    int action = 0;

    token_buffer[0].token = startToken;
    first_token = &token_buffer[0];
    if (startToken == T_FEED_JS_SCRIPT && !lexer->qmlMode()) {
        Directives ignoreDirectives;
        Directives *directives = driver->directives();
        if (!directives)
            directives = &ignoreDirectives;
        DiagnosticMessage error;
        if (!lexer->scanDirectives(directives, &error)) {
            diagnostic_messages.append(error);
            return false;
        }
        token_buffer[1].token = lexer->tokenKind();
        token_buffer[1].dval = lexer->tokenValue();
        token_buffer[1].loc = location(lexer);
        token_buffer[1].spell = lexer->tokenSpell();
        last_token = &token_buffer[2];
    } else {
        last_token = &token_buffer[1];
    }

    tos = -1;
    program = 0;

    do {
        if (++tos == stack_size)
            reallocateStack();

        state_stack[tos] = action;

    _Lcheck_token:
        if (yytoken == -1 && -TERMINAL_COUNT != action_index[action]) {
            yyprevlloc = yylloc;

            if (first_token == last_token) {
                yytoken = lexer->lex();
                yylval = lexer->tokenValue();
                yytokenspell = lexer->tokenSpell();
                yylloc = location(lexer);
            } else {
                yytoken = first_token->token;
                yylval = first_token->dval;
                yytokenspell = first_token->spell;
                yylloc = first_token->loc;
                ++first_token;
                if (first_token == last_token)
                    first_token = last_token = &token_buffer[0];
            }
        }

#ifdef PARSER_DEBUG
       qDebug() << "   in state" << action;
#endif

        action = t_action(action, yytoken);
#ifdef PARSER_DEBUG
       qDebug() << "   current token" << yytoken << (yytoken >= 0 ? spell[yytoken] : "(null)") << "new state" << action;
#endif
        if (action > 0) {
            if (action != ACCEPT_STATE) {
                yytoken = -1;
                sym(1).dval = yylval;
                stringRef(1) = yytokenspell;
                loc(1) = yylloc;
            } else {
              --tos;
              return ! hadErrors;
            }
        } else if (action < 0) {
          const int r = -action - 1;
          tos -= rhs[r];

#ifdef PARSER_DEBUG
          qDebug() << "        reducing through rule " << -action;
#endif

          switch (r) {
./

--------------------------------------------------------------------------------------------------------
-- Declarative UI
--------------------------------------------------------------------------------------------------------

TopLevel: T_FEED_UI_PROGRAM UiProgram;
/.
    case $rule_number: {
        sym(1).Node = sym(2).Node;
        program = sym(1).Node;
    } break;
./

TopLevel: T_FEED_JS_STATEMENT Statement;
/.
    case $rule_number: {
        sym(1).Node = sym(2).Node;
        program = sym(1).Node;
    } break;
./

TopLevel: T_FEED_JS_EXPRESSION Expression;
/.
    case $rule_number: {
        sym(1).Node = sym(2).Node;
        program = sym(1).Node;
    } break;
./

TopLevel: T_FEED_UI_OBJECT_MEMBER UiObjectMember;
/.
    case $rule_number: {
        sym(1).Node = sym(2).Node;
        program = sym(1).Node;
    } break;
./

TopLevel: T_FEED_JS_SCRIPT Script;
/.
    case $rule_number: {
        sym(1).Node = sym(2).Node;
        program = sym(1).Node;
    } break;
./

TopLevel: T_FEED_JS_MODULE Module;
/.
    case $rule_number: {
        sym(1).Node = sym(2).Node;
        program = sym(1).Node;
    } break;
./


UiProgram: UiHeaderItemListOpt UiRootMember;
/.
    case $rule_number: {
        sym(1).UiProgram = new (pool) AST::UiProgram(sym(1).UiHeaderItemList, sym(2).UiObjectMemberList->finish());
    } break;
./

UiHeaderItemListOpt: Empty;
UiHeaderItemListOpt: UiHeaderItemList;
/.
    case $rule_number: {
        sym(1).Node = sym(1).UiHeaderItemList->finish();
    } break;
./

UiHeaderItemList: UiPragma;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::UiHeaderItemList(sym(1).UiPragma);
    } break;
./

UiHeaderItemList: UiImport;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::UiHeaderItemList(sym(1).UiImport);
    } break;
./

UiHeaderItemList: UiHeaderItemList UiPragma;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::UiHeaderItemList(sym(1).UiHeaderItemList, sym(2).UiPragma);
    } break;
./

UiHeaderItemList: UiHeaderItemList UiImport;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::UiHeaderItemList(sym(1).UiHeaderItemList, sym(2).UiImport);
    } break;
./

PragmaId: JsIdentifier;

UiPragma: T_PRAGMA PragmaId T_AUTOMATIC_SEMICOLON;
UiPragma: T_PRAGMA PragmaId T_SEMICOLON;
/.
    case $rule_number: {
        AST::UiPragma *pragma = new (pool) AST::UiPragma(stringRef(2));
        pragma->pragmaToken = loc(1);
        pragma->semicolonToken = loc(3);
        sym(1).Node = pragma;
    } break;
./

ImportId: MemberExpression;

UiImport: UiImportHead T_AUTOMATIC_SEMICOLON;
UiImport: UiImportHead T_SEMICOLON;
/.
    case $rule_number: {
        sym(1).UiImport->semicolonToken = loc(2);
    } break;
./

UiImport: UiImportHead T_NUMERIC_LITERAL T_AUTOMATIC_SEMICOLON;
UiImport: UiImportHead T_NUMERIC_LITERAL T_SEMICOLON;
/.
    case $rule_number: {
        sym(1).UiImport->versionToken = loc(2);
        sym(1).UiImport->semicolonToken = loc(3);
    } break;
./

UiImport: UiImportHead T_NUMERIC_LITERAL T_AS QmlIdentifier T_AUTOMATIC_SEMICOLON;
UiImport: UiImportHead T_NUMERIC_LITERAL T_AS QmlIdentifier T_SEMICOLON;
/.
    case $rule_number: {
        sym(1).UiImport->versionToken = loc(2);
        sym(1).UiImport->asToken = loc(3);
        sym(1).UiImport->importIdToken = loc(4);
        sym(1).UiImport->importId = stringRef(4);
        sym(1).UiImport->semicolonToken = loc(5);
    } break;
./

UiImport: UiImportHead T_AS QmlIdentifier T_AUTOMATIC_SEMICOLON;
UiImport: UiImportHead T_AS QmlIdentifier T_SEMICOLON;
/.
    case $rule_number: {
        sym(1).UiImport->asToken = loc(2);
        sym(1).UiImport->importIdToken = loc(3);
        sym(1).UiImport->importId = stringRef(3);
        sym(1).UiImport->semicolonToken = loc(4);
    } break;
./

UiImportHead: T_IMPORT ImportId;
/.
    case $rule_number: {
        AST::UiImport *node = 0;

        if (AST::StringLiteral *importIdLiteral = AST::cast<AST::StringLiteral *>(sym(2).Expression)) {
            node = new (pool) AST::UiImport(importIdLiteral->value);
            node->fileNameToken = loc(2);
        } else if (AST::UiQualifiedId *qualifiedId = reparseAsQualifiedId(sym(2).Expression)) {
            node = new (pool) AST::UiImport(qualifiedId);
            node->fileNameToken = loc(2);
        }

        sym(1).Node = node;

        if (node) {
            node->importToken = loc(1);
        } else {
           diagnostic_messages.append(DiagnosticMessage(Severity::Error, loc(1),
             QLatin1String("Expected a qualified name id or a string literal")));

            return false; // ### remove me
        }
    } break;
./

Empty: ;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

UiRootMember: UiObjectDefinition;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::UiObjectMemberList(sym(1).UiObjectMember);
    } break;
./

UiObjectMemberList: UiObjectMember;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::UiObjectMemberList(sym(1).UiObjectMember);
    } break;
./

UiObjectMemberList: UiObjectMemberList UiObjectMember;
/.
    case $rule_number: {
        AST::UiObjectMemberList *node = new (pool) AST:: UiObjectMemberList(sym(1).UiObjectMemberList, sym(2).UiObjectMember);
        sym(1).Node = node;
    } break;
./

UiArrayMemberList: UiObjectDefinition;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::UiArrayMemberList(sym(1).UiObjectMember);
    } break;
./

UiArrayMemberList: UiArrayMemberList T_COMMA UiObjectDefinition;
/.
    case $rule_number: {
        AST::UiArrayMemberList *node = new (pool) AST::UiArrayMemberList(sym(1).UiArrayMemberList, sym(3).UiObjectMember);
        node->commaToken = loc(2);
        sym(1).Node = node;
    } break;
./

UiObjectInitializer: T_LBRACE T_RBRACE;
/.
    case $rule_number: {
        AST::UiObjectInitializer *node = new (pool) AST::UiObjectInitializer((AST::UiObjectMemberList*)0);
        node->lbraceToken = loc(1);
        node->rbraceToken = loc(2);
        sym(1).Node = node;
    } break;
./

UiObjectInitializer: T_LBRACE UiObjectMemberList T_RBRACE;
/.
    case $rule_number: {
        AST::UiObjectInitializer *node = new (pool) AST::UiObjectInitializer(sym(2).UiObjectMemberList->finish());
        node->lbraceToken = loc(1);
        node->rbraceToken = loc(3);
        sym(1).Node = node;
    } break;
./

UiObjectDefinition: UiQualifiedId UiObjectInitializer;
/.
    case $rule_number: {
        AST::UiObjectDefinition *node = new (pool) AST::UiObjectDefinition(sym(1).UiQualifiedId, sym(2).UiObjectInitializer);
        sym(1).Node = node;
    } break;
./

UiObjectMember: UiObjectDefinition;

UiObjectMember: UiQualifiedId T_COLON ExpressionStatementLookahead T_LBRACKET UiArrayMemberList T_RBRACKET;
/.
    case $rule_number: {
        AST::UiArrayBinding *node = new (pool) AST::UiArrayBinding(sym(1).UiQualifiedId, sym(5).UiArrayMemberList->finish());
        node->colonToken = loc(2);
        node->lbracketToken = loc(4);
        node->rbracketToken = loc(6);
        sym(1).Node = node;
    } break;
./

UiObjectMember: UiQualifiedId T_COLON ExpressionStatementLookahead UiQualifiedId UiObjectInitializer;
/.
    case $rule_number: {
        AST::UiObjectBinding *node = new (pool) AST::UiObjectBinding(
            sym(1).UiQualifiedId, sym(4).UiQualifiedId, sym(5).UiObjectInitializer);
        node->colonToken = loc(2);
        sym(1).Node = node;
    } break;
./

UiObjectMember: UiQualifiedId T_ON UiQualifiedId  UiObjectInitializer;
/.
    case $rule_number: {
        AST::UiObjectBinding *node = new (pool) AST::UiObjectBinding(
          sym(3).UiQualifiedId, sym(1).UiQualifiedId, sym(4).UiObjectInitializer);
        node->colonToken = loc(2);
        node->hasOnToken = true;
        sym(1).Node = node;
    } break;
./


UiObjectLiteral: T_LBRACE ExpressionStatementLookahead UiPropertyDefinitionList T_RBRACE;
/.  case $rule_number: Q_FALLTHROUGH(); ./
UiObjectLiteral: T_LBRACE ExpressionStatementLookahead UiPropertyDefinitionList T_COMMA T_RBRACE;
/.
    case $rule_number: {
        AST::ObjectPattern *l = new (pool) AST::ObjectPattern(sym(3).PatternPropertyList->finish());
        l->lbraceToken = loc(1);
        l->rbraceToken = loc(4);
        AST::ExpressionStatement *node = new (pool) AST::ExpressionStatement(l);
        sym(1).Node = node;
    } break;
./


UiScriptStatement: ExpressionStatementLookahead T_FORCE_DECLARATION ExpressionStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
UiScriptStatement: ExpressionStatementLookahead T_FORCE_BLOCK Block;
/.  case $rule_number: Q_FALLTHROUGH(); ./
UiScriptStatement: ExpressionStatementLookahead T_FORCE_BLOCK UiObjectLiteral;
/.
    case $rule_number: {
        sym(1).Node = sym(3).Node;
    } break;
./


UiScriptStatement: ExpressionStatementLookahead EmptyStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
UiScriptStatement: ExpressionStatementLookahead ExpressionStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
UiScriptStatement: ExpressionStatementLookahead IfStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
UiScriptStatement: ExpressionStatementLookahead WithStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
UiScriptStatement: ExpressionStatementLookahead SwitchStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
UiScriptStatement: ExpressionStatementLookahead TryStatement;
/.
    case $rule_number: {
        sym(1).Node = sym(2).Node;
    } break;
./

UiObjectMember: UiQualifiedId T_COLON UiScriptStatement;
/.
case $rule_number:
{
    AST::UiScriptBinding *node = new (pool) AST::UiScriptBinding(sym(1).UiQualifiedId, sym(3).Statement);
    node->colonToken = loc(2);
    sym(1).Node = node;
    } break;
./

UiPropertyType: T_VAR;
/.  case $rule_number: Q_FALLTHROUGH(); ./
UiPropertyType: T_RESERVED_WORD;
/.  case $rule_number: Q_FALLTHROUGH(); ./
UiPropertyType: T_IDENTIFIER;
/.
    case $rule_number: {
        AST::UiQualifiedId *node = new (pool) AST::UiQualifiedId(stringRef(1));
        node->identifierToken = loc(1);
        sym(1).Node = node;
    } break;
./

UiPropertyType: UiPropertyType T_DOT T_IDENTIFIER;
/.
    case $rule_number: {
        AST::UiQualifiedId *node = new (pool) AST::UiQualifiedId(sym(1).UiQualifiedId, stringRef(3));
        node->identifierToken = loc(3);
        sym(1).Node = node;
    } break;
./

UiParameterListOpt: ;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

UiParameterListOpt: UiParameterList;
/.
    case $rule_number: {
        sym(1).Node = sym(1).UiParameterList->finish();
    } break;
./

UiParameterList: UiPropertyType QmlIdentifier;
/.
    case $rule_number: {
        AST::UiParameterList *node = new (pool) AST::UiParameterList(sym(1).UiQualifiedId->finish(), stringRef(2));
        node->propertyTypeToken = loc(1);
        node->identifierToken = loc(2);
        sym(1).Node = node;
    } break;
./

UiParameterList: UiParameterList T_COMMA UiPropertyType QmlIdentifier;
/.
    case $rule_number: {
        AST::UiParameterList *node = new (pool) AST::UiParameterList(sym(1).UiParameterList, sym(3).UiQualifiedId->finish(), stringRef(4));
        node->propertyTypeToken = loc(3);
        node->commaToken = loc(2);
        node->identifierToken = loc(4);
        sym(1).Node = node;
    } break;
./

UiObjectMember: T_SIGNAL T_IDENTIFIER T_LPAREN UiParameterListOpt T_RPAREN T_AUTOMATIC_SEMICOLON;
UiObjectMember: T_SIGNAL T_IDENTIFIER T_LPAREN UiParameterListOpt T_RPAREN T_SEMICOLON;
/.
    case $rule_number: {
        AST::UiPublicMember *node = new (pool) AST::UiPublicMember(nullptr, stringRef(2));
        node->type = AST::UiPublicMember::Signal;
        node->propertyToken = loc(1);
        node->typeToken = loc(2);
        node->identifierToken = loc(2);
        node->parameters = sym(4).UiParameterList;
        node->semicolonToken = loc(6);
        sym(1).Node = node;
    } break;
./

UiObjectMember: T_SIGNAL T_IDENTIFIER T_AUTOMATIC_SEMICOLON;
UiObjectMember: T_SIGNAL T_IDENTIFIER T_SEMICOLON;
/.
    case $rule_number: {
        AST::UiPublicMember *node = new (pool) AST::UiPublicMember(nullptr, stringRef(2));
        node->type = AST::UiPublicMember::Signal;
        node->propertyToken = loc(1);
        node->typeToken = loc(2);
        node->identifierToken = loc(2);
        node->semicolonToken = loc(3);
        sym(1).Node = node;
    } break;
./

UiObjectMember: T_PROPERTY T_IDENTIFIER T_LT UiPropertyType T_GT QmlIdentifier T_AUTOMATIC_SEMICOLON;
UiObjectMember: T_PROPERTY T_IDENTIFIER T_LT UiPropertyType T_GT QmlIdentifier T_SEMICOLON;
/.
    case $rule_number: {
        AST::UiPublicMember *node = new (pool) AST::UiPublicMember(sym(4).UiQualifiedId->finish(), stringRef(6));
        node->typeModifier = stringRef(2);
        node->propertyToken = loc(1);
        node->typeModifierToken = loc(2);
        node->typeToken = loc(4);
        node->identifierToken = loc(6);
        node->semicolonToken = loc(7);
        sym(1).Node = node;
    } break;
./

UiObjectMember: T_PROPERTY UiPropertyType QmlIdentifier T_AUTOMATIC_SEMICOLON;
UiObjectMember: T_PROPERTY UiPropertyType QmlIdentifier T_SEMICOLON;
/.
    case $rule_number: {
        AST::UiPublicMember *node = new (pool) AST::UiPublicMember(sym(2).UiQualifiedId->finish(), stringRef(3));
        node->propertyToken = loc(1);
        node->typeToken = loc(2);
        node->identifierToken = loc(3);
        node->semicolonToken = loc(4);
        sym(1).Node = node;
    } break;
./

UiObjectMember: T_DEFAULT T_PROPERTY UiPropertyType QmlIdentifier T_AUTOMATIC_SEMICOLON;
UiObjectMember: T_DEFAULT T_PROPERTY UiPropertyType QmlIdentifier T_SEMICOLON;
/.
    case $rule_number: {
        AST::UiPublicMember *node = new (pool) AST::UiPublicMember(sym(3).UiQualifiedId->finish(), stringRef(4));
        node->isDefaultMember = true;
        node->defaultToken = loc(1);
        node->propertyToken = loc(2);
        node->typeToken = loc(3);
        node->identifierToken = loc(4);
        node->semicolonToken = loc(5);
        sym(1).Node = node;
    } break;
./

UiObjectMember: T_DEFAULT T_PROPERTY T_IDENTIFIER T_LT UiPropertyType T_GT QmlIdentifier T_AUTOMATIC_SEMICOLON;
UiObjectMember: T_DEFAULT T_PROPERTY T_IDENTIFIER T_LT UiPropertyType T_GT QmlIdentifier T_SEMICOLON;
/.
    case $rule_number: {
        AST::UiPublicMember *node = new (pool) AST::UiPublicMember(sym(5).UiQualifiedId->finish(), stringRef(7));
        node->isDefaultMember = true;
        node->defaultToken = loc(1);
        node->typeModifier = stringRef(3);
        node->propertyToken = loc(2);
        node->typeModifierToken = loc(2);
        node->typeToken = loc(4);
        node->identifierToken = loc(7);
        node->semicolonToken = loc(8);
        sym(1).Node = node;
    } break;
./

UiObjectMember: T_PROPERTY UiPropertyType QmlIdentifier T_COLON UiScriptStatement;
/.
    case $rule_number: {
        AST::UiPublicMember *node = new (pool) AST::UiPublicMember(sym(2).UiQualifiedId->finish(), stringRef(3), sym(5).Statement);
        node->propertyToken = loc(1);
        node->typeToken = loc(2);
        node->identifierToken = loc(3);
        node->colonToken = loc(4);
        sym(1).Node = node;
    } break;
./

UiObjectMember: T_READONLY T_PROPERTY UiPropertyType QmlIdentifier T_COLON UiScriptStatement;
/.
    case $rule_number: {
        AST::UiPublicMember *node = new (pool) AST::UiPublicMember(sym(3).UiQualifiedId->finish(), stringRef(4), sym(6).Statement);
        node->isReadonlyMember = true;
        node->readonlyToken = loc(1);
        node->propertyToken = loc(2);
        node->typeToken = loc(3);
        node->identifierToken = loc(4);
        node->colonToken = loc(5);
        sym(1).Node = node;
    } break;
./

UiObjectMember: T_DEFAULT T_PROPERTY UiPropertyType QmlIdentifier T_COLON UiScriptStatement;
/.
    case $rule_number: {
        AST::UiPublicMember *node = new (pool) AST::UiPublicMember(sym(3).UiQualifiedId->finish(), stringRef(4), sym(6).Statement);
        node->isDefaultMember = true;
        node->defaultToken = loc(1);
        node->propertyToken = loc(2);
        node->typeToken = loc(3);
        node->identifierToken = loc(4);
        node->colonToken = loc(5);
        sym(1).Node = node;
    } break;
./

UiObjectMember: T_PROPERTY T_IDENTIFIER T_LT UiPropertyType T_GT QmlIdentifier T_COLON T_LBRACKET UiArrayMemberList T_RBRACKET;
/.
    case $rule_number: {
        AST::UiPublicMember *node = new (pool) AST::UiPublicMember(sym(4).UiQualifiedId->finish(), stringRef(6));
        node->typeModifier = stringRef(2);
        node->propertyToken = loc(1);
        node->typeModifierToken = loc(2);
        node->typeToken = loc(4);
        node->identifierToken = loc(6);
        node->semicolonToken = loc(7); // insert a fake ';' before ':'

        AST::UiQualifiedId *propertyName = new (pool) AST::UiQualifiedId(stringRef(6));
        propertyName->identifierToken = loc(6);
        propertyName->next = 0;

        AST::UiArrayBinding *binding = new (pool) AST::UiArrayBinding(propertyName, sym(9).UiArrayMemberList->finish());
        binding->colonToken = loc(7);
        binding->lbracketToken = loc(8);
        binding->rbracketToken = loc(10);

        node->binding = binding;

        sym(1).Node = node;
    } break;
./

UiObjectMember: T_PROPERTY UiPropertyType QmlIdentifier T_COLON ExpressionStatementLookahead UiQualifiedId UiObjectInitializer;
/.
    case $rule_number: {
        AST::UiPublicMember *node = new (pool) AST::UiPublicMember(sym(2).UiQualifiedId->finish(), stringRef(3));
        node->propertyToken = loc(1);
        node->typeToken = loc(2);
        node->identifierToken = loc(3);
        node->semicolonToken = loc(4); // insert a fake ';' before ':'

        AST::UiQualifiedId *propertyName = new (pool) AST::UiQualifiedId(stringRef(3));
        propertyName->identifierToken = loc(3);
        propertyName->next = 0;

        AST::UiObjectBinding *binding = new (pool) AST::UiObjectBinding(
          propertyName, sym(6).UiQualifiedId, sym(7).UiObjectInitializer);
        binding->colonToken = loc(4);

        node->binding = binding;

        sym(1).Node = node;
    } break;
./

UiObjectMember: T_READONLY T_PROPERTY UiPropertyType QmlIdentifier T_COLON ExpressionStatementLookahead UiQualifiedId UiObjectInitializer;
/.
    case $rule_number: {
        AST::UiPublicMember *node = new (pool) AST::UiPublicMember(sym(3).UiQualifiedId->finish(), stringRef(4));
        node->isReadonlyMember = true;
        node->readonlyToken = loc(1);
        node->propertyToken = loc(2);
        node->typeToken = loc(3);
        node->identifierToken = loc(4);
        node->semicolonToken = loc(5); // insert a fake ';' before ':'

        AST::UiQualifiedId *propertyName = new (pool) AST::UiQualifiedId(stringRef(4));
        propertyName->identifierToken = loc(4);
        propertyName->next = 0;

        AST::UiObjectBinding *binding = new (pool) AST::UiObjectBinding(
          propertyName, sym(7).UiQualifiedId, sym(8).UiObjectInitializer);
        binding->colonToken = loc(5);

        node->binding = binding;

        sym(1).Node = node;
    } break;
./

UiObjectMember: FunctionDeclaration;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::UiSourceElement(sym(1).Node);
    } break;
./

UiObjectMember: VariableStatement;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::UiSourceElement(sym(1).Node);
    } break;
./

UiQualifiedId: MemberExpression;
/.
    case $rule_number: {
      if (AST::ArrayMemberExpression *mem = AST::cast<AST::ArrayMemberExpression *>(sym(1).Expression)) {
        diagnostic_messages.append(DiagnosticMessage(Severity::Warning, mem->lbracketToken,
          QLatin1String("Ignored annotation")));

        sym(1).Expression = mem->base;
      }

      if (AST::UiQualifiedId *qualifiedId = reparseAsQualifiedId(sym(1).Expression)) {
        sym(1).UiQualifiedId = qualifiedId;
      } else {
        sym(1).UiQualifiedId = 0;

        diagnostic_messages.append(DiagnosticMessage(Severity::Error, loc(1),
          QLatin1String("Expected a qualified name id")));

        return false; // ### recover
      }
    } break;
./

UiObjectMember: T_ENUM T_IDENTIFIER T_LBRACE EnumMemberList T_RBRACE;
/.
    case $rule_number: {
        AST::UiEnumDeclaration *enumDeclaration = new (pool) AST::UiEnumDeclaration(stringRef(2), sym(4).UiEnumMemberList->finish());
        enumDeclaration->enumToken = loc(1);
        enumDeclaration->rbraceToken = loc(5);
        sym(1).Node = enumDeclaration;
        break;
    }
./

EnumMemberList: T_IDENTIFIER;
/.
    case $rule_number: {
        AST::UiEnumMemberList *node = new (pool) AST::UiEnumMemberList(stringRef(1));
        node->memberToken = loc(1);
        sym(1).Node = node;
        break;
    }
./

EnumMemberList: T_IDENTIFIER T_EQ T_NUMERIC_LITERAL;
/.
    case $rule_number: {
        AST::UiEnumMemberList *node = new (pool) AST::UiEnumMemberList(stringRef(1), sym(3).dval);
        node->memberToken = loc(1);
        node->valueToken = loc(3);
        sym(1).Node = node;
        break;
    }
./

EnumMemberList: EnumMemberList T_COMMA T_IDENTIFIER;
/.
    case $rule_number: {
        AST::UiEnumMemberList *node = new (pool) AST::UiEnumMemberList(sym(1).UiEnumMemberList, stringRef(3));
        node->memberToken = loc(3);
        sym(1).Node = node;
        break;
    }
./

EnumMemberList: EnumMemberList T_COMMA T_IDENTIFIER T_EQ T_NUMERIC_LITERAL;
/.
    case $rule_number: {
        AST::UiEnumMemberList *node = new (pool) AST::UiEnumMemberList(sym(1).UiEnumMemberList, stringRef(3), sym(5).dval);
        node->memberToken = loc(3);
        node->valueToken = loc(5);
        sym(1).Node = node;
        break;
    }
./

QmlIdentifier: T_IDENTIFIER;
QmlIdentifier: T_PROPERTY;
QmlIdentifier: T_SIGNAL;
QmlIdentifier: T_READONLY;
QmlIdentifier: T_ON;
QmlIdentifier: T_GET;
QmlIdentifier: T_SET;
QmlIdentifier: T_FROM;
QmlIdentifier: T_OF;

JsIdentifier: T_IDENTIFIER;
JsIdentifier: T_PROPERTY;
JsIdentifier: T_SIGNAL;
JsIdentifier: T_READONLY;
JsIdentifier: T_ON;
JsIdentifier: T_GET;
JsIdentifier: T_SET;
JsIdentifier: T_FROM;
JsIdentifier: T_STATIC;
JsIdentifier: T_OF;
JsIdentifier: T_AS;

IdentifierReference: JsIdentifier;
BindingIdentifier: IdentifierReference;

--------------------------------------------------------------------------------------------------------
-- Expressions
--------------------------------------------------------------------------------------------------------

PrimaryExpression: T_THIS;
/.
    case $rule_number: {
        AST::ThisExpression *node = new (pool) AST::ThisExpression();
        node->thisToken = loc(1);
        sym(1).Node = node;
    } break;
./

PrimaryExpression: IdentifierReference;
/.
    case $rule_number: {
        AST::IdentifierExpression *node = new (pool) AST::IdentifierExpression(stringRef(1));
        node->identifierToken = loc(1);
        sym(1).Node = node;
    } break;
./

PrimaryExpression: Literal;
PrimaryExpression: ArrayLiteral;
PrimaryExpression: ObjectLiteral;
PrimaryExpression: FunctionExpression;
PrimaryExpression: ClassExpression;
PrimaryExpression: GeneratorExpression;
PrimaryExpression: RegularExpressionLiteral;
PrimaryExpression: TemplateLiteral;

PrimaryExpression: CoverParenthesizedExpressionAndArrowParameterList;
/.
    case $rule_number: {
        if (coverExpressionType != CE_ParenthesizedExpression) {
            syntaxError(coverExpressionErrorLocation, "Expected token ')'.");
            return false;
        }
    } break;
./

-- Parsing of the CoverParenthesizedExpressionAndArrowParameterList is restricted to the one rule below when this is parsed as a primary expression
CoverParenthesizedExpressionAndArrowParameterList: T_LPAREN Expression_In T_RPAREN;
/.
    case $rule_number: {
        AST::NestedExpression *node = new (pool) AST::NestedExpression(sym(2).Expression);
        node->lparenToken = loc(1);
        node->rparenToken = loc(3);
        sym(1).Node = node;
        coverExpressionType = CE_ParenthesizedExpression;
    } break;
./

CoverParenthesizedExpressionAndArrowParameterList: T_LPAREN T_RPAREN;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
        coverExpressionErrorLocation = loc(2);
        coverExpressionType = CE_FormalParameterList;
    } break;
./

CoverParenthesizedExpressionAndArrowParameterList: T_LPAREN BindingRestElement T_RPAREN;
/.
    case $rule_number: {
        AST::FormalParameterList *node = (new (pool) AST::FormalParameterList(nullptr, sym(2).PatternElement))->finish(pool);
        sym(1).Node = node;
        coverExpressionErrorLocation = loc(2);
        coverExpressionType = CE_FormalParameterList;
    } break;
./

CoverParenthesizedExpressionAndArrowParameterList: T_LPAREN Expression_In T_COMMA BindingRestElementOpt T_RPAREN;
/.
    case $rule_number: {
        AST::FormalParameterList *list = sym(2).Expression->reparseAsFormalParameterList(pool);
        if (!list) {
            syntaxError(loc(1), "Invalid Arrow parameter list.");
            return false;
        }
        if (sym(4).Node) {
            list = new (pool) AST::FormalParameterList(list, sym(4).PatternElement);
        }
        coverExpressionErrorLocation = loc(4);
        coverExpressionType = CE_FormalParameterList;
        sym(1).Node = list->finish(pool);
    } break;
./

Literal: T_NULL;
/.
    case $rule_number: {
        AST::NullExpression *node = new (pool) AST::NullExpression();
        node->nullToken = loc(1);
        sym(1).Node = node;
    } break;
./

Literal: T_TRUE;
/.
    case $rule_number: {
        AST::TrueLiteral *node = new (pool) AST::TrueLiteral();
        node->trueToken = loc(1);
        sym(1).Node = node;
    } break;
./

Literal: T_FALSE;
/.
    case $rule_number: {
        AST::FalseLiteral *node = new (pool) AST::FalseLiteral();
        node->falseToken = loc(1);
        sym(1).Node = node;
    } break;
./

Literal: T_NUMERIC_LITERAL;
/.
    case $rule_number: {
        AST::NumericLiteral *node = new (pool) AST::NumericLiteral(sym(1).dval);
        node->literalToken = loc(1);
        sym(1).Node = node;
    } break;
./

Literal: T_MULTILINE_STRING_LITERAL;
/.  case $rule_number: Q_FALLTHROUGH(); ./

Literal: T_STRING_LITERAL;
/.
    case $rule_number: {
        AST::StringLiteral *node = new (pool) AST::StringLiteral(stringRef(1));
        node->literalToken = loc(1);
        sym(1).Node = node;
    } break;
./

RegularExpressionLiteral: T_DIVIDE_;
/:
#define J_SCRIPT_REGEXPLITERAL_RULE1 $rule_number
:/
/.
{
    Lexer::RegExpBodyPrefix prefix;
    case $rule_number:
        prefix = Lexer::NoPrefix;
        goto scan_regexp;
./

RegularExpressionLiteral: T_DIVIDE_EQ;
/:
#define J_SCRIPT_REGEXPLITERAL_RULE2 $rule_number
:/
/.
    case $rule_number:
        prefix = Lexer::EqualPrefix;
        goto scan_regexp;

    scan_regexp: {
        bool rx = lexer->scanRegExp(prefix);
        if (!rx) {
            diagnostic_messages.append(DiagnosticMessage(Severity::Error, location(lexer), lexer->errorMessage()));
            return false;
        }

        loc(1).length = lexer->tokenLength();
        yylloc = loc(1); // adjust the location of the current token

        AST::RegExpLiteral *node = new (pool) AST::RegExpLiteral(driver->newStringRef(lexer->regExpPattern()), lexer->regExpFlags());
        node->literalToken = loc(1);
        sym(1).Node = node;
    } break;
}
./


ArrayLiteral: T_LBRACKET ElisionOpt T_RBRACKET;
/.
    case $rule_number: {
        AST::PatternElementList *list = nullptr;
        if (sym(2).Elision)
            list = (new (pool) AST::PatternElementList(sym(2).Elision, nullptr))->finish();
        AST::ArrayPattern *node = new (pool) AST::ArrayPattern(list);
        node->lbracketToken = loc(1);
        node->rbracketToken = loc(3);
        sym(1).Node = node;
    } break;
./

ArrayLiteral: T_LBRACKET ElementList T_RBRACKET;
/.
    case $rule_number: {
        AST::ArrayPattern *node = new (pool) AST::ArrayPattern(sym(2).PatternElementList->finish());
        node->lbracketToken = loc(1);
        node->rbracketToken = loc(3);
        sym(1).Node = node;
    } break;
./

ArrayLiteral: T_LBRACKET ElementList T_COMMA ElisionOpt T_RBRACKET;
/.
    case $rule_number: {
        auto *list = sym(2).PatternElementList;
        if (sym(4).Elision) {
            AST::PatternElementList *l = new (pool) AST::PatternElementList(sym(4).Elision, nullptr);
            list = list->append(l);
        }
        AST::ArrayPattern *node = new (pool) AST::ArrayPattern(list->finish());
        node->lbracketToken = loc(1);
        node->commaToken = loc(3);
        node->rbracketToken = loc(5);
        sym(1).Node = node;
        Q_ASSERT(node->isValidArrayLiteral());
    } break;
./

ElementList: AssignmentExpression_In;
/.
    case $rule_number: {
        AST::PatternElement *e = new (pool) AST::PatternElement(sym(1).Expression);
        sym(1).Node = new (pool) AST::PatternElementList(nullptr, e);
    } break;
./

ElementList: Elision AssignmentExpression_In;
/.
    case $rule_number: {
        AST::PatternElement *e = new (pool) AST::PatternElement(sym(2).Expression);
        sym(1).Node = new (pool) AST::PatternElementList(sym(1).Elision->finish(), e);
    } break;
./

ElementList: ElisionOpt SpreadElement;
/.
    case $rule_number: {
        AST::PatternElementList *node = new (pool) AST::PatternElementList(sym(1).Elision, sym(2).PatternElement);
        sym(1).Node = node;
    } break;
./

ElementList: ElementList T_COMMA ElisionOpt AssignmentExpression_In;
/.
    case $rule_number: {
        AST::PatternElement *e = new (pool) AST::PatternElement(sym(4).Expression);
        AST::PatternElementList *node = new (pool) AST::PatternElementList(sym(3).Elision, e);
        sym(1).Node = sym(1).PatternElementList->append(node);
    } break;
./

ElementList: ElementList T_COMMA ElisionOpt SpreadElement;
/.
    case $rule_number: {
        AST::PatternElementList *node = new (pool) AST::PatternElementList(sym(3).Elision, sym(4).PatternElement);
        sym(1).Node = sym(1).PatternElementList->append(node);
    } break;
./

Elision: T_COMMA;
/.
    case $rule_number: {
        AST::Elision *node = new (pool) AST::Elision();
        node->commaToken = loc(1);
        sym(1).Node = node;
    } break;
./

Elision: Elision T_COMMA;
/.
    case $rule_number: {
        AST::Elision *node = new (pool) AST::Elision(sym(1).Elision);
        node->commaToken = loc(2);
        sym(1).Node = node;
    } break;
./

ElisionOpt: ;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

ElisionOpt: Elision;
/.
    case $rule_number: {
        sym(1).Node = sym(1).Elision->finish();
    } break;
./

SpreadElement: T_ELLIPSIS AssignmentExpression;
/.
    case $rule_number: {
        AST::PatternElement *node = new (pool) AST::PatternElement(sym(2).Expression, AST::PatternElement::SpreadElement);
        sym(1).Node = node;
    } break;
./

ObjectLiteral: T_LBRACE T_RBRACE;
/.
    case $rule_number: {
        AST::ObjectPattern *node = new (pool) AST::ObjectPattern();
        node->lbraceToken = loc(1);
        node->rbraceToken = loc(2);
        sym(1).Node = node;
    } break;
./

ObjectLiteral: T_LBRACE PropertyDefinitionList T_RBRACE;
/.
    case $rule_number: {
        AST::ObjectPattern *node = new (pool) AST::ObjectPattern(sym(2).PatternPropertyList->finish());
        node->lbraceToken = loc(1);
        node->rbraceToken = loc(3);
        sym(1).Node = node;
    } break;
./

ObjectLiteral: T_LBRACE PropertyDefinitionList T_COMMA T_RBRACE;
/.
    case $rule_number: {
        AST::ObjectPattern *node = new (pool) AST::ObjectPattern(sym(2).PatternPropertyList->finish());
        node->lbraceToken = loc(1);
        node->rbraceToken = loc(4);
        sym(1).Node = node;
    } break;
./


UiPropertyDefinitionList: UiPropertyDefinition;
/.  case $rule_number: Q_FALLTHROUGH(); ./
PropertyDefinitionList: PropertyDefinition;
/.
    case $rule_number: {
      sym(1).Node = new (pool) AST::PatternPropertyList(sym(1).PatternProperty);
    } break;
./

UiPropertyDefinitionList: UiPropertyDefinitionList T_COMMA UiPropertyDefinition;
/.  case $rule_number: Q_FALLTHROUGH(); ./
PropertyDefinitionList: PropertyDefinitionList T_COMMA PropertyDefinition;
/.
    case $rule_number: {
        AST::PatternPropertyList *node = new (pool) AST::PatternPropertyList(sym(1).PatternPropertyList, sym(3).PatternProperty);
        sym(1).Node = node;
    } break;
./

PropertyDefinition: IdentifierReference;
/.
    case $rule_number: {
        AST::IdentifierPropertyName *name = new (pool) AST::IdentifierPropertyName(stringRef(1));
        name->propertyNameToken = loc(1);
        AST::IdentifierExpression *expr = new (pool) AST::IdentifierExpression(stringRef(1));
        expr->identifierToken = loc(1);
        AST::PatternProperty *node = new (pool) AST::PatternProperty(name, expr);
        node->colonToken = loc(2);
        sym(1).Node = node;
    } break;
./

-- Using this production should result in a syntax error when used in an ObjectLiteral
PropertyDefinition: CoverInitializedName;

CoverInitializedName: IdentifierReference Initializer_In;
/.
    case $rule_number: {
        AST::IdentifierPropertyName *name = new (pool) AST::IdentifierPropertyName(stringRef(1));
        name->propertyNameToken = loc(1);
        AST::IdentifierExpression *left = new (pool) AST::IdentifierExpression(stringRef(1));
        left->identifierToken = loc(1);
        // if initializer is an anonymous function expression, we need to assign identifierref as it's name
        if (auto *f = asAnonymousFunctionDefinition(sym(2).Expression))
            f->name = stringRef(1);
        if (auto *c = asAnonymousClassDefinition(sym(2).Expression))
            c->name = stringRef(1);
        AST::BinaryExpression *assignment = new (pool) AST::BinaryExpression(left, QSOperator::Assign, sym(2).Expression);
        AST::PatternProperty *node = new (pool) AST::PatternProperty(name, assignment);
        node->colonToken = loc(1);
        sym(1).Node = node;

    } break;
./

UiPropertyDefinition: UiPropertyName T_COLON AssignmentExpression_In;
/.  case $rule_number: Q_FALLTHROUGH(); ./
PropertyDefinition: PropertyName T_COLON AssignmentExpression_In;
/.
    case $rule_number: {
        AST::PatternProperty *node = new (pool) AST::PatternProperty(sym(1).PropertyName, sym(3).Expression);
        if (auto *f = asAnonymousFunctionDefinition(sym(3).Expression)) {
            if (!AST::cast<AST::ComputedPropertyName *>(sym(1).PropertyName))
                f->name = driver->newStringRef(sym(1).PropertyName->asString());
        }
        if (auto *c = asAnonymousClassDefinition(sym(3).Expression)) {
            if (!AST::cast<AST::ComputedPropertyName *>(sym(1).PropertyName))
                c->name = driver->newStringRef(sym(1).PropertyName->asString());
        }
        node->colonToken = loc(2);
        sym(1).Node = node;
    } break;
./

PropertyDefinition: MethodDefinition;

PropertyName: LiteralPropertyName;
PropertyName: ComputedPropertyName;

LiteralPropertyName: IdentifierName;
/.
    case $rule_number: {
        AST::IdentifierPropertyName *node = new (pool) AST::IdentifierPropertyName(stringRef(1));
        node->propertyNameToken = loc(1);
        sym(1).Node = node;
    } break;
./

UiPropertyName: T_STRING_LITERAL;
/.  case $rule_number: Q_FALLTHROUGH(); ./
LiteralPropertyName: T_STRING_LITERAL;
/.
    case $rule_number: {
        AST::StringLiteralPropertyName *node = new (pool) AST::StringLiteralPropertyName(stringRef(1));
        node->propertyNameToken = loc(1);
        sym(1).Node = node;
    } break;
./

UiPropertyName: T_NUMERIC_LITERAL;
/.  case $rule_number: Q_FALLTHROUGH(); ./
LiteralPropertyName: T_NUMERIC_LITERAL;
/.
    case $rule_number: {
        AST::NumericLiteralPropertyName *node = new (pool) AST::NumericLiteralPropertyName(sym(1).dval);
        node->propertyNameToken = loc(1);
        sym(1).Node = node;
    } break;
./

IdentifierName: IdentifierReference;
IdentifierName: ReservedIdentifier;

ReservedIdentifier: T_BREAK;
ReservedIdentifier: T_CASE;
ReservedIdentifier: T_CATCH;
ReservedIdentifier: T_CONTINUE;
ReservedIdentifier: T_DEFAULT;
ReservedIdentifier: T_DELETE;
ReservedIdentifier: T_DO;
ReservedIdentifier: T_ELSE;
ReservedIdentifier: T_ENUM;
ReservedIdentifier: T_FALSE;
ReservedIdentifier: T_FINALLY;
ReservedIdentifier: T_FOR;
ReservedIdentifier: T_FUNCTION;
ReservedIdentifier: T_IF;
ReservedIdentifier: T_IN;
ReservedIdentifier: T_INSTANCEOF;
ReservedIdentifier: T_NEW;
ReservedIdentifier: T_NULL;
ReservedIdentifier: T_RETURN;
ReservedIdentifier: T_SWITCH;
ReservedIdentifier: T_THIS;
ReservedIdentifier: T_THROW;
ReservedIdentifier: T_TRUE;
ReservedIdentifier: T_TRY;
ReservedIdentifier: T_TYPEOF;
ReservedIdentifier: T_VAR;
ReservedIdentifier: T_VOID;
ReservedIdentifier: T_WHILE;
ReservedIdentifier: T_CONST;
ReservedIdentifier: T_LET;
ReservedIdentifier: T_DEBUGGER;
ReservedIdentifier: T_RESERVED_WORD;
ReservedIdentifier: T_SUPER;
ReservedIdentifier: T_WITH;
ReservedIdentifier: T_CLASS;
ReservedIdentifier: T_EXTENDS;
ReservedIdentifier: T_EXPORT;
ReservedIdentifier: T_IMPORT;

ComputedPropertyName: T_LBRACKET AssignmentExpression_In T_RBRACKET;
/.
    case $rule_number: {
        AST::ComputedPropertyName *node = new (pool) AST::ComputedPropertyName(sym(2).Expression);
        node->propertyNameToken = loc(1);
        sym(1).Node = node;
    } break;
./

Initializer: T_EQ AssignmentExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Initializer_In: T_EQ AssignmentExpression_In;
/.
case $rule_number: {
    sym(1) = sym(2);
} break;
./


InitializerOpt: ;
/.  case $rule_number: Q_FALLTHROUGH(); ./
InitializerOpt_In: ;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

InitializerOpt: Initializer;
InitializerOpt_In: Initializer_In;

TemplateLiteral: T_NO_SUBSTITUTION_TEMPLATE;
/.  case $rule_number: Q_FALLTHROUGH(); ./

TemplateSpans: T_TEMPLATE_TAIL;
/.
    case $rule_number: {
        AST::TemplateLiteral *node = new (pool) AST::TemplateLiteral(stringRef(1), nullptr);
        node->literalToken = loc(1);
        sym(1).Node = node;
    } break;
./

TemplateSpans: T_TEMPLATE_MIDDLE Expression TemplateSpans;
/.  case $rule_number: Q_FALLTHROUGH(); ./

TemplateLiteral: T_TEMPLATE_HEAD Expression TemplateSpans;
/.
    case $rule_number: {
        AST::TemplateLiteral *node = new (pool) AST::TemplateLiteral(stringRef(1), sym(2).Expression);
        node->next = sym(3).Template;
        node->literalToken = loc(1);
        sym(1).Node = node;
    } break;
./


MemberExpression: PrimaryExpression;

Super: T_SUPER;
/.
    case $rule_number: {
        AST::SuperLiteral *node = new (pool) AST::SuperLiteral();
        node->superToken = loc(1);
        sym(1).Node = node;
    } break;
./


MemberExpression: Super T_LBRACKET Expression_In T_RBRACKET;
/.  case $rule_number: Q_FALLTHROUGH(); ./
MemberExpression: MemberExpression T_LBRACKET Expression_In T_RBRACKET;
/.
    case $rule_number: {
        AST::ArrayMemberExpression *node = new (pool) AST::ArrayMemberExpression(sym(1).Expression, sym(3).Expression);
        node->lbracketToken = loc(2);
        node->rbracketToken = loc(4);
        sym(1).Node = node;
    } break;
./


-- the identifier has to be "target", catched at codegen time
NewTarget: T_NEW T_DOT T_IDENTIFIER;
/.  case $rule_number:
    {
        AST::IdentifierExpression *node = new (pool) AST::IdentifierExpression(stringRef(1));
        node->identifierToken= loc(1);
        sym(1).Node = node;
    } Q_FALLTHROUGH();
./
MemberExpression: Super T_DOT IdentifierName;
/.  case $rule_number: Q_FALLTHROUGH(); ./
MemberExpression: MemberExpression T_DOT IdentifierName;
/.
    case $rule_number: {
        AST::FieldMemberExpression *node = new (pool) AST::FieldMemberExpression(sym(1).Expression, stringRef(3));
        node->dotToken = loc(2);
        node->identifierToken = loc(3);
        sym(1).Node = node;
    } break;
./

MemberExpression: MetaProperty;

MemberExpression: T_NEW MemberExpression T_LPAREN Arguments T_RPAREN;
/.
    case $rule_number: {
        AST::NewMemberExpression *node = new (pool) AST::NewMemberExpression(sym(2).Expression, sym(4).ArgumentList);
        node->newToken = loc(1);
        node->lparenToken = loc(3);
        node->rparenToken = loc(5);
        sym(1).Node = node;
    } break;
./

MetaProperty: NewTarget;


NewExpression: MemberExpression;

NewExpression: T_NEW NewExpression;
/.
    case $rule_number: {
        AST::NewExpression *node = new (pool) AST::NewExpression(sym(2).Expression);
        node->newToken = loc(1);
        sym(1).Node = node;
    } break;
./


CallExpression: CallExpression TemplateLiteral;
/.  case $rule_number: Q_FALLTHROUGH(); ./
MemberExpression: MemberExpression TemplateLiteral;
/.
    case $rule_number: {
        AST::TaggedTemplate *node = new (pool) AST::TaggedTemplate(sym(1).Expression, sym(2).Template);
        sym(1).Node = node;
    } break;
./

CallExpression: MemberExpression T_LPAREN Arguments T_RPAREN;
/.
    case $rule_number: {
        AST::CallExpression *node = new (pool) AST::CallExpression(sym(1).Expression, sym(3).ArgumentList);
        node->lparenToken = loc(2);
        node->rparenToken = loc(4);
        sym(1).Node = node;
    } break;
./

CallExpression: Super T_LPAREN Arguments T_RPAREN;
/.  case $rule_number: Q_FALLTHROUGH(); ./
CallExpression: CallExpression T_LPAREN Arguments T_RPAREN;
/.
    case $rule_number: {
        AST::CallExpression *node = new (pool) AST::CallExpression(sym(1).Expression, sym(3).ArgumentList);
        node->lparenToken = loc(2);
        node->rparenToken = loc(4);
        sym(1).Node = node;
    } break;
./

CallExpression: CallExpression T_LBRACKET Expression_In T_RBRACKET;
/.
    case $rule_number: {
        AST::ArrayMemberExpression *node = new (pool) AST::ArrayMemberExpression(sym(1).Expression, sym(3).Expression);
        node->lbracketToken = loc(2);
        node->rbracketToken = loc(4);
        sym(1).Node = node;
    } break;
./

CallExpression: CallExpression T_DOT IdentifierName;
/.
    case $rule_number: {
        AST::FieldMemberExpression *node = new (pool) AST::FieldMemberExpression(sym(1).Expression, stringRef(3));
        node->dotToken = loc(2);
        node->identifierToken = loc(3);
        sym(1).Node = node;
    } break;
./

Arguments: ;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

Arguments: ArgumentList;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Arguments: ArgumentList T_COMMA;
/.
    case $rule_number: {
        sym(1).Node = sym(1).ArgumentList->finish();
    } break;
./

ArgumentList: AssignmentExpression_In;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::ArgumentList(sym(1).Expression);
    } break;
./

ArgumentList: T_ELLIPSIS AssignmentExpression_In;
/.
    case $rule_number: {
        AST::ArgumentList *node = new (pool) AST::ArgumentList(sym(2).Expression);
        node->isSpreadElement = true;
        sym(1).Node = node;
    } break;
./

ArgumentList: ArgumentList T_COMMA AssignmentExpression_In;
/.
    case $rule_number: {
        AST::ArgumentList *node = new (pool) AST::ArgumentList(sym(1).ArgumentList, sym(3).Expression);
        node->commaToken = loc(2);
        sym(1).Node = node;
    } break;
./

ArgumentList: ArgumentList T_COMMA T_ELLIPSIS AssignmentExpression_In;
/.
    case $rule_number: {
        AST::ArgumentList *node = new (pool) AST::ArgumentList(sym(1).ArgumentList, sym(4).Expression);
        node->commaToken = loc(2);
        node->isSpreadElement = true;
        sym(1).Node = node;
    } break;
./

LeftHandSideExpression: NewExpression;
LeftHandSideExpression: CallExpression;

UpdateExpression: LeftHandSideExpression;

UpdateExpression: LeftHandSideExpression T_PLUS_PLUS;
/.
    case $rule_number: {
        AST::PostIncrementExpression *node = new (pool) AST::PostIncrementExpression(sym(1).Expression);
        node->incrementToken = loc(2);
        sym(1).Node = node;
    } break;
./

UpdateExpression: LeftHandSideExpression T_MINUS_MINUS;
/.
    case $rule_number: {
        AST::PostDecrementExpression *node = new (pool) AST::PostDecrementExpression(sym(1).Expression);
        node->decrementToken = loc(2);
        sym(1).Node = node;
    } break;
./

UpdateExpression: T_PLUS_PLUS UnaryExpression;
/.
    case $rule_number: {
        AST::PreIncrementExpression *node = new (pool) AST::PreIncrementExpression(sym(2).Expression);
        node->incrementToken = loc(1);
        sym(1).Node = node;
    } break;
./

UpdateExpression: T_MINUS_MINUS UnaryExpression;
/.
    case $rule_number: {
        AST::PreDecrementExpression *node = new (pool) AST::PreDecrementExpression(sym(2).Expression);
        node->decrementToken = loc(1);
        sym(1).Node = node;
    } break;
./

UnaryExpression: UpdateExpression;

UnaryExpression: T_DELETE UnaryExpression;
/.
    case $rule_number: {
        AST::DeleteExpression *node = new (pool) AST::DeleteExpression(sym(2).Expression);
        node->deleteToken = loc(1);
        sym(1).Node = node;
    } break;
./

UnaryExpression: T_VOID UnaryExpression;
/.
    case $rule_number: {
        AST::VoidExpression *node = new (pool) AST::VoidExpression(sym(2).Expression);
        node->voidToken = loc(1);
        sym(1).Node = node;
    } break;
./

UnaryExpression: T_TYPEOF UnaryExpression;
/.
    case $rule_number: {
        AST::TypeOfExpression *node = new (pool) AST::TypeOfExpression(sym(2).Expression);
        node->typeofToken = loc(1);
        sym(1).Node = node;
    } break;
./

UnaryExpression: T_PLUS UnaryExpression;
/.
    case $rule_number: {
        AST::UnaryPlusExpression *node = new (pool) AST::UnaryPlusExpression(sym(2).Expression);
        node->plusToken = loc(1);
        sym(1).Node = node;
    } break;
./

UnaryExpression: T_MINUS UnaryExpression;
/.
    case $rule_number: {
        AST::UnaryMinusExpression *node = new (pool) AST::UnaryMinusExpression(sym(2).Expression);
        node->minusToken = loc(1);
        sym(1).Node = node;
    } break;
./

UnaryExpression: T_TILDE UnaryExpression;
/.
    case $rule_number: {
        AST::TildeExpression *node = new (pool) AST::TildeExpression(sym(2).Expression);
        node->tildeToken = loc(1);
        sym(1).Node = node;
    } break;
./

UnaryExpression: T_NOT UnaryExpression;
/.
    case $rule_number: {
        AST::NotExpression *node = new (pool) AST::NotExpression(sym(2).Expression);
        node->notToken = loc(1);
        sym(1).Node = node;
    } break;
./

ExponentiationExpression: UnaryExpression;

ExponentiationExpression: UpdateExpression T_STAR_STAR ExponentiationExpression;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::Exp, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

MultiplicativeExpression: ExponentiationExpression;

MultiplicativeExpression: MultiplicativeExpression MultiplicativeOperator ExponentiationExpression;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, sym(2).ival, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

MultiplicativeOperator: T_STAR;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::Mul;
    } break;
./

MultiplicativeOperator: T_DIVIDE_;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::Div;
    } break;
./

MultiplicativeOperator: T_REMAINDER;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::Mod;
    } break;
./

AdditiveExpression: MultiplicativeExpression;

AdditiveExpression: AdditiveExpression T_PLUS MultiplicativeExpression;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::Add, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

AdditiveExpression: AdditiveExpression T_MINUS MultiplicativeExpression;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::Sub, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

ShiftExpression: AdditiveExpression;

ShiftExpression: ShiftExpression T_LT_LT AdditiveExpression;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::LShift, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

ShiftExpression: ShiftExpression T_GT_GT AdditiveExpression;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::RShift, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

ShiftExpression: ShiftExpression T_GT_GT_GT AdditiveExpression;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::URShift, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

RelationalExpression_In: ShiftExpression;
RelationalExpression: ShiftExpression;

RelationalExpression_In: RelationalExpression_In RelationalOperator ShiftExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
RelationalExpression: RelationalExpression RelationalOperator ShiftExpression;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, sym(2).ival, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

RelationalOperator: T_LT;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::Lt;
    } break;
./
RelationalOperator: T_GT;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::Gt;
    } break;
./
RelationalOperator: T_LE;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::Le;
    } break;
./
RelationalOperator: T_GE;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::Ge;
    } break;
./
RelationalOperator: T_INSTANCEOF;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InstanceOf;
    } break;
./

RelationalExpression_In: RelationalExpression_In T_IN ShiftExpression;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::In, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

EqualityExpression_In: RelationalExpression_In;
EqualityExpression: RelationalExpression;

EqualityExpression_In: EqualityExpression_In EqualityOperator RelationalExpression_In;
/.  case $rule_number: Q_FALLTHROUGH(); ./
EqualityExpression: EqualityExpression EqualityOperator RelationalExpression;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, sym(2).ival, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

EqualityOperator: T_EQ_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::Equal;
    } break;
./
EqualityOperator: T_NOT_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::NotEqual;
    } break;
./
EqualityOperator: T_EQ_EQ_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::StrictEqual;
    } break;
./
EqualityOperator: T_NOT_EQ_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::StrictNotEqual;
    } break;
./


BitwiseANDExpression: EqualityExpression;
BitwiseANDExpression_In: EqualityExpression_In;

BitwiseANDExpression: BitwiseANDExpression T_AND EqualityExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
BitwiseANDExpression_In: BitwiseANDExpression_In T_AND EqualityExpression_In;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::BitAnd, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./


BitwiseXORExpression: BitwiseANDExpression;
BitwiseXORExpression_In: BitwiseANDExpression_In;

BitwiseXORExpression: BitwiseXORExpression T_XOR BitwiseANDExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
BitwiseXORExpression_In: BitwiseXORExpression_In T_XOR BitwiseANDExpression_In;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::BitXor, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

BitwiseORExpression: BitwiseXORExpression;
BitwiseORExpression_In: BitwiseXORExpression_In;

BitwiseORExpression: BitwiseORExpression T_OR BitwiseXORExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
BitwiseORExpression_In: BitwiseORExpression_In T_OR BitwiseXORExpression_In;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::BitOr, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

LogicalANDExpression: BitwiseORExpression;
LogicalANDExpression_In: BitwiseORExpression_In;

LogicalANDExpression: LogicalANDExpression T_AND_AND BitwiseORExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
LogicalANDExpression_In: LogicalANDExpression_In T_AND_AND BitwiseORExpression_In;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::And, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

LogicalORExpression: LogicalANDExpression;
LogicalORExpression_In: LogicalANDExpression_In;

LogicalORExpression: LogicalORExpression T_OR_OR LogicalANDExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
LogicalORExpression_In: LogicalORExpression_In T_OR_OR LogicalANDExpression_In;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::Or, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./


ConditionalExpression: LogicalORExpression;
ConditionalExpression_In: LogicalORExpression_In;

ConditionalExpression: LogicalORExpression T_QUESTION AssignmentExpression_In T_COLON AssignmentExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
ConditionalExpression_In: LogicalORExpression_In T_QUESTION AssignmentExpression_In T_COLON AssignmentExpression_In;
/.
    case $rule_number: {
        AST::ConditionalExpression *node = new (pool) AST::ConditionalExpression(sym(1).Expression, sym(3).Expression, sym(5).Expression);
        node->questionToken = loc(2);
        node->colonToken = loc(4);
        sym(1).Node = node;
    } break;
./

AssignmentExpression: ConditionalExpression;
AssignmentExpression_In: ConditionalExpression_In;

AssignmentExpression: YieldExpression;
AssignmentExpression_In: YieldExpression_In;

AssignmentExpression: ArrowFunction;
AssignmentExpression_In: ArrowFunction_In;

AssignmentExpression: LeftHandSideExpression T_EQ AssignmentExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
AssignmentExpression_In: LeftHandSideExpression T_EQ AssignmentExpression_In;
/.
    case $rule_number: {
        // need to convert the LHS to an AssignmentPattern if it was an Array/ObjectLiteral
        if (AST::Pattern *p = sym(1).Expression->patternCast()) {
            AST::SourceLocation errorLoc;
            QString errorMsg;
            if (!p->convertLiteralToAssignmentPattern(pool, &errorLoc, &errorMsg)) {
                syntaxError(errorLoc, errorMsg);
                return false;
            }
        }
        // if lhs is an identifier expression and rhs is an anonymous function expression, we need to assign the name of lhs to the function
        if (auto *f = asAnonymousFunctionDefinition(sym(3).Expression)) {
            if (auto *id = AST::cast<AST::IdentifierExpression *>(sym(1).Expression))
                f->name = id->name;
        }
        if (auto *c = asAnonymousClassDefinition(sym(3).Expression)) {
            if (auto *id = AST::cast<AST::IdentifierExpression *>(sym(1).Expression))
                c->name = id->name;
        }

        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, QSOperator::Assign, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

AssignmentExpression: LeftHandSideExpression AssignmentOperator AssignmentExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
AssignmentExpression_In: LeftHandSideExpression AssignmentOperator AssignmentExpression_In;
/.
    case $rule_number: {
        AST::BinaryExpression *node = new (pool) AST::BinaryExpression(sym(1).Expression, sym(2).ival, sym(3).Expression);
        node->operatorToken = loc(2);
        sym(1).Node = node;
    } break;
./

AssignmentOperator: T_STAR_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InplaceMul;
    } break;
./

AssignmentOperator: T_STAR_STAR_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InplaceExp;
    } break;
./

AssignmentOperator: T_DIVIDE_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InplaceDiv;
    } break;
./

AssignmentOperator: T_REMAINDER_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InplaceMod;
    } break;
./

AssignmentOperator: T_PLUS_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InplaceAdd;
    } break;
./

AssignmentOperator: T_MINUS_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InplaceSub;
    } break;
./

AssignmentOperator: T_LT_LT_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InplaceLeftShift;
    } break;
./

AssignmentOperator: T_GT_GT_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InplaceRightShift;
    } break;
./

AssignmentOperator: T_GT_GT_GT_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InplaceURightShift;
    } break;
./

AssignmentOperator: T_AND_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InplaceAnd;
    } break;
./

AssignmentOperator: T_XOR_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InplaceXor;
    } break;
./

AssignmentOperator: T_OR_EQ;
/.
    case $rule_number: {
        sym(1).ival = QSOperator::InplaceOr;
    } break;
./

Expression: AssignmentExpression;
Expression_In: AssignmentExpression_In;

Expression: Expression T_COMMA AssignmentExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Expression_In: Expression_In T_COMMA AssignmentExpression_In;
/.
    case $rule_number: {
          AST::Expression *node = new (pool) AST::Expression(sym(1).Expression, sym(3).Expression);
          node->commaToken = loc(2);
          sym(1).Node = node;
    } break;
./

ExpressionOpt: ;
/.  case $rule_number: Q_FALLTHROUGH(); ./
ExpressionOpt_In: ;
/.
    case $rule_number: {
      sym(1).Node = nullptr;
    } break;
./

ExpressionOpt: Expression;
ExpressionOpt_In: Expression_In;

-- Statements

Statement: ExpressionStatementLookahead T_FORCE_BLOCK BlockStatement;
/.
    case $rule_number: {
        sym(1).Node = sym(3).Node;
    } break;
./

Statement: ExpressionStatementLookahead VariableStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Statement: ExpressionStatementLookahead EmptyStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Statement: ExpressionStatementLookahead ExpressionStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Statement: ExpressionStatementLookahead IfStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Statement: ExpressionStatementLookahead BreakableStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Statement: ExpressionStatementLookahead ContinueStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Statement: ExpressionStatementLookahead BreakStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Statement: ExpressionStatementLookahead ReturnStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Statement: ExpressionStatementLookahead WithStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Statement: ExpressionStatementLookahead LabelledStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Statement: ExpressionStatementLookahead ThrowStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Statement: ExpressionStatementLookahead TryStatement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
Statement: ExpressionStatementLookahead DebuggerStatement;
/.
    case $rule_number: {
        sym(1).Node = sym(2).Node;
    } break;
./

Declaration: HoistableDeclaration;
Declaration: ClassDeclaration;
Declaration: LexicalDeclaration_In;

HoistableDeclaration: FunctionDeclaration;
HoistableDeclaration: GeneratorDeclaration;

HoistableDeclaration_Default: FunctionDeclaration_Default;
HoistableDeclaration_Default: GeneratorDeclaration_Default;

BreakableStatement: IterationStatement;
BreakableStatement: SwitchStatement;

BlockStatement: Block;

Block: T_LBRACE StatementListOpt T_RBRACE;
/.
    case $rule_number: {
        AST::Block *node = new (pool) AST::Block(sym(2).StatementList);
        node->lbraceToken = loc(1);
        node->rbraceToken = loc(3);
        sym(1).Node = node;
    } break;
./

StatementList: StatementListItem;

StatementList: StatementList StatementListItem;
/.
    case $rule_number: {
        sym(1).StatementList = sym(1).StatementList->append(sym(2).StatementList);
    } break;
./

StatementListItem: Statement;
/.
    case $rule_number: {
        sym(1).StatementList = new (pool) AST::StatementList(sym(1).Statement);
    } break;
./

StatementListItem: ExpressionStatementLookahead T_FORCE_DECLARATION Declaration T_AUTOMATIC_SEMICOLON;
StatementListItem: ExpressionStatementLookahead T_FORCE_DECLARATION Declaration T_SEMICOLON;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::StatementList(sym(3).FunctionDeclaration);
    } break;
./

StatementListOpt: ExpressionStatementLookahead;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

StatementListOpt: StatementList;
/.
    case $rule_number: {
        sym(1).Node = sym(1).StatementList->finish();
    } break;
./

LetOrConst: T_LET;
/.
    case $rule_number: {
        sym(1).scope = AST::VariableScope::Let;
    } break;
./
LetOrConst: T_CONST;
/.
    case $rule_number: {
        sym(1).scope = AST::VariableScope::Const;
    } break;
./

Var: T_VAR;
/.
    case $rule_number: {
        sym(1).scope = AST::VariableScope::Var;
    } break;
./

LexicalDeclaration: LetOrConst BindingList;
/.  case $rule_number: Q_FALLTHROUGH(); ./
LexicalDeclaration_In: LetOrConst BindingList_In;
/.  case $rule_number: Q_FALLTHROUGH(); ./
VarDeclaration: Var VariableDeclarationList;
/.  case $rule_number: Q_FALLTHROUGH(); ./
VarDeclaration_In: Var VariableDeclarationList_In;
/.
    case $rule_number: {
        AST::VariableStatement *node = new (pool) AST::VariableStatement(sym(2).VariableDeclarationList->finish(sym(1).scope));
        node->declarationKindToken = loc(1);
        sym(1).Node = node;
    } break;
./

VariableStatement: VarDeclaration_In T_AUTOMATIC_SEMICOLON;
VariableStatement: VarDeclaration_In T_SEMICOLON;

BindingList: LexicalBinding_In;
/.  case $rule_number: Q_FALLTHROUGH(); ./
BindingList_In: LexicalBinding_In;
/.  case $rule_number: Q_FALLTHROUGH(); ./
VariableDeclarationList: VariableDeclaration;
/.  case $rule_number: Q_FALLTHROUGH(); ./
VariableDeclarationList_In: VariableDeclaration_In;
/.
    case $rule_number: {
  sym(1).Node = new (pool) AST::VariableDeclarationList(sym(1).PatternElement);
    } break;
./

BindingList: BindingList T_COMMA LexicalBinding;
/.  case $rule_number: Q_FALLTHROUGH(); ./
BindingList_In: BindingList_In T_COMMA LexicalBinding_In;
/.  case $rule_number: Q_FALLTHROUGH(); ./
VariableDeclarationList: VariableDeclarationList T_COMMA VariableDeclaration;
/.  case $rule_number: Q_FALLTHROUGH(); ./
VariableDeclarationList_In: VariableDeclarationList_In T_COMMA VariableDeclaration_In;
/.
    case $rule_number: {
        AST::VariableDeclarationList *node = new (pool) AST::VariableDeclarationList(sym(1).VariableDeclarationList, sym(3).PatternElement);
        node->commaToken = loc(2);
        sym(1).Node = node;
    } break;
./

LexicalBinding: BindingIdentifier InitializerOpt;
/.  case $rule_number: Q_FALLTHROUGH(); ./
LexicalBinding_In: BindingIdentifier InitializerOpt_In;
/.  case $rule_number: Q_FALLTHROUGH(); ./
VariableDeclaration: BindingIdentifier InitializerOpt;
/.  case $rule_number: Q_FALLTHROUGH(); ./
VariableDeclaration_In: BindingIdentifier InitializerOpt_In;
/.
    case $rule_number: {
        auto *node = new (pool) AST::PatternElement(stringRef(1), sym(2).Expression);
        node->identifierToken = loc(1);
        sym(1).Node = node;
        // if initializer is an anonymous function expression, we need to assign identifierref as it's name
        if (auto *f = asAnonymousFunctionDefinition(sym(2).Expression))
            f->name = stringRef(1);
        if (auto *c = asAnonymousClassDefinition(sym(2).Expression))
            c->name = stringRef(1);
    } break;
./

LexicalBinding: BindingPattern Initializer;
/.  case $rule_number: Q_FALLTHROUGH(); ./
LexicalBinding_In: BindingPattern Initializer_In;
/.  case $rule_number: Q_FALLTHROUGH(); ./
VariableDeclaration: BindingPattern Initializer;
/.  case $rule_number: Q_FALLTHROUGH(); ./
VariableDeclaration_In: BindingPattern Initializer_In;
/.
    case $rule_number: {
        auto *node = new (pool) AST::PatternElement(sym(1).Pattern, sym(2).Expression);
        node->identifierToken = loc(1);
        sym(1).Node = node;
    } break;
./

BindingPattern:  T_LBRACE ObjectBindingPattern T_RBRACE;
/.
    case $rule_number: {
        auto *node = new (pool) AST::ObjectPattern(sym(2).PatternPropertyList);
        node->lbraceToken = loc(1);
        node->rbraceToken = loc(3);
        node->parseMode = AST::Pattern::Binding;
        sym(1).Node = node;
    } break;
./

BindingPattern: T_LBRACKET ArrayBindingPattern T_RBRACKET;
/.
    case $rule_number: {
        auto *node = new (pool) AST::ArrayPattern(sym(2).PatternElementList);
        node->lbracketToken = loc(1);
        node->rbracketToken = loc(3);
        node->parseMode = AST::Pattern::Binding;
        sym(1).Node = node;
    } break;
./

ObjectBindingPattern: ;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

ObjectBindingPattern: BindingPropertyList;
/. case $rule_number: ./
ObjectBindingPattern: BindingPropertyList T_COMMA;
/.
    case $rule_number: {
        sym(1).Node = sym(1).PatternPropertyList->finish();
    } break;
./

ArrayBindingPattern: ElisionOpt BindingRestElementOpt;
/.
    case $rule_number: {
        if (sym(1).Elision || sym(2).Node) {
            auto *l = new (pool) AST::PatternElementList(sym(1).Elision, sym(2).PatternElement);
            sym(1).Node = l->finish();
        } else {
            sym(1).Node = nullptr;
        }
    } break;
./

ArrayBindingPattern: BindingElementList;
/.
    case $rule_number: {
        sym(1).Node = sym(1).PatternElementList->finish();
    } break;
./

ArrayBindingPattern: BindingElementList T_COMMA ElisionOpt BindingRestElementOpt;
/.
    case $rule_number: {
        if (sym(3).Elision || sym(4).Node) {
            auto *l = new (pool) AST::PatternElementList(sym(3).Elision, sym(4).PatternElement);
            l = sym(1).PatternElementList->append(l);
            sym(1).Node = l;
        }
        sym(1).Node = sym(1).PatternElementList->finish();
    } break;
./

BindingPropertyList: BindingProperty;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::PatternPropertyList(sym(1).PatternProperty);
    } break;
./

BindingPropertyList: BindingPropertyList T_COMMA BindingProperty;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::PatternPropertyList(sym(1).PatternPropertyList, sym(3).PatternProperty);
    } break;
./

BindingElementList: BindingElisionElement;

BindingElementList: BindingElementList T_COMMA BindingElisionElement;
/.
    case $rule_number: {
        sym(1).PatternElementList = sym(1).PatternElementList->append(sym(3).PatternElementList);
    } break;
./

BindingElisionElement: ElisionOpt BindingElement;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::PatternElementList(sym(1).Elision, sym(2).PatternElement);
    } break;
./


BindingProperty: BindingIdentifier InitializerOpt_In;
/.
    case $rule_number: {
        AST::StringLiteralPropertyName *name = new (pool) AST::StringLiteralPropertyName(stringRef(1));
        name->propertyNameToken = loc(1);
        // if initializer is an anonymous function expression, we need to assign identifierref as it's name
        if (auto *f = asAnonymousFunctionDefinition(sym(2).Expression))
            f->name = stringRef(1);
        if (auto *c = asAnonymousClassDefinition(sym(2).Expression))
            c->name = stringRef(1);
        sym(1).Node = new (pool) AST::PatternProperty(name, stringRef(1), sym(2).Expression);
    } break;
./

BindingProperty: PropertyName T_COLON BindingIdentifier InitializerOpt_In;
/.
    case $rule_number: {
        AST::PatternProperty *node = new (pool) AST::PatternProperty(sym(1).PropertyName, stringRef(3), sym(4).Expression);
        sym(1).Node = node;
    } break;
./

BindingProperty: PropertyName T_COLON BindingPattern InitializerOpt_In;
/.
    case $rule_number: {
        AST::PatternProperty *node = new (pool) AST::PatternProperty(sym(1).PropertyName, sym(3).Pattern, sym(4).Expression);
        sym(1).Node = node;
    } break;
./

BindingElement: BindingIdentifier InitializerOpt_In;
/.
    case $rule_number: {
      AST::PatternElement *node = new (pool) AST::PatternElement(stringRef(1), sym(2).Expression);
      node->identifierToken = loc(1);
      // if initializer is an anonymous function expression, we need to assign identifierref as it's name
      if (auto *f = asAnonymousFunctionDefinition(sym(2).Expression))
          f->name = stringRef(1);
      if (auto *c = asAnonymousClassDefinition(sym(2).Expression))
          c->name = stringRef(1);
      sym(1).Node = node;
    } break;
./

BindingElement: BindingPattern InitializerOpt_In;
/.
    case $rule_number: {
        AST::PatternElement *node = new (pool) AST::PatternElement(sym(1).Pattern, sym(2).Expression);
        sym(1).Node = node;
    } break;
./

BindingRestElement: T_ELLIPSIS BindingIdentifier;
/.
    case $rule_number: {
        AST::PatternElement *node = new (pool) AST::PatternElement(stringRef(2), nullptr, AST::PatternElement::RestElement);
        node->identifierToken = loc(2);
        sym(1).Node = node;
    } break;
./

BindingRestElement: T_ELLIPSIS BindingPattern;
/.
    case $rule_number: {
        AST::PatternElement *node = new (pool) AST::PatternElement(sym(2).Pattern, nullptr, AST::PatternElement::RestElement);
        sym(1).Node = node;
    } break;
./

BindingRestElementOpt: ;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

BindingRestElementOpt: BindingRestElement;


EmptyStatement: T_SEMICOLON;
/.
    case $rule_number: {
        AST::EmptyStatement *node = new (pool) AST::EmptyStatement();
        node->semicolonToken = loc(1);
        sym(1).Node = node;
    } break;
./

-- Spec says it should have a "[lookahead ∉ { {, function, class, let [ }]" before the Expression statement.
-- This is implemented with the rule below that is run before any statement and inserts a T_EXPRESSION_STATEMENT_OK
-- token if it's ok to parse as an expression statement.
ExpressionStatementLookahead: ;
/:
#define J_SCRIPT_EXPRESSIONSTATEMENTLOOKAHEAD_RULE $rule_number
:/
/.
    case $rule_number: {
        int token = lookaheadToken(lexer);
        if (token == T_LBRACE)
            pushToken(T_FORCE_BLOCK);
        else if (token == T_FUNCTION || token == T_CLASS || token == T_LET || token == T_CONST)
            pushToken(T_FORCE_DECLARATION);
    } break;
./

ExpressionStatement: Expression_In T_AUTOMATIC_SEMICOLON;
ExpressionStatement: Expression_In T_SEMICOLON;
/.
    case $rule_number: {
        AST::ExpressionStatement *node = new (pool) AST::ExpressionStatement(sym(1).Expression);
        node->semicolonToken = loc(2);
        sym(1).Node = node;
    } break;
./

IfStatement: T_IF T_LPAREN Expression_In T_RPAREN Statement T_ELSE Statement;
/.
    case $rule_number: {
        AST::IfStatement *node = new (pool) AST::IfStatement(sym(3).Expression, sym(5).Statement, sym(7).Statement);
        node->ifToken = loc(1);
        node->lparenToken = loc(2);
        node->rparenToken = loc(4);
        node->elseToken = loc(6);
        sym(1).Node = node;
    } break;
./

IfStatement: T_IF T_LPAREN Expression_In T_RPAREN Statement;
/.
    case $rule_number: {
        AST::IfStatement *node = new (pool) AST::IfStatement(sym(3).Expression, sym(5).Statement);
        node->ifToken = loc(1);
        node->lparenToken = loc(2);
        node->rparenToken = loc(4);
        sym(1).Node = node;
    } break;
./


IterationStatement: T_DO Statement T_WHILE T_LPAREN Expression_In T_RPAREN T_AUTOMATIC_SEMICOLON;
IterationStatement: T_DO Statement T_WHILE T_LPAREN Expression_In T_RPAREN T_COMPATIBILITY_SEMICOLON;  -- for JSC/V8 compatibility
IterationStatement: T_DO Statement T_WHILE T_LPAREN Expression_In T_RPAREN T_SEMICOLON;
/.
    case $rule_number: {
        AST::DoWhileStatement *node = new (pool) AST::DoWhileStatement(sym(2).Statement, sym(5).Expression);
        node->doToken = loc(1);
        node->whileToken = loc(3);
        node->lparenToken = loc(4);
        node->rparenToken = loc(6);
        node->semicolonToken = loc(7);
        sym(1).Node = node;
    } break;
./

IterationStatement: T_WHILE T_LPAREN Expression_In T_RPAREN Statement;
/.
    case $rule_number: {
        AST::WhileStatement *node = new (pool) AST::WhileStatement(sym(3).Expression, sym(5).Statement);
        node->whileToken = loc(1);
        node->lparenToken = loc(2);
        node->rparenToken = loc(4);
        sym(1).Node = node;
    } break;
./

IterationStatement: T_FOR T_LPAREN ExpressionOpt T_SEMICOLON ExpressionOpt_In T_SEMICOLON ExpressionOpt_In T_RPAREN Statement; -- [lookahead != { let [ }]
/.
    case $rule_number: {
        AST::ForStatement *node = new (pool) AST::ForStatement(sym(3).Expression, sym(5).Expression, sym(7).Expression, sym(9).Statement);
        node->forToken = loc(1);
        node->lparenToken = loc(2);
        node->firstSemicolonToken = loc(4);
        node->secondSemicolonToken = loc(6);
        node->rparenToken = loc(8);
        sym(1).Node = node;
    } break;
./

IterationStatement: T_FOR T_LPAREN VarDeclaration T_SEMICOLON ExpressionOpt_In T_SEMICOLON ExpressionOpt_In T_RPAREN Statement;
/.  case $rule_number: Q_FALLTHROUGH(); ./
IterationStatement: T_FOR T_LPAREN LexicalDeclaration T_SEMICOLON ExpressionOpt_In T_SEMICOLON ExpressionOpt_In T_RPAREN Statement;
/.
    case $rule_number: {
        // ### get rid of the static_cast!
        AST::ForStatement *node = new (pool) AST::ForStatement(
          static_cast<AST::VariableStatement *>(sym(3).Node)->declarations, sym(5).Expression,
          sym(7).Expression, sym(9).Statement);
        node->forToken = loc(1);
        node->lparenToken = loc(2);
        node->firstSemicolonToken = loc(4);
        node->secondSemicolonToken = loc(6);
        node->rparenToken = loc(8);
        sym(1).Node = node;
    } break;
./

InOrOf: T_IN;
/.
    case $rule_number: {
        sym(1).forEachType = AST::ForEachType::In;
    } break;
./

InOrOf: T_OF;
/.
    case $rule_number: {
        sym(1).forEachType = AST::ForEachType::Of;
    } break;
./

IterationStatement: T_FOR T_LPAREN LeftHandSideExpression InOrOf Expression_In T_RPAREN Statement;
/.
    case $rule_number: {
        // need to convert the LHS to an AssignmentPattern if it was an Array/ObjectLiteral
        if (AST::Pattern *p = sym(3).Expression->patternCast()) {
            AST::SourceLocation errorLoc;
            QString errorMsg;
            if (!p->convertLiteralToAssignmentPattern(pool, &errorLoc, &errorMsg)) {
                syntaxError(errorLoc, errorMsg);
                return false;
            }
        }
        AST::ForEachStatement *node = new (pool) AST::ForEachStatement(sym(3).Expression, sym(5).Expression, sym(7).Statement);
        node->forToken = loc(1);
        node->lparenToken = loc(2);
        node->inOfToken = loc(4);
        node->rparenToken = loc(6);
        node->type = sym(4).forEachType;
        sym(1).Node = node;
    } break;
./

IterationStatement: T_FOR T_LPAREN ForDeclaration InOrOf Expression_In T_RPAREN Statement;
/.
    case $rule_number: {
        AST::ForEachStatement *node = new (pool) AST::ForEachStatement(sym(3).PatternElement, sym(5).Expression, sym(7).Statement);
        node->forToken = loc(1);
        node->lparenToken = loc(2);
        node->inOfToken = loc(4);
        node->rparenToken = loc(6);
        node->type = sym(4).forEachType;
        sym(1).Node = node;
    } break;
./

ForDeclaration: LetOrConst BindingIdentifier;
/.  case $rule_number: Q_FALLTHROUGH(); ./
ForDeclaration: Var BindingIdentifier;
/.
    case $rule_number: {
        auto *node = new (pool) AST::PatternElement(stringRef(2), nullptr);
        node->identifierToken = loc(2);
        node->scope = sym(1).scope;
        node->isForDeclaration = true;
        sym(1).Node = node;
    } break;
./

ForDeclaration: LetOrConst BindingPattern;
/.  case $rule_number: Q_FALLTHROUGH(); ./
ForDeclaration: Var BindingPattern;
/.
    case $rule_number: {
        auto *node = new (pool) AST::PatternElement(sym(2).Pattern, nullptr);
        node->scope = sym(1).scope;
        node->isForDeclaration = true;
        sym(1).Node = node;
    } break;
./

ContinueStatement: T_CONTINUE T_AUTOMATIC_SEMICOLON;
ContinueStatement: T_CONTINUE T_SEMICOLON;
/.
    case $rule_number: {
        AST::ContinueStatement *node = new (pool) AST::ContinueStatement();
        node->continueToken = loc(1);
        node->semicolonToken = loc(2);
        sym(1).Node = node;
    } break;
./

ContinueStatement: T_CONTINUE IdentifierReference T_AUTOMATIC_SEMICOLON;
ContinueStatement: T_CONTINUE IdentifierReference T_SEMICOLON;
/.
    case $rule_number: {
        AST::ContinueStatement *node = new (pool) AST::ContinueStatement(stringRef(2));
        node->continueToken = loc(1);
        node->identifierToken = loc(2);
        node->semicolonToken = loc(3);
        sym(1).Node = node;
    } break;
./

BreakStatement: T_BREAK T_AUTOMATIC_SEMICOLON;
BreakStatement: T_BREAK T_SEMICOLON;
/.
    case $rule_number: {
        AST::BreakStatement *node = new (pool) AST::BreakStatement(QStringRef());
        node->breakToken = loc(1);
        node->semicolonToken = loc(2);
        sym(1).Node = node;
    } break;
./

BreakStatement: T_BREAK IdentifierReference T_AUTOMATIC_SEMICOLON;
BreakStatement: T_BREAK IdentifierReference T_SEMICOLON;
/.
    case $rule_number: {
        AST::BreakStatement *node = new (pool) AST::BreakStatement(stringRef(2));
        node->breakToken = loc(1);
        node->identifierToken = loc(2);
        node->semicolonToken = loc(3);
        sym(1).Node = node;
    } break;
./

ReturnStatement: T_RETURN ExpressionOpt_In T_AUTOMATIC_SEMICOLON;
ReturnStatement: T_RETURN ExpressionOpt_In T_SEMICOLON;
/.
    case $rule_number: {
        if (!functionNestingLevel) {
            syntaxError(loc(1), "Return statement not allowed outside of Function declaration.");
            return false;
        }
        AST::ReturnStatement *node = new (pool) AST::ReturnStatement(sym(2).Expression);
        node->returnToken = loc(1);
        node->semicolonToken = loc(3);
        sym(1).Node = node;
    } break;
./

WithStatement: T_WITH T_LPAREN Expression_In T_RPAREN Statement;
/.
    case $rule_number: {
        AST::WithStatement *node = new (pool) AST::WithStatement(sym(3).Expression, sym(5).Statement);
        node->withToken = loc(1);
        node->lparenToken = loc(2);
        node->rparenToken = loc(4);
        sym(1).Node = node;
    } break;
./

SwitchStatement: T_SWITCH T_LPAREN Expression_In T_RPAREN CaseBlock;
/.
    case $rule_number: {
        AST::SwitchStatement *node = new (pool) AST::SwitchStatement(sym(3).Expression, sym(5).CaseBlock);
        node->switchToken = loc(1);
        node->lparenToken = loc(2);
        node->rparenToken = loc(4);
        sym(1).Node = node;
    } break;
./

CaseBlock: T_LBRACE CaseClausesOpt T_RBRACE;
/.
    case $rule_number: {
        AST::CaseBlock *node = new (pool) AST::CaseBlock(sym(2).CaseClauses);
        node->lbraceToken = loc(1);
        node->rbraceToken = loc(3);
        sym(1).Node = node;
    } break;
./

CaseBlock: T_LBRACE CaseClausesOpt DefaultClause CaseClausesOpt T_RBRACE;
/.
    case $rule_number: {
        AST::CaseBlock *node = new (pool) AST::CaseBlock(sym(2).CaseClauses, sym(3).DefaultClause, sym(4).CaseClauses);
        node->lbraceToken = loc(1);
        node->rbraceToken = loc(5);
        sym(1).Node = node;
    } break;
./

CaseClauses: CaseClause;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::CaseClauses(sym(1).CaseClause);
    } break;
./

CaseClauses: CaseClauses CaseClause;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::CaseClauses(sym(1).CaseClauses, sym(2).CaseClause);
    } break;
./

CaseClausesOpt: ;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

CaseClausesOpt: CaseClauses;
/.
    case $rule_number: {
        sym(1).Node = sym(1).CaseClauses->finish();
    } break;
./

CaseClause: T_CASE Expression_In T_COLON StatementListOpt;
/.
    case $rule_number: {
        AST::CaseClause *node = new (pool) AST::CaseClause(sym(2).Expression, sym(4).StatementList);
        node->caseToken = loc(1);
        node->colonToken = loc(3);
        sym(1).Node = node;
    } break;
./

DefaultClause: T_DEFAULT T_COLON StatementListOpt;
/.
    case $rule_number: {
        AST::DefaultClause *node = new (pool) AST::DefaultClause(sym(3).StatementList);
        node->defaultToken = loc(1);
        node->colonToken = loc(2);
        sym(1).Node = node;
    } break;
./

LabelledStatement: IdentifierReference T_COLON LabelledItem;
/.
    case $rule_number: {
        AST::LabelledStatement *node = new (pool) AST::LabelledStatement(stringRef(1), sym(3).Statement);
        node->identifierToken = loc(1);
        node->colonToken = loc(2);
        sym(1).Node = node;
    } break;
./

LabelledItem: Statement;

LabelledItem: ExpressionStatementLookahead T_FORCE_DECLARATION FunctionDeclaration;
/.
    case $rule_number: {
        syntaxError(loc(3), "FunctionDeclarations are not allowed after a label.");
        return false;
    } break;
./

ThrowStatement: T_THROW Expression_In T_AUTOMATIC_SEMICOLON;
ThrowStatement: T_THROW Expression_In T_SEMICOLON;
/.
    case $rule_number: {
        AST::ThrowStatement *node = new (pool) AST::ThrowStatement(sym(2).Expression);
        node->throwToken = loc(1);
        node->semicolonToken = loc(3);
        sym(1).Node = node;
    } break;
./

TryStatement: T_TRY Block Catch;
/.
    case $rule_number: {
        AST::TryStatement *node = new (pool) AST::TryStatement(sym(2).Statement, sym(3).Catch);
        node->tryToken = loc(1);
        sym(1).Node = node;
    } break;
./

TryStatement: T_TRY Block Finally;
/.
    case $rule_number: {
        AST::TryStatement *node = new (pool) AST::TryStatement(sym(2).Statement, sym(3).Finally);
        node->tryToken = loc(1);
        sym(1).Node = node;
    } break;
./

TryStatement: T_TRY Block Catch Finally;
/.
    case $rule_number: {
        AST::TryStatement *node = new (pool) AST::TryStatement(sym(2).Statement, sym(3).Catch, sym(4).Finally);
        node->tryToken = loc(1);
        sym(1).Node = node;
    } break;
./

Catch: T_CATCH T_LPAREN CatchParameter T_RPAREN Block;
/.
    case $rule_number: {
        AST::Catch *node = new (pool) AST::Catch(sym(3).PatternElement, sym(5).Block);
        node->catchToken = loc(1);
        node->lparenToken = loc(2);
        node->identifierToken = loc(3);
        node->rparenToken = loc(4);
        sym(1).Node = node;
    } break;
./

Finally: T_FINALLY Block;
/.
    case $rule_number: {
        AST::Finally *node = new (pool) AST::Finally(sym(2).Block);
        node->finallyToken = loc(1);
        sym(1).Node = node;
    } break;
./

CatchParameter: BindingIdentifier;
/.
    case $rule_number: {
        AST::PatternElement *node = new (pool) AST::PatternElement(stringRef(1));
        node->identifierToken = loc(1);
        node->scope = AST::VariableScope::Let;
        sym(1).Node = node;
    } break;
./

CatchParameter: BindingPattern;
/.
    case $rule_number: {
        AST::PatternElement *node = new (pool) AST::PatternElement(sym(1).Pattern);
        node->scope = AST::VariableScope::Let;
        sym(1).Node = node;
    } break;
./

DebuggerStatement: T_DEBUGGER T_AUTOMATIC_SEMICOLON; -- automatic semicolon
DebuggerStatement: T_DEBUGGER T_SEMICOLON;
/.
    case $rule_number: {
        AST::DebuggerStatement *node = new (pool) AST::DebuggerStatement();
        node->debuggerToken = loc(1);
        node->semicolonToken = loc(2);
        sym(1).Node = node;
    } break;
./

-- tell the parser to prefer function declarations to function expressions.
-- That is, the `Function' symbol is used to mark the start of a function
-- declaration.
-- This is still required for parsing QML, where MemberExpression and FunctionDeclaration would
-- otherwise conflict.
Function: T_FUNCTION %prec REDUCE_HERE;

FunctionDeclaration: Function BindingIdentifier T_LPAREN FormalParameters T_RPAREN FunctionLBrace FunctionBody FunctionRBrace;
/.
    case $rule_number: {
        AST::FunctionDeclaration *node = new (pool) AST::FunctionDeclaration(stringRef(2), sym(4).FormalParameterList, sym(7).StatementList);
        node->functionToken = loc(1);
        node->identifierToken = loc(2);
        node->lparenToken = loc(3);
        node->rparenToken = loc(5);
        node->lbraceToken = loc(6);
        node->rbraceToken = loc(8);
        sym(1).Node = node;
    } break;
./


FunctionDeclaration_Default: FunctionDeclaration;
FunctionDeclaration_Default: Function T_LPAREN FormalParameters T_RPAREN FunctionLBrace FunctionBody FunctionRBrace;
/.
    case $rule_number: {
        AST::FunctionDeclaration *node = new (pool) AST::FunctionDeclaration(QStringRef(), sym(3).FormalParameterList, sym(6).StatementList);
        node->functionToken = loc(1);
        node->lparenToken = loc(2);
        node->rparenToken = loc(4);
        node->lbraceToken = loc(5);
        node->rbraceToken = loc(7);
        sym(1).Node = node;
    } break;
./

FunctionExpression: T_FUNCTION BindingIdentifier T_LPAREN FormalParameters T_RPAREN FunctionLBrace FunctionBody FunctionRBrace;
/.
    case $rule_number: {
        AST::FunctionExpression *node = new (pool) AST::FunctionExpression(stringRef(2), sym(4).FormalParameterList, sym(7).StatementList);
        node->functionToken = loc(1);
        if (! stringRef(2).isNull())
          node->identifierToken = loc(2);
        node->lparenToken = loc(3);
        node->rparenToken = loc(5);
        node->lbraceToken = loc(6);
        node->rbraceToken = loc(8);
        sym(1).Node = node;
    } break;
./

FunctionExpression: T_FUNCTION T_LPAREN FormalParameters T_RPAREN FunctionLBrace FunctionBody FunctionRBrace;
/.
    case $rule_number: {
        AST::FunctionExpression *node = new (pool) AST::FunctionExpression(QStringRef(), sym(3).FormalParameterList, sym(6).StatementList);
        node->functionToken = loc(1);
        node->lparenToken = loc(2);
        node->rparenToken = loc(4);
        node->lbraceToken = loc(5);
        node->rbraceToken = loc(7);
        sym(1).Node = node;
    } break;
./

StrictFormalParameters: FormalParameters;

FormalParameters: ;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

FormalParameters: BindingRestElement;
/.
    case $rule_number: {
        AST::FormalParameterList *node = (new (pool) AST::FormalParameterList(nullptr, sym(1).PatternElement))->finish(pool);
        sym(1).Node = node;
    } break;
./

FormalParameters: FormalParameterList;
/. case $rule_number: ./
FormalParameters: FormalParameterList T_COMMA;
/.
    case $rule_number: {
        sym(1).Node = sym(1).FormalParameterList->finish(pool);
    } break;
./

FormalParameters: FormalParameterList T_COMMA BindingRestElement;
/.
    case $rule_number: {
        AST::FormalParameterList *node = (new (pool) AST::FormalParameterList(sym(1).FormalParameterList, sym(3).PatternElement))->finish(pool);
        sym(1).Node = node;
    } break;
./

FormalParameterList: BindingElement;
/.
    case $rule_number: {
        AST::FormalParameterList *node = new (pool) AST::FormalParameterList(nullptr, sym(1).PatternElement);
        sym(1).Node = node;
    } break;
./


FormalParameterList: FormalParameterList T_COMMA BindingElement;
/.
    case $rule_number: {
        AST::FormalParameterList *node = new (pool) AST::FormalParameterList(sym(1).FormalParameterList, sym(3).PatternElement);
        sym(1).Node = node;
    } break;
./

FormalParameter: BindingElement;

FunctionLBrace: T_LBRACE;
/.
    case $rule_number: {
        ++functionNestingLevel;
    } break;
./

FunctionRBrace: T_RBRACE;
/.
    case $rule_number: {
        --functionNestingLevel;
    } break;
./


FunctionBody: StatementListOpt;

ArrowFunction: ArrowParameters T_ARROW ConciseBodyLookahead AssignmentExpression; -- [lookahead ≠ {]
/.  case $rule_number: Q_FALLTHROUGH(); ./
ArrowFunction_In: ArrowParameters T_ARROW ConciseBodyLookahead AssignmentExpression_In; -- [lookahead ≠ {]
/.
    case $rule_number: {
        AST::ReturnStatement *ret = new (pool) AST::ReturnStatement(sym(4).Expression);
        ret->returnToken = sym(4).Node->firstSourceLocation();
        ret->semicolonToken = sym(4).Node->lastSourceLocation();
        AST::StatementList *statements = (new (pool) AST::StatementList(ret))->finish();
        AST::FunctionExpression *f = new (pool) AST::FunctionExpression(QStringRef(), sym(1).FormalParameterList, statements);
        f->isArrowFunction = true;
        f->functionToken = sym(1).Node ? sym(1).Node->firstSourceLocation() : loc(1);
        f->lbraceToken = sym(4).Node->firstSourceLocation();
        f->rbraceToken = sym(4).Node->lastSourceLocation();
        sym(1).Node = f;
    } break;
./

ArrowFunction: ArrowParameters T_ARROW ConciseBodyLookahead T_FORCE_BLOCK FunctionLBrace FunctionBody FunctionRBrace;
/.  case $rule_number: Q_FALLTHROUGH(); ./
ArrowFunction_In: ArrowParameters T_ARROW ConciseBodyLookahead T_FORCE_BLOCK FunctionLBrace FunctionBody FunctionRBrace;
/.
    case $rule_number: {
        AST::FunctionExpression *f = new (pool) AST::FunctionExpression(QStringRef(), sym(1).FormalParameterList, sym(6).StatementList);
        f->isArrowFunction = true;
        f->functionToken = sym(1).Node ? sym(1).Node->firstSourceLocation() : loc(1);
        f->lbraceToken = loc(6);
        f->rbraceToken = loc(7);
        sym(1).Node = f;
    } break;
./

ArrowParameters: BindingIdentifier;
/.
    case $rule_number: {
        AST::PatternElement *e = new (pool) AST::PatternElement(stringRef(1), nullptr, AST::PatternElement::Binding);
        e->identifierToken = loc(1);
        sym(1).FormalParameterList = (new (pool) AST::FormalParameterList(nullptr, e))->finish(pool);
    } break;
./

-- CoverParenthesizedExpressionAndArrowParameterList for ArrowParameters i being refined to:
-- ArrowFormalParameters: T_LPAREN StrictFormalParameters T_RPAREN
ArrowParameters: CoverParenthesizedExpressionAndArrowParameterList;
/.
    case $rule_number: {
        if (coverExpressionType != CE_FormalParameterList) {
            AST::NestedExpression *ne = static_cast<AST::NestedExpression *>(sym(1).Node);
            AST::FormalParameterList *list = ne->expression->reparseAsFormalParameterList(pool);
            if (!list) {
                syntaxError(loc(1), "Invalid Arrow parameter list.");
                return false;
            }
            sym(1).Node = list->finish(pool);
        }
    } break;
./

ConciseBodyLookahead: ;
/:
#define J_SCRIPT_CONCISEBODYLOOKAHEAD_RULE $rule_number
:/
/.
    case $rule_number: {
        if (lookaheadToken(lexer) == T_LBRACE)
            pushToken(T_FORCE_BLOCK);
    } break;
./

MethodDefinition: PropertyName T_LPAREN StrictFormalParameters T_RPAREN FunctionLBrace FunctionBody FunctionRBrace;
/.
    case $rule_number: {
        AST::FunctionExpression *f = new (pool) AST::FunctionExpression(stringRef(1), sym(3).FormalParameterList, sym(6).StatementList);
        f->functionToken = sym(1).PropertyName->firstSourceLocation();
        f->lparenToken = loc(2);
        f->rparenToken = loc(4);
        f->lbraceToken = loc(5);
        f->rbraceToken = loc(7);
        AST::PatternProperty *node = new (pool) AST::PatternProperty(sym(1).PropertyName, f);
        node->colonToken = loc(2);
        sym(1).Node = node;
    } break;
./

MethodDefinition: T_STAR PropertyName GeneratorLParen StrictFormalParameters T_RPAREN FunctionLBrace GeneratorBody GeneratorRBrace;
/.
    case $rule_number: {
        AST::FunctionExpression *f = new (pool) AST::FunctionExpression(stringRef(2), sym(4).FormalParameterList, sym(7).StatementList);
        f->functionToken = sym(2).PropertyName->firstSourceLocation();
        f->lparenToken = loc(3);
        f->rparenToken = loc(5);
        f->lbraceToken = loc(6);
        f->rbraceToken = loc(8);
        f->isGenerator = true;
        AST::PatternProperty *node = new (pool) AST::PatternProperty(sym(2).PropertyName, f);
        node->colonToken = loc(2);
        sym(1).Node = node;
    } break;
./


MethodDefinition: T_GET PropertyName T_LPAREN T_RPAREN FunctionLBrace FunctionBody FunctionRBrace;
/.
    case $rule_number: {
        AST::FunctionExpression *f = new (pool) AST::FunctionExpression(stringRef(2), nullptr, sym(6).StatementList);
        f->functionToken = sym(2).PropertyName->firstSourceLocation();
        f->lparenToken = loc(3);
        f->rparenToken = loc(4);
        f->lbraceToken = loc(5);
        f->rbraceToken = loc(7);
        AST::PatternProperty *node = new (pool) AST::PatternProperty(sym(2).PropertyName, f, AST::PatternProperty::Getter);
        node->colonToken = loc(2);
        sym(1).Node = node;
    } break;
./

MethodDefinition: T_SET PropertyName T_LPAREN PropertySetParameterList T_RPAREN FunctionLBrace FunctionBody FunctionRBrace;
/.
    case $rule_number: {
        AST::FunctionExpression *f = new (pool) AST::FunctionExpression(stringRef(2), sym(4).FormalParameterList, sym(7).StatementList);
        f->functionToken = sym(2).PropertyName->firstSourceLocation();
        f->lparenToken = loc(3);
        f->rparenToken = loc(5);
        f->lbraceToken = loc(6);
        f->rbraceToken = loc(8);
        AST::PatternProperty *node = new (pool) AST::PatternProperty(sym(2).PropertyName, f, AST::PatternProperty::Setter);
        node->colonToken = loc(2);
        sym(1).Node = node;
    } break;
./


PropertySetParameterList: FormalParameter;
/.
    case $rule_number: {
        AST::FormalParameterList *node = (new (pool) AST::FormalParameterList(nullptr, sym(1).PatternElement))->finish(pool);
        sym(1).Node = node;
    } break;
./

GeneratorLParen: T_LPAREN;
/.
    case $rule_number: {
        lexer->enterGeneratorBody();
    } break;
./

GeneratorRBrace: T_RBRACE;
/.
    case $rule_number: {
        --functionNestingLevel;
        lexer->leaveGeneratorBody();
    } break;
./

GeneratorDeclaration: Function T_STAR BindingIdentifier GeneratorLParen FormalParameters T_RPAREN FunctionLBrace GeneratorBody GeneratorRBrace;
/.
    case $rule_number: {
        AST::FunctionDeclaration *node = new (pool) AST::FunctionDeclaration(stringRef(3), sym(5).FormalParameterList, sym(8).StatementList);
        node->functionToken = loc(1);
        node->identifierToken = loc(3);
        node->lparenToken = loc(4);
        node->rparenToken = loc(6);
        node->lbraceToken = loc(7);
        node->rbraceToken = loc(9);
        node->isGenerator = true;
        sym(1).Node = node;
    } break;
./

GeneratorDeclaration_Default: GeneratorDeclaration;
GeneratorDeclaration_Default: Function T_STAR GeneratorLParen FormalParameters T_RPAREN FunctionLBrace GeneratorBody GeneratorRBrace;
/.
    case $rule_number: {
        AST::FunctionDeclaration *node = new (pool) AST::FunctionDeclaration(QStringRef(), sym(4).FormalParameterList, sym(7).StatementList);
        node->functionToken = loc(1);
        node->lparenToken = loc(3);
        node->rparenToken = loc(5);
        node->lbraceToken = loc(6);
        node->rbraceToken = loc(8);
        node->isGenerator = true;
        sym(1).Node = node;
    } break;
./

GeneratorExpression: T_FUNCTION T_STAR BindingIdentifier GeneratorLParen FormalParameters T_RPAREN FunctionLBrace GeneratorBody GeneratorRBrace;
/.
    case $rule_number: {
        AST::FunctionExpression *node = new (pool) AST::FunctionExpression(stringRef(3), sym(5).FormalParameterList, sym(8).StatementList);
        node->functionToken = loc(1);
        if (!stringRef(3).isNull())
          node->identifierToken = loc(3);
        node->lparenToken = loc(4);
        node->rparenToken = loc(6);
        node->lbraceToken = loc(7);
        node->rbraceToken = loc(9);
        node->isGenerator = true;
        sym(1).Node = node;
    } break;
./

GeneratorExpression: T_FUNCTION T_STAR GeneratorLParen FormalParameters T_RPAREN FunctionLBrace GeneratorBody GeneratorRBrace;
/.
    case $rule_number: {
        AST::FunctionExpression *node = new (pool) AST::FunctionExpression(QStringRef(), sym(4).FormalParameterList, sym(7).StatementList);
        node->functionToken = loc(1);
        node->lparenToken = loc(3);
        node->rparenToken = loc(5);
        node->lbraceToken = loc(6);
        node->rbraceToken = loc(8);
        node->isGenerator = true;
        sym(1).Node = node;
    } break;
./

GeneratorBody: FunctionBody;

YieldExpression: T_YIELD;
/.  case $rule_number: Q_FALLTHROUGH(); ./
YieldExpression_In: T_YIELD;
/.
    case $rule_number: {
        AST::YieldExpression *node = new (pool) AST::YieldExpression();
        node->yieldToken = loc(1);
        sym(1).Node = node;
    } break;
./

YieldExpression: T_YIELD T_STAR AssignmentExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
YieldExpression_In: T_YIELD T_STAR AssignmentExpression_In;
/.
    case $rule_number: {
        AST::YieldExpression *node = new (pool) AST::YieldExpression(sym(3).Expression);
        node->yieldToken = loc(1);
        node->isYieldStar = true;
        sym(1).Node = node;
    } break;
./

YieldExpression: T_YIELD AssignmentExpression;
/.  case $rule_number: Q_FALLTHROUGH(); ./
YieldExpression_In: T_YIELD AssignmentExpression_In;
/.
    case $rule_number: {
        AST::YieldExpression *node = new (pool) AST::YieldExpression(sym(2).Expression);
        node->yieldToken = loc(1);
        sym(1).Node = node;
    } break;
./


ClassDeclaration: T_CLASS BindingIdentifier ClassHeritageOpt ClassLBrace ClassBodyOpt ClassRBrace;
/.
    case $rule_number: {
        AST::ClassDeclaration *node = new (pool) AST::ClassDeclaration(stringRef(2), sym(3).Expression, sym(5).ClassElementList);
        node->classToken = loc(1);
        node->identifierToken = loc(2);
        node->lbraceToken = loc(4);
        node->rbraceToken = loc(6);
        sym(1).Node = node;
    } break;
./

ClassExpression: T_CLASS BindingIdentifier ClassHeritageOpt ClassLBrace ClassBodyOpt ClassRBrace;
/.
    case $rule_number: {
        AST::ClassExpression *node = new (pool) AST::ClassExpression(stringRef(2), sym(3).Expression, sym(5).ClassElementList);
        node->classToken = loc(1);
        node->identifierToken = loc(2);
        node->lbraceToken = loc(4);
        node->rbraceToken = loc(6);
        sym(1).Node = node;
    } break;
./

ClassDeclaration_Default: T_CLASS ClassHeritageOpt ClassLBrace ClassBodyOpt ClassRBrace;
/.
    case $rule_number: {
        AST::ClassDeclaration *node = new (pool) AST::ClassDeclaration(QStringRef(), sym(2).Expression, sym(4).ClassElementList);
        node->classToken = loc(1);
        node->lbraceToken = loc(3);
        node->rbraceToken = loc(5);
        sym(1).Node = node;
    } break;
./

ClassExpression: T_CLASS ClassHeritageOpt ClassLBrace ClassBodyOpt ClassRBrace;
/.
    case $rule_number: {
        AST::ClassExpression *node = new (pool) AST::ClassExpression(QStringRef(), sym(2).Expression, sym(4).ClassElementList);
        node->classToken = loc(1);
        node->lbraceToken = loc(3);
        node->rbraceToken = loc(5);
        sym(1).Node = node;
    } break;
./

ClassDeclaration_Default: ClassDeclaration;

ClassLBrace: T_LBRACE;
/.
    case $rule_number: {
        lexer->setStaticIsKeyword(true);
    } break;
./

ClassRBrace: T_RBRACE;
/. case $rule_number: ./
ClassStaticQualifier: T_STATIC;
/.
    case $rule_number: {
        lexer->setStaticIsKeyword(false);
    } break;
./

ClassHeritageOpt: ;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

ClassHeritageOpt: T_EXTENDS LeftHandSideExpression;
/.
    case $rule_number: {
        sym(1).Node = sym(2).Node;
    } break;
./

ClassBodyOpt: ;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

ClassBodyOpt: ClassElementList;
/.
    case $rule_number: {
        if (sym(1).Node)
            sym(1).Node = sym(1).ClassElementList->finish();
    } break;
./

ClassElementList: ClassElement;

ClassElementList: ClassElementList ClassElement;
/.
    case $rule_number: {
        if (sym(2).Node)
            sym(1).ClassElementList = sym(1).ClassElementList->append(sym(2).ClassElementList);
    } break;
./

ClassElement: MethodDefinition;
/.
    case $rule_number: {
        AST::ClassElementList *node = new (pool) AST::ClassElementList(sym(1).PatternProperty, false);
        sym(1).Node = node;
    } break;
./

ClassElement: ClassStaticQualifier MethodDefinition;
/.
    case $rule_number: {
        lexer->setStaticIsKeyword(true);
        AST::ClassElementList *node = new (pool) AST::ClassElementList(sym(2).PatternProperty, true);
        sym(1).Node = node;
    } break;
./

ClassElement: T_SEMICOLON;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

-- Scripts and Modules

Script: ;
/.
    case $rule_number: {
        sym(1).Node = nullptr;
    } break;
./

Script: ScriptBody;

ScriptBody: StatementList;
/.
    case $rule_number: {
        sym(1).Node = new (pool) AST::Program(sym(1).StatementList->finish());
    } break;
./

Module: ModuleBodyOpt;
/.  case $rule_number: {
        sym(1).Node = new (pool) AST::ESModule(sym(1).StatementList);
    } break;
./

ModuleBody: ModuleItemList;
/.
    case $rule_number: {
        sym(1).StatementList = sym(1).StatementList->finish();
    } break;
./

ModuleBodyOpt: ;
/.
    case $rule_number: {
        sym(1).StatementList = nullptr;
    } break;
./
ModuleBodyOpt: ModuleBody;

ModuleItemList: ModuleItem;

ModuleItemList: ModuleItemList ModuleItem;
/.
    case $rule_number: {
        sym(1).StatementList = sym(1).StatementList->append(sym(2).StatementList);
    } break;
./

ModuleItem: ImportDeclaration T_AUTOMATIC_SEMICOLON;
/. case $rule_number:  Q_FALLTHROUGH(); ./
ModuleItem: ImportDeclaration T_SEMICOLON;
/. case $rule_number:  Q_FALLTHROUGH(); ./
ModuleItem: ExportDeclaration T_AUTOMATIC_SEMICOLON;
/. case $rule_number:  Q_FALLTHROUGH(); ./
ModuleItem: ExportDeclaration T_SEMICOLON;
/.
    case $rule_number: {
        sym(1).StatementList = new (pool) AST::StatementList(sym(1).Node);
    } break;
./

ModuleItem: StatementListItem;

ImportDeclaration: T_IMPORT ImportClause FromClause;
/.
    case $rule_number: {
        auto decl = new (pool) AST::ImportDeclaration(sym(2).ImportClause, sym(3).FromClause);
        decl->importToken = loc(1);
        sym(1).Node = decl;
    } break;
./
ImportDeclaration: T_IMPORT ModuleSpecifier;
/.
    case $rule_number: {
        auto decl = new (pool) AST::ImportDeclaration(stringRef(2));
        decl->importToken = loc(1);
        decl->moduleSpecifierToken = loc(2);
        sym(1).Node = decl;
    } break;
./

ImportClause: ImportedDefaultBinding;
/.
    case $rule_number: {
        auto clause = new (pool) AST::ImportClause(stringRef(1));
        clause->importedDefaultBindingToken = loc(1);
        sym(1).ImportClause = clause;
    } break;
./
ImportClause: NameSpaceImport;
/.
    case $rule_number: {
        sym(1).ImportClause = new (pool) AST::ImportClause(sym(1).NameSpaceImport);
    } break;
./
ImportClause: NamedImports;
/.
    case $rule_number: {
        sym(1).ImportClause = new (pool) AST::ImportClause(sym(1).NamedImports);
    } break;
./
ImportClause: ImportedDefaultBinding T_COMMA NameSpaceImport;
/.
    case $rule_number: {
        auto importClause = new (pool) AST::ImportClause(stringRef(1), sym(3).NameSpaceImport);
        importClause->importedDefaultBindingToken = loc(1);
        sym(1).ImportClause = importClause;
    } break;
./
ImportClause: ImportedDefaultBinding T_COMMA NamedImports;
/.
    case $rule_number: {
        auto importClause = new (pool) AST::ImportClause(stringRef(1), sym(3).NamedImports);
        importClause->importedDefaultBindingToken = loc(1);
        sym(1).ImportClause = importClause;
    } break;
./

ImportedDefaultBinding: ImportedBinding;

NameSpaceImport: T_STAR T_AS ImportedBinding;
/.
    case $rule_number: {
        auto import = new (pool) AST::NameSpaceImport(stringRef(3));
        import->starToken = loc(1);
        import->importedBindingToken = loc(3);
        sym(1).NameSpaceImport = import;
    } break;
./

NamedImports: T_LBRACE T_RBRACE;
/.
    case $rule_number: {
        auto namedImports = new (pool) AST::NamedImports();
        namedImports->leftBraceToken = loc(1);
        namedImports->rightBraceToken = loc(2);
        sym(1).NamedImports = namedImports;
    } break;
./
NamedImports: T_LBRACE ImportsList T_RBRACE;
/.
    case $rule_number: {
        auto namedImports = new (pool) AST::NamedImports(sym(2).ImportsList->finish());
        namedImports->leftBraceToken = loc(1);
        namedImports->rightBraceToken = loc(3);
        sym(1).NamedImports = namedImports;
    } break;
./
NamedImports: T_LBRACE ImportsList T_COMMA T_RBRACE;
/.
    case $rule_number: {
        auto namedImports = new (pool) AST::NamedImports(sym(2).ImportsList->finish());
        namedImports->leftBraceToken = loc(1);
        namedImports->rightBraceToken = loc(4);
        sym(1).NamedImports = namedImports;
    } break;
./

FromClause: T_FROM ModuleSpecifier;
/.
    case $rule_number: {
        auto clause = new (pool) AST::FromClause(stringRef(2));
        clause->fromToken = loc(1);
        clause->moduleSpecifierToken = loc(2);
        sym(1).FromClause = clause;
    } break;
./

ImportsList: ImportSpecifier;
/.
    case $rule_number: {
        auto importsList = new (pool) AST::ImportsList(sym(1).ImportSpecifier);
        importsList->importSpecifierToken = loc(1);
        sym(1).ImportsList = importsList;
    } break;
./
ImportsList: ImportsList T_COMMA ImportSpecifier;
/.
    case $rule_number: {
        auto importsList = new (pool) AST::ImportsList(sym(1).ImportsList, sym(3).ImportSpecifier);
        importsList->importSpecifierToken = loc(3);
        sym(1).ImportsList = importsList;
    } break;
./

ImportSpecifier: ImportedBinding;
/.
    case $rule_number: {
        auto importSpecifier = new (pool) AST::ImportSpecifier(stringRef(1));
        importSpecifier->importedBindingToken = loc(1);
        sym(1).ImportSpecifier = importSpecifier;
    } break;
./
ImportSpecifier: IdentifierName T_AS ImportedBinding;
/.
    case $rule_number: {
    auto importSpecifier = new (pool) AST::ImportSpecifier(stringRef(1), stringRef(3));
    importSpecifier->identifierToken = loc(1);
    importSpecifier->importedBindingToken = loc(3);
    sym(1).ImportSpecifier = importSpecifier;
    } break;
./

ModuleSpecifier: T_STRING_LITERAL;

ImportedBinding: BindingIdentifier;

ExportDeclarationLookahead: ;
/:
#define J_SCRIPT_EXPORTDECLARATIONLOOKAHEAD_RULE $rule_number
:/
/.
    case $rule_number: {
        int token = lookaheadToken(lexer);
        if (token == T_FUNCTION || token == T_CLASS)
            pushToken(T_FORCE_DECLARATION);
    } break;
./

ExportDeclaration: T_EXPORT T_STAR FromClause;
/.
    case $rule_number: {
        auto exportDeclaration = new (pool) AST::ExportDeclaration(sym(3).FromClause);
        exportDeclaration->exportToken = loc(1);
        sym(1).ExportDeclaration = exportDeclaration;
    } break;
./
ExportDeclaration: T_EXPORT ExportClause FromClause;
/.
    case $rule_number: {
        auto exportDeclaration = new (pool) AST::ExportDeclaration(sym(2).ExportClause, sym(3).FromClause);
        exportDeclaration->exportToken = loc(1);
        sym(1).ExportDeclaration = exportDeclaration;
    } break;
./
ExportDeclaration: T_EXPORT ExportClause;
/.
    case $rule_number: {
        auto exportDeclaration = new (pool) AST::ExportDeclaration(sym(2).ExportClause);
        exportDeclaration->exportToken = loc(1);
        sym(1).ExportDeclaration = exportDeclaration;
    } break;
./
ExportDeclaration: T_EXPORT VariableStatement;
/. case $rule_number:  Q_FALLTHROUGH(); ./
ExportDeclaration: T_EXPORT Declaration;
/.
    case $rule_number: {
        auto exportDeclaration = new (pool) AST::ExportDeclaration(/*exportDefault=*/false, sym(2).Node);
        exportDeclaration->exportToken = loc(1);
        sym(1).ExportDeclaration = exportDeclaration;
    } break;
./
ExportDeclaration: T_EXPORT T_DEFAULT ExportDeclarationLookahead T_FORCE_DECLARATION HoistableDeclaration_Default;
/.
    case $rule_number: {
        if (auto *f = AST::cast<AST::FunctionDeclaration*>(sym(5).Node)) {
            if (f->name.isEmpty()) {
                f->name = stringRef(2);
                f->identifierToken = loc(2);
            }
        }
    } Q_FALLTHROUGH();
./
ExportDeclaration: T_EXPORT T_DEFAULT ExportDeclarationLookahead T_FORCE_DECLARATION ClassDeclaration_Default;
/.
    case $rule_number: {
        // Emulate 15.2.3.11
        if (auto *cls = AST::cast<AST::ClassDeclaration*>(sym(5).Node)) {
            if (cls->name.isEmpty()) {
                cls->name = stringRef(2);
                cls->identifierToken = loc(2);
            }
        }

        auto exportDeclaration = new (pool) AST::ExportDeclaration(/*exportDefault=*/true, sym(5).Node);
        exportDeclaration->exportToken = loc(1);
        sym(1).ExportDeclaration = exportDeclaration;
    } break;
./
ExportDeclaration: T_EXPORT T_DEFAULT ExportDeclarationLookahead AssignmentExpression_In; -- [lookahead ∉ { function, class }]
/.
    case $rule_number: {
        // if lhs is an identifier expression and rhs is an anonymous function expression, we need to assign the name of lhs to the function
        if (auto *f = asAnonymousFunctionDefinition(sym(4).Node)) {
            f->name = stringRef(2);
        }
        if (auto *c = asAnonymousClassDefinition(sym(4).Expression)) {
            c->name = stringRef(2);
        }

        auto exportDeclaration = new (pool) AST::ExportDeclaration(/*exportDefault=*/true, sym(4).Node);
        exportDeclaration->exportToken = loc(1);
        sym(1).ExportDeclaration = exportDeclaration;
    } break;
./

ExportClause: T_LBRACE T_RBRACE;
/.
    case $rule_number: {
        auto exportClause = new (pool) AST::ExportClause();
        exportClause->leftBraceToken = loc(1);
        exportClause->rightBraceToken = loc(2);
        sym(1).ExportClause = exportClause;
    } break;
./
ExportClause: T_LBRACE ExportsList T_RBRACE;
/.
    case $rule_number: {
        auto exportClause = new (pool) AST::ExportClause(sym(2).ExportsList->finish());
        exportClause->leftBraceToken = loc(1);
        exportClause->rightBraceToken = loc(3);
        sym(1).ExportClause = exportClause;
    } break;
./
ExportClause: T_LBRACE ExportsList T_COMMA T_RBRACE;
/.
    case $rule_number: {
        auto exportClause = new (pool) AST::ExportClause(sym(2).ExportsList->finish());
        exportClause->leftBraceToken = loc(1);
        exportClause->rightBraceToken = loc(4);
        sym(1).ExportClause = exportClause;
    } break;
./

ExportsList: ExportSpecifier;
/.
    case $rule_number: {
        sym(1).ExportsList = new (pool) AST::ExportsList(sym(1).ExportSpecifier);
    } break;
./
ExportsList: ExportsList T_COMMA ExportSpecifier;
/.
    case $rule_number: {
        sym(1).ExportsList = new (pool) AST::ExportsList(sym(1).ExportsList, sym(3).ExportSpecifier);
    } break;
./

ExportSpecifier: IdentifierName;
/.
    case $rule_number: {
        auto exportSpecifier = new (pool) AST::ExportSpecifier(stringRef(1));
        exportSpecifier->identifierToken = loc(1);
        sym(1).ExportSpecifier = exportSpecifier;
    } break;
./
ExportSpecifier: IdentifierName T_AS IdentifierName;
/.
    case $rule_number: {
        auto exportSpecifier = new (pool) AST::ExportSpecifier(stringRef(1), stringRef(3));
        exportSpecifier->identifierToken = loc(1);
        exportSpecifier->exportedIdentifierToken = loc(3);
        sym(1).ExportSpecifier = exportSpecifier;
    } break;
./

-- Old top level code

/.
    // ------------ end of switch statement
            } // switch
            action = nt_action(state_stack[tos], lhs[r] - TERMINAL_COUNT);
        } // if
    } while (action != 0);

#ifdef PARSER_DEBUG
    qDebug() << "Done or error.";
#endif

    if (first_token == last_token) {
        const int errorState = state_stack[tos];

        // automatic insertion of `;'
        if (yytoken != -1 && ((t_action(errorState, T_AUTOMATIC_SEMICOLON) && lexer->canInsertAutomaticSemicolon(yytoken))
                              || t_action(errorState, T_COMPATIBILITY_SEMICOLON))) {
#ifdef PARSER_DEBUG
            qDebug() << "Inserting automatic semicolon.";
#endif
            SavedToken &tk = token_buffer[0];
            tk.token = yytoken;
            tk.dval = yylval;
            tk.spell = yytokenspell;
            tk.loc = yylloc;

            yylloc = yyprevlloc;
            yylloc.offset += yylloc.length;
            yylloc.startColumn += yylloc.length;
            yylloc.length = 0;

            //const QString msg = QCoreApplication::translate("QmlParser", "Missing `;'");
            //diagnostic_messages.append(DiagnosticMessage(Severity::Warning, yylloc, msg));

            first_token = &token_buffer[0];
            last_token = &token_buffer[1];

            yytoken = T_SEMICOLON;
            yylval = 0;

            action = errorState;

            goto _Lcheck_token;
        }

        hadErrors = true;

        token_buffer[0].token = yytoken;
        token_buffer[0].dval = yylval;
        token_buffer[0].spell = yytokenspell;
        token_buffer[0].loc = yylloc;

        token_buffer[1].token = yytoken       = lexer->lex();
        token_buffer[1].dval  = yylval        = lexer->tokenValue();
        token_buffer[1].spell = yytokenspell  = lexer->tokenSpell();
        token_buffer[1].loc   = yylloc        = location(lexer);

        if (t_action(errorState, yytoken)) {
            QString msg;
            int token = token_buffer[0].token;
            if (token < 0 || token >= TERMINAL_COUNT)
                msg = QCoreApplication::translate("QmlParser", "Syntax error");
            else
                msg = QCoreApplication::translate("QmlParser", "Unexpected token `%1'").arg(QLatin1String(spell[token]));
            diagnostic_messages.append(DiagnosticMessage(Severity::Error, token_buffer[0].loc, msg));

            action = errorState;
            goto _Lcheck_token;
        }

        static int tokens[] = {
            T_PLUS,
            T_EQ,

            T_COMMA,
            T_COLON,
            T_SEMICOLON,

            T_RPAREN, T_RBRACKET, T_RBRACE,

            T_NUMERIC_LITERAL,
            T_IDENTIFIER,

            T_LPAREN, T_LBRACKET, T_LBRACE,

            EOF_SYMBOL
        };

        for (int *tk = tokens; *tk != EOF_SYMBOL; ++tk) {
            int a = t_action(errorState, *tk);
            if (a > 0 && t_action(a, yytoken)) {
                const QString msg = QCoreApplication::translate("QmlParser", "Expected token `%1'").arg(QLatin1String(spell[*tk]));
                diagnostic_messages.append(DiagnosticMessage(Severity::Error, token_buffer[0].loc, msg));

                yytoken = *tk;
                yylval = 0;
                yylloc = token_buffer[0].loc;
                yylloc.length = 0;

                first_token = &token_buffer[0];
                last_token = &token_buffer[2];

                action = errorState;
                goto _Lcheck_token;
            }
        }

        for (int tk = 1; tk < TERMINAL_COUNT; ++tk) {
            if (tk == T_AUTOMATIC_SEMICOLON || tk == T_FEED_UI_PROGRAM    ||
                tk == T_FEED_JS_STATEMENT   || tk == T_FEED_JS_EXPRESSION)
               continue;

            int a = t_action(errorState, tk);
            if (a > 0 && t_action(a, yytoken)) {
                const QString msg = QCoreApplication::translate("QmlParser", "Expected token `%1'").arg(QLatin1String(spell[tk]));
                diagnostic_messages.append(DiagnosticMessage(Severity::Error, token_buffer[0].loc, msg));

                yytoken = tk;
                yylval = 0;
                yylloc = token_buffer[0].loc;
                yylloc.length = 0;

                action = errorState;
                goto _Lcheck_token;
            }
        }

        const QString msg = QCoreApplication::translate("QmlParser", "Syntax error");
        diagnostic_messages.append(DiagnosticMessage(Severity::Error, token_buffer[0].loc, msg));
    }

    return false;
}

QT_QML_END_NAMESPACE


./
/:
QT_QML_END_NAMESPACE



#endif // QMLJSPARSER_P_H
:/
