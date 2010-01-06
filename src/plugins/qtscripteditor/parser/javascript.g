---------------------------------------------------------------------------
--
-- This file is part of Qt Creator
--
-- Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
--
-- Contact: Nokia Corporation (qt-info@nokia.com)
--
-- Commercial Usage
--
-- Licensees holding valid Qt Commercial licenses may use this file in
-- accordance with the Qt Commercial License Agreement provided with the
-- Software or, alternatively, in accordance with the terms contained in
-- a written agreement between you and Nokia.
--
-- GNU Lesser General Public License Usage
--
-- Alternatively, this file may be used under the terms of the GNU Lesser
-- General Public License version 2.1 as published by the Free Software
-- Foundation and appearing in the file LICENSE.LGPL included in the
-- packaging of this file.  Please review the following information to
-- ensure the GNU Lesser General Public License version 2.1 requirements
-- will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
--
-- If you are unsure which license is appropriate for your use, please
-- contact the sales department at http://qt.nokia.com/contact.
--
---------------------------------------------------------------------------

%parser         JavaScriptGrammar
%decl           javascriptparser_p.h
%impl           javascriptparser.cpp
%expect         3
%expect-rr      1

%token T_AND "&"                T_AND_AND "&&"              T_AND_EQ "&="
%token T_BREAK "break"          T_CASE "case"               T_CATCH "catch"
%token T_COLON ":"              T_COMMA ";"                 T_CONTINUE "continue"
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
%token T_STAR_EQ "*="           T_STRING_LITERAL "string literal"
%token T_SWITCH "switch"        T_THIS "this"               T_THROW "throw"
%token T_TILDE "~"              T_TRY "try"                 T_TYPEOF "typeof"
%token T_VAR "var"              T_VOID "void"               T_WHILE "while"
%token T_WITH "with"            T_XOR "^"                   T_XOR_EQ "^="
%token T_NULL "null"            T_TRUE "true"               T_FALSE "false"
%token T_CONST "const"
%token T_DEBUGGER "debugger"
%token T_RESERVED_WORD "reserved word"

%start Program

/.
/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtCore/QtDebug>
#include <string.h>
#include "javascriptengine_p.h"
#include "javascriptlexer_p.h"
#include "javascriptast_p.h"
#include "javascriptnodepool_p.h"

#define J_SCRIPT_UPDATE_POSITION(node, startloc, endloc) do { \
    node->startLine = startloc.startLine; \
    node->startColumn = startloc.startColumn; \
    node->endLine = endloc.endLine; \
    node->endColumn = endloc.endColumn; \
} while (0)

./

/:
/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file is automatically generated from javascript.g.
// Changes will be lost.
// To re-generate, run: qlalr --no-debug --no-lines --qt javascript.g
//

#ifndef JAVASCRIPTPARSER_P_H
#define JAVASCRIPTPARSER_P_H

#include "javascriptgrammar_p.h"
#include "javascriptastfwd_p.h"
#include <QtCore/QList>

QT_BEGIN_NAMESPACE

class QString;
class JavaScriptEnginePrivate;
class JavaScriptNameIdImpl;

class JavaScriptParser: protected $table
{
public:
    union Value {
      int ival;
      double dval;
      JavaScriptNameIdImpl *sval;
      JavaScript::AST::ArgumentList *ArgumentList;
      JavaScript::AST::CaseBlock *CaseBlock;
      JavaScript::AST::CaseClause *CaseClause;
      JavaScript::AST::CaseClauses *CaseClauses;
      JavaScript::AST::Catch *Catch;
      JavaScript::AST::DefaultClause *DefaultClause;
      JavaScript::AST::ElementList *ElementList;
      JavaScript::AST::Elision *Elision;
      JavaScript::AST::ExpressionNode *Expression;
      JavaScript::AST::Finally *Finally;
      JavaScript::AST::FormalParameterList *FormalParameterList;
      JavaScript::AST::FunctionBody *FunctionBody;
      JavaScript::AST::FunctionDeclaration *FunctionDeclaration;
      JavaScript::AST::Node *Node;
      JavaScript::AST::PropertyName *PropertyName;
      JavaScript::AST::PropertyNameAndValueList *PropertyNameAndValueList;
      JavaScript::AST::SourceElement *SourceElement;
      JavaScript::AST::SourceElements *SourceElements;
      JavaScript::AST::Statement *Statement;
      JavaScript::AST::StatementList *StatementList;
      JavaScript::AST::VariableDeclaration *VariableDeclaration;
      JavaScript::AST::VariableDeclarationList *VariableDeclarationList;
    };

    struct Location {
      int startLine;
      int startColumn;
      int endLine;
      int endColumn;
    };

    struct DiagnosticMessage {
        enum Kind { Warning, Error };

        DiagnosticMessage()
            : kind(Error), line(0), column(0) {}

        DiagnosticMessage(Kind kind, int line, int column, const QString &message)
            : kind(kind), line(line), column(column), message(message) {}

        bool isWarning() const
        { return kind == Warning; }

        bool isError() const
        { return kind == Error; }

        Kind kind;
        int line;
        int column;
        QString message;
    };

public:
    JavaScriptParser();
    ~JavaScriptParser();

    bool parse(JavaScriptEnginePrivate *driver);

    QList<DiagnosticMessage> diagnosticMessages() const
    { return diagnostic_messages; }

