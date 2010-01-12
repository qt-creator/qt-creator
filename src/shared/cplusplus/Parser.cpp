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
// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "Parser.h"
#include "Token.h"
#include "Lexer.h"
#include "Control.h"
#include "AST.h"
#include "Literals.h"
#include "ObjectiveCTypeQualifiers.h"
#include <cstdio> // for putchar

#define CPLUSPLUS_NO_DEBUG_RULE

using namespace CPlusPlus;

namespace {

class DebugRule {
    const char *name;
    static int depth;

public:
    DebugRule(const char *name)
        : name(name)
    {
        for (int i = 0; i < depth; ++i)
            putchar(' ');

        ++depth;
        printf("%s\n", name);
    }

    ~DebugRule()
    { --depth; }
};

int DebugRule::depth = 0;

} // end of anonymous namespace

#ifndef CPLUSPLUS_NO_DEBUG_RULE
#  define DEBUG_THIS_RULE() DebugRule __debug_rule__(__func__)
#else
#  define DEBUG_THIS_RULE() do {} while (0)
#endif

class Parser::Rewind
{
    Parser *_parser;
    MemoryPool::State _state;

public:
    inline Rewind(Parser *parser)
        : _parser(parser) {}

    inline void operator()(unsigned tokenIndex)
    { rewind(tokenIndex); }

    inline void mark()
    { _state = _parser->_pool->state(); }

    inline void rewind(unsigned tokenIndex)
    {
        _parser->rewind(tokenIndex);

        if (_state.isValid())
            _parser->_pool->rewind(_state);
    }
};

Parser::Parser(TranslationUnit *unit)
    : _translationUnit(unit),
      _control(_translationUnit->control()),
      _pool(_translationUnit->memoryPool()),
      _tokenIndex(1),
      _templateArguments(0),
      _qtMocRunEnabled(false),
      _objCEnabled(false),
      _inFunctionBody(false),
      _inObjCImplementationContext(false)
{ }

Parser::~Parser()
{ }

bool Parser::qtMocRunEnabled() const
{ return _qtMocRunEnabled; }

void Parser::setQtMocRunEnabled(bool onoff)
{ _qtMocRunEnabled = onoff; }

bool Parser::objCEnabled() const
{ return _objCEnabled; }

void Parser::setObjCEnabled(bool onoff)
{ _objCEnabled = onoff; }

bool Parser::switchTemplateArguments(bool templateArguments)
{
    bool previousTemplateArguments = _templateArguments;
    _templateArguments = templateArguments;
    return previousTemplateArguments;
}

bool Parser::blockErrors(bool block)
{ return _translationUnit->blockErrors(block); }

bool Parser::skipUntil(int token)
{
    while (int tk = LA()) {
        if (tk == token)
            return true;

        consumeToken();
    }

    return false;
}

void Parser::skipUntilDeclaration()
{
    for (; ; consumeToken()) {
        switch (LA()) {
        case T_EOF_SYMBOL:

        // names
        case T_IDENTIFIER:
        case T_COLON_COLON:
        case T_TILDE:
        case T_OPERATOR:

        // empty declaration
        case T_SEMICOLON:

        // member specification
        case T_USING:
        case T_TEMPLATE:
        case T_PUBLIC:
        case T_PROTECTED:
        case T_PRIVATE:
        case T_Q_SIGNALS:
        case T_Q_SLOTS:

        // declarations
        case T_ENUM:
        case T_NAMESPACE:
        case T_ASM:
        case T_EXPORT:
        case T_AT_CLASS:
        case T_AT_INTERFACE:
        case T_AT_PROTOCOL:
        case T_AT_IMPLEMENTATION:
        case T_AT_END:
            return;

        default:
            if (lookAtBuiltinTypeSpecifier() || lookAtClassKey() ||
                lookAtFunctionSpecifier() || lookAtStorageClassSpecifier())
                return;
        } // switch
    }    
}

bool Parser::skipUntilStatement()
{
    while (int tk = LA()) {
        switch (tk) {
            case T_SEMICOLON:
            case T_LBRACE:
            case T_RBRACE:
            case T_CONST:
            case T_VOLATILE:
            case T_IDENTIFIER:
            case T_CASE:
            case T_DEFAULT:
            case T_IF:
            case T_SWITCH:
            case T_WHILE:
            case T_DO:
            case T_FOR:
            case T_BREAK:
            case T_CONTINUE:
            case T_RETURN:
            case T_GOTO:
            case T_TRY:
            case T_CATCH:
            case T_THROW:
            case T_CHAR:
            case T_WCHAR_T:
            case T_BOOL:
            case T_SHORT:
            case T_INT:
            case T_LONG:
            case T_SIGNED:
            case T_UNSIGNED:
            case T_FLOAT:
            case T_DOUBLE:
            case T_VOID:
            case T_CLASS:
            case T_STRUCT:
            case T_UNION:
            case T_ENUM:
            case T_COLON_COLON:
            case T_TEMPLATE:
            case T_USING:
                return true;

            case T_AT_SYNCHRONIZED:
                if (objCEnabled())
                    return true;

            default:
                consumeToken();
        }
    }

    return false;
}

bool Parser::skip(int l, int r)
{
    int count = 0;

    while (int tk = LA()) {
        if (tk == l)
            ++count;
        else if (tk == r)
            --count;
        else if (l != T_LBRACE && (tk == T_LBRACE ||
                                   tk == T_RBRACE ||
                                   tk == T_SEMICOLON))
            return false;

        if (count == 0)
            return true;

        consumeToken();
    }

    return false;
}

void Parser::match(int kind, unsigned *token)
{
    if (LA() == kind)
        *token = consumeToken();
    else {
        *token = 0;
        _translationUnit->error(_tokenIndex, "expected token `%s' got `%s'",
                                Token::name(kind), tok().spell());
    }
}

bool Parser::parseClassOrNamespaceName(NameAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_IDENTIFIER) {
        unsigned identifier_token = cursor();

        if (LA(2) == T_LESS && parseTemplateId(node) && LA() == T_COLON_COLON)
            return true;

        rewind(identifier_token);

        if (LA(2) == T_COLON_COLON) {
            SimpleNameAST *ast = new (_pool) SimpleNameAST;
            ast->identifier_token = consumeToken();
            node = ast;
            return true;
        }
    } else if (LA() == T_TEMPLATE) {
        unsigned template_token = consumeToken();
        if (parseTemplateId(node))
            return true;
        rewind(template_token);
    }
    return false;
}

bool Parser::parseTemplateId(NameAST *&node)
{
    DEBUG_THIS_RULE();

    const unsigned start = cursor();

    if (LA() == T_IDENTIFIER && LA(2) == T_LESS) {
        TemplateIdAST *ast = new (_pool) TemplateIdAST;
        ast->identifier_token = consumeToken();
        ast->less_token = consumeToken();
        if (LA() == T_GREATER || parseTemplateArgumentList(
                ast->template_argument_list)) {
            if (LA() == T_GREATER) {
                ast->greater_token = consumeToken();
                node = ast;
                return true;
            }
        }
    }

    rewind(start);

    return false;
}

bool Parser::parseNestedNameSpecifier(NestedNameSpecifierListAST *&node,
                                      bool /*acceptTemplateId*/)
{
    DEBUG_THIS_RULE();
    NestedNameSpecifierListAST **nested_name_specifier = &node;
    NameAST *class_or_namespace_name = 0;
    if (parseClassOrNamespaceName(class_or_namespace_name) && LA() == T_COLON_COLON) {
        unsigned scope_token = consumeToken();

        NestedNameSpecifierAST *name = new (_pool) NestedNameSpecifierAST;
        name->class_or_namespace_name = class_or_namespace_name;
        name->scope_token = scope_token;

        *nested_name_specifier = new (_pool) NestedNameSpecifierListAST(name);
        nested_name_specifier = &(*nested_name_specifier)->next;

        while (parseClassOrNamespaceName(class_or_namespace_name) && LA() == T_COLON_COLON) {
            scope_token = consumeToken();

            name = new (_pool) NestedNameSpecifierAST;
            name->class_or_namespace_name = class_or_namespace_name;
            name->scope_token = scope_token;

            *nested_name_specifier = new (_pool) NestedNameSpecifierListAST(name);
            nested_name_specifier = &(*nested_name_specifier)->next;
        }

        // ### ugly hack
        rewind(scope_token);
        consumeToken();
        return true;
    }

    return false;
}

bool Parser::parseNestedNameSpecifierOpt(NestedNameSpecifierListAST *&name, bool acceptTemplateId)
{
    DEBUG_THIS_RULE();
    unsigned start = cursor();
    if (! parseNestedNameSpecifier(name, acceptTemplateId))
        rewind(start);
    return true;
}

bool Parser::parseName(NameAST *&node, bool acceptTemplateId)
{
    DEBUG_THIS_RULE();
    unsigned global_scope_token = 0;
    if (LA() == T_COLON_COLON)
        global_scope_token = consumeToken();

    NestedNameSpecifierListAST *nested_name_specifier = 0;
    parseNestedNameSpecifierOpt(nested_name_specifier,
                                /*acceptTemplateId=*/ true);

    NameAST *unqualified_name = 0;
    if (parseUnqualifiedName(unqualified_name,
                             /*acceptTemplateId=*/ acceptTemplateId || nested_name_specifier != 0)) {
        if (! global_scope_token && ! nested_name_specifier) {
            node = unqualified_name;
            return true;
        }

        QualifiedNameAST *ast = new (_pool) QualifiedNameAST;
        ast->global_scope_token = global_scope_token;
        ast->nested_name_specifier_list = nested_name_specifier;
        ast->unqualified_name = unqualified_name;
        node = ast;
        return true;
    }

    return false;
}

bool Parser::parseTranslationUnit(TranslationUnitAST *&node)
{
    DEBUG_THIS_RULE();
    TranslationUnitAST *ast = new (_pool) TranslationUnitAST;
    DeclarationListAST **decl = &ast->declaration_list;

    while (LA()) {
        unsigned start_declaration = cursor();

        DeclarationAST *declaration = 0;

        if (parseDeclaration(declaration)) {
            *decl = new (_pool) DeclarationListAST;
            (*decl)->value = declaration;
            decl = &(*decl)->next;
        } else {
            _translationUnit->error(start_declaration, "expected a declaration");
            rewind(start_declaration + 1);
            skipUntilDeclaration();
        }

        _templateArgumentList.clear();
    }

    node = ast;
    return true;
}

bool Parser::parseEmptyDeclaration(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_SEMICOLON) {
        EmptyDeclarationAST *ast = new (_pool) EmptyDeclarationAST;
        ast->semicolon_token = consumeToken();
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseDeclaration(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    switch (LA()) {
    case T_SEMICOLON:
        return parseEmptyDeclaration(node);

    case T_NAMESPACE:
        return parseNamespace(node);

    case T_USING:
        return parseUsing(node);

    case T_ASM:
        return parseAsmDefinition(node);

    case T_TEMPLATE:
    case T_EXPORT:
        return parseTemplateDeclaration(node);

    // ObjcC++
    case T_AT_CLASS:
        return parseObjCClassForwardDeclaration(node);

    case T_AT_INTERFACE:
        return parseObjCInterface(node);

    case T_AT_PROTOCOL:
        return parseObjCProtocol(node);

    case T_AT_IMPLEMENTATION:
        return parseObjCImplementation(node);

    case T_AT_END:
        // TODO: should this be done here, or higher-up?
        _translationUnit->error(cursor(), "skip stray token `%s'", tok().spell());
        consumeToken();
        break;

    default: {
        if (_objCEnabled && LA() == T___ATTRIBUTE__) {
            const unsigned start = cursor();
            SpecifierListAST *attributes = 0, **attr = &attributes;
            while (parseAttributeSpecifier(*attr))
                attr = &(*attr)->next;
            if (LA() == T_AT_INTERFACE)
                return parseObjCInterface(node, attributes);
            else if (LA() == T_AT_PROTOCOL)
                return parseObjCProtocol(node, attributes);
            else if (LA() == T_AT_PROPERTY)
                return parseObjCPropertyDeclaration(node, attributes);
            rewind(start);
        }

        if (LA() == T_EXTERN && LA(2) == T_TEMPLATE)
            return parseTemplateDeclaration(node);
        else if (LA() == T_EXTERN && LA(2) == T_STRING_LITERAL)
            return parseLinkageSpecification(node);
        else
            return parseSimpleDeclaration(node);
    }   break; // default

    } // end switch

    return false;
}

bool Parser::parseLinkageSpecification(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_EXTERN && LA(2) == T_STRING_LITERAL) {
        LinkageSpecificationAST *ast = new (_pool) LinkageSpecificationAST;
        ast->extern_token = consumeToken();
        ast->extern_type_token = consumeToken();

        if (LA() == T_LBRACE)
            parseLinkageBody(ast->declaration);
        else
            parseDeclaration(ast->declaration);

        node = ast;
        return true;
    }

    return false;
}

bool Parser::parseLinkageBody(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_LBRACE) {
        LinkageBodyAST *ast = new (_pool) LinkageBodyAST;
        ast->lbrace_token = consumeToken();
        DeclarationListAST **declaration_ptr = &ast->declaration_list;

        while (int tk = LA()) {
            if (tk == T_RBRACE)
                break;

            unsigned start_declaration = cursor();
            DeclarationAST *declaration = 0;
            if (parseDeclaration(declaration)) {
                *declaration_ptr = new (_pool) DeclarationListAST;
                (*declaration_ptr)->value = declaration;
                declaration_ptr = &(*declaration_ptr)->next;
            } else {
                _translationUnit->error(start_declaration, "expected a declaration");
                rewind(start_declaration + 1);
                skipUntilDeclaration();
            }

            _templateArgumentList.clear();
        }
        match(T_RBRACE, &ast->rbrace_token);
        node = ast;
        return true;
    }
    return false;
}

// ### rename parseNamespaceAliarOrDeclaration?
bool Parser::parseNamespace(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_NAMESPACE)
        return false;

    unsigned namespace_token = consumeToken();

    if (LA() == T_IDENTIFIER && LA(2) == T_EQUAL) {
        NamespaceAliasDefinitionAST *ast =
                new (_pool) NamespaceAliasDefinitionAST;
        ast->namespace_token = namespace_token;
        ast->namespace_name_token = consumeToken();
        ast->equal_token = consumeToken();
        parseName(ast->name);
        match(T_SEMICOLON, &ast->semicolon_token);
        node = ast;
        return true;
    }

    NamespaceAST *ast = new (_pool) NamespaceAST;
    ast->namespace_token = namespace_token;
    if (LA() == T_IDENTIFIER)
        ast->identifier_token = consumeToken();
    SpecifierListAST **attr_ptr = &ast->attribute_list;
    while (LA() == T___ATTRIBUTE__) {
        parseAttributeSpecifier(*attr_ptr);
        attr_ptr = &(*attr_ptr)->next;
    }
    if (LA() == T_LBRACE)
        parseLinkageBody(ast->linkage_body);
    node = ast;
    return true;
}

bool Parser::parseUsing(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_USING)
        return false;

    if (LA(2) == T_NAMESPACE)
        return parseUsingDirective(node);

    UsingAST *ast = new (_pool) UsingAST;
    ast->using_token = consumeToken();

    if (LA() == T_TYPENAME)
        ast->typename_token = consumeToken();

    parseName(ast->name);
    match(T_SEMICOLON, &ast->semicolon_token);
    node = ast;
    return true;
}

bool Parser::parseUsingDirective(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_USING && LA(2) == T_NAMESPACE) {
        UsingDirectiveAST *ast = new (_pool) UsingDirectiveAST;
        ast->using_token = consumeToken();
        ast->namespace_token = consumeToken();
        if (! parseName(ast->name))
            _translationUnit->warning(cursor(), "expected `namespace name' before `%s'",
                                      tok().spell());
        match(T_SEMICOLON, &ast->semicolon_token);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseConversionFunctionId(NameAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_OPERATOR)
        return false;
    unsigned operator_token = consumeToken();
    SpecifierListAST *type_specifier = 0;
    if (! parseTypeSpecifier(type_specifier)) {
        return false;
    }
    PtrOperatorListAST *ptr_operators = 0, **ptr_operators_tail = &ptr_operators;
    while (parsePtrOperator(*ptr_operators_tail))
        ptr_operators_tail = &(*ptr_operators_tail)->next;

    ConversionFunctionIdAST *ast = new (_pool) ConversionFunctionIdAST;
    ast->operator_token = operator_token;
    ast->type_specifier_list = type_specifier;
    ast->ptr_operator_list = ptr_operators;
    node = ast;
    return true;
}

bool Parser::parseOperatorFunctionId(NameAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_OPERATOR)
        return false;
    unsigned operator_token = consumeToken();

    OperatorAST *op = 0;
    if (! parseOperator(op))
        return false;

    OperatorFunctionIdAST *ast = new (_pool) OperatorFunctionIdAST;
    ast->operator_token = operator_token;
    ast->op = op;
    node = ast;
    return true;
}

Parser::TemplateArgumentListEntry *Parser::templateArgumentListEntry(unsigned tokenIndex)
{
    std::map<unsigned, TemplateArgumentListEntry>::iterator it =_templateArgumentList.find(tokenIndex);
    if (it != _templateArgumentList.end())
        return &it->second;

    return 0;
}

bool Parser::parseTemplateArgumentList(TemplateArgumentListAST *&node)
{
    if (TemplateArgumentListEntry *entry = templateArgumentListEntry(cursor())) {
        rewind(entry->cursor);
        node = entry->ast;
        return entry->ast != 0;
    }

    unsigned start = cursor();

    DEBUG_THIS_RULE();
    TemplateArgumentListAST **template_argument_ptr = &node;
    ExpressionAST *template_argument = 0;
    if (parseTemplateArgument(template_argument)) {
        *template_argument_ptr = new (_pool) TemplateArgumentListAST;
        (*template_argument_ptr)->value = template_argument;
        template_argument_ptr = &(*template_argument_ptr)->next;
        while (LA() == T_COMMA) {
            consumeToken(); // consume T_COMMA

            if (parseTemplateArgument(template_argument)) {
                *template_argument_ptr = new (_pool) TemplateArgumentListAST;
                (*template_argument_ptr)->value = template_argument;
                template_argument_ptr = &(*template_argument_ptr)->next;
            }
        }

        _templateArgumentList.insert(std::make_pair(cursor(), TemplateArgumentListEntry(start, cursor(), node)));
        return true;
    }

    _templateArgumentList.insert(std::make_pair(cursor(), TemplateArgumentListEntry(start, cursor(), 0)));

    return false;
}

bool Parser::parseAsmDefinition(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_ASM)
        return false;

    AsmDefinitionAST *ast = new (_pool) AsmDefinitionAST;
    ast->asm_token = consumeToken();

    if (LA() == T_VOLATILE)
        ast->volatile_token = consumeToken();

    match(T_LPAREN, &ast->lparen_token);
    unsigned string_literal_token = 0;
    match(T_STRING_LITERAL, &string_literal_token);
    while (LA() == T_STRING_LITERAL) {
        consumeToken();
    }
    if (LA() == T_COLON) {
        consumeToken(); // skip T_COLON
        parseAsmOperandList();
        if (LA() == T_COLON) {
            consumeToken();
            parseAsmOperandList();
            if (LA() == T_COLON) {
                consumeToken();
                parseAsmClobberList();
            }
        } else if (LA() == T_COLON_COLON) {
            consumeToken();
            parseAsmClobberList();
        }
    } else if (LA() == T_COLON_COLON) {
        consumeToken();
        parseAsmClobberList();
    }
    match(T_RPAREN, &ast->rparen_token);
    match(T_SEMICOLON, &ast->semicolon_token);
    node = ast;
    return true;
}

bool Parser::parseAsmOperandList()
{
    DEBUG_THIS_RULE();
    if (LA() != T_STRING_LITERAL)
        return true;

    if (parseAsmOperand()) {
        while (LA() == T_COMMA) {
            consumeToken();
            parseAsmOperand();
        }
        return true;
    }

    return false;
}

bool Parser::parseAsmOperand()
{
    DEBUG_THIS_RULE();
    unsigned string_literal_token = 0;
    match(T_STRING_LITERAL, &string_literal_token);

    if (LA() == T_LBRACKET) {
        /*unsigned lbracket_token = */ consumeToken();
        match(T_STRING_LITERAL, &string_literal_token);
        unsigned rbracket_token = 0;
        match(T_RBRACKET, &rbracket_token);
    }

    unsigned lparen_token = 0, rparen_token = 0;
    match(T_LPAREN, &lparen_token);
    ExpressionAST *expression = 0;
    parseExpression(expression);
    match(T_RPAREN, &rparen_token);
    return true;
}

bool Parser::parseAsmClobberList()
{
    DEBUG_THIS_RULE();
    if (LA() != T_STRING_LITERAL)
        return false;

    unsigned string_literal_token = consumeToken();

    while (LA() == T_COMMA) {
        consumeToken();
        match(T_STRING_LITERAL, &string_literal_token);
    }

    return true;
}

bool Parser::parseTemplateDeclaration(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (! (LA(1) == T_TEMPLATE || ((LA(1) == T_EXPORT || LA(1) == T_EXTERN)
            && LA(2) == T_TEMPLATE)))
        return false;

    TemplateDeclarationAST *ast = new (_pool) TemplateDeclarationAST;

    if (LA() == T_EXPORT || LA() == T_EXPORT)
        ast->export_token = consumeToken();

    ast->template_token = consumeToken();

    if (LA() == T_LESS) {
        ast->less_token = consumeToken();
        if (LA() == T_GREATER || parseTemplateParameterList(ast->template_parameter_list))
            match(T_GREATER, &ast->greater_token);
    }

    parseDeclaration(ast->declaration);
    node = ast;
    return true;
}

bool Parser::parseOperator(OperatorAST *&node) // ### FIXME
{
    DEBUG_THIS_RULE();
    OperatorAST *ast = new (_pool) OperatorAST;

    switch (LA()) {
    case T_NEW:
    case T_DELETE: {
        ast->op_token = consumeToken();
        if (LA() == T_LBRACKET) {
            ast->open_token = consumeToken();
            match(T_RBRACKET, &ast->close_token);
        }
    } break;

    case T_PLUS:
    case T_MINUS:
    case T_STAR:
    case T_SLASH:
    case T_PERCENT:
    case T_CARET:
    case T_AMPER:
    case T_PIPE:
    case T_TILDE:
    case T_EXCLAIM:
    case T_LESS:
    case T_GREATER:
    case T_COMMA:
    case T_AMPER_EQUAL:
    case T_CARET_EQUAL:
    case T_SLASH_EQUAL:
    case T_EQUAL:
    case T_EQUAL_EQUAL:
    case T_EXCLAIM_EQUAL:
    case T_GREATER_EQUAL:
    case T_GREATER_GREATER_EQUAL:
    case T_LESS_EQUAL:
    case T_LESS_LESS_EQUAL:
    case T_MINUS_EQUAL:
    case T_PERCENT_EQUAL:
    case T_PIPE_EQUAL:
    case T_PLUS_EQUAL:
    case T_STAR_EQUAL:
    case T_TILDE_EQUAL:
    case T_LESS_LESS:
    case T_GREATER_GREATER:
    case T_AMPER_AMPER:
    case T_PIPE_PIPE:
    case T_PLUS_PLUS:
    case T_MINUS_MINUS:
    case T_ARROW_STAR:
    case T_DOT_STAR:
    case T_ARROW:
        ast->op_token = consumeToken();
        break;

    default:
        if (LA() == T_LPAREN && LA(2) == T_RPAREN) {
            ast->op_token = ast->open_token = consumeToken();
            ast->close_token = consumeToken();
        } else if (LA() == T_LBRACKET && LA(2) == T_RBRACKET) {
            ast->op_token = ast->open_token = consumeToken();
            ast->close_token = consumeToken();
        } else {
            return false;
        }
    }

    node = ast;
    return true;
}

bool Parser::parseCvQualifiers(SpecifierListAST *&node)
{
    DEBUG_THIS_RULE();

    unsigned start = cursor();

    SpecifierListAST **ast = &node;
    while (*ast)
        ast = &(*ast)->next;

    while (int tk = LA()) {
        if (tk == T_CONST || tk == T_VOLATILE) {
            SimpleSpecifierAST *spec = new (_pool) SimpleSpecifierAST;
            spec->specifier_token = consumeToken();
            *ast = new (_pool) SpecifierListAST(spec);
            ast = &(*ast)->next;
        } else if(LA() == T___ATTRIBUTE__) {
            parseAttributeSpecifier(*ast);
            ast = &(*ast)->next;
        } else {
            break;
        }
    }

    return start != cursor();
}

bool Parser::parsePtrOperator(PtrOperatorListAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_AMPER) {
        ReferenceAST *ast = new (_pool) ReferenceAST;
        ast->amp_token = consumeToken();
        node = new (_pool) PtrOperatorListAST(ast);
        return true;
    } else if (LA() == T_STAR) {
        PointerAST *ast = new (_pool) PointerAST;
        ast->star_token = consumeToken();
        parseCvQualifiers(ast->cv_qualifier_list);
        node = new (_pool) PtrOperatorListAST(ast);
        return true;
    } else if (LA() == T_COLON_COLON || LA() == T_IDENTIFIER) {
        unsigned scope_or_identifier_token = cursor();

        unsigned global_scope_token = 0;
        if (LA() == T_COLON_COLON)
            global_scope_token = consumeToken();

        NestedNameSpecifierListAST *nested_name_specifiers = 0;
        bool has_nested_name_specifier = parseNestedNameSpecifier(nested_name_specifiers, true);
        if (has_nested_name_specifier && LA() == T_STAR) {
            PointerToMemberAST *ast = new (_pool) PointerToMemberAST;
            ast->global_scope_token = global_scope_token;
            ast->nested_name_specifier_list = nested_name_specifiers;
            ast->star_token = consumeToken();
            parseCvQualifiers(ast->cv_qualifier_list);
            node = new (_pool) PtrOperatorListAST(ast);
            return true;
        }
        rewind(scope_or_identifier_token);
    }
    return false;
}

bool Parser::parseTemplateArgument(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    unsigned start = cursor();
    if (parseTypeId(node) && (LA() == T_COMMA || LA() == T_GREATER))
        return true;

    rewind(start);
    bool previousTemplateArguments = switchTemplateArguments(true);
    bool parsed = parseLogicalOrExpression(node);
    (void) switchTemplateArguments(previousTemplateArguments);
    return parsed;
}

bool Parser::parseDeclSpecifierSeq(SpecifierListAST *&decl_specifier_seq,
                                   bool onlyTypeSpecifiers,
                                   bool simplified)
{
    DEBUG_THIS_RULE();
    bool has_type_specifier = false;
    NameAST *named_type_specifier = 0;
    SpecifierListAST **decl_specifier_seq_ptr = &decl_specifier_seq;
    for (;;) {
        if (lookAtCVQualifier()) {
            SimpleSpecifierAST *spec = new (_pool) SimpleSpecifierAST;
            spec->specifier_token = consumeToken();
            *decl_specifier_seq_ptr = new (_pool) SpecifierListAST(spec);
            decl_specifier_seq_ptr = &(*decl_specifier_seq_ptr)->next;
        } else if (! onlyTypeSpecifiers && lookAtStorageClassSpecifier()) {
            SimpleSpecifierAST *spec = new (_pool) SimpleSpecifierAST;
            spec->specifier_token = consumeToken();
            *decl_specifier_seq_ptr = new (_pool) SpecifierListAST(spec);
            decl_specifier_seq_ptr = &(*decl_specifier_seq_ptr)->next;
        } else if (! named_type_specifier && lookAtBuiltinTypeSpecifier()) {
            parseBuiltinTypeSpecifier(*decl_specifier_seq_ptr);
            decl_specifier_seq_ptr = &(*decl_specifier_seq_ptr)->next;
            has_type_specifier = true;
        } else if (! has_type_specifier && (LA() == T_COLON_COLON ||
                                            LA() == T_IDENTIFIER)) {
            if (! parseName(named_type_specifier))
                return false;
            NamedTypeSpecifierAST *spec = new (_pool) NamedTypeSpecifierAST;
            spec->name = named_type_specifier;
            *decl_specifier_seq_ptr = new (_pool) SpecifierListAST(spec);
            decl_specifier_seq_ptr = &(*decl_specifier_seq_ptr)->next;
            has_type_specifier = true;
        } else if (! simplified && ! has_type_specifier && (LA() == T_TYPENAME ||
                                                            LA() == T_ENUM     ||
                                                            lookAtClassKey())) {
            unsigned startOfElaboratedTypeSpecifier = cursor();
            if (! parseElaboratedTypeSpecifier(*decl_specifier_seq_ptr)) {
                _translationUnit->error(startOfElaboratedTypeSpecifier,
                                        "expected an elaborated type specifier");
                break;
            }
            decl_specifier_seq_ptr = &(*decl_specifier_seq_ptr)->next;
            has_type_specifier = true;
        } else
            break;
    }

    return decl_specifier_seq != 0;
}

bool Parser::parseDeclaratorOrAbstractDeclarator(DeclaratorAST *&node)
{
    DEBUG_THIS_RULE();
    unsigned start = cursor();
    bool blocked = blockErrors(true);
    if (parseDeclarator(node)) {
        blockErrors(blocked);
        return true;
    }
    blockErrors(blocked);
    rewind(start);
    return parseAbstractDeclarator(node);
}

bool Parser::parseCoreDeclarator(DeclaratorAST *&node)
{
    DEBUG_THIS_RULE();
    unsigned start = cursor();
    SpecifierListAST *attributes = 0;
    SpecifierListAST **attribute_ptr = &attributes;
    while (LA() == T___ATTRIBUTE__) {
        parseAttributeSpecifier(*attribute_ptr);
        attribute_ptr = &(*attribute_ptr)->next;
    }

    PtrOperatorListAST *ptr_operators = 0, **ptr_operators_tail = &ptr_operators;
    while (parsePtrOperator(*ptr_operators_tail))
        ptr_operators_tail = &(*ptr_operators_tail)->next;

    if (LA() == T_COLON_COLON || LA() == T_IDENTIFIER || LA() == T_TILDE
            || LA() == T_OPERATOR) {
        NameAST *name = 0;
        if (parseName(name)) {
            DeclaratorIdAST *declarator_id = new (_pool) DeclaratorIdAST;
            declarator_id->name = name;
            DeclaratorAST *ast = new (_pool) DeclaratorAST;
            ast->attribute_list = attributes;
            ast->ptr_operator_list = ptr_operators;
            ast->core_declarator = declarator_id;
            node = ast;
            return true;
        }
    } else if (LA() == T_LPAREN) {
        if (attributes)
            _translationUnit->warning(attributes->firstToken(), "unexpected attribtues");

        unsigned lparen_token = consumeToken();
        DeclaratorAST *declarator = 0;
        if (parseDeclarator(declarator) && LA() == T_RPAREN) {
            NestedDeclaratorAST *nested_declarator = new (_pool) NestedDeclaratorAST;
            nested_declarator->lparen_token = lparen_token;
            nested_declarator->declarator = declarator;
            nested_declarator->rparen_token = consumeToken();
            DeclaratorAST *ast = new (_pool) DeclaratorAST;
            ast->ptr_operator_list = ptr_operators;
            ast->core_declarator = nested_declarator;
            node = ast;
            return true;
        }
    }
    rewind(start);
    return false;
}

bool Parser::parseDeclarator(DeclaratorAST *&node, bool stopAtCppInitializer)
{
    DEBUG_THIS_RULE();
    if (! parseCoreDeclarator(node))
        return false;

    PostfixDeclaratorListAST **postfix_ptr = &node->postfix_declarator_list;

    for (;;) {
        unsigned startOfPostDeclarator = cursor();

        if (LA() == T_LPAREN) {
            if (stopAtCppInitializer) {
                unsigned lparen_token = cursor();
                ExpressionAST *initializer = 0;

                bool blocked = blockErrors(true);
                if (parseInitializer(initializer, &node->equals_token)) {
                    if (NestedExpressionAST *expr = initializer->asNestedExpression()) {
                        if (expr->expression && expr->rparen_token && (LA() == T_COMMA || LA() == T_SEMICOLON)) {
                            rewind(lparen_token);

                            // check for ambiguous declarators.

                            consumeToken();
                            ParameterDeclarationClauseAST *parameter_declaration_clause = 0;
                            if (parseParameterDeclarationClause(parameter_declaration_clause) && LA() == T_RPAREN) {
                                unsigned rparen_token = consumeToken();

                                FunctionDeclaratorAST *ast = new (_pool) FunctionDeclaratorAST;
                                ast->lparen_token = lparen_token;
                                ast->parameters = parameter_declaration_clause;
                                ast->as_cpp_initializer = initializer;
                                ast->rparen_token = rparen_token;
                                *postfix_ptr = new (_pool) PostfixDeclaratorListAST(ast);
                                postfix_ptr = &(*postfix_ptr)->next;

                                blockErrors(blocked);
                                return true;
                            }


                            blockErrors(blocked);
                            rewind(lparen_token);
                            return true;
                        }
                    }
                }

                blockErrors(blocked);
                rewind(lparen_token);
            }

            FunctionDeclaratorAST *ast = new (_pool) FunctionDeclaratorAST;
            ast->lparen_token = consumeToken();
            parseParameterDeclarationClause(ast->parameters);
            if (LA() != T_RPAREN) {
                rewind(startOfPostDeclarator);
                break;
            }

            ast->rparen_token = consumeToken();
            parseCvQualifiers(ast->cv_qualifier_list);
            parseExceptionSpecification(ast->exception_specification);
            *postfix_ptr = new (_pool) PostfixDeclaratorListAST(ast);
            postfix_ptr = &(*postfix_ptr)->next;
        } else if (LA() == T_LBRACKET) {
            ArrayDeclaratorAST *ast = new (_pool) ArrayDeclaratorAST;
            ast->lbracket_token = consumeToken();
            if (LA() == T_RBRACKET || parseConstantExpression(ast->expression)) {
                match(T_RBRACKET, &ast->rbracket_token);
            }
            *postfix_ptr = new (_pool) PostfixDeclaratorListAST(ast);
            postfix_ptr = &(*postfix_ptr)->next;
        } else
            break;
    }

    if (LA() == T___ASM__ && LA(2) == T_LPAREN) { // ### store the asm specifier in the AST
        consumeToken(); // skip __asm__
        consumeToken(); // skip T_LPAREN

        if (skipUntil(T_RPAREN))
            consumeToken(); // skip T_RPAREN
    }

    SpecifierListAST **spec_ptr = &node->post_attribute_list;
    while (LA() == T___ATTRIBUTE__) {
        parseAttributeSpecifier(*spec_ptr);
        spec_ptr = &(*spec_ptr)->next;
    }

    return true;
}

bool Parser::parseAbstractCoreDeclarator(DeclaratorAST *&node)
{
    DEBUG_THIS_RULE();

    PtrOperatorListAST *ptr_operators = 0, **ptr_operators_tail = &ptr_operators;
    while (parsePtrOperator(*ptr_operators_tail))
        ptr_operators_tail = &(*ptr_operators_tail)->next;

    unsigned after_ptr_operators = cursor();

    if (LA() == T_LPAREN) {
        unsigned lparen_token = consumeToken();
        DeclaratorAST *declarator = 0;
        if (parseAbstractDeclarator(declarator) && LA() == T_RPAREN) {
            NestedDeclaratorAST *nested_declarator = new (_pool) NestedDeclaratorAST;
            nested_declarator->lparen_token = lparen_token;
            nested_declarator->declarator = declarator;
            nested_declarator->rparen_token = consumeToken();
            DeclaratorAST *ast = new (_pool) DeclaratorAST;
            ast->ptr_operator_list = ptr_operators;
            ast->core_declarator = nested_declarator;
            node = ast;
            return true;
        }
    }

    rewind(after_ptr_operators);
    if (ptr_operators) {
        DeclaratorAST *ast = new (_pool) DeclaratorAST;
        ast->ptr_operator_list = ptr_operators;
        node = ast;
    }

    return true;
}