    inline DiagnosticMessage diagnosticMessage() const
    {
        foreach (const DiagnosticMessage &d, diagnostic_messages) {
            if (! d.kind == DiagnosticMessage::Warning)
                return d;
        }

        return DiagnosticMessage();
    }

    inline QString errorMessage() const
    { return diagnosticMessage().message; }

    inline int errorLineNumber() const
    { return diagnosticMessage().line; }

    inline int errorColumnNumber() const
    { return diagnosticMessage().column; }

protected:
    inline void reallocateStack();

    inline Value &sym(int index)
    { return sym_stack [tos + index - 1]; }

    inline Location &loc(int index)
    { return location_stack [tos + index - 1]; }

protected:
    int tos;
    int stack_size;
    Value *sym_stack;
    int *state_stack;
    Location *location_stack;

    // error recovery
    enum { TOKEN_BUFFER_SIZE = 3 };

    struct SavedToken {
       int token;
       double dval;
       Location loc;
    };

    double yylval;
    Location yylloc;
    Location yyprevlloc;

    SavedToken token_buffer[TOKEN_BUFFER_SIZE];
    SavedToken *first_token;
    SavedToken *last_token;

    QList<DiagnosticMessage> diagnostic_messages;
};

inline void JavaScriptParser::reallocateStack()
{
    if (! stack_size)
        stack_size = 128;
    else
        stack_size <<= 1;

    sym_stack = reinterpret_cast<Value*> (qRealloc(sym_stack, stack_size * sizeof(Value)));
    state_stack = reinterpret_cast<int*> (qRealloc(state_stack, stack_size * sizeof(int)));
    location_stack = reinterpret_cast<Location*> (qRealloc(location_stack, stack_size * sizeof(Location)));
}

:/


/.

#include "javascriptparser_p.h"

//
// This file is automatically generated from javascript.g.
// Changes will be lost.
// To re-generate, run: qlalr --no-debug --no-lines --qt javascript.g
//

QT_BEGIN_NAMESPACE

inline static bool automatic(JavaScriptEnginePrivate *driver, int token)
{
    return token == $table::T_RBRACE
        || token == 0
        || driver->lexer()->prevTerminator();
}


JavaScriptParser::JavaScriptParser():
    tos(0),
    stack_size(0),
    sym_stack(0),
    state_stack(0),
    location_stack(0),
    first_token(0),
    last_token(0)
{
}

JavaScriptParser::~JavaScriptParser()
{
    if (stack_size) {
        qFree(sym_stack);
        qFree(state_stack);
        qFree(location_stack);
    }
}

static inline JavaScriptParser::Location location(JavaScript::Lexer *lexer)
{
    JavaScriptParser::Location loc;
    loc.startLine = lexer->startLineNo();
    loc.startColumn = lexer->startColumnNo();
    loc.endLine = lexer->endLineNo();
    loc.endColumn = lexer->endColumnNo();
    return loc;
}