bool Parser::parseAbstractDeclarator(DeclaratorAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseAbstractCoreDeclarator(node))
        return false;

    PostfixDeclaratorListAST *postfix_declarators = 0,
        **postfix_ptr = &postfix_declarators;

    for (;;) {
        if (LA() == T_LPAREN) {
            FunctionDeclaratorAST *ast = new (_pool) FunctionDeclaratorAST;
            ast->lparen_token = consumeToken();
            if (LA() == T_RPAREN || parseParameterDeclarationClause(ast->parameters)) {
                if (LA() == T_RPAREN)
                    ast->rparen_token = consumeToken();
            }
            parseCvQualifiers(ast->cv_qualifier_list);
            parseExceptionSpecification(ast->exception_specification);
            *postfix_ptr = new (_pool) PostfixDeclaratorListAST(ast);
            postfix_ptr = &(*postfix_ptr)->next;
        } else if (LA() == T_LBRACKET) {
            ArrayDeclaratorAST *ast = new (_pool) ArrayDeclaratorAST;
            ast->lbracket_token = consumeToken();
            if (LA() == T_RBRACKET || parseConstantExpression(ast->expression)) {
                if (LA() == T_RBRACKET)
                    ast->rbracket_token = consumeToken();
            }
            *postfix_ptr = new (_pool) PostfixDeclaratorListAST(ast);
            postfix_ptr = &(*postfix_ptr)->next;
        } else
            break;
    }

    if (postfix_declarators) {
        if (! node)
            node = new (_pool) DeclaratorAST;

        node->postfix_declarator_list = postfix_declarators;
    }

    return true;
}

bool Parser::parseEnumSpecifier(SpecifierListAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_ENUM) {
        unsigned enum_token = consumeToken();
        NameAST *name = 0;
        parseName(name);
        if (LA() == T_LBRACE) {
            EnumSpecifierAST *ast = new (_pool) EnumSpecifierAST;
            ast->enum_token = enum_token;
            ast->name = name;
            ast->lbrace_token = consumeToken();
            unsigned comma_token = 0;
            EnumeratorListAST **enumerator_ptr = &ast->enumerator_list;
            while (int tk = LA()) {
                if (tk == T_RBRACE)
                    break;

                if (LA() != T_IDENTIFIER) {
                    _translationUnit->error(cursor(), "expected identifier before '%s'", tok().spell());
                    skipUntil(T_IDENTIFIER);
                }

                if (parseEnumerator(*enumerator_ptr)) {
                    enumerator_ptr = &(*enumerator_ptr)->next;
                }

                if (LA() != T_RBRACE)
                    match(T_COMMA, &comma_token);
            }
            match(T_RBRACE, &ast->rbrace_token);
            node = new (_pool) SpecifierListAST(ast);
            return true;
        }
    }
    return false;
}

bool Parser::parseTemplateParameterList(DeclarationListAST *&node)
{
    DEBUG_THIS_RULE();
    DeclarationListAST **template_parameter_ptr = &node;
    DeclarationAST *declaration = 0;
    if (parseTemplateParameter(declaration)) {
        *template_parameter_ptr = new (_pool) DeclarationListAST;
        (*template_parameter_ptr)->value = declaration;
        template_parameter_ptr = &(*template_parameter_ptr)->next;

        while (LA() == T_COMMA) {
            consumeToken(); // XXX Store this token somewhere

            declaration = 0;
            if (parseTemplateParameter(declaration)) {
                *template_parameter_ptr = new (_pool) DeclarationListAST;
                (*template_parameter_ptr)->value = declaration;
                template_parameter_ptr = &(*template_parameter_ptr)->next;
            }
        }
        return true;
    }
    return false;
}

bool Parser::parseTemplateParameter(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (parseTypeParameter(node))
        return true;
    bool previousTemplateArguments = switchTemplateArguments(true);
    bool parsed = parseParameterDeclaration(node);
    (void) switchTemplateArguments(previousTemplateArguments);
    return parsed;
}

bool Parser::parseTypenameTypeParameter(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_CLASS || LA() == T_TYPENAME) {
        TypenameTypeParameterAST *ast = new (_pool) TypenameTypeParameterAST;
        ast->classkey_token = consumeToken();
        parseName(ast->name);
        if (LA() == T_EQUAL) {
            ast->equal_token = consumeToken();
            parseTypeId(ast->type_id);
        }
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseTemplateTypeParameter(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_TEMPLATE) {
        TemplateTypeParameterAST *ast = new (_pool) TemplateTypeParameterAST;
        ast->template_token = consumeToken();
        if (LA() == T_LESS)
            ast->less_token = consumeToken();
        parseTemplateParameterList(ast->template_parameter_list);
        if (LA() == T_GREATER)
            ast->greater_token = consumeToken();
        if (LA() == T_CLASS)
            ast->class_token = consumeToken();

        // parse optional name
        parseName(ast->name);

        if (LA() == T_EQUAL) {
            ast->equal_token = consumeToken();
            parseTypeId(ast->type_id);
        }
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseTypeParameter(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_CLASS || LA() == T_TYPENAME)
        return parseTypenameTypeParameter(node);
    else if (LA() == T_TEMPLATE)
        return parseTemplateTypeParameter(node);
    else
        return false;
}

bool Parser::parseTypeId(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    SpecifierListAST *type_specifier = 0;
    if (parseTypeSpecifier(type_specifier)) {
        TypeIdAST *ast = new (_pool) TypeIdAST;
        ast->type_specifier_list = type_specifier;
        parseAbstractDeclarator(ast->declarator);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseParameterDeclarationClause(ParameterDeclarationClauseAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_RPAREN)
        return true; // nothing to do

    DeclarationListAST *parameter_declarations = 0;

    unsigned dot_dot_dot_token = 0;
    if (LA() == T_DOT_DOT_DOT)
        dot_dot_dot_token = consumeToken();
    else {
        parseParameterDeclarationList(parameter_declarations);

        if (LA() == T_DOT_DOT_DOT) {
            dot_dot_dot_token = consumeToken();
        } else if (LA() == T_COMMA && LA(2) == T_DOT_DOT_DOT) {
            consumeToken(); // skip comma
            dot_dot_dot_token = consumeToken();
        }
    }

    if (parameter_declarations || dot_dot_dot_token) {
        ParameterDeclarationClauseAST *ast = new (_pool) ParameterDeclarationClauseAST;
        ast->parameter_declaration_list = parameter_declarations;
        ast->dot_dot_dot_token = dot_dot_dot_token;
        node = ast;
    }

    return true;
}

bool Parser::parseParameterDeclarationList(DeclarationListAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_DOT_DOT_DOT || (LA() == T_COMMA && LA(2) == T_DOT_DOT_DOT))
        return false; // nothing to do.

    DeclarationListAST **parameter_declaration_ptr = &node;
    DeclarationAST *declaration = 0;
    if (parseParameterDeclaration(declaration)) {
        *parameter_declaration_ptr = new (_pool) DeclarationListAST;
        (*parameter_declaration_ptr)->value = declaration;
        parameter_declaration_ptr = &(*parameter_declaration_ptr)->next;
        while (LA() == T_COMMA) {
            consumeToken();

            if (LA() == T_DOT_DOT_DOT)
                break;

            declaration = 0;
            if (parseParameterDeclaration(declaration)) {
                *parameter_declaration_ptr = new (_pool) DeclarationListAST;
                (*parameter_declaration_ptr)->value = declaration;
                parameter_declaration_ptr = &(*parameter_declaration_ptr)->next;
            }
        }
        return true;
    }
    return false;
}

bool Parser::parseParameterDeclaration(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    SpecifierListAST *decl_specifier_seq = 0;
    if (parseDeclSpecifierSeq(decl_specifier_seq)) {
        ParameterDeclarationAST *ast = new (_pool) ParameterDeclarationAST;
        ast->type_specifier_list = decl_specifier_seq;
        parseDeclaratorOrAbstractDeclarator(ast->declarator);
        if (LA() == T_EQUAL) {
            ast->equal_token = consumeToken();
            parseLogicalOrExpression(ast->expression);
        }

        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseClassSpecifier(SpecifierListAST *&node)
{
    DEBUG_THIS_RULE();
    if (! lookAtClassKey())
        return false;

    unsigned classkey_token = consumeToken();

    SpecifierListAST *attributes = 0, **attr_ptr = &attributes;
    while (LA() == T___ATTRIBUTE__) {
        parseAttributeSpecifier(*attr_ptr);
        attr_ptr = &(*attr_ptr)->next;
    }

    if (LA(1) == T_IDENTIFIER && LA(2) == T_IDENTIFIER) {
        _translationUnit->warning(cursor(), "skip identifier `%s'",
                                  tok().spell());
        consumeToken();
    }

    NameAST *name = 0;
    parseName(name);

    bool parsed = false;

    const bool previousInFunctionBody = _inFunctionBody;
    _inFunctionBody = false;

    unsigned colon_token = 0;

    if (LA() == T_COLON || LA() == T_LBRACE) {
        BaseSpecifierListAST *base_clause_list = 0;

        if (LA() == T_COLON) {
            colon_token = cursor();

            parseBaseClause(base_clause_list);

            if (LA() != T_LBRACE) {
                _translationUnit->error(cursor(), "expected `{' before `%s'", tok().spell());

                const unsigned saved = cursor();

                for (int n = 0; n < 3 && LA() != T_EOF_SYMBOL; ++n, consumeToken()) {
                    if (LA() == T_LBRACE)
                        break;
                }

                if (LA() != T_LBRACE)
                    rewind(saved);
            }
        }

        ClassSpecifierAST *ast = new (_pool) ClassSpecifierAST;
        ast->classkey_token = classkey_token;
        ast->attribute_list = attributes;
        ast->name = name;
        ast->colon_token = colon_token;
        ast->base_clause_list = base_clause_list;

        if (LA() == T_LBRACE)
            ast->lbrace_token = consumeToken();

        DeclarationListAST **declaration_ptr = &ast->member_specifier_list;
        while (int tk = LA()) {
            if (tk == T_RBRACE) {
                ast->rbrace_token = consumeToken();
                break;
            }

            unsigned start_declaration = cursor();
            DeclarationAST *declaration = 0;
            if (parseMemberSpecification(declaration)) {
                *declaration_ptr = new (_pool) DeclarationListAST;
                (*declaration_ptr)->value = declaration;
                declaration_ptr = &(*declaration_ptr)->next;
            } else {
                _translationUnit->error(start_declaration, "expected a declaration");
                rewind(start_declaration + 1);
                skipUntilDeclaration();
            }
        }
        node = new (_pool) SpecifierListAST(ast);
        parsed = true;
    }

    _inFunctionBody = previousInFunctionBody;

    return parsed;
}

bool Parser::parseAccessSpecifier(SpecifierAST *&node)
{
    DEBUG_THIS_RULE();
    switch (LA()) {
    case T_PUBLIC:
    case T_PROTECTED:
    case T_PRIVATE: {
        SimpleSpecifierAST *ast = new (_pool) SimpleSpecifierAST;
        ast->specifier_token = consumeToken();
        node = ast;
        return true;
    }

    default:
        return false;
    } // switch
}

bool Parser::parseAccessDeclaration(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_PUBLIC || LA() == T_PROTECTED || LA() == T_PRIVATE || LA() == T_Q_SIGNALS) {
        bool isSignals = LA() == T_Q_SIGNALS;
        AccessDeclarationAST *ast = new (_pool) AccessDeclarationAST;
        ast->access_specifier_token = consumeToken();
        if (! isSignals && LA() == T_Q_SLOTS)
            ast->slots_token = consumeToken();
        match(T_COLON, &ast->colon_token);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseMemberSpecification(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    switch (LA()) {
    case T_SEMICOLON:
        return parseEmptyDeclaration(node);

    case T_USING:
        return parseUsing(node);

    case T_TEMPLATE:
        return parseTemplateDeclaration(node);

    case T_Q_SIGNALS:
    case T_PUBLIC:
    case T_PROTECTED:
    case T_PRIVATE:
        return parseAccessDeclaration(node);

    default:
        return parseSimpleDeclaration(node, /*acceptStructDeclarator=*/true);
    } // switch
}

bool Parser::parseCtorInitializer(CtorInitializerAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_COLON) {
        unsigned colon_token = consumeToken();

        CtorInitializerAST *ast = new (_pool) CtorInitializerAST;
        ast->colon_token = colon_token;

        parseMemInitializerList(ast->member_initializer_list);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseElaboratedTypeSpecifier(SpecifierListAST *&node)
{
    DEBUG_THIS_RULE();
    if (lookAtClassKey() || LA() == T_ENUM || LA() == T_TYPENAME) {
        unsigned classkey_token = consumeToken();
        NameAST *name = 0;
        if (parseName(name)) {
            ElaboratedTypeSpecifierAST *ast = new (_pool) ElaboratedTypeSpecifierAST;
            ast->classkey_token = classkey_token;
            ast->name = name;
            node = new (_pool) SpecifierListAST(ast);
            return true;
        }
    }
    return false;
}

bool Parser::parseExceptionSpecification(ExceptionSpecificationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_THROW) {
        ExceptionSpecificationAST *ast = new (_pool) ExceptionSpecificationAST;
        ast->throw_token = consumeToken();
        if (LA() == T_LPAREN)
            ast->lparen_token = consumeToken();
        if (LA() == T_DOT_DOT_DOT)
            ast->dot_dot_dot_token = consumeToken();
        else
            parseTypeIdList(ast->type_id_list);
        if (LA() == T_RPAREN)
            ast->rparen_token = consumeToken();
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseEnumerator(EnumeratorListAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_IDENTIFIER) {
        EnumeratorAST *ast = new (_pool) EnumeratorAST;
        ast->identifier_token = consumeToken();

        if (LA() == T_EQUAL) {
            ast->equal_token = consumeToken();
            parseConstantExpression(ast->expression);
        }

        node = new (_pool) EnumeratorListAST;
        node->value = ast;
        return true;
    }
    return false;
}

bool Parser::parseInitDeclarator(DeclaratorAST *&node,
        bool acceptStructDeclarator)
{
    DEBUG_THIS_RULE();
    unsigned start = cursor();

    if (acceptStructDeclarator && LA() == T_COLON) {
        // anonymous bit-field declaration.
        // ### TODO create the AST
    } else if (! parseDeclarator(node, /*stopAtCppInitializer = */ ! acceptStructDeclarator)) {
        return false;
    }

    if (LA() == T_ASM && LA(2) == T_LPAREN) { // ### FIXME
        consumeToken();

        if (skip(T_LPAREN, T_RPAREN))
            consumeToken();
    }

    if (acceptStructDeclarator && node &&
            ! node->postfix_declarator_list &&
            node->core_declarator &&
            node->core_declarator->asNestedDeclarator()) {
        rewind(start);
        return false;
    }

    if (acceptStructDeclarator && LA() == T_COLON
            && (! node || ! node->postfix_declarator_list)) {
        unsigned colon_token = consumeToken();
        ExpressionAST *expression = 0;
        if (parseConstantExpression(expression) && (LA() == T_COMMA ||
                                                    LA() == T_SEMICOLON)) {
            // recognized a bitfielddeclarator.
            // ### TODO create the AST
            return true;
        }
        rewind(colon_token);
    } else if (LA() == T_EQUAL || (! acceptStructDeclarator && LA() == T_LPAREN)) {
        parseInitializer(node->initializer, &node->equals_token);
    }
    return true;
}

bool Parser::parseBaseClause(BaseSpecifierListAST *&node)
{
    DEBUG_THIS_RULE();

    if (LA() == T_COLON) {
        consumeToken(); // ### remove me

        BaseSpecifierListAST **ast = &node;
        if (parseBaseSpecifier(*ast)) {
            ast = &(*ast)->next;

            while (LA() == T_COMMA) {
                consumeToken(); // consume T_COMMA

                if (parseBaseSpecifier(*ast))
                    ast = &(*ast)->next;
            }
        }

        return true;
    }
    return false;
}

bool Parser::parseInitializer(ExpressionAST *&node, unsigned *equals_token)
{
    DEBUG_THIS_RULE();
    if (LA() == T_LPAREN) {
        return parsePrimaryExpression(node);
    } else if (LA() == T_EQUAL) {
        (*equals_token) = consumeToken();
        return parseInitializerClause(node);
    }
    return false;
}

bool Parser::parseMemInitializerList(MemInitializerListAST *&node)
{
    DEBUG_THIS_RULE();
    MemInitializerListAST **initializer = &node;

    if (parseMemInitializer(*initializer)) {
        initializer = &(*initializer)->next;

        while (true) {

            if (LA() == T_LBRACE)
                break;

            else if (LA() == T_COMMA || (LA() == T_IDENTIFIER && (LA(2) == T_LPAREN || LA(2) == T_COLON_COLON))) {
                if (LA() != T_COMMA)
                    _translationUnit->error(cursor(), "expected `,'");
                else
                    consumeToken();

                if (parseMemInitializer(*initializer))
                    initializer = &(*initializer)->next;
                else
                    _translationUnit->error(cursor(), "expected a member initializer");

            } else break;
        }

        if (LA() != T_LBRACE)
            _translationUnit->error(cursor(), "expected `{'");

        return true;
    }

    return false;
}

bool Parser::parseMemInitializer(MemInitializerListAST *&node)
{
    DEBUG_THIS_RULE();
    NameAST *name = 0;
    if (! parseName(name))
        return false;

    MemInitializerAST *ast = new (_pool) MemInitializerAST;
    ast->name = name;
    match(T_LPAREN, &ast->lparen_token);
    parseExpressionList(ast->expression_list);
    match(T_RPAREN, &ast->rparen_token);

    node = new (_pool) MemInitializerListAST;
    node->value = ast;
    return true;
}

bool Parser::parseTypeIdList(ExpressionListAST *&node)
{
    DEBUG_THIS_RULE();
    ExpressionListAST **expression_list_ptr = &node;
    ExpressionAST *typeId = 0;
    if (parseTypeId(typeId)) {
        *expression_list_ptr = new (_pool) ExpressionListAST;
        (*expression_list_ptr)->value = typeId;
        expression_list_ptr = &(*expression_list_ptr)->next;
        while (LA() == T_COMMA) {
            consumeToken();

            if (parseTypeId(typeId)) {
                *expression_list_ptr = new (_pool) ExpressionListAST;
                (*expression_list_ptr)->value = typeId;
                expression_list_ptr = &(*expression_list_ptr)->next;
            }
        }
        return true;
    }

    return false;
}

bool Parser::parseExpressionList(ExpressionListAST *&node)
{
    DEBUG_THIS_RULE();
    ExpressionListAST **expression_list_ptr = &node;
    ExpressionAST *expression = 0;
    if (parseAssignmentExpression(expression)) {
        *expression_list_ptr = new (_pool) ExpressionListAST;
        (*expression_list_ptr)->value = expression;
        expression_list_ptr = &(*expression_list_ptr)->next;
        while (LA() == T_COMMA) {
            consumeToken(); // consume T_COMMA

            if (parseAssignmentExpression(expression)) {
                *expression_list_ptr = new (_pool) ExpressionListAST;
                (*expression_list_ptr)->value = expression;
                expression_list_ptr = &(*expression_list_ptr)->next;
            }
        }
        return true;
    }
    return false;
}

bool Parser::parseBaseSpecifier(BaseSpecifierListAST *&node)
{
    DEBUG_THIS_RULE();
    BaseSpecifierAST *ast = new (_pool) BaseSpecifierAST;

    if (LA() == T_VIRTUAL) {
        ast->virtual_token = consumeToken();

        int tk = LA();
        if (tk == T_PUBLIC || tk == T_PROTECTED || tk == T_PRIVATE)
            ast->access_specifier_token = consumeToken();
    } else {
        int tk = LA();
        if (tk == T_PUBLIC || tk == T_PROTECTED || tk == T_PRIVATE)
            ast->access_specifier_token = consumeToken();

        if (LA() == T_VIRTUAL)
            ast->virtual_token = consumeToken();
    }

    parseName(ast->name);
    if (! ast->name)
        _translationUnit->error(cursor(), "expected class-name");

    node = new (_pool) BaseSpecifierListAST;
    node->value = ast;
    return true;
}

bool Parser::parseInitializerList(ExpressionListAST *&node)
{
    DEBUG_THIS_RULE();
    ExpressionListAST **initializer_ptr = &node;
    ExpressionAST *initializer = 0;
    if (parseInitializerClause(initializer)) {
        *initializer_ptr = new (_pool) ExpressionListAST;
        (*initializer_ptr)->value = initializer;
        initializer_ptr = &(*initializer_ptr)->next;
        while (LA() == T_COMMA) {
            consumeToken(); // consume T_COMMA
            initializer = 0;
            parseInitializerClause(initializer);
            *initializer_ptr = new (_pool) ExpressionListAST;
            (*initializer_ptr)->value = initializer;
            initializer_ptr = &(*initializer_ptr)->next;
        }
    }
    return true;
}

bool Parser::parseInitializerClause(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_LBRACE) {
        ArrayInitializerAST *ast = new (_pool) ArrayInitializerAST;
        ast->lbrace_token = consumeToken();
        parseInitializerList(ast->expression_list);
        match(T_RBRACE, &ast->rbrace_token);
        node = ast;
        return true;
    }
    return parseAssignmentExpression(node);
}

bool Parser::parseUnqualifiedName(NameAST *&node, bool acceptTemplateId)
{
    DEBUG_THIS_RULE();
    if (LA() == T_TILDE && LA(2) == T_IDENTIFIER) {
        DestructorNameAST *ast = new (_pool) DestructorNameAST;
        ast->tilde_token = consumeToken();
        ast->identifier_token = consumeToken();
        node = ast;
        return true;
    } else if (LA() == T_OPERATOR) {
        unsigned operator_token = cursor();
        if (parseOperatorFunctionId(node))
            return true;
        rewind(operator_token);
        return parseConversionFunctionId(node);
     } else if (LA() == T_IDENTIFIER) {
         unsigned identifier_token = cursor();
         if (acceptTemplateId && LA(2) == T_LESS && parseTemplateId(node)) {
             if (! _templateArguments || (LA() == T_COMMA  || LA() == T_GREATER ||
                                          LA() == T_LPAREN || LA() == T_RPAREN  ||
                                          LA() == T_COLON_COLON))
                 return true;
         }
         rewind(identifier_token);
         SimpleNameAST *ast = new (_pool) SimpleNameAST;
         ast->identifier_token = consumeToken();
         node = ast;
         return true;
    } else if (LA() == T_TEMPLATE) {
        unsigned template_token = consumeToken();
        if (parseTemplateId(node))
            return true;
        rewind(template_token);
    }
    return false;
}

bool Parser::parseStringLiteral(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! (LA() == T_STRING_LITERAL || LA() == T_WIDE_STRING_LITERAL))
        return false;

    StringLiteralAST **ast = reinterpret_cast<StringLiteralAST **> (&node);

    while (LA() == T_STRING_LITERAL || LA() == T_WIDE_STRING_LITERAL) {
        *ast = new (_pool) StringLiteralAST;
        (*ast)->literal_token = consumeToken();
        ast = &(*ast)->next;
    }
    return true;
}

bool Parser::parseExpressionStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    ExpressionAST *expression = 0;
    if (LA() == T_SEMICOLON || parseExpression(expression)) {
        ExpressionStatementAST *ast = new (_pool) ExpressionStatementAST;
        ast->expression = expression;
        match(T_SEMICOLON, &ast->semicolon_token);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    switch (LA()) {
    case T_WHILE:
        return parseWhileStatement(node);

    case T_DO:
        return parseDoStatement(node);

    case T_Q_FOREACH:
        return parseForeachStatement(node);

    case T_FOR:
        return parseForStatement(node);

    case T_IF:
        return parseIfStatement(node);

    case T_SWITCH:
        return parseSwitchStatement(node);

    case T_TRY:
        return parseTryBlockStatement(node);

    case T_CASE:
    case T_DEFAULT:
        return parseLabeledStatement(node);

    case T_BREAK:
        return parseBreakStatement(node);

    case T_CONTINUE:
        return parseContinueStatement(node);

    case T_GOTO:
        return parseGotoStatement(node);

    case T_RETURN:
        return parseReturnStatement(node);

    case T_LBRACE:
        return parseCompoundStatement(node);

    case T_ASM:
    case T_NAMESPACE:
    case T_USING:
    case T_TEMPLATE:
    case T_CLASS: case T_STRUCT: case T_UNION:
        return parseDeclarationStatement(node);

    case T_SEMICOLON: {
        ExpressionStatementAST *ast = new (_pool) ExpressionStatementAST;
        ast->semicolon_token = consumeToken();
        node = ast;
        return true;
    }

    case T_AT_SYNCHRONIZED:
        if (objCEnabled())
            return parseObjCSynchronizedStatement(node);

    case T_Q_D:
    case T_Q_Q: {
        QtMemberDeclarationAST *ast = new (_pool) QtMemberDeclarationAST;
        ast->q_token = consumeToken();
        match(T_LPAREN, &ast->lparen_token);
        parseTypeId(ast->type_id);
        match(T_RPAREN, &ast->rparen_token);
        node = ast;
    } return true;

    default:
        if (LA() == T_IDENTIFIER && LA(2) == T_COLON)
            return parseLabeledStatement(node);

        return parseExpressionOrDeclarationStatement(node);
    } // switch
    return false;
}

bool Parser::parseBreakStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_BREAK) {
        BreakStatementAST *ast = new (_pool) BreakStatementAST;
        ast->break_token = consumeToken();
        match(T_SEMICOLON, &ast->semicolon_token);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseContinueStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_CONTINUE) {
        ContinueStatementAST *ast = new (_pool) ContinueStatementAST;
        ast->continue_token = consumeToken();
        match(T_SEMICOLON, &ast->semicolon_token);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseGotoStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_GOTO) {
        GotoStatementAST *ast = new (_pool) GotoStatementAST;
        ast->goto_token = consumeToken();
        match(T_IDENTIFIER, &ast->identifier_token);
        match(T_SEMICOLON, &ast->semicolon_token);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseReturnStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_RETURN) {
        ReturnStatementAST *ast = new (_pool) ReturnStatementAST;
        ast->return_token = consumeToken();
        parseExpression(ast->expression);
        match(T_SEMICOLON, &ast->semicolon_token);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::isPointerDeclaration(DeclarationStatementAST *ast) const
{
    if (! ast)
        return false;

    if (SimpleDeclarationAST *declaration = ast->declaration->asSimpleDeclaration()) {
        if (SpecifierListAST *spec = declaration->decl_specifier_list) {
            if (spec->value->asNamedTypeSpecifier() && ! spec->next) {
                if (DeclaratorListAST *declarators = declaration->declarator_list) {
                    if (DeclaratorAST *declarator = declarators->value) {
                        if (declarator->ptr_operator_list && declarator->equals_token && declarator->initializer) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool Parser::maybeAmbiguousStatement(DeclarationStatementAST *ast) const
{
    if (! ast)
        return false;

    if (SimpleDeclarationAST *declaration = ast->declaration->asSimpleDeclaration()) {
        if (SpecifierListAST *spec = declaration->decl_specifier_list) {
            if (spec->value->asNamedTypeSpecifier() && ! spec->next) {
                if (DeclaratorListAST *declarators = declaration->declarator_list) {
                    if (DeclaratorAST *declarator = declarators->value) {
                        if (declarator->core_declarator &&
                            declarator->core_declarator->asNestedDeclarator()) {
                            // recognized name(id-expression)
                            return true;
                        }
                    }
                }
            }

        } else if (DeclaratorListAST *declarators = declaration->declarator_list) {
            // no decl_specifiers...
            if (DeclaratorAST *declarator = declarators->value) {
                if (declarator->postfix_declarator_list && declarator->postfix_declarator_list->value->asFunctionDeclarator()
                                                     && ! declarator->initializer) {
                    return false;
                }                
            }

            return true;
        }
    }

    return false;
}

bool Parser::parseExpressionOrDeclarationStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_SEMICOLON)
        return parseExpressionStatement(node);

    unsigned start = cursor();
    bool blocked = blockErrors(true);

    if (parseDeclarationStatement(node)) {
        DeclarationStatementAST *stmt = static_cast<DeclarationStatementAST *>(node);

        if (isPointerDeclaration(stmt)) {
            blockErrors(blocked);
            return true;
        }

        if (! maybeAmbiguousStatement(stmt)) {
            unsigned end_of_declaration_statement = cursor();
            rewind(start);

            StatementAST *expression = 0;
            if (parseExpressionStatement(expression) && cursor() == end_of_declaration_statement) {
                // it's an ambiguous expression-or-declaration statement.
                ExpressionOrDeclarationStatementAST *ast = new (_pool) ExpressionOrDeclarationStatementAST;
                ast->declaration = node;
                ast->expression = expression;
                node = ast;
            }

            rewind(end_of_declaration_statement);
            blockErrors(blocked);
            return true;
        }
    }

    // it's not a declaration statement.
    blockErrors(blocked);
    rewind(start);
    return parseExpressionStatement(node);
}

bool Parser::parseCondition(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    unsigned start = cursor();

    bool blocked = blockErrors(true);
    SpecifierListAST *type_specifier = 0;
    if (parseTypeSpecifier(type_specifier)) {
        DeclaratorAST *declarator = 0;
        if (parseInitDeclarator(declarator, /*acceptStructDeclarator=*/false)) {
            if (declarator->initializer) {
                ConditionAST *ast = new (_pool) ConditionAST;
                ast->type_specifier_list = type_specifier;
                ast->declarator = declarator;
                node = ast;
                blockErrors(blocked);
                return true;
            }
        }
    }

    blockErrors(blocked);
    rewind(start);
    return parseExpression(node);
}

bool Parser::parseWhileStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_WHILE) {
        WhileStatementAST *ast = new (_pool) WhileStatementAST;
        ast->while_token = consumeToken();
        match(T_LPAREN, &ast->lparen_token);
        parseCondition(ast->condition);
        match(T_RPAREN, &ast->rparen_token);
        parseStatement(ast->statement);
        node = ast;
        return true;
    }
    return true;
}

bool Parser::parseDoStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_DO) {
        DoStatementAST *ast = new (_pool) DoStatementAST;
        ast->do_token = consumeToken();
        parseStatement(ast->statement);
        match(T_WHILE, &ast->while_token);
        match(T_LPAREN, &ast->lparen_token);
        parseExpression(ast->expression);
        match(T_RPAREN, &ast->rparen_token);
        match(T_SEMICOLON, &ast->semicolon_token);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseForeachStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_Q_FOREACH) {
        ForeachStatementAST *ast = new (_pool) ForeachStatementAST;
        ast->foreach_token = consumeToken();
        match(T_LPAREN, &ast->lparen_token);

        unsigned startOfTypeSpecifier = cursor();
        bool blocked = blockErrors(true);

        if (parseTypeSpecifier(ast->type_specifier_list))
            parseDeclarator(ast->declarator);

        if (! ast->type_specifier_list || ! ast->declarator) {
            ast->type_specifier_list = 0;
            ast->declarator = 0;

            blockErrors(blocked);
            rewind(startOfTypeSpecifier);
            parseAssignmentExpression(ast->initializer);
        }

        blockErrors(blocked);

        match(T_COMMA, &ast->comma_token);
        parseExpression(ast->expression);
        match(T_RPAREN, &ast->rparen_token);
        parseStatement(ast->statement);

        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseForStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_FOR)
        return false;

    unsigned for_token = consumeToken();
    unsigned lparen_token = 0;
    match(T_LPAREN, &lparen_token);

    unsigned startOfTypeSpecifier = cursor();
    bool blocked = blockErrors(true);

    if (objCEnabled()) {
        ObjCFastEnumerationAST *ast = new (_pool) ObjCFastEnumerationAST;
        ast->for_token = for_token;
        ast->lparen_token = lparen_token;

        if (parseTypeSpecifier(ast->type_specifier_list))
            parseDeclarator(ast->declarator);

        if ((ast->type_specifier_list || ast->declarator) && !peekAtObjCContextKeyword(Token_in)) {
            // woops, probably parsed too much: "in" got parsed as a declarator. Let's redo it:
            ast->type_specifier_list = 0;
            ast->declarator = 0;

            rewind(startOfTypeSpecifier);
            parseDeclarator(ast->declarator);
        }

        if (! ast->type_specifier_list || ! ast->declarator) {
            ast->type_specifier_list = 0;
            ast->declarator = 0;

            rewind(startOfTypeSpecifier);
            parseAssignmentExpression(ast->initializer);
        }

        if (parseObjCContextKeyword(Token_in, ast->in_token)) {
            blockErrors(blocked);

            parseExpression(ast->fast_enumeratable_expression);
            match(T_RPAREN, &ast->rparen_token);
            parseStatement(ast->statement);

            node = ast;
            return true;
        }

        // there was no "in" token, so we continue with a normal for-statement
        rewind(startOfTypeSpecifier);
    }

    blockErrors(blocked);

    // Normal C/C++ for-statement parsing
    ForStatementAST *ast = new (_pool) ForStatementAST;

    ast->for_token = for_token;
    ast->lparen_token = lparen_token;
    parseForInitStatement(ast->initializer);
    parseCondition(ast->condition);
    match(T_SEMICOLON, &ast->semicolon_token);
    parseExpression(ast->expression);
    match(T_RPAREN, &ast->rparen_token);
    parseStatement(ast->statement);

    node = ast;
    return true;
}

bool Parser::parseForInitStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    return parseExpressionOrDeclarationStatement(node);
}

bool Parser::parseCompoundStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_LBRACE) {
        CompoundStatementAST *ast = new (_pool) CompoundStatementAST;
        ast->lbrace_token = consumeToken();
        StatementListAST **statement_ptr = &ast->statement_list;
        while (int tk = LA()) {
            if (tk == T_RBRACE)
                break;

            unsigned start_statement = cursor();
            StatementAST *statement = 0;
            if (! parseStatement(statement)) {
                rewind(start_statement + 1);
                skipUntilStatement();
            } else {
                *statement_ptr = new (_pool) StatementListAST;
                (*statement_ptr)->value = statement;
                statement_ptr = &(*statement_ptr)->next;
            }
        }
        match(T_RBRACE, &ast->rbrace_token);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseIfStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_IF) {
        IfStatementAST *ast = new (_pool) IfStatementAST;
        ast->if_token = consumeToken();
        match(T_LPAREN, &ast->lparen_token);
        parseCondition(ast->condition);
        match(T_RPAREN, &ast->rparen_token);
        if (! parseStatement(ast->statement))
            _translationUnit->error(cursor(), "expected statement");
        if (LA() == T_ELSE) {
            ast->else_token = consumeToken();
            if (! parseStatement(ast->else_statement))
                _translationUnit->error(cursor(), "expected statement");
        }
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseSwitchStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_SWITCH) {
        SwitchStatementAST *ast = new (_pool) SwitchStatementAST;
        ast->switch_token = consumeToken();
        match(T_LPAREN, &ast->lparen_token);
        parseCondition(ast->condition);
        match(T_RPAREN, &ast->rparen_token);
        parseStatement(ast->statement);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseLabeledStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    switch (LA()) {
    case T_IDENTIFIER:
        if (LA(2) == T_COLON) {
            LabeledStatementAST *ast = new (_pool) LabeledStatementAST;
            ast->label_token = consumeToken();
            ast->colon_token = consumeToken();
            parseStatement(ast->statement);
            node = ast;
            return true;
        }
        break;

    case T_DEFAULT: {
        LabeledStatementAST *ast = new (_pool) LabeledStatementAST;
        ast->label_token = consumeToken();
        match(T_COLON, &ast->colon_token);
        parseStatement(ast->statement);
        node = ast;
        return true;
    }

    case T_CASE: {
        CaseStatementAST *ast = new (_pool) CaseStatementAST;
        ast->case_token = consumeToken();
        parseConstantExpression(ast->expression);
        match(T_COLON, &ast->colon_token);
        parseStatement(ast->statement);
        node = ast;
        return true;
    }

    default:
        break;
    } // switch
    return false;
}

bool Parser::parseBlockDeclaration(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    switch (LA()) {
    case T_USING:
        return parseUsing(node);

    case T_ASM:
        return parseAsmDefinition(node);

    case T_NAMESPACE:
        return parseNamespaceAliasDefinition(node);

    default:
        return parseSimpleDeclaration(node);
    } // switch

}

bool Parser::parseNamespaceAliasDefinition(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_NAMESPACE && LA(2) == T_IDENTIFIER && LA(3) == T_EQUAL) {
        NamespaceAliasDefinitionAST *ast = new (_pool) NamespaceAliasDefinitionAST;
        ast->namespace_token = consumeToken();
        ast->namespace_name_token = consumeToken();
        ast->equal_token = consumeToken();
        parseName(ast->name);
        match(T_SEMICOLON, &ast->semicolon_token);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseDeclarationStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    unsigned start = cursor();
    DeclarationAST *declaration = 0;
    if (! parseBlockDeclaration(declaration))
        return false;

    if (SimpleDeclarationAST *simpleDeclaration = declaration->asSimpleDeclaration()) {
        if (! simpleDeclaration->decl_specifier_list) {
            rewind(start);
            return false;
        }
    }

    DeclarationStatementAST *ast = new (_pool) DeclarationStatementAST;
    ast->declaration = declaration;
    node = ast;
    return true;
}

bool Parser::lookAtCVQualifier() const
{
    switch (LA()) {
    case T_CONST:
    case T_VOLATILE:
        return true;
    default:
        return false;
    }
}

bool Parser::lookAtFunctionSpecifier() const
{
    switch (LA()) {
    case T_INLINE:
    case T_VIRTUAL:
    case T_EXPLICIT:
        return true;
    default:
        return false;
    }
}

bool Parser::lookAtStorageClassSpecifier() const
{
    switch (LA()) {
    case T_FRIEND:
    case T_AUTO:
    case T_REGISTER:
    case T_STATIC:
    case T_EXTERN:
    case T_MUTABLE:
    case T_TYPEDEF:
        return true;
    default:
        return false;
    }
}

bool Parser::lookAtBuiltinTypeSpecifier() const
{
    switch (LA()) {
    case T_CHAR:
    case T_WCHAR_T:
    case T_BOOL:
    case T_SHORT:
    case T_INT:
    case T_LONG:
    case T_SIGNED:
    case T_UNSIGNED:
    case T_FLOAT:
    case T_DOUBLE:
    case T_VOID:
        return true;
    // [gcc] extensions
    case T___TYPEOF__:
    case T___ATTRIBUTE__:
        return true;
    default:
        return false;
    }
}

bool Parser::lookAtClassKey() const
{
    switch (LA()) {
    case T_CLASS:
    case T_STRUCT:
    case T_UNION:
        return true;
    default:
        return false;
    }
}

bool Parser::parseAttributeSpecifier(SpecifierListAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T___ATTRIBUTE__)
        return false;

    AttributeSpecifierAST *ast = new (_pool) AttributeSpecifierAST;
    ast->attribute_token = consumeToken();
    match(T_LPAREN, &ast->first_lparen_token);
    match(T_LPAREN, &ast->second_lparen_token);
    parseAttributeList(ast->attribute_list);
    match(T_RPAREN, &ast->first_rparen_token);
    match(T_RPAREN, &ast->second_rparen_token);
    node = new (_pool) SpecifierListAST(ast);
    return true;
}