bool JavaScriptParser::parse(JavaScriptEnginePrivate *driver)
{
    JavaScript::Lexer *lexer = driver->lexer();
    bool hadErrors = false;
    int yytoken = -1;
    int action = 0;

    first_token = last_token = 0;

    tos = -1;

    do {
        if (++tos == stack_size)
            reallocateStack();

        state_stack[tos] = action;

    _Lcheck_token:
        if (yytoken == -1 && -TERMINAL_COUNT != action_index[action]) {
                yyprevlloc = yylloc;

            if (first_token == last_token) {
                yytoken = lexer->lex();
                yylval = lexer->dval();
                yylloc = location(lexer);
            } else {
                yytoken = first_token->token;
                yylval = first_token->dval;
                yylloc = first_token->loc;
                ++first_token;
            }
        }

        action = t_action(action, yytoken);
        if (action > 0) {
            if (action != ACCEPT_STATE) {
                yytoken = -1;
                sym(1).dval = yylval;
                loc(1) = yylloc;
            } else {
              --tos;
              return ! hadErrors;
            }
        } else if (action < 0) {
          const int r = -action - 1;
          tos -= rhs[r];

          switch (r) {
./

PrimaryExpression: T_THIS ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ThisExpression> (driver->nodePool());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PrimaryExpression: T_IDENTIFIER ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::IdentifierExpression> (driver->nodePool(), sym(1).sval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PrimaryExpression: T_NULL ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::NullExpression> (driver->nodePool());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PrimaryExpression: T_TRUE ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::TrueLiteral> (driver->nodePool());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PrimaryExpression: T_FALSE ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::FalseLiteral> (driver->nodePool());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PrimaryExpression: T_NUMERIC_LITERAL ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::NumericLiteral> (driver->nodePool(), sym(1).dval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PrimaryExpression: T_STRING_LITERAL ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::StringLiteral> (driver->nodePool(), sym(1).sval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PrimaryExpression: T_DIVIDE_ ;
/:
#define J_SCRIPT_REGEXPLITERAL_RULE1 $rule_number
:/
/.
case $rule_number: {
  bool rx = lexer->scanRegExp(JavaScript::Lexer::NoPrefix);
  if (!rx) {
    diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error, lexer->startLineNo(),
        lexer->startColumnNo(), lexer->errorMessage()));
      return false;
  }
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::RegExpLiteral> (driver->nodePool(), lexer->pattern, lexer->flags);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PrimaryExpression: T_DIVIDE_EQ ;
/:
#define J_SCRIPT_REGEXPLITERAL_RULE2 $rule_number
:/
/.
case $rule_number: {
  bool rx = lexer->scanRegExp(JavaScript::Lexer::EqualPrefix);
  if (!rx) {
    diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error, lexer->startLineNo(),
        lexer->startColumnNo(), lexer->errorMessage()));
      return false;
  }
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::RegExpLiteral> (driver->nodePool(), lexer->pattern, lexer->flags);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PrimaryExpression: T_LBRACKET ElisionOpt T_RBRACKET ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ArrayLiteral> (driver->nodePool(), sym(2).Elision);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

PrimaryExpression: T_LBRACKET ElementList T_RBRACKET ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ArrayLiteral> (driver->nodePool(), sym(2).ElementList->finish ());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

PrimaryExpression: T_LBRACKET ElementList T_COMMA ElisionOpt T_RBRACKET ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ArrayLiteral> (driver->nodePool(), sym(2).ElementList->finish (), sym(4).Elision);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(5));
} break;
./

-- PrimaryExpression: T_LBRACE T_RBRACE ;
-- /.
-- case $rule_number: {
--   sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ObjectLiteral> (driver->nodePool());
--   J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
-- } break;
-- ./

PrimaryExpression: T_LBRACE PropertyNameAndValueListOpt T_RBRACE ;
/.
case $rule_number: {
  if (sym(2).Node)
    sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ObjectLiteral> (driver->nodePool(), sym(2).PropertyNameAndValueList->finish ());
  else
    sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ObjectLiteral> (driver->nodePool());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

PrimaryExpression: T_LBRACE PropertyNameAndValueList T_COMMA T_RBRACE ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ObjectLiteral> (driver->nodePool(), sym(2).PropertyNameAndValueList->finish ());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(4));
} break;
./

PrimaryExpression: T_LPAREN Expression T_RPAREN ;
/.
case $rule_number: {
  sym(1) = sym(2);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

ElementList: ElisionOpt AssignmentExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ElementList> (driver->nodePool(), sym(1).Elision, sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

ElementList: ElementList T_COMMA ElisionOpt AssignmentExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ElementList> (driver->nodePool(), sym(1).ElementList, sym(3).Elision, sym(4).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(4));
} break;
./

Elision: T_COMMA ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::Elision> (driver->nodePool());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

Elision: Elision T_COMMA ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::Elision> (driver->nodePool(), sym(1).Elision);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

ElisionOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

ElisionOpt: Elision ;
/.
case $rule_number: {
  sym(1).Elision = sym(1).Elision->finish ();
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PropertyNameAndValueList: PropertyName T_COLON AssignmentExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::PropertyNameAndValueList> (driver->nodePool(), sym(1).PropertyName, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

PropertyNameAndValueList: PropertyNameAndValueList T_COMMA PropertyName T_COLON AssignmentExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::PropertyNameAndValueList> (driver->nodePool(), sym(1).PropertyNameAndValueList, sym(3).PropertyName, sym(5).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(5));
} break;
./

PropertyName: T_IDENTIFIER ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::IdentifierPropertyName> (driver->nodePool(), sym(1).sval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PropertyName: T_STRING_LITERAL ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::StringLiteralPropertyName> (driver->nodePool(), sym(1).sval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PropertyName: T_NUMERIC_LITERAL ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::NumericLiteralPropertyName> (driver->nodePool(), sym(1).dval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

PropertyName: ReservedIdentifier ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::IdentifierPropertyName> (driver->nodePool(), sym(1).sval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

ReservedIdentifier: T_BREAK ;
/.
case $rule_number:
./
ReservedIdentifier: T_CASE ;
/.
case $rule_number:
./
ReservedIdentifier: T_CATCH ;
/.
case $rule_number:
./
ReservedIdentifier: T_CONTINUE ;
/.
case $rule_number:
./
ReservedIdentifier: T_DEFAULT ;
/.
case $rule_number:
./
ReservedIdentifier: T_DELETE ;
/.
case $rule_number:
./
ReservedIdentifier: T_DO ;
/.
case $rule_number:
./
ReservedIdentifier: T_ELSE ;
/.
case $rule_number:
./
ReservedIdentifier: T_FALSE ;
/.
case $rule_number:
./
ReservedIdentifier: T_FINALLY ;
/.
case $rule_number:
./
ReservedIdentifier: T_FOR ;
/.
case $rule_number:
./
ReservedIdentifier: T_FUNCTION ;
/.
case $rule_number:
./
ReservedIdentifier: T_IF ;
/.
case $rule_number:
./
ReservedIdentifier: T_IN ;
/.
case $rule_number:
./
ReservedIdentifier: T_INSTANCEOF ;
/.
case $rule_number:
./
ReservedIdentifier: T_NEW ;
/.
case $rule_number:
./
ReservedIdentifier: T_NULL ;
/.
case $rule_number:
./
ReservedIdentifier: T_RETURN ;
/.
case $rule_number:
./
ReservedIdentifier: T_SWITCH ;
/.
case $rule_number:
./
ReservedIdentifier: T_THIS ;
/.
case $rule_number:
./
ReservedIdentifier: T_THROW ;
/.
case $rule_number:
./
ReservedIdentifier: T_TRUE ;
/.
case $rule_number:
./
ReservedIdentifier: T_TRY ;
/.
case $rule_number:
./
ReservedIdentifier: T_TYPEOF ;
/.
case $rule_number:
./
ReservedIdentifier: T_VAR ;
/.
case $rule_number:
./
ReservedIdentifier: T_VOID ;
/.
case $rule_number:
./
ReservedIdentifier: T_WHILE ;
/.
case $rule_number:
./
ReservedIdentifier: T_CONST ;
/.
case $rule_number:
./
ReservedIdentifier: T_DEBUGGER ;
/.
case $rule_number:
./
ReservedIdentifier: T_RESERVED_WORD ;
/.
case $rule_number:
./
ReservedIdentifier: T_WITH ;
/.
case $rule_number:
{
  sym(1).sval = driver->intern(lexer->characterBuffer(), lexer->characterCount());
} break;
./

PropertyIdentifier: T_IDENTIFIER ;
PropertyIdentifier: ReservedIdentifier ;

MemberExpression: PrimaryExpression ;
MemberExpression: FunctionExpression ;

MemberExpression: MemberExpression T_LBRACKET Expression T_RBRACKET ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ArrayMemberExpression> (driver->nodePool(), sym(1).Expression, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(4));
} break;
./

MemberExpression: MemberExpression T_DOT PropertyIdentifier ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::FieldMemberExpression> (driver->nodePool(), sym(1).Expression, sym(3).sval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(4));
} break;
./

MemberExpression: T_NEW MemberExpression Arguments ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::NewMemberExpression> (driver->nodePool(), sym(2).Expression, sym(3).ArgumentList);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

NewExpression: MemberExpression ;

NewExpression: T_NEW NewExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::NewExpression> (driver->nodePool(), sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

CallExpression: MemberExpression Arguments ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::CallExpression> (driver->nodePool(), sym(1).Expression, sym(2).ArgumentList);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

CallExpression: CallExpression Arguments ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::CallExpression> (driver->nodePool(), sym(1).Expression, sym(2).ArgumentList);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

CallExpression: CallExpression T_LBRACKET Expression T_RBRACKET ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ArrayMemberExpression> (driver->nodePool(), sym(1).Expression, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(4));
} break;
./

CallExpression: CallExpression T_DOT PropertyIdentifier ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::FieldMemberExpression> (driver->nodePool(), sym(1).Expression, sym(3).sval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

Arguments: T_LPAREN T_RPAREN ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

Arguments: T_LPAREN ArgumentList T_RPAREN ;
/.
case $rule_number: {
  sym(1).Node = sym(2).ArgumentList->finish ();
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

ArgumentList: AssignmentExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ArgumentList> (driver->nodePool(), sym(1).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

ArgumentList: ArgumentList T_COMMA AssignmentExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ArgumentList> (driver->nodePool(), sym(1).ArgumentList, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

LeftHandSideExpression: NewExpression ;
LeftHandSideExpression: CallExpression ;
PostfixExpression: LeftHandSideExpression ;

PostfixExpression: LeftHandSideExpression T_PLUS_PLUS ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::PostIncrementExpression> (driver->nodePool(), sym(1).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

PostfixExpression: LeftHandSideExpression T_MINUS_MINUS ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::PostDecrementExpression> (driver->nodePool(), sym(1).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

UnaryExpression: PostfixExpression ;

UnaryExpression: T_DELETE UnaryExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::DeleteExpression> (driver->nodePool(), sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

UnaryExpression: T_VOID UnaryExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::VoidExpression> (driver->nodePool(), sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

UnaryExpression: T_TYPEOF UnaryExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::TypeOfExpression> (driver->nodePool(), sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

UnaryExpression: T_PLUS_PLUS UnaryExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::PreIncrementExpression> (driver->nodePool(), sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

UnaryExpression: T_MINUS_MINUS UnaryExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::PreDecrementExpression> (driver->nodePool(), sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

UnaryExpression: T_PLUS UnaryExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::UnaryPlusExpression> (driver->nodePool(), sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

UnaryExpression: T_MINUS UnaryExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::UnaryMinusExpression> (driver->nodePool(), sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

UnaryExpression: T_TILDE UnaryExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::TildeExpression> (driver->nodePool(), sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

UnaryExpression: T_NOT UnaryExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::NotExpression> (driver->nodePool(), sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

MultiplicativeExpression: UnaryExpression ;

MultiplicativeExpression: MultiplicativeExpression T_STAR UnaryExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Mul, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

MultiplicativeExpression: MultiplicativeExpression T_DIVIDE_ UnaryExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Div, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

MultiplicativeExpression: MultiplicativeExpression T_REMAINDER UnaryExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Mod, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

AdditiveExpression: MultiplicativeExpression ;

AdditiveExpression: AdditiveExpression T_PLUS MultiplicativeExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Add, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

AdditiveExpression: AdditiveExpression T_MINUS MultiplicativeExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Sub, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

ShiftExpression: AdditiveExpression ;

ShiftExpression: ShiftExpression T_LT_LT AdditiveExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::LShift, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

ShiftExpression: ShiftExpression T_GT_GT AdditiveExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::RShift, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

ShiftExpression: ShiftExpression T_GT_GT_GT AdditiveExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::URShift, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

RelationalExpression: ShiftExpression ;

RelationalExpression: RelationalExpression T_LT ShiftExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Lt, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

RelationalExpression: RelationalExpression T_GT ShiftExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Gt, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

RelationalExpression: RelationalExpression T_LE ShiftExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Le, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

RelationalExpression: RelationalExpression T_GE ShiftExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Ge, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

RelationalExpression: RelationalExpression T_INSTANCEOF ShiftExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::InstanceOf, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

RelationalExpression: RelationalExpression T_IN ShiftExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::In, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

RelationalExpressionNotIn: ShiftExpression ;

RelationalExpressionNotIn: RelationalExpressionNotIn T_LT ShiftExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Lt, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

RelationalExpressionNotIn: RelationalExpressionNotIn T_GT ShiftExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Gt, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

RelationalExpressionNotIn: RelationalExpressionNotIn T_LE ShiftExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Le, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

RelationalExpressionNotIn: RelationalExpressionNotIn T_GE ShiftExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Ge, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

RelationalExpressionNotIn: RelationalExpressionNotIn T_INSTANCEOF ShiftExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::InstanceOf, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

EqualityExpression: RelationalExpression ;

EqualityExpression: EqualityExpression T_EQ_EQ RelationalExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Equal, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

EqualityExpression: EqualityExpression T_NOT_EQ RelationalExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::NotEqual, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

EqualityExpression: EqualityExpression T_EQ_EQ_EQ RelationalExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::StrictEqual, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

EqualityExpression: EqualityExpression T_NOT_EQ_EQ RelationalExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::StrictNotEqual, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

EqualityExpressionNotIn: RelationalExpressionNotIn ;

EqualityExpressionNotIn: EqualityExpressionNotIn T_EQ_EQ RelationalExpressionNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Equal, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

EqualityExpressionNotIn: EqualityExpressionNotIn T_NOT_EQ RelationalExpressionNotIn;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::NotEqual, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

EqualityExpressionNotIn: EqualityExpressionNotIn T_EQ_EQ_EQ RelationalExpressionNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::StrictEqual, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

EqualityExpressionNotIn: EqualityExpressionNotIn T_NOT_EQ_EQ RelationalExpressionNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::StrictNotEqual, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

BitwiseANDExpression: EqualityExpression ;

BitwiseANDExpression: BitwiseANDExpression T_AND EqualityExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::BitAnd, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

BitwiseANDExpressionNotIn: EqualityExpressionNotIn ;

BitwiseANDExpressionNotIn: BitwiseANDExpressionNotIn T_AND EqualityExpressionNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::BitAnd, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

BitwiseXORExpression: BitwiseANDExpression ;

BitwiseXORExpression: BitwiseXORExpression T_XOR BitwiseANDExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::BitXor, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

BitwiseXORExpressionNotIn: BitwiseANDExpressionNotIn ;

BitwiseXORExpressionNotIn: BitwiseXORExpressionNotIn T_XOR BitwiseANDExpressionNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::BitXor, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

BitwiseORExpression: BitwiseXORExpression ;

BitwiseORExpression: BitwiseORExpression T_OR BitwiseXORExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::BitOr, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

BitwiseORExpressionNotIn: BitwiseXORExpressionNotIn ;

BitwiseORExpressionNotIn: BitwiseORExpressionNotIn T_OR BitwiseXORExpressionNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::BitOr, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

LogicalANDExpression: BitwiseORExpression ;

LogicalANDExpression: LogicalANDExpression T_AND_AND BitwiseORExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::And, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

LogicalANDExpressionNotIn: BitwiseORExpressionNotIn ;

LogicalANDExpressionNotIn: LogicalANDExpressionNotIn T_AND_AND BitwiseORExpressionNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::And, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

LogicalORExpression: LogicalANDExpression ;

LogicalORExpression: LogicalORExpression T_OR_OR LogicalANDExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Or, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

LogicalORExpressionNotIn: LogicalANDExpressionNotIn ;

LogicalORExpressionNotIn: LogicalORExpressionNotIn T_OR_OR LogicalANDExpressionNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, QSOperator::Or, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

ConditionalExpression: LogicalORExpression ;

ConditionalExpression: LogicalORExpression T_QUESTION AssignmentExpression T_COLON AssignmentExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ConditionalExpression> (driver->nodePool(), sym(1).Expression, sym(3).Expression, sym(5).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

ConditionalExpressionNotIn: LogicalORExpressionNotIn ;

ConditionalExpressionNotIn: LogicalORExpressionNotIn T_QUESTION AssignmentExpressionNotIn T_COLON AssignmentExpressionNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ConditionalExpression> (driver->nodePool(), sym(1).Expression, sym(3).Expression, sym(5).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

AssignmentExpression: ConditionalExpression ;

AssignmentExpression: LeftHandSideExpression AssignmentOperator AssignmentExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, sym(2).ival, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

AssignmentExpressionNotIn: ConditionalExpressionNotIn ;

AssignmentExpressionNotIn: LeftHandSideExpression AssignmentOperator AssignmentExpressionNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BinaryExpression> (driver->nodePool(), sym(1).Expression, sym(2).ival, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

AssignmentOperator: T_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::Assign;
} break;
./

AssignmentOperator: T_STAR_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceMul;
} break;
./

AssignmentOperator: T_DIVIDE_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceDiv;
} break;
./

AssignmentOperator: T_REMAINDER_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceMod;
} break;
./

AssignmentOperator: T_PLUS_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceAdd;
} break;
./

AssignmentOperator: T_MINUS_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceSub;
} break;
./

AssignmentOperator: T_LT_LT_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceLeftShift;
} break;
./

AssignmentOperator: T_GT_GT_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceRightShift;
} break;
./

AssignmentOperator: T_GT_GT_GT_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceURightShift;
} break;
./

AssignmentOperator: T_AND_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceAnd;
} break;
./

AssignmentOperator: T_XOR_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceXor;
} break;
./

AssignmentOperator: T_OR_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceOr;
} break;
./

Expression: AssignmentExpression ;

Expression: Expression T_COMMA AssignmentExpression ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::Expression> (driver->nodePool(), sym(1).Expression, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

ExpressionOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

ExpressionOpt: Expression ;

ExpressionNotIn: AssignmentExpressionNotIn ;

ExpressionNotIn: ExpressionNotIn T_COMMA AssignmentExpressionNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::Expression> (driver->nodePool(), sym(1).Expression, sym(3).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

ExpressionNotInOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

ExpressionNotInOpt: ExpressionNotIn ;

Statement: Block ;
Statement: VariableStatement ;
Statement: EmptyStatement ;
Statement: ExpressionStatement ;
Statement: IfStatement ;
Statement: IterationStatement ;
Statement: ContinueStatement ;
Statement: BreakStatement ;
Statement: ReturnStatement ;
Statement: WithStatement ;
Statement: LabelledStatement ;
Statement: SwitchStatement ;
Statement: ThrowStatement ;
Statement: TryStatement ;
Statement: DebuggerStatement ;


Block: T_LBRACE StatementListOpt T_RBRACE ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::Block> (driver->nodePool(), sym(2).StatementList);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

StatementList: Statement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::StatementList> (driver->nodePool(), sym(1).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

StatementList: StatementList Statement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::StatementList> (driver->nodePool(), sym(1).StatementList, sym(2).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

StatementListOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

StatementListOpt: StatementList ;
/.
case $rule_number: {
  sym(1).Node = sym(1).StatementList->finish ();
} break;
./

VariableStatement: VariableDeclarationKind VariableDeclarationList T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
VariableStatement: VariableDeclarationKind VariableDeclarationList T_SEMICOLON ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::VariableStatement> (driver->nodePool(), sym(2).VariableDeclarationList->finish (/*readOnly=*/sym(1).ival == T_CONST));
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

VariableDeclarationKind: T_CONST ;
/.
case $rule_number: {
  sym(1).ival = T_CONST;
} break;
./

VariableDeclarationKind: T_VAR ;
/.
case $rule_number: {
  sym(1).ival = T_VAR;
} break;
./

VariableDeclarationList: VariableDeclaration ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::VariableDeclarationList> (driver->nodePool(), sym(1).VariableDeclaration);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

VariableDeclarationList: VariableDeclarationList T_COMMA VariableDeclaration ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::VariableDeclarationList> (driver->nodePool(), sym(1).VariableDeclarationList, sym(3).VariableDeclaration);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

VariableDeclarationListNotIn: VariableDeclarationNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::VariableDeclarationList> (driver->nodePool(), sym(1).VariableDeclaration);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

VariableDeclarationListNotIn: VariableDeclarationListNotIn T_COMMA VariableDeclarationNotIn ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::VariableDeclarationList> (driver->nodePool(), sym(1).VariableDeclarationList, sym(3).VariableDeclaration);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

VariableDeclaration: T_IDENTIFIER InitialiserOpt ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::VariableDeclaration> (driver->nodePool(), sym(1).sval, sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

VariableDeclarationNotIn: T_IDENTIFIER InitialiserNotInOpt ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::VariableDeclaration> (driver->nodePool(), sym(1).sval, sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

Initialiser: T_EQ AssignmentExpression ;
/.
case $rule_number: {
  sym(1) = sym(2);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

InitialiserOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

InitialiserOpt: Initialiser ;

InitialiserNotIn: T_EQ AssignmentExpressionNotIn ;
/.
case $rule_number: {
  sym(1) = sym(2);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

InitialiserNotInOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

InitialiserNotInOpt: InitialiserNotIn ;

EmptyStatement: T_SEMICOLON ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::EmptyStatement> (driver->nodePool());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

ExpressionStatement: Expression T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
ExpressionStatement: Expression T_SEMICOLON ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ExpressionStatement> (driver->nodePool(), sym(1).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

IfStatement: T_IF T_LPAREN Expression T_RPAREN Statement T_ELSE Statement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::IfStatement> (driver->nodePool(), sym(3).Expression, sym(5).Statement, sym(7).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(7));
} break;
./

IfStatement: T_IF T_LPAREN Expression T_RPAREN Statement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::IfStatement> (driver->nodePool(), sym(3).Expression, sym(5).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(5));
} break;
./


IterationStatement: T_DO Statement T_WHILE T_LPAREN Expression T_RPAREN T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
IterationStatement: T_DO Statement T_WHILE T_LPAREN Expression T_RPAREN T_SEMICOLON ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::DoWhileStatement> (driver->nodePool(), sym(2).Statement, sym(5).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(7));
} break;
./

IterationStatement: T_WHILE T_LPAREN Expression T_RPAREN Statement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::WhileStatement> (driver->nodePool(), sym(3).Expression, sym(5).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(5));
} break;
./

IterationStatement: T_FOR T_LPAREN ExpressionNotInOpt T_SEMICOLON ExpressionOpt T_SEMICOLON ExpressionOpt T_RPAREN Statement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ForStatement> (driver->nodePool(), sym(3).Expression, sym(5).Expression, sym(7).Expression, sym(9).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(9));
} break;
./

IterationStatement: T_FOR T_LPAREN T_VAR VariableDeclarationListNotIn T_SEMICOLON ExpressionOpt T_SEMICOLON ExpressionOpt T_RPAREN Statement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::LocalForStatement> (driver->nodePool(), sym(4).VariableDeclarationList->finish (/*readOnly=*/false), sym(6).Expression, sym(8).Expression, sym(10).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(10));
} break;
./

IterationStatement: T_FOR T_LPAREN LeftHandSideExpression T_IN Expression T_RPAREN Statement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ForEachStatement> (driver->nodePool(), sym(3).Expression, sym(5).Expression, sym(7).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(7));
} break;
./

IterationStatement: T_FOR T_LPAREN T_VAR VariableDeclarationNotIn T_IN Expression T_RPAREN Statement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::LocalForEachStatement> (driver->nodePool(), sym(4).VariableDeclaration, sym(6).Expression, sym(8).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(8));
} break;
./

ContinueStatement: T_CONTINUE T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
ContinueStatement: T_CONTINUE T_SEMICOLON ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ContinueStatement> (driver->nodePool());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

ContinueStatement: T_CONTINUE T_IDENTIFIER T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
ContinueStatement: T_CONTINUE T_IDENTIFIER T_SEMICOLON ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ContinueStatement> (driver->nodePool(), sym(2).sval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

BreakStatement: T_BREAK T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
BreakStatement: T_BREAK T_SEMICOLON ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BreakStatement> (driver->nodePool());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

BreakStatement: T_BREAK T_IDENTIFIER T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
BreakStatement: T_BREAK T_IDENTIFIER T_SEMICOLON ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::BreakStatement> (driver->nodePool(), sym(2).sval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

ReturnStatement: T_RETURN ExpressionOpt T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
ReturnStatement: T_RETURN ExpressionOpt T_SEMICOLON ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ReturnStatement> (driver->nodePool(), sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

WithStatement: T_WITH T_LPAREN Expression T_RPAREN Statement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::WithStatement> (driver->nodePool(), sym(3).Expression, sym(5).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(5));
} break;
./

SwitchStatement: T_SWITCH T_LPAREN Expression T_RPAREN CaseBlock ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::SwitchStatement> (driver->nodePool(), sym(3).Expression, sym(5).CaseBlock);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(5));
} break;
./

CaseBlock: T_LBRACE CaseClausesOpt T_RBRACE ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::CaseBlock> (driver->nodePool(), sym(2).CaseClauses);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

CaseBlock: T_LBRACE CaseClausesOpt DefaultClause CaseClausesOpt T_RBRACE ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::CaseBlock> (driver->nodePool(), sym(2).CaseClauses, sym(3).DefaultClause, sym(4).CaseClauses);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(5));
} break;
./

CaseClauses: CaseClause ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::CaseClauses> (driver->nodePool(), sym(1).CaseClause);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

CaseClauses: CaseClauses CaseClause ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::CaseClauses> (driver->nodePool(), sym(1).CaseClauses, sym(2).CaseClause);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

CaseClausesOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

CaseClausesOpt: CaseClauses ;
/.
case $rule_number: {
  sym(1).Node = sym(1).CaseClauses->finish ();
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

CaseClause: T_CASE Expression T_COLON StatementListOpt ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::CaseClause> (driver->nodePool(), sym(2).Expression, sym(4).StatementList);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(4));
} break;
./

DefaultClause: T_DEFAULT T_COLON StatementListOpt ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::DefaultClause> (driver->nodePool(), sym(3).StatementList);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

LabelledStatement: T_IDENTIFIER T_COLON Statement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::LabelledStatement> (driver->nodePool(), sym(1).sval, sym(3).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

ThrowStatement: T_THROW Expression T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
ThrowStatement: T_THROW Expression T_SEMICOLON ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::ThrowStatement> (driver->nodePool(), sym(2).Expression);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

TryStatement: T_TRY Block Catch ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::TryStatement> (driver->nodePool(), sym(2).Statement, sym(3).Catch);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

TryStatement: T_TRY Block Finally ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::TryStatement> (driver->nodePool(), sym(2).Statement, sym(3).Finally);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

TryStatement: T_TRY Block Catch Finally ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::TryStatement> (driver->nodePool(), sym(2).Statement, sym(3).Catch, sym(4).Finally);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(4));
} break;
./

Catch: T_CATCH T_LPAREN T_IDENTIFIER T_RPAREN Block ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::Catch> (driver->nodePool(), sym(3).sval, sym(5).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(5));
} break;
./

Finally: T_FINALLY Block ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::Finally> (driver->nodePool(), sym(2).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

DebuggerStatement: T_DEBUGGER T_AUTOMATIC_SEMICOLON ; -- automatic semicolon
DebuggerStatement: T_DEBUGGER T_SEMICOLON ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::DebuggerStatement> (driver->nodePool());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

FunctionDeclaration: T_FUNCTION T_IDENTIFIER T_LPAREN FormalParameterListOpt T_RPAREN T_LBRACE FunctionBodyOpt T_RBRACE ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::FunctionDeclaration> (driver->nodePool(), sym(2).sval, sym(4).FormalParameterList, sym(7).FunctionBody);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(8));
} break;
./

FunctionExpression: T_FUNCTION IdentifierOpt T_LPAREN FormalParameterListOpt T_RPAREN T_LBRACE FunctionBodyOpt T_RBRACE ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::FunctionExpression> (driver->nodePool(), sym(2).sval, sym(4).FormalParameterList, sym(7).FunctionBody);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(8));
} break;
./

FormalParameterList: T_IDENTIFIER ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::FormalParameterList> (driver->nodePool(), sym(1).sval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

FormalParameterList: FormalParameterList T_COMMA T_IDENTIFIER ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::FormalParameterList> (driver->nodePool(), sym(1).FormalParameterList, sym(3).sval);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(3));
} break;
./

FormalParameterListOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

FormalParameterListOpt: FormalParameterList ;
/.
case $rule_number: {
  sym(1).Node = sym(1).FormalParameterList->finish ();
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

FunctionBodyOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

FunctionBodyOpt: FunctionBody ;

FunctionBody: SourceElements ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::FunctionBody> (driver->nodePool(), sym(1).SourceElements->finish ());
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

Program: SourceElements ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::Program> (driver->nodePool(), sym(1).SourceElements->finish ());
  driver->changeAbstractSyntaxTree(sym(1).Node);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

SourceElements: SourceElement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::SourceElements> (driver->nodePool(), sym(1).SourceElement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

SourceElements: SourceElements SourceElement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::SourceElements> (driver->nodePool(), sym(1).SourceElements, sym(2).SourceElement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(2));
} break;
./

SourceElement: Statement ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::StatementSourceElement> (driver->nodePool(), sym(1).Statement);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

SourceElement: FunctionDeclaration ;
/.
case $rule_number: {
  sym(1).Node = JavaScript::makeAstNode<JavaScript::AST::FunctionSourceElement> (driver->nodePool(), sym(1).FunctionDeclaration);
  J_SCRIPT_UPDATE_POSITION(sym(1).Node, loc(1), loc(1));
} break;
./

IdentifierOpt: ;
/.
case $rule_number: {
  sym(1).sval = 0;
} break;
./

IdentifierOpt: T_IDENTIFIER ;

PropertyNameAndValueListOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

PropertyNameAndValueListOpt: PropertyNameAndValueList ;

/.
            } // switch
            action = nt_action(state_stack[tos], lhs[r] - TERMINAL_COUNT);
        } // if
    } while (action != 0);

    if (first_token == last_token) {
        const int errorState = state_stack[tos];

        // automatic insertion of `;'
        if (t_action(errorState, T_AUTOMATIC_SEMICOLON) && automatic(driver, yytoken)) {
            SavedToken &tk = token_buffer[0];
            tk.token = yytoken;
            tk.dval = yylval;
            tk.loc = yylloc;

            const QString msg = QString::fromUtf8("Missing `;'");

            diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Warning,
                yyprevlloc.startLine, yyprevlloc.startColumn, msg));

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
        token_buffer[0].loc = yylloc;

        token_buffer[1].token = yytoken = lexer->lex();
        token_buffer[1].dval  = yylval  = lexer->dval();
        token_buffer[1].loc   = yylloc  = location(lexer);

        if (token_buffer[0].token != -1 && t_action(errorState, yytoken)) {
            QString msg = QString::fromUtf8("Removed token");
            if (const char *tokenSpell = spell[token_buffer[0].token]) {
                msg += QLatin1String(": `");
                msg += QLatin1String(tokenSpell);
                msg += QLatin1Char('\'');
            }

            diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error,
                token_buffer[0].loc.startLine, token_buffer[0].loc.startColumn, msg));

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
                QString msg = QString::fromUtf8("Inserted token");
                if (const char *tokenSpell = spell[*tk]) {
                    msg += QLatin1String(": `");
                    msg += QLatin1String(tokenSpell);
                    msg += QLatin1Char('\'');
                }

                diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error,
                    token_buffer[0].loc.startLine, token_buffer[0].loc.startColumn, msg));

                yytoken = *tk;
                yylval = 0;
                yylloc = token_buffer[0].loc;

                first_token = &token_buffer[0];
                last_token = &token_buffer[2];

                action = errorState;
                goto _Lcheck_token;
            }
        }

        for (int tk = 1; tk < TERMINAL_COUNT; ++tk) {
            int a = t_action(errorState, tk);
            if (a > 0 && t_action(a, yytoken)) {
                QString msg = QString::fromUtf8("Inserted token");
                if (const char *tokenSpell = spell[tk]) {
                    msg += QLatin1String(": `");
                    msg += QLatin1String(tokenSpell);
                    msg += QLatin1Char('\'');
                }

                diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error,
                    token_buffer[0].loc.startLine, token_buffer[0].loc.startColumn, msg));

                yytoken = tk;
                yylval = 0;
                yylloc = token_buffer[0].loc;

                action = errorState;
                goto _Lcheck_token;
            }
        }

        QString msg = QString::fromUtf8("Unexpected token");
        if (token_buffer[0].token >= 0 && token_buffer[0].token < TERMINAL_COUNT) {
            if (const char *tokenSpell = spell[token_buffer[0].token]) {
                msg += QLatin1String(": `");
                msg += QLatin1String(tokenSpell);
                msg += QLatin1Char('\'');
            }
        } // ### FIXME: at this point, we should give more information than just "Unexpected token". However, we need more error recovery support from qlalr.

        diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error,
            token_buffer[0].loc.startLine, token_buffer[0].loc.startColumn, msg));
    }

    return false;
}

QT_END_NAMESPACE


./
/:
QT_END_NAMESPACE



#endif // JAVASCRIPTPARSER_P_H
:/