bool Parser::parseAttributeList(AttributeListAST *&) // ### create the AST
{
    DEBUG_THIS_RULE();

    while (LA() == T_CONST || LA() == T_IDENTIFIER) {
        if (LA() == T_CONST)
            consumeToken();
        else if (LA() == T_IDENTIFIER) {
            ExpressionAST *expression = 0;
            parseExpression(expression);
        }

        if (LA() != T_COMMA)
            break;

        consumeToken(); // skip T_COMMA
    }

    return true;
}

bool Parser::parseBuiltinTypeSpecifier(SpecifierListAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T___ATTRIBUTE__) {
        return parseAttributeSpecifier(node);
    } else if (LA() == T___TYPEOF__) {
        TypeofSpecifierAST *ast = new (_pool) TypeofSpecifierAST;
        ast->typeof_token = consumeToken();
        if (LA() == T_LPAREN) {
            unsigned lparen_token = consumeToken();
            if (parseTypeId(ast->expression) && LA() == T_RPAREN) {
                ast->lparen_token = lparen_token;
                ast->rparen_token = consumeToken();
                node = new (_pool) SpecifierListAST(ast);
                return true;
            }
            rewind(lparen_token);
        }
        parseUnaryExpression(ast->expression);
        node = new (_pool) SpecifierListAST(ast);
        return true;
    } else if (lookAtBuiltinTypeSpecifier()) {
        SimpleSpecifierAST *ast = new (_pool) SimpleSpecifierAST;
        ast->specifier_token = consumeToken();
        node = new (_pool) SpecifierListAST(ast);
        return true;
    }
    return false;
}

bool Parser::parseSimpleDeclaration(DeclarationAST *&node,
                                    bool acceptStructDeclarator)
{
    DEBUG_THIS_RULE();
    unsigned qt_invokable_token = 0;
    if (acceptStructDeclarator && (LA() == T_Q_SIGNAL || LA() == T_Q_SLOT))
        qt_invokable_token = consumeToken();

    // parse a simple declaration, a function definition,
    // or a contructor declaration.
    bool has_type_specifier = false;
    bool has_complex_type_specifier = false;
    unsigned startOfNamedTypeSpecifier = 0;
    NameAST *named_type_specifier = 0;
    SpecifierListAST *decl_specifier_seq = 0,
         **decl_specifier_seq_ptr = &decl_specifier_seq;
    for (;;) {
        if (lookAtCVQualifier() || lookAtFunctionSpecifier()
                || lookAtStorageClassSpecifier()) {
            SimpleSpecifierAST *spec = new (_pool) SimpleSpecifierAST;
            spec->specifier_token = consumeToken();
            *decl_specifier_seq_ptr = new (_pool) SpecifierListAST(spec);
            decl_specifier_seq_ptr = &(*decl_specifier_seq_ptr)->next;
        } else if (LA() == T___ATTRIBUTE__) {
            parseAttributeSpecifier(*decl_specifier_seq_ptr);
            decl_specifier_seq_ptr = &(*decl_specifier_seq_ptr)->next;
        } else if (! named_type_specifier && ! has_complex_type_specifier && lookAtBuiltinTypeSpecifier()) {
            parseBuiltinTypeSpecifier(*decl_specifier_seq_ptr);
            decl_specifier_seq_ptr = &(*decl_specifier_seq_ptr)->next;
            has_type_specifier = true;
        } else if (! has_type_specifier && (LA() == T_COLON_COLON ||
                                            LA() == T_IDENTIFIER)) {
            startOfNamedTypeSpecifier = cursor();
            if (parseName(named_type_specifier)) {
                NamedTypeSpecifierAST *spec = new (_pool) NamedTypeSpecifierAST;
                spec->name = named_type_specifier;
                *decl_specifier_seq_ptr = new (_pool) SpecifierListAST(spec);
                decl_specifier_seq_ptr = &(*decl_specifier_seq_ptr)->next;
                has_type_specifier = true;
            } else {
                rewind(startOfNamedTypeSpecifier);
                break;
            }
        } else if (! has_type_specifier && LA() == T_ENUM) {
            unsigned startOfTypeSpecifier = cursor();
            if (! parseElaboratedTypeSpecifier(*decl_specifier_seq_ptr) || LA() == T_LBRACE) {
                rewind(startOfTypeSpecifier);
                if (! parseEnumSpecifier(*decl_specifier_seq_ptr)) {
                    _translationUnit->error(startOfTypeSpecifier,
                                            "expected an enum specifier");
                    break;
                }
                has_complex_type_specifier = true;
            }
            decl_specifier_seq_ptr = &(*decl_specifier_seq_ptr)->next;
            has_type_specifier = true;
        } else if (! has_type_specifier && LA() == T_TYPENAME) {
            unsigned startOfElaboratedTypeSpecifier = cursor();
            if (! parseElaboratedTypeSpecifier(*decl_specifier_seq_ptr)) {
                _translationUnit->error(startOfElaboratedTypeSpecifier,
                                        "expected an elaborated type specifier");
                break;
            }
            decl_specifier_seq_ptr = &(*decl_specifier_seq_ptr)->next;
            has_type_specifier = true;
        } else if (! has_type_specifier && lookAtClassKey()) {
            unsigned startOfTypeSpecifier = cursor();
            if (! parseElaboratedTypeSpecifier(*decl_specifier_seq_ptr) ||
                (LA() == T_COLON || LA() == T_LBRACE || (LA(0) == T_IDENTIFIER && LA(1) == T_IDENTIFIER &&
                                                         (LA(2) == T_COLON || LA(2) == T_LBRACE)))) {
                rewind(startOfTypeSpecifier);
                if (! parseClassSpecifier(*decl_specifier_seq_ptr)) {
                    _translationUnit->error(startOfTypeSpecifier,
                                            "wrong type specifier");
                    break;
                }
                has_complex_type_specifier = true;
            }
            decl_specifier_seq_ptr = &(*decl_specifier_seq_ptr)->next;
            has_type_specifier = true;
        } else
            break;
    }

    DeclaratorListAST *declarator_list = 0,
        **declarator_ptr = &declarator_list;

    DeclaratorAST *declarator = 0;

    if (LA() != T_SEMICOLON) {
        const bool maybeCtor = (LA() == T_LPAREN && named_type_specifier);
        if (! parseInitDeclarator(declarator, acceptStructDeclarator) && maybeCtor) {
            rewind(startOfNamedTypeSpecifier);
            named_type_specifier = 0;
            // pop the named type specifier from the decl-specifier-seq
            SpecifierListAST **spec_ptr = &decl_specifier_seq;
            for (; *spec_ptr; spec_ptr = &(*spec_ptr)->next) {
                if (! (*spec_ptr)->next) {
                    *spec_ptr = 0;
                    break;
                }
            }
            if (! parseInitDeclarator(declarator, acceptStructDeclarator))
                return false;
        }
    }

    // if there is no valid declarator
    // and it doesn't look like a fwd or a class declaration
    // then it's not a declarations
    if (! declarator && ! maybeForwardOrClassDeclaration(decl_specifier_seq))
        return false;

    DeclaratorAST *firstDeclarator = declarator;

    if (declarator) {
        *declarator_ptr = new (_pool) DeclaratorListAST;
        (*declarator_ptr)->value = declarator;
        declarator_ptr = &(*declarator_ptr)->next;
    }

    if (LA() == T_COMMA || LA() == T_SEMICOLON || has_complex_type_specifier) {
        while (LA() == T_COMMA) {
            consumeToken(); // consume T_COMMA

            declarator = 0;
            if (parseInitDeclarator(declarator, acceptStructDeclarator)) {
                *declarator_ptr = new (_pool) DeclaratorListAST;
                (*declarator_ptr)->value = declarator;
                declarator_ptr = &(*declarator_ptr)->next;
            }
        }
        SimpleDeclarationAST *ast = new (_pool) SimpleDeclarationAST;
        ast->qt_invokable_token = qt_invokable_token;
        ast->decl_specifier_list = decl_specifier_seq;
        ast->declarator_list = declarator_list;
        match(T_SEMICOLON, &ast->semicolon_token);
        node = ast;
        return true;
    } else if (! _inFunctionBody && declarator && (LA() == T_COLON || LA() == T_LBRACE || LA() == T_TRY)) {
        CtorInitializerAST *ctor_initializer = 0;
        bool hasCtorInitializer = false;
        if (LA() == T_COLON) {
            hasCtorInitializer = true;
            parseCtorInitializer(ctor_initializer);

            if (LA() != T_LBRACE) {
                const unsigned pos = cursor();

                for (int n = 0; n < 3 && LA(); consumeToken(), ++n)
                    if (LA() == T_LBRACE)
                        break;

                if (LA() != T_LBRACE) {
                    _translationUnit->error(pos, "unexpected token `%s'", _translationUnit->spell(pos));
                    rewind(pos);
                }
            }
        }

        if (LA() == T_LBRACE || hasCtorInitializer) {
            FunctionDefinitionAST *ast = new (_pool) FunctionDefinitionAST;
            ast->qt_invokable_token = qt_invokable_token;
            ast->decl_specifier_list = decl_specifier_seq;
            ast->declarator = firstDeclarator;
            ast->ctor_initializer = ctor_initializer;
            parseFunctionBody(ast->function_body);
            node = ast;
            return true; // recognized a function definition.
        } else if (LA() == T_TRY) {
            FunctionDefinitionAST *ast = new (_pool) FunctionDefinitionAST;
            ast->qt_invokable_token = qt_invokable_token;
            ast->decl_specifier_list = decl_specifier_seq;
            ast->declarator = firstDeclarator;
            ast->ctor_initializer = ctor_initializer;
            parseTryBlockStatement(ast->function_body);
            node = ast;
            return true; // recognized a function definition.
        }
    }

    _translationUnit->error(cursor(), "unexpected token `%s'", tok().spell());
    return false;
}

bool Parser::maybeForwardOrClassDeclaration(SpecifierListAST *decl_specifier_seq) const
{
    // look at the decl_specifier for possible fwd or class declarations.
    if (SpecifierListAST *it = decl_specifier_seq) {
        while (it) {
            SimpleSpecifierAST *spec = it->value->asSimpleSpecifier();
            if (spec && _translationUnit->tokenKind(spec->specifier_token) == T_FRIEND)
                it = it->next;
            else
                break;
        }

        if (it) {
            SpecifierAST *spec = it->value;

            if (! it->next && (spec->asElaboratedTypeSpecifier() ||
                               spec->asEnumSpecifier() ||
                               spec->asClassSpecifier()))
                return true;
        }
    }

    return false;
}

bool Parser::parseFunctionBody(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (_translationUnit->skipFunctionBody()) {
        unsigned token_lbrace = 0;
        match(T_LBRACE, &token_lbrace);
        if (! token_lbrace)
            return false;

        const Token &tk = _translationUnit->tokenAt(token_lbrace);
        if (tk.close_brace)
            rewind(tk.close_brace);
        unsigned token_rbrace = 0;
        match(T_RBRACE, &token_rbrace);
        return true;
    }

    _inFunctionBody = true;
    const bool parsed = parseCompoundStatement(node);
    _inFunctionBody = false;
    return parsed;
}

bool Parser::parseTryBlockStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_TRY) {
        TryBlockStatementAST *ast = new (_pool) TryBlockStatementAST;
        ast->try_token = consumeToken();
        parseCompoundStatement(ast->statement);
        CatchClauseListAST **catch_clause_ptr = &ast->catch_clause_list;
        while (parseCatchClause(*catch_clause_ptr))
            catch_clause_ptr = &(*catch_clause_ptr)->next;
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseCatchClause(CatchClauseListAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_CATCH) {
        CatchClauseAST *ast = new (_pool) CatchClauseAST;
        ast->catch_token = consumeToken();
        match(T_LPAREN, &ast->lparen_token);
        parseExceptionDeclaration(ast->exception_declaration);
        match(T_RPAREN, &ast->rparen_token);
        parseCompoundStatement(ast->statement);
        node = new (_pool) CatchClauseListAST(ast);
        return true;
    }
    return false;
}

bool Parser::parseExceptionDeclaration(ExceptionDeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_DOT_DOT_DOT) {
        ExceptionDeclarationAST *ast = new (_pool) ExceptionDeclarationAST;
        ast->dot_dot_dot_token = consumeToken();
        node = ast;
        return true;
    }

    SpecifierListAST *type_specifier = 0;
    if (parseTypeSpecifier(type_specifier)) {
        ExceptionDeclarationAST *ast = new (_pool) ExceptionDeclarationAST;
        ast->type_specifier_list = type_specifier;
        parseDeclaratorOrAbstractDeclarator(ast->declarator);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseBoolLiteral(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_TRUE || LA() == T_FALSE) {
        BoolLiteralAST *ast = new (_pool) BoolLiteralAST;
        ast->literal_token = consumeToken();
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseNumericLiteral(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_NUMERIC_LITERAL  ||
        LA() == T_CHAR_LITERAL     ||
        LA() == T_WIDE_CHAR_LITERAL) {
        NumericLiteralAST *ast = new (_pool) NumericLiteralAST;
        ast->literal_token = consumeToken();
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseThisExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_THIS) {
        ThisExpressionAST *ast = new (_pool) ThisExpressionAST;
        ast->this_token = consumeToken();
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parsePrimaryExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    switch (LA()) {
    case T_STRING_LITERAL:
    case T_WIDE_STRING_LITERAL:
        return parseStringLiteral(node);

    case T_CHAR_LITERAL: // ### FIXME don't use NumericLiteral for chars
    case T_WIDE_CHAR_LITERAL:
    case T_NUMERIC_LITERAL:
        return parseNumericLiteral(node);

    case T_TRUE:
    case T_FALSE:
        return parseBoolLiteral(node);

    case T_THIS:
        return parseThisExpression(node);

    case T_LPAREN:
        return parseNestedExpression(node);

    case T_SIGNAL:
    case T_SLOT:
        return parseQtMethod(node);

    case T_LBRACKET:
    case T_AT_STRING_LITERAL:
    case T_AT_ENCODE:
    case T_AT_PROTOCOL:
    case T_AT_SELECTOR:
        return parseObjCExpression(node);

    default: {
        NameAST *name = 0;
        if (parseNameId(name)) {
            node = name;
            return true;
        }
        break;
    } // default

    } // switch

    return false;
}

bool Parser::parseObjCExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    switch (LA()) {
    case T_AT_ENCODE:
        return parseObjCEncodeExpression(node);

    case T_AT_PROTOCOL:
        return parseObjCProtocolExpression(node);

    case T_AT_SELECTOR:
        return parseObjCSelectorExpression(node);

    case T_LBRACKET:
        return parseObjCMessageExpression(node);

    case T_AT_STRING_LITERAL:
        return parseObjCStringLiteral(node);

    default:
        break;
    } // switch
    return false;
}

bool Parser::parseObjCStringLiteral(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_AT_STRING_LITERAL)
        return false;

    StringLiteralAST **ast = reinterpret_cast<StringLiteralAST **> (&node);

    while (LA() == T_AT_STRING_LITERAL) {
        *ast = new (_pool) StringLiteralAST;
        (*ast)->literal_token = consumeToken();
        ast = &(*ast)->next;
    }
    return true;
}

bool Parser::parseObjCSynchronizedStatement(StatementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_AT_SYNCHRONIZED)
        return false;

    ObjCSynchronizedStatementAST *ast = new (_pool) ObjCSynchronizedStatementAST;

    ast->synchronized_token = consumeToken();
    match(T_LPAREN, &ast->lparen_token);
    parseExpression(ast->synchronized_object);
    match(T_RPAREN, &ast->rparen_token);
    parseStatement(ast->statement);

    node = ast;
    return true;
}

bool Parser::parseObjCEncodeExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_AT_ENCODE)
        return false;

    ObjCEncodeExpressionAST *ast = new (_pool) ObjCEncodeExpressionAST;
    ast->encode_token = consumeToken();
    parseObjCTypeName(ast->type_name);
    node = ast;
    return true;
}

bool Parser::parseObjCProtocolExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_AT_PROTOCOL)
        return false;

    ObjCProtocolExpressionAST *ast = new (_pool) ObjCProtocolExpressionAST;
    ast->protocol_token = consumeToken();
    match(T_LPAREN, &(ast->lparen_token));
    match(T_IDENTIFIER, &(ast->identifier_token));
    match(T_RPAREN, &(ast->rparen_token));
    node = ast;
    return true;
}

bool Parser::parseObjCSelectorExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_AT_SELECTOR)
        return false;

    ObjCSelectorExpressionAST *ast = new (_pool) ObjCSelectorExpressionAST;
    ast->selector_token = consumeToken();
    match(T_LPAREN, &(ast->lparen_token));

    unsigned identifier_token = 0;
    match(T_IDENTIFIER, &identifier_token);
    if (LA() == T_COLON) {
        ObjCSelectorWithArgumentsAST *args = new (_pool) ObjCSelectorWithArgumentsAST;
        ast->selector = args;
        ObjCSelectorArgumentListAST *last = new (_pool) ObjCSelectorArgumentListAST;
        args->selector_argument_list = last;
        last->value = new (_pool) ObjCSelectorArgumentAST;
        last->value->name_token = identifier_token;
        last->value->colon_token = consumeToken();

        while (LA() != T_RPAREN) {
            last->next = new (_pool) ObjCSelectorArgumentListAST;
            last = last->next;
            last->value = new (_pool) ObjCSelectorArgumentAST;
            match(T_IDENTIFIER, &(last->value->name_token));
            match(T_COLON, &(last->value->colon_token));
        }
    } else {
        ObjCSelectorWithoutArgumentsAST *args = new (_pool) ObjCSelectorWithoutArgumentsAST;
        ast->selector = args;
        args->name_token = identifier_token;
    }

    match(T_RPAREN, &(ast->rparen_token));
    node = ast;
    return true;
}

bool Parser::parseObjCMessageExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_LBRACKET)
        return false;

    unsigned start = cursor();

    unsigned lbracket_token = consumeToken();
    ExpressionAST *receiver_expression = 0;
    ObjCSelectorAST *selector = 0;
    ObjCMessageArgumentListAST *argument_list = 0;

    if (parseObjCMessageReceiver(receiver_expression) &&
        parseObjCMessageArguments(selector, argument_list)) {

        ObjCMessageExpressionAST *ast = new (_pool) ObjCMessageExpressionAST;
        ast->lbracket_token = lbracket_token;
        ast->receiver_expression = receiver_expression;
        ast->selector = selector;
        ast->argument_list = argument_list;

        match(T_RBRACKET, &(ast->rbracket_token));
        node = ast;

        return true;
    }

    rewind(start);
    return false;
}

bool Parser::parseObjCMessageReceiver(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    return parseExpression(node);
}

bool Parser::parseObjCMessageArguments(ObjCSelectorAST *&selNode, ObjCMessageArgumentListAST *& argNode)
{
    DEBUG_THIS_RULE();
    if (LA() == T_RBRACKET)
        return false; // nothing to do.

    unsigned start = cursor();

    ObjCSelectorArgumentAST *selectorArgument = 0;
    ObjCMessageArgumentAST *messageArgument = 0;

    if (parseObjCSelectorArg(selectorArgument, messageArgument)) {
        ObjCSelectorArgumentListAST *selAst = new (_pool) ObjCSelectorArgumentListAST;
        selAst->value = selectorArgument;
        ObjCSelectorArgumentListAST *lastSelector = selAst;

        ObjCMessageArgumentListAST *argAst = new (_pool) ObjCMessageArgumentListAST;
        argAst->value = messageArgument;
        ObjCMessageArgumentListAST *lastArgument = argAst;

        while (parseObjCSelectorArg(selectorArgument, messageArgument)) {
            // accept the selector args.
            lastSelector->next = new (_pool) ObjCSelectorArgumentListAST;
            lastSelector = lastSelector->next;
            lastSelector->value = selectorArgument;

            lastArgument->next = new (_pool) ObjCMessageArgumentListAST;
            lastArgument = lastArgument->next;
            lastArgument->value = messageArgument;
        }

        if (LA() == T_COMMA) {
            ExpressionAST **lastExpression = &(lastArgument->value->parameter_value_expression);

            while (LA() == T_COMMA) {
                BinaryExpressionAST *binaryExpression = new (_pool) BinaryExpressionAST;
                binaryExpression->left_expression = *lastExpression;
                binaryExpression->binary_op_token = consumeToken(); // T_COMMA
                parseAssignmentExpression(binaryExpression->right_expression);
                lastExpression = &(binaryExpression->right_expression);
            }
        }

        ObjCSelectorWithArgumentsAST *selWithArgs = new (_pool) ObjCSelectorWithArgumentsAST;
        selWithArgs->selector_argument_list = selAst;

        selNode = selWithArgs;
        argNode = argAst;
        return true;
    } else {
        rewind(start);
        unsigned name_token = 0;
        if (!parseObjCSelector(name_token))
            return false;
        ObjCSelectorWithoutArgumentsAST *sel = new (_pool) ObjCSelectorWithoutArgumentsAST;
        sel->name_token = name_token;
        selNode = sel;
        argNode = 0;
        return true;
    }

    return false;
}

bool Parser::parseObjCSelectorArg(ObjCSelectorArgumentAST *&selNode, ObjCMessageArgumentAST *&argNode)
{
    DEBUG_THIS_RULE();
    unsigned selector_token = 0;
    if (!parseObjCSelector(selector_token))
        return false;

    if (LA() != T_COLON)
        return false;

    selNode = new (_pool) ObjCSelectorArgumentAST;
    selNode->name_token = selector_token;
    selNode->colon_token = consumeToken();

    argNode = new (_pool) ObjCMessageArgumentAST;
    ExpressionAST **expr = &(argNode->parameter_value_expression);
    unsigned expressionStart = cursor();
    if (parseAssignmentExpression(*expr) && LA() == T_COLON && (*expr)->asCastExpression()) {
        rewind(expressionStart);
        parseUnaryExpression(*expr);
        //
    }
    return true;
}

bool Parser::parseObjCMethodSignature()
{
    DEBUG_THIS_RULE();
    unsigned selector_token = 0;
    if (parseObjCSelector(selector_token)) {
        while (LA() == T_COMMA) {
            consumeToken(); // skip T_COMMA
            parseObjCSelector(selector_token);
        }
        return true;
    }
    return false;
}

bool Parser::parseNameId(NameAST *&name)
{
    DEBUG_THIS_RULE();
    unsigned start = cursor();
    if (! parseName(name))
        return false;

    if (LA() == T_RPAREN || LA() == T_COMMA)
        return true;

    QualifiedNameAST *qualified_name_id = name->asQualifiedName();

    TemplateIdAST *template_id = 0;
    if (qualified_name_id) {
        if (NameAST *unqualified_name = qualified_name_id->unqualified_name)
            template_id = unqualified_name->asTemplateId();
    } else {
        template_id = name->asTemplateId();
    }

    if (! template_id)
        return true; // it's not a template-id, there's nothing to rewind.

    else if (LA() == T_LPAREN) {
        // a template-id followed by a T_LPAREN
        if (TemplateArgumentListAST *template_arguments = template_id->template_argument_list) {
            if (! template_arguments->next && template_arguments->value &&
                    template_arguments->value->asBinaryExpression()) {

                unsigned saved = cursor();
                ExpressionAST *expr = 0;

                bool blocked = blockErrors(true);
                bool lookAtCastExpression = parseCastExpression(expr);
                (void) blockErrors(blocked);

                if (lookAtCastExpression) {
                    if (CastExpressionAST *cast_expression = expr->asCastExpression()) {
                        if (cast_expression->lparen_token && cast_expression->rparen_token
                                && cast_expression->type_id && cast_expression->expression) {
                            rewind(start);

                            name = 0;
                            return parseName(name, false);
                        }
                    }
                }
                rewind(saved);
            }
        }
    }

    switch (LA()) {
    case T_COMMA:
    case T_SEMICOLON:
    case T_LBRACKET:
    case T_LPAREN:
        return true;

    case T_IDENTIFIER:
    case T_STATIC_CAST:
    case T_DYNAMIC_CAST:
    case T_REINTERPRET_CAST:
    case T_CONST_CAST:
        rewind(start);
        return parseName(name, false);

    default:
        if (tok().isLiteral() || tok().isOperator()) {
            rewind(start);
            return parseName(name, false);
        }
    } // switch

    return true;
}

bool Parser::parseNestedExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_LPAREN) {
        unsigned lparen_token = consumeToken();

        if (LA() == T_LBRACE) {
            NestedExpressionAST *ast = new (_pool) NestedExpressionAST;
            ast->lparen_token = lparen_token;

            // ### ast
            StatementAST *statement = 0;
            parseCompoundStatement(statement);
            match(T_RPAREN, &ast->rparen_token);
            node = ast;
            return true;
        }

        bool previousTemplateArguments = switchTemplateArguments(false);

        ExpressionAST *expression = 0;
        if (parseExpression(expression) && LA() == T_RPAREN) {
            NestedExpressionAST *ast = new (_pool) NestedExpressionAST;
            ast->lparen_token = lparen_token;
            ast->expression = expression;
            ast->rparen_token = consumeToken();
            node = ast;
            (void) switchTemplateArguments(previousTemplateArguments);
            return true;
        }
        (void) switchTemplateArguments(previousTemplateArguments);
    }
    return false;
}

bool Parser::parseCppCastExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_DYNAMIC_CAST     || LA() == T_STATIC_CAST ||
        LA() == T_REINTERPRET_CAST || LA() == T_CONST_CAST) {
        CppCastExpressionAST *ast = new (_pool) CppCastExpressionAST;
        ast->cast_token = consumeToken();
        match(T_LESS, &ast->less_token);
        parseTypeId(ast->type_id);
        match(T_GREATER, &ast->greater_token);
        match(T_LPAREN, &ast->lparen_token);
        parseExpression(ast->expression);
        match(T_RPAREN, &ast->rparen_token);
        node = ast;
        return true;
    }
    return false;
}

// typename ::opt  nested-name-specifier identifier ( expression-listopt )
// typename ::opt  nested-name-specifier templateopt  template-id ( expression-listopt )
bool Parser::parseTypenameCallExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_TYPENAME) {
        unsigned typename_token = consumeToken();
        NameAST *name = 0;
        if (parseName(name) && LA() == T_LPAREN) {
            TypenameCallExpressionAST *ast = new (_pool) TypenameCallExpressionAST;
            ast->typename_token = typename_token;
            ast->name = name;
            ast->lparen_token = consumeToken();
            parseExpressionList(ast->expression_list);
            match(T_RPAREN, &ast->rparen_token);
            node = ast;
            return true;
        }
    }
    return false;
}

// typeid ( expression )
// typeid ( type-id )
bool Parser::parseTypeidExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_TYPEID) {
        TypeidExpressionAST *ast = new (_pool) TypeidExpressionAST;
        ast->typeid_token = consumeToken();
        if (LA() == T_LPAREN)
            ast->lparen_token = consumeToken();
        unsigned saved = cursor();
        if (! (parseTypeId(ast->expression) && LA() == T_RPAREN)) {
            rewind(saved);
            parseExpression(ast->expression);
        }
        match(T_RPAREN, &ast->rparen_token);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseCorePostfixExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();

    switch (LA()) {
    case T_DYNAMIC_CAST:
    case T_STATIC_CAST:
    case T_REINTERPRET_CAST:
    case T_CONST_CAST:
        return parseCppCastExpression(node);

    case T_TYPENAME:
        return parseTypenameCallExpression(node);

    case T_TYPEID:
        return parseTypeidExpression(node);

    default: {
        unsigned start = cursor();
        SpecifierListAST *type_specifier = 0;
        bool blocked = blockErrors(true);
        if (lookAtBuiltinTypeSpecifier() &&
                parseSimpleTypeSpecifier(type_specifier) &&
                LA() == T_LPAREN) {
            unsigned lparen_token = consumeToken();
            ExpressionListAST *expression_list = 0;
            parseExpressionList(expression_list);
            if (LA() == T_RPAREN) {
                unsigned rparen_token = consumeToken();
                TypeConstructorCallAST *ast = new (_pool) TypeConstructorCallAST;
                ast->type_specifier_list = type_specifier;
                ast->lparen_token = lparen_token;
                ast->expression_list = expression_list;
                ast->rparen_token = rparen_token;
                node = ast;
                blockErrors(blocked);
                return true;
            }
        }
        rewind(start);

        // look for compound literals
        if (LA() == T_LPAREN) {
            unsigned lparen_token = consumeToken();
            ExpressionAST *type_id = 0;
            if (parseTypeId(type_id) && LA() == T_RPAREN) {
                unsigned rparen_token = consumeToken();
                if (LA() == T_LBRACE) {
                    blockErrors(blocked);

                    CompoundLiteralAST *ast = new (_pool) CompoundLiteralAST;
                    ast->lparen_token = lparen_token;
                    ast->type_id = type_id;
                    ast->rparen_token = rparen_token;
                    parseInitializerClause(ast->initializer);
                    node = ast;
                    return true;
                }
            }
            rewind(start);
        }

        blockErrors(blocked);
        return parsePrimaryExpression(node);
    } // default
    } // switch
}

bool Parser::parsePostfixExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (parseCorePostfixExpression(node)) {
        PostfixListAST *postfix_expressions = 0,
            **postfix_ptr = &postfix_expressions;
        while (LA()) {
            if (LA() == T_LPAREN) {
                CallAST *ast = new (_pool) CallAST;
                ast->lparen_token = consumeToken();
                parseExpressionList(ast->expression_list);
                match(T_RPAREN, &ast->rparen_token);
                *postfix_ptr = new (_pool) PostfixListAST(ast);
                postfix_ptr = &(*postfix_ptr)->next;
            } else if (LA() == T_LBRACKET) {
                ArrayAccessAST *ast = new (_pool) ArrayAccessAST;
                ast->lbracket_token = consumeToken();
                parseExpression(ast->expression);
                match(T_RBRACKET, &ast->rbracket_token);
                *postfix_ptr = new (_pool) PostfixListAST(ast);
                postfix_ptr = &(*postfix_ptr)->next;
            } else if (LA() == T_PLUS_PLUS || LA() == T_MINUS_MINUS) {
                PostIncrDecrAST *ast = new (_pool) PostIncrDecrAST;
                ast->incr_decr_token = consumeToken();
                *postfix_ptr = new (_pool) PostfixListAST(ast);
                postfix_ptr = &(*postfix_ptr)->next;
            } else if (LA() == T_DOT || LA() == T_ARROW) {
                MemberAccessAST *ast = new (_pool) MemberAccessAST;
                ast->access_token = consumeToken();
                if (LA() == T_TEMPLATE)
                    ast->template_token = consumeToken();
                if (! parseNameId(ast->member_name))
                    _translationUnit->error(cursor(), "expected unqualified-id before token `%s'",
                                            tok().spell());
                *postfix_ptr = new (_pool) PostfixListAST(ast);
                postfix_ptr = &(*postfix_ptr)->next;
            } else break;
        } // while

        if (postfix_expressions) {
            PostfixExpressionAST *ast = new (_pool) PostfixExpressionAST;
            ast->base_expression = node;
            ast->postfix_expression_list = postfix_expressions;
            node = ast;
        }
        return true;
    }
    return false;
}

bool Parser::parseUnaryExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    switch (LA()) {
    case T_PLUS_PLUS:
    case T_MINUS_MINUS:
    case T_STAR:
    case T_AMPER:
    case T_PLUS:
    case T_MINUS:
    case T_EXCLAIM: {
        unsigned op = cursor();
        UnaryExpressionAST *ast = new (_pool) UnaryExpressionAST;
        ast->unary_op_token = consumeToken();
        if (! parseCastExpression(ast->expression)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
        }
        node = ast;
        return true;
    }

    case T_TILDE: {
        if (LA(2) == T_IDENTIFIER && LA(3) == T_LPAREN)
            break; // prefer destructor names

        UnaryExpressionAST *ast = new (_pool) UnaryExpressionAST;
        ast->unary_op_token = consumeToken();
        parseCastExpression(ast->expression);
        node = ast;
        return true;
    }

    case T_SIZEOF: {
        SizeofExpressionAST *ast = new (_pool) SizeofExpressionAST;
        ast->sizeof_token = consumeToken();

        if (LA() == T_LPAREN) {
            unsigned lparen_token = consumeToken();
            if (parseTypeId(ast->expression) && LA() == T_RPAREN) {
                ast->lparen_token = lparen_token;
                ast->rparen_token = consumeToken();
                node = ast;
                return true;
            } else {
                rewind(lparen_token);
            }
        }

        parseUnaryExpression(ast->expression);
        node = ast;
        return true;
    }

    default:
        break;
    } // switch

    if (LA() == T_NEW || (LA(1) == T_COLON_COLON &&
                          LA(2) == T_NEW))
        return parseNewExpression(node);
    else if (LA() == T_DELETE || (LA(1) == T_COLON_COLON &&
                                  LA(2) == T_DELETE))
        return parseDeleteExpression(node);
    else
        return parsePostfixExpression(node);
}

// new-placement ::= T_LPAREN expression-list T_RPAREN
bool Parser::parseNewPlacement(NewPlacementAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_LPAREN) {
        unsigned lparen_token = consumeToken();
        ExpressionListAST *expression_list = 0;
        if (parseExpressionList(expression_list) && expression_list && LA() == T_RPAREN) {
            unsigned rparen_token = consumeToken();
            NewPlacementAST *ast = new (_pool) NewPlacementAST;
            ast->lparen_token = lparen_token;
            ast->expression_list = expression_list;
            ast->rparen_token = rparen_token;
            node = ast;
            return true;
        }
    }

    return false;
}

// new-expression ::= T_COLON_COLON? T_NEW new-placement.opt
//                    new-type-id new-initializer.opt
// new-expression ::= T_COLON_COLON? T_NEW new-placement.opt
//                    T_LPAREN type-id T_RPAREN new-initializer.opt
bool Parser::parseNewExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! (LA() == T_NEW || (LA() == T_COLON_COLON && LA(2) == T_NEW)))
        return false;

    NewExpressionAST *ast = new (_pool) NewExpressionAST;
    if (LA() == T_COLON_COLON)
        ast->scope_token = consumeToken();

    ast->new_token = consumeToken();

    NewPlacementAST *new_placement = 0;

    if (parseNewPlacement(new_placement)) {
        unsigned after_new_placement = cursor();

        NewTypeIdAST *new_type_id = 0;
        if (parseNewTypeId(new_type_id)) {
            ast->new_placement = new_placement;
            ast->new_type_id = new_type_id;
            parseNewInitializer(ast->new_initializer);
            // recognized new-placement.opt new-type-id new-initializer.opt
            node = ast;
            return true;
        }

        rewind(after_new_placement);
        if (LA() == T_LPAREN) {
            unsigned lparen_token = consumeToken();
            ExpressionAST *type_id = 0;
            if (parseTypeId(type_id) && LA() == T_RPAREN) {
                ast->new_placement = new_placement;
                ast->lparen_token = lparen_token;
                ast->type_id = type_id;
                ast->rparen_token = consumeToken();
                parseNewInitializer(ast->new_initializer);
                node = ast;
                return true;
            }
        }
    }

    rewind(ast->new_token + 1);

    if (LA() == T_LPAREN) {
        unsigned lparen_token = consumeToken();
        ExpressionAST *type_id = 0;
        if (parseTypeId(type_id) && LA() == T_RPAREN) {
            ast->lparen_token = lparen_token;
            ast->type_id = type_id;
            ast->rparen_token = consumeToken();
            parseNewInitializer(ast->new_initializer);
            node = ast;
            return true;
        }
    }

    parseNewTypeId(ast->new_type_id);
    parseNewInitializer(ast->new_initializer);
    node = ast;
    return true;
}

bool Parser::parseNewTypeId(NewTypeIdAST *&node)
{
    DEBUG_THIS_RULE();
    SpecifierListAST *typeSpec = 0;
    if (! parseTypeSpecifier(typeSpec))
        return false;

    NewTypeIdAST *ast = new (_pool) NewTypeIdAST;
    ast->type_specifier_list = typeSpec;

    PtrOperatorListAST **ptrop_it = &ast->ptr_operator_list;
    while (parsePtrOperator(*ptrop_it))
        ptrop_it = &(*ptrop_it)->next;

    NewArrayDeclaratorListAST **it = &ast->new_array_declarator_list;
    while (parseNewArrayDeclarator(*it))
        it = &(*it)->next;

    node = ast;
    return true;
}


bool Parser::parseNewArrayDeclarator(NewArrayDeclaratorListAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_LBRACKET)
        return false;

    NewArrayDeclaratorAST *ast = new (_pool) NewArrayDeclaratorAST;
    ast->lbracket_token = consumeToken();
    parseExpression(ast->expression);
    match(T_RBRACKET, &ast->rbracket_token);

    node = new (_pool) NewArrayDeclaratorListAST;
    node->value = ast;
    return true;
}

bool Parser::parseNewInitializer(NewInitializerAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_LPAREN) {
        unsigned lparen_token = consumeToken();
        ExpressionAST *expression = 0;
        if (LA() == T_RPAREN || parseExpression(expression)) {
            NewInitializerAST *ast = new (_pool) NewInitializerAST;
            ast->lparen_token = lparen_token;
            ast->expression = expression;
            match(T_RPAREN, &ast->rparen_token);
            node = ast;
            return true;
        }
    }
    return false;
}

bool Parser::parseDeleteExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_DELETE || (LA() == T_COLON_COLON && LA(2) == T_DELETE)) {
        DeleteExpressionAST *ast = new (_pool) DeleteExpressionAST;

        if (LA() == T_COLON_COLON)
            ast->scope_token = consumeToken();

        ast->delete_token = consumeToken();

        if (LA() == T_LBRACKET) {
            ast->lbracket_token = consumeToken();
            match(T_RBRACKET, &ast->rbracket_token);
        }

        parseCastExpression(ast->expression);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseCastExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_LPAREN) {
        unsigned lparen_token = consumeToken();
        ExpressionAST *type_id = 0;
        if (parseTypeId(type_id) && LA() == T_RPAREN) {
            unsigned rparen_token = consumeToken();
            ExpressionAST *expression = 0;
            if (parseCastExpression(expression)) {
                CastExpressionAST *ast = new (_pool) CastExpressionAST;
                ast->lparen_token = lparen_token;
                ast->type_id = type_id;
                ast->rparen_token = rparen_token;
                ast->expression = expression;
                node = ast;
                return true;
            }
        }
        rewind(lparen_token);
    }
    return parseUnaryExpression(node);
}

bool Parser::parsePmExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseCastExpression(node))
        return false;

    while (LA() == T_ARROW_STAR || LA() == T_DOT_STAR) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parseCastExpression(rightExpr)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
            return false;
        }

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }
    return true;
}

bool Parser::parseMultiplicativeExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parsePmExpression(node))
        return false;

    while (LA() == T_STAR || LA() == T_SLASH || LA() == T_PERCENT) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parsePmExpression(rightExpr)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
            return false;
        }

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }
    return true;
}

bool Parser::parseAdditiveExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseMultiplicativeExpression(node))
        return false;

    while (LA() == T_PLUS || LA() == T_MINUS) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parseMultiplicativeExpression(rightExpr)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
            return false;
        }

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }
    return true;
}

bool Parser::parseShiftExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseAdditiveExpression(node))
        return false;

    while (LA() == T_LESS_LESS || LA() == T_GREATER_GREATER) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parseAdditiveExpression(rightExpr)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
            return false;
        }

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }
    return true;
}

bool Parser::parseRelationalExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseShiftExpression(node))
        return false;

    while (LA() == T_LESS || (LA() == T_GREATER && ! _templateArguments) ||
           LA() == T_LESS_EQUAL || LA() == T_GREATER_EQUAL) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parseShiftExpression(rightExpr)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
            return false;
        }

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }
    return true;
}

bool Parser::parseEqualityExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseRelationalExpression(node))
        return false;

    while (LA() == T_EQUAL_EQUAL || LA() == T_EXCLAIM_EQUAL) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parseRelationalExpression(rightExpr)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
            return false;
        }

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }
    return true;
}

bool Parser::parseAndExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseEqualityExpression(node))
        return false;

    while (LA() == T_AMPER) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parseEqualityExpression(rightExpr)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
            return false;
        }

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }
    return true;
}

bool Parser::parseExclusiveOrExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseAndExpression(node))
        return false;

    while (LA() == T_CARET) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parseAndExpression(rightExpr)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
            return false;
        }

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }
    return true;
}

bool Parser::parseInclusiveOrExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseExclusiveOrExpression(node))
        return false;

    while (LA() == T_PIPE) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parseExclusiveOrExpression(rightExpr)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
            return false;
        }

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }

    return true;
}

bool Parser::parseLogicalAndExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseInclusiveOrExpression(node))
        return false;

    while (LA() == T_AMPER_AMPER) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parseInclusiveOrExpression(rightExpr)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
            return false;
        }

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }
    return true;
}

bool Parser::parseLogicalOrExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseLogicalAndExpression(node))
        return false;

    while (LA() == T_PIPE_PIPE) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parseLogicalAndExpression(rightExpr)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
            return false;
        }

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }

    return true;
}

bool Parser::parseConditionalExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseLogicalOrExpression(node))
        return false;

    if (LA() != T_QUESTION)
        return true;

    ConditionalExpressionAST *ast = new (_pool) ConditionalExpressionAST;
    ast->condition = node;
    ast->question_token = consumeToken();
    parseAssignmentExpression(ast->left_expression);
    match(T_COLON, &ast->colon_token);
    parseAssignmentExpression(ast->right_expression);
    node = ast;
    return true;
}

bool Parser::lookAtAssignmentOperator() const
{
    switch (LA()) {
    case T_EQUAL:
    case T_AMPER_EQUAL:
    case T_CARET_EQUAL:
    case T_SLASH_EQUAL:
    case T_GREATER_GREATER_EQUAL:
    case T_LESS_LESS_EQUAL:
    case T_MINUS_EQUAL:
    case T_PERCENT_EQUAL:
    case T_PIPE_EQUAL:
    case T_PLUS_EQUAL:
    case T_STAR_EQUAL:
    case T_TILDE_EQUAL:
        return true;
    default:
        return false;
    } // switch
}

bool Parser::parseAssignmentExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_THROW)
        return parseThrowExpression(node);
    else if (! parseConditionalExpression(node))
        return false;

    if (lookAtAssignmentOperator()) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parseAssignmentExpression(rightExpr)) {
            _translationUnit->error(op, "expected expression after token `%s'",
                                    _translationUnit->spell(op));
            return false;
        }

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }

    return true;
}

bool Parser::parseQtMethod(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_SIGNAL || LA() == T_SLOT) {
        QtMethodAST *ast = new (_pool) QtMethodAST;
        ast->method_token = consumeToken();
        match(T_LPAREN, &ast->lparen_token);
        if (! parseDeclarator(ast->declarator))
            _translationUnit->error(cursor(), "expected a function declarator before token `%s'",
                                    tok().spell());
        match(T_RPAREN, &ast->rparen_token);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::parseConstantExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    return parseConditionalExpression(node);
}

bool Parser::parseExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    return parseCommaExpression(node);
}

bool Parser::parseCommaExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (! parseAssignmentExpression(node))
        return false;

    while (LA() == T_COMMA) {
        unsigned op = consumeToken();

        ExpressionAST *rightExpr = 0;
        if (! parseAssignmentExpression(rightExpr))
            return false;

        BinaryExpressionAST *ast = new (_pool) BinaryExpressionAST;
        ast->binary_op_token = op;
        ast->left_expression = node;
        ast->right_expression = rightExpr;
        node = ast;
    }
    return true;
}

bool Parser::parseThrowExpression(ExpressionAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() == T_THROW) {
        ThrowExpressionAST *ast = new (_pool) ThrowExpressionAST;
        ast->throw_token = consumeToken();
        parseAssignmentExpression(ast->expression);
        node = ast;
        return true;
    }
    return false;
}

bool Parser::lookAtObjCSelector() const
{
    switch (LA()) {
    case T_IDENTIFIER:
    case T_OR:
    case T_AND:
    case T_NOT:
    case T_XOR:
    case T_BITOR:
    case T_COMPL:
    case T_OR_EQ:
    case T_AND_EQ:
    case T_BITAND:
    case T_NOT_EQ:
    case T_XOR_EQ:
        return true;

    default:
        if (tok().isKeyword())
            return true;
    } // switch

    return false;
}

// objc-class-declaraton ::= T_AT_CLASS (T_IDENTIFIER @ T_COMMA) T_SEMICOLON
//
bool Parser::parseObjCClassForwardDeclaration(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_AT_CLASS)
        return false;

    ObjCClassForwardDeclarationAST *ast = new (_pool) ObjCClassForwardDeclarationAST;

    ast->class_token = consumeToken();
    unsigned identifier_token = 0;
    match(T_IDENTIFIER, &identifier_token);

    ast->identifier_list = new (_pool) ObjCIdentifierListAST;
    SimpleNameAST *name = new (_pool) SimpleNameAST;
    name->identifier_token = identifier_token;
    ast->identifier_list->value = name;
    ObjCIdentifierListAST **nextId = &(ast->identifier_list->next);

    while (LA() == T_COMMA) {
        consumeToken(); // consume T_COMMA
        match(T_IDENTIFIER, &identifier_token);

        *nextId = new (_pool) ObjCIdentifierListAST;
        name = new (_pool) SimpleNameAST;
        name->identifier_token = identifier_token;
        (*nextId)->value = name;
        nextId = &((*nextId)->next);
    }

    match(T_SEMICOLON, &(ast->semicolon_token));
    node = ast;
    return true;
}

// objc-interface ::= attribute-specifier-list-opt objc-class-interface
// objc-interface ::= objc-category-interface
//
// objc-class-interface ::= T_AT_INTERFACE T_IDENTIFIER (T_COLON T_IDENTIFIER)?
//                          objc-protocol-refs-opt
//                          objc-class-instance-variables-opt
//                          objc-interface-declaration-list
//                          T_AT_END
//
// objc-category-interface ::= T_AT_INTERFACE T_IDENTIFIER
//                             T_LPAREN T_IDENTIFIER? T_RPAREN
//                             objc-protocol-refs-opt
//                             objc-interface-declaration-list
//                             T_AT_END
//
bool Parser::parseObjCInterface(DeclarationAST *&node,
                                SpecifierListAST *attributes)
{
    DEBUG_THIS_RULE();
    if (! attributes && LA() == T___ATTRIBUTE__) {
        SpecifierListAST **attr = &attributes;
        while (parseAttributeSpecifier(*attr))
            attr = &(*attr)->next;
    }

    if (LA() != T_AT_INTERFACE)
        return false;

    unsigned objc_interface_token = consumeToken();
    unsigned identifier_token = 0;
    match(T_IDENTIFIER, &identifier_token);

    if (LA() == T_LPAREN) {
        // a category interface

        if (attributes)
            _translationUnit->error(attributes->firstToken(),
                                    "invalid attributes for category interface declaration");

        ObjCClassDeclarationAST *ast = new (_pool) ObjCClassDeclarationAST;
        ast->attribute_list = attributes;
        ast->interface_token = objc_interface_token;
        SimpleNameAST *class_name = new (_pool) SimpleNameAST;
        class_name->identifier_token= identifier_token;
        ast->class_name = class_name;

        match(T_LPAREN, &(ast->lparen_token));
        if (LA() == T_IDENTIFIER) {
            SimpleNameAST *category_name = new (_pool) SimpleNameAST;
            category_name->identifier_token = consumeToken();
            ast->category_name = category_name;
        }

        match(T_RPAREN, &(ast->rparen_token));

        parseObjCProtocolRefs(ast->protocol_refs);

        DeclarationListAST **nextMembers = &(ast->member_declaration_list);
        DeclarationAST *declaration = 0;
        while (parseObjCInterfaceMemberDeclaration(declaration)) {
            *nextMembers = new (_pool) DeclarationListAST;
            (*nextMembers)->value = declaration;
            nextMembers = &((*nextMembers)->next);
        }

        match(T_AT_END, &(ast->end_token));

        node = ast;
        return true;
    } else {
        // a class interface declaration
        ObjCClassDeclarationAST *ast = new (_pool) ObjCClassDeclarationAST;
        ast->attribute_list = attributes;
        ast->interface_token = objc_interface_token;
        SimpleNameAST* class_name = new (_pool) SimpleNameAST;
        class_name->identifier_token = identifier_token;
        ast->class_name = class_name;

        if (LA() == T_COLON) {
            ast->colon_token = consumeToken();
            SimpleNameAST *superclass = new (_pool) SimpleNameAST;
            match(T_IDENTIFIER, &(superclass->identifier_token));
            ast->superclass = superclass;
        }

        parseObjCProtocolRefs(ast->protocol_refs);
        parseObjClassInstanceVariables(ast->inst_vars_decl);

        DeclarationListAST **nextMembers = &(ast->member_declaration_list);
        DeclarationAST *declaration = 0;
        while (parseObjCInterfaceMemberDeclaration(declaration)) {
            *nextMembers = new (_pool) DeclarationListAST;
            (*nextMembers)->value = declaration;
            nextMembers = &((*nextMembers)->next);
        }

        match(T_AT_END, &(ast->end_token));

        node = ast;
        return true;
    }
}

// objc-protocol ::= T_AT_PROTOCOL (T_IDENTIFIER @ T_COMMA) T_SEMICOLON
//
bool Parser::parseObjCProtocol(DeclarationAST *&node,
                               SpecifierListAST *attributes)
{
    DEBUG_THIS_RULE();
    if (! attributes && LA() == T___ATTRIBUTE__) {
        SpecifierListAST **attr = &attributes;
        while (parseAttributeSpecifier(*attr))
            attr = &(*attr)->next;
    }

    if (LA() != T_AT_PROTOCOL)
        return false;

    unsigned protocol_token = consumeToken();
    unsigned identifier_token = 0;
    match(T_IDENTIFIER, &identifier_token);

    if (LA() == T_COMMA || LA() == T_SEMICOLON) {
        // a protocol forward declaration

        ObjCProtocolForwardDeclarationAST *ast = new (_pool) ObjCProtocolForwardDeclarationAST;
        ast->attribute_list = attributes;
        ast->protocol_token = protocol_token;
        ast->identifier_list = new (_pool) ObjCIdentifierListAST;
        SimpleNameAST *name = new (_pool) SimpleNameAST;
        name->identifier_token = identifier_token;
        ast->identifier_list->value = name;
        ObjCIdentifierListAST **nextId = &(ast->identifier_list->next);

        while (LA() == T_COMMA) {
            consumeToken(); // consume T_COMMA
            match(T_IDENTIFIER, &identifier_token);

            *nextId = new (_pool) ObjCIdentifierListAST;
            name = new (_pool) SimpleNameAST;
            name->identifier_token = identifier_token;
            (*nextId)->value = name;
            nextId = &((*nextId)->next);
        }

        match(T_SEMICOLON, &ast->semicolon_token);
        node = ast;
        return true;
    } else {
        // a protocol definition
        ObjCProtocolDeclarationAST *ast = new (_pool) ObjCProtocolDeclarationAST;
        ast->attribute_list = attributes;
        ast->protocol_token = protocol_token;
        SimpleNameAST *name = new (_pool) SimpleNameAST;
        name->identifier_token = identifier_token;
        ast->name = name;

        parseObjCProtocolRefs(ast->protocol_refs);

        DeclarationListAST **nextMembers = &(ast->member_declaration_list);
        DeclarationAST *declaration = 0;
        while (parseObjCInterfaceMemberDeclaration(declaration)) {
            *nextMembers = new (_pool) DeclarationListAST;
            (*nextMembers)->value = declaration;
            nextMembers = &((*nextMembers)->next);
        }

        match(T_AT_END, &(ast->end_token));

        node = ast;
        return true;
    }
}

// objc-implementation ::= T_AT_IMPLEMENTAION T_IDENTIFIER (T_COLON T_IDENTIFIER)?
//                         objc-class-instance-variables-opt
// objc-implementation ::= T_AT_IMPLEMENTAION T_IDENTIFIER T_LPAREN T_IDENTIFIER T_RPAREN
//
bool Parser::parseObjCImplementation(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_AT_IMPLEMENTATION)
        return false;

    unsigned implementation_token = consumeToken();
    unsigned identifier_token = 0;
    match(T_IDENTIFIER, &identifier_token);

    if (LA() == T_LPAREN) {
        // a category implementation
        ObjCClassDeclarationAST *ast = new (_pool) ObjCClassDeclarationAST;
        ast->implementation_token = implementation_token;
        SimpleNameAST *class_name = new (_pool) SimpleNameAST;
        class_name->identifier_token = identifier_token;
        ast->class_name = class_name;

        match(T_LPAREN, &(ast->lparen_token));
        SimpleNameAST *category_name = new (_pool) SimpleNameAST;
        match(T_IDENTIFIER, &(category_name->identifier_token));
        ast->category_name = category_name;
        match(T_RPAREN, &(ast->rparen_token));

        parseObjCMethodDefinitionList(ast->member_declaration_list);
        match(T_AT_END, &(ast->end_token));

        node = ast;
    } else {
        // a class implementation
        ObjCClassDeclarationAST *ast = new (_pool) ObjCClassDeclarationAST;
        ast->implementation_token = implementation_token;
        SimpleNameAST *class_name = new (_pool) SimpleNameAST;
        class_name->identifier_token = identifier_token;
        ast->class_name = class_name;

        if (LA() == T_COLON) {
            ast->colon_token = consumeToken();
            SimpleNameAST *superclass = new (_pool) SimpleNameAST;
            match(T_IDENTIFIER, &(superclass->identifier_token));
            ast->superclass = superclass;
        }

        parseObjClassInstanceVariables(ast->inst_vars_decl);
        parseObjCMethodDefinitionList(ast->member_declaration_list);
        match(T_AT_END, &(ast->end_token));

        node = ast;
    }

    return true;
}

bool Parser::parseObjCMethodDefinitionList(DeclarationListAST *&node)
{
    DEBUG_THIS_RULE();
    DeclarationListAST **next = &node;

    while (LA() && LA() != T_AT_END) {
        unsigned start = cursor();
        DeclarationAST *declaration = 0;

        switch (LA()) {
        case T_PLUS:
        case T_MINUS:
            parseObjCMethodDefinition(declaration);

            if (start == cursor())
                consumeToken();
            break;

        case T_SEMICOLON:
            consumeToken();
            break;

        case T_AT_SYNTHESIZE: {
            ObjCSynthesizedPropertiesDeclarationAST *ast = new (_pool) ObjCSynthesizedPropertiesDeclarationAST;
            ast->synthesized_token = consumeToken();
            ObjCSynthesizedPropertyListAST *last = new (_pool) ObjCSynthesizedPropertyListAST;
            ast->property_identifier_list = last;
            last->value = new (_pool) ObjCSynthesizedPropertyAST;
            match(T_IDENTIFIER, &(last->value->property_identifier));

            if (LA() == T_EQUAL) {
                last->value->equals_token = consumeToken();

                match(T_IDENTIFIER, &(last->value->property_alias_identifier));
            }

            while (LA() == T_COMMA) {
                consumeToken(); // consume T_COMMA

                last->next = new (_pool) ObjCSynthesizedPropertyListAST;
                last = last->next;

                last->value = new (_pool) ObjCSynthesizedPropertyAST;
                match(T_IDENTIFIER, &(last->value->property_identifier));

                if (LA() == T_EQUAL) {
                    last->value->equals_token = consumeToken();

                    match(T_IDENTIFIER, &(last->value->property_alias_identifier));
                }
            }

            match(T_SEMICOLON, &(ast->semicolon_token));

            declaration = ast;
            break;
        }

        case T_AT_DYNAMIC: {
            ObjCDynamicPropertiesDeclarationAST *ast = new (_pool) ObjCDynamicPropertiesDeclarationAST;
            ast->dynamic_token = consumeToken();
            ast->property_identifier_list = new (_pool) ObjCIdentifierListAST;
            SimpleNameAST *name = new (_pool) SimpleNameAST;
            match(T_IDENTIFIER, &(name->identifier_token));
            ast->property_identifier_list->value = name;

            ObjCIdentifierListAST *last = ast->property_identifier_list;
            while (LA() == T_COMMA) {
                consumeToken(); // consume T_COMMA

                last->next = new (_pool) ObjCIdentifierListAST;
                last = last->next;
                name = new (_pool) SimpleNameAST;
                match(T_IDENTIFIER, &name->identifier_token);
                last->value = name;
            }

            match(T_SEMICOLON, &(ast->semicolon_token));

            declaration = ast;
            break;
        }

        default:
            if (LA() == T_EXTERN && LA(2) == T_STRING_LITERAL) {
                parseDeclaration(declaration);
            } else {
                if (! parseBlockDeclaration(declaration)) {
                    rewind(start);
                    _translationUnit->error(cursor(),
                                            "skip token `%s'", tok().spell());

                    consumeToken();
                }
            }
            break;
        } // switch

        if (declaration) {
            *next = new (_pool) DeclarationListAST;
            (*next)->value = declaration;
            next = &((*next)->next);
        }
    }

    return true;
}

bool Parser::parseObjCMethodDefinition(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    ObjCMethodPrototypeAST *method_prototype = 0;
    if (! parseObjCMethodPrototype(method_prototype))
        return false;

    ObjCMethodDeclarationAST *ast = new (_pool) ObjCMethodDeclarationAST;
    ast->method_prototype = method_prototype;

    // Objective-C allows you to write:
    // - (void) foo; { body; }
    // so a method is a forward declaration when it doesn't have a _body_.
    // However, we still need to read the semicolon.
    if (LA() == T_SEMICOLON) {
        ast->semicolon_token = consumeToken();
    }

    parseFunctionBody(ast->function_body);

    node = ast;
    return true;
}

// objc-protocol-refs ::= T_LESS (T_IDENTIFIER @ T_COMMA) T_GREATER
//
bool Parser::parseObjCProtocolRefs(ObjCProtocolRefsAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_LESS)
        return false;

    ObjCProtocolRefsAST *ast = new (_pool) ObjCProtocolRefsAST;

    match(T_LESS, &(ast->less_token));

    unsigned identifier_token = 0;
    match(T_IDENTIFIER, &identifier_token);
    ast->identifier_list = new (_pool) ObjCIdentifierListAST;
    SimpleNameAST *name = new (_pool) SimpleNameAST;
    name->identifier_token = identifier_token;
    ast->identifier_list->value = name;
    ObjCIdentifierListAST **nextId = &(ast->identifier_list->next);

    while (LA() == T_COMMA) {
        consumeToken(); // consume T_COMMA
        match(T_IDENTIFIER, &identifier_token);

        *nextId = new (_pool) ObjCIdentifierListAST;
        name = new (_pool) SimpleNameAST;
        name->identifier_token = identifier_token;
        (*nextId)->value = name;
        nextId = &((*nextId)->next);
    }

    match(T_GREATER, &(ast->greater_token));
    node = ast;
    return true;
}

// objc-class-instance-variables ::= T_LBRACE
//                                   objc-instance-variable-decl-list-opt
//                                   T_RBRACE
//
bool Parser::parseObjClassInstanceVariables(ObjCInstanceVariablesDeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_LBRACE)
        return false;

    ObjCInstanceVariablesDeclarationAST *ast = new (_pool) ObjCInstanceVariablesDeclarationAST;
    match(T_LBRACE, &(ast->lbrace_token));

    for (DeclarationListAST **next = &(ast->instance_variable_list); LA(); next = &((*next)->next)) {
        if (LA() == T_RBRACE)
            break;

        const unsigned start = cursor();

        *next = new (_pool) DeclarationListAST;
        parseObjCInstanceVariableDeclaration((*next)->value);

        if (start == cursor()) {
            // skip stray token.
            _translationUnit->error(cursor(), "skip stray token `%s'", tok().spell());
            consumeToken();
        }
    }

    match(T_RBRACE, &(ast->rbrace_token));

    node = ast;
    return true;
}

// objc-interface-declaration ::= T_AT_REQUIRED
// objc-interface-declaration ::= T_AT_OPTIONAL
// objc-interface-declaration ::= T_SEMICOLON
// objc-interface-declaration ::= objc-property-declaration
// objc-interface-declaration ::= objc-method-prototype
bool Parser::parseObjCInterfaceMemberDeclaration(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    switch (LA()) {
    case T_AT_END:
        return false;

    case T_AT_REQUIRED:
    case T_AT_OPTIONAL:
        consumeToken();
        return true;

    case T_SEMICOLON:
        consumeToken();
        return true;

    case T_AT_PROPERTY: {
        return parseObjCPropertyDeclaration(node);
    }

    case T_PLUS:
    case T_MINUS: {
        ObjCMethodDeclarationAST *ast = new (_pool) ObjCMethodDeclarationAST;
        if (parseObjCMethodPrototype(ast->method_prototype)) {
            match(T_SEMICOLON, &(ast->semicolon_token));
            node = ast;
            return true;
        } else {
            return false;
        }
    }

    case T_ENUM:
    case T_CLASS:
    case T_STRUCT:
    case T_UNION: {
        return parseSimpleDeclaration(node, /*accept struct declarators */ true);
    }

    default: {
        return parseSimpleDeclaration(node, /*accept struct declarators */ true);
    } // default

    } // switch
}

// objc-instance-variable-declaration ::= objc-visibility-specifier
// objc-instance-variable-declaration ::= block-declaration
//
bool Parser::parseObjCInstanceVariableDeclaration(DeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    switch (LA()) {
        case T_AT_PRIVATE:
        case T_AT_PROTECTED:
        case T_AT_PUBLIC:
        case T_AT_PACKAGE: {
            ObjCVisibilityDeclarationAST *ast = new (_pool) ObjCVisibilityDeclarationAST;
            ast->visibility_token = consumeToken();
            node = ast;
            return true;
        }

        default:
            return parseSimpleDeclaration(node, true);
    }
}

// objc-property-declaration ::=
//    T_AT_PROPERTY T_LPAREN (property-attribute @ T_COMMA) T_RPAREN simple-declaration
//
bool Parser::parseObjCPropertyDeclaration(DeclarationAST *&node, SpecifierListAST *attributes)
{
    DEBUG_THIS_RULE();
    if (LA() != T_AT_PROPERTY)
        return false;

    ObjCPropertyDeclarationAST *ast = new (_pool) ObjCPropertyDeclarationAST;
    ast->attribute_list = attributes;
    ast->property_token = consumeToken();

    if (LA() == T_LPAREN) {
        match(T_LPAREN, &(ast->lparen_token));

        ObjCPropertyAttributeAST *property_attribute = 0;
        if (parseObjCPropertyAttribute(property_attribute)) {
            ast->property_attribute_list = new (_pool) ObjCPropertyAttributeListAST;
            ast->property_attribute_list->value = property_attribute;
            ObjCPropertyAttributeListAST *last = ast->property_attribute_list;

            while (LA() == T_COMMA) {
                consumeToken(); // consume T_COMMA
                last->next = new (_pool) ObjCPropertyAttributeListAST;
                last = last->next;
                if (!parseObjCPropertyAttribute(last->value)) {
                    _translationUnit->error(_tokenIndex, "expected token `%s' got `%s'",
                                            Token::name(T_IDENTIFIER), tok().spell());
                    while (LA() != T_RPAREN)
                        consumeToken();
                    break;
                }
            }
        }

        match(T_RPAREN, &(ast->rparen_token));
    }

    parseSimpleDeclaration(ast->simple_declaration, /*accept-struct-declarators = */ true);

    node = ast;
    return true;
}

// objc-method-prototype ::= (T_PLUS | T_MINUS) objc-method-decl objc-method-attrs-opt
//
// objc-method-decl ::= objc-type-name? objc-selector
// objc-method-decl ::= objc-type-name? objc-keyword-decl-list objc-parmlist-opt
//
bool Parser::parseObjCMethodPrototype(ObjCMethodPrototypeAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_PLUS && LA() != T_MINUS)
        return false;

    ObjCMethodPrototypeAST *ast = new (_pool) ObjCMethodPrototypeAST;
    ast->method_type_token = consumeToken();

    parseObjCTypeName(ast->type_name);

    if ((lookAtObjCSelector() && LA(2) == T_COLON) || LA() == T_COLON) {
        ObjCSelectorArgumentAST *argument = 0;
        ObjCMessageArgumentDeclarationAST *declaration = 0;
        parseObjCKeywordDeclaration(argument, declaration);

        ObjCSelectorWithArgumentsAST *sel = new (_pool) ObjCSelectorWithArgumentsAST;
        ast->selector = sel;
        ObjCSelectorArgumentListAST *lastSel = new (_pool) ObjCSelectorArgumentListAST;
        sel->selector_argument_list = lastSel;
        sel->selector_argument_list->value = argument;

        ast->argument_list = new (_pool) ObjCMessageArgumentDeclarationListAST;
        ast->argument_list->value = declaration;
        ObjCMessageArgumentDeclarationListAST *lastArg = ast->argument_list;

        while (parseObjCKeywordDeclaration(argument, declaration)) {
            lastSel->next = new (_pool) ObjCSelectorArgumentListAST;
            lastSel = lastSel->next;
            lastSel->value = argument;

            lastArg->next = new (_pool) ObjCMessageArgumentDeclarationListAST;
            lastArg = lastArg->next;
            lastArg->value = declaration;
        }

        while (LA() == T_COMMA) {
            consumeToken();

            if (LA() == T_DOT_DOT_DOT) {
                ast->dot_dot_dot_token = consumeToken();
                break;
            }

            // TODO: Is this still valid, and if so, should it be stored in the AST? (EV)
            DeclarationAST *parameter_declaration = 0;
            parseParameterDeclaration(parameter_declaration);
        }
    } else if (lookAtObjCSelector()) {
        ObjCSelectorWithoutArgumentsAST *sel = new (_pool) ObjCSelectorWithoutArgumentsAST;
        parseObjCSelector(sel->name_token);
        ast->selector = sel;
    } else {
        _translationUnit->error(cursor(), "expected a selector");
    }

    SpecifierListAST **attr = &ast->attribute_list;
    while (parseAttributeSpecifier(*attr))
        attr = &(*attr)->next;

    node = ast;
    return true;
}

// objc-property-attribute ::= getter '=' identifier
// objc-property-attribute ::= setter '=' identifier ':'
// objc-property-attribute ::= readonly
// objc-property-attribute ::= readwrite
// objc-property-attribute ::= assign
// objc-property-attribute ::= retain
// objc-property-attribute ::= copy
// objc-property-attribute ::= nonatomic
bool Parser::parseObjCPropertyAttribute(ObjCPropertyAttributeAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_IDENTIFIER)
        return false;

    node = new (_pool) ObjCPropertyAttributeAST;

    const Identifier *id = tok().identifier;
    const int k = classifyObjectiveCTypeQualifiers(id->chars(), id->size());
    switch (k) {
    case Token_copy:
    case Token_assign:
    case Token_retain:
    case Token_readonly:
    case Token_readwrite:
    case Token_nonatomic:
        node->attribute_identifier_token = consumeToken();
        return true;

    case Token_getter: {
        node->attribute_identifier_token = consumeToken();
        match(T_EQUAL, &(node->equals_token));
        ObjCSelectorWithoutArgumentsAST *selector = new (_pool) ObjCSelectorWithoutArgumentsAST;
        match(T_IDENTIFIER, &(selector->name_token));
        node->method_selector = selector;
        return true;
    }

    case Token_setter: {
        node->attribute_identifier_token = consumeToken();
        match(T_EQUAL, &(node->equals_token));
        ObjCSelectorWithArgumentsAST *selector = new (_pool) ObjCSelectorWithArgumentsAST;
        selector->selector_argument_list = new (_pool) ObjCSelectorArgumentListAST;
        selector->selector_argument_list->value = new (_pool) ObjCSelectorArgumentAST;
        match(T_IDENTIFIER, &(selector->selector_argument_list->value->name_token));
        match(T_COLON, &(selector->selector_argument_list->value->colon_token));
        node->method_selector = selector;
        return true;
    }

    default:
        return false;
    }
}

// objc-type-name ::= T_LPAREN objc-type-qualifiers-opt type-id T_RPAREN
//
bool Parser::parseObjCTypeName(ObjCTypeNameAST *&node)
{
    DEBUG_THIS_RULE();
    if (LA() != T_LPAREN)
        return false;

    ObjCTypeNameAST *ast = new (_pool) ObjCTypeNameAST;
    match(T_LPAREN, &(ast->lparen_token));
    parseObjCTypeQualifiers(ast->type_qualifier);
    parseTypeId(ast->type_id);
    match(T_RPAREN, &(ast->rparen_token));
    node = ast;
    return true;
}

// objc-selector ::= T_IDENTIFIER | keyword
//
bool Parser::parseObjCSelector(unsigned &selector_token)
{
    DEBUG_THIS_RULE();
    if (! lookAtObjCSelector())
        return false;

    selector_token = consumeToken();
    return true;
}

// objc-keyword-decl ::= objc-selector? T_COLON objc-type-name? objc-keyword-attributes-opt T_IDENTIFIER
//
bool Parser::parseObjCKeywordDeclaration(ObjCSelectorArgumentAST *&argument, ObjCMessageArgumentDeclarationAST *&node)
{
    DEBUG_THIS_RULE();
    if (! (LA() == T_COLON || (lookAtObjCSelector() && LA(2) == T_COLON)))
        return false;

    node = new (_pool) ObjCMessageArgumentDeclarationAST;
    argument = new (_pool) ObjCSelectorArgumentAST;

    parseObjCSelector(argument->name_token);
    match(T_COLON, &(argument->colon_token));

    parseObjCTypeName(node->type_name);

    SpecifierListAST **attr = &node->attribute_list;
    while (parseAttributeSpecifier(*attr))
        attr = &(*attr)->next;

    match(T_IDENTIFIER, &(node->param_name_token));

    return true;
}

bool Parser::parseObjCTypeQualifiers(unsigned &type_qualifier)
{
    DEBUG_THIS_RULE();
    if (LA() != T_IDENTIFIER)
        return false;

    const Identifier *id = tok().identifier;
    const int k = classifyObjectiveCTypeQualifiers(id->chars(), id->size());
    if (k == Token_identifier)
        return false;
    type_qualifier = consumeToken();
    return true;
}

bool Parser::peekAtObjCContextKeyword(int kind)
{
    if (LA() != T_IDENTIFIER)
        return false;

    const Identifier *id = tok().identifier;
    const int k = classifyObjectiveCTypeQualifiers(id->chars(), id->size());
    return k == kind;
}

bool Parser::parseObjCContextKeyword(int kind, unsigned &in_token)
{
    DEBUG_THIS_RULE();

    if (peekAtObjCContextKeyword(kind)) {
        in_token = consumeToken();
        return true;
    } else {
        return false;
    }
}


