
#line 271 "./glsl.g"

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "glslparsertable_p.h"
#include "glsllexer.h"
#include "glslast.h"
#include "glslengine.h"
#include <vector>
#include <stack>

namespace GLSL {

class GLSL_EXPORT Parser: public GLSLParserTable
{
public:
    union Value {
        void *ptr;
        const QString *string;
        AST *ast;
        List<AST *> *ast_list;
        DeclarationAST *declaration;
        List<DeclarationAST *> *declaration_list;
        ExpressionAST *expression;
        List<ExpressionAST *> *expression_list;
        StatementAST *statement;
        List<StatementAST *> *statement_list;
        TypeAST *type;
        StructTypeAST::Field *field;
        List<StructTypeAST::Field *> *field_list;
        TranslationUnitAST *translation_unit;
        FunctionIdentifierAST *function_identifier;
        List<ArrayTypeAST::ArraySpecAST *> *array_specifier;
        AST::Kind kind;
        TypeAST::Precision precision;
        struct {
            StatementAST *thenClause;
            StatementAST *elseClause;
        } ifstmt;
        struct {
            ExpressionAST *condition;
            ExpressionAST *increment;
        } forstmt;
        struct {
            FunctionIdentifierAST *id;
            List<ExpressionAST *> *arguments;
        } function;
        int qualifier;
        LayoutQualifierAST *layout;
        List<LayoutQualifierAST *> *layout_list;
        struct {
            TypeAST::Precision precision;
            int qualifier;
            List<LayoutQualifierAST *> *layout_list;
            List<NamedTypeAST *> *type_name_list;
        } type_qualifier;
        struct {
            TypeAST *type;
            const QString *name;
        } param_declarator;
        ParameterDeclarationAST *param_declaration;
        FunctionDeclarationAST *function_declaration;
        InterfaceBlockAST *interface_block;
        List<IdentifierExpressionAST *> *identifier_list;
        List<NamedTypeAST *> *type_name_list;
    };

    Parser(Engine *engine, const char *source, unsigned size, int variant);
    ~Parser();

    TranslationUnitAST *parse() {
        if (AST *u = parse(T_FEED_GLSL))
            return u->asTranslationUnit();
        return nullptr;
    }

    ExpressionAST *parseExpression() {
        if (AST *u = parse(T_FEED_EXPRESSION))
            return u->asExpression();
        return nullptr;
    }

    AST *parse(int startToken);

private:
    // 1-based
    int &location(int n) { return _locationStack[_tos + n - 1]; }
    Value &sym(int n) { return _symStack[_tos + n - 1]; }
    AST *&ast(int n) { return _symStack[_tos + n - 1].ast; }
    const QString *&string(int n) { return _symStack[_tos + n - 1].string; }
    ExpressionAST *&expression(int n) { return _symStack[_tos + n - 1].expression; }
    StatementAST *&statement(int n) { return _symStack[_tos + n - 1].statement; }
    TypeAST *&type(int n) { return _symStack[_tos + n - 1].type; }
    FunctionDeclarationAST *&function(int n) { return _symStack[_tos + n - 1].function_declaration; }

    inline int consumeToken() {
        if (_index < int(_tokens.size()))
            return _index++;
        return static_cast<int>(_tokens.size()) - 1;
    }
    inline const Token &tokenAt(int index) const {
        if (index == 0)
            return _startToken;
        return _tokens.at(index);
    }
    inline int tokenKind(int index) const {
        if (index == 0)
            return _startToken.kind;
        return _tokens.at(index).kind;
    }
    void reduce(int ruleno);

    void warning(const DiagnosticMessage::Location &loc, const QString &message)
    {
        _engine->warning(loc, message);
    }

    void error(const DiagnosticMessage::Location &loc, const QString &message)
    {
        _engine->error(loc, message);
    }

    static bool isInterfaceBlockStorageIdentifier(int qualifier)
    {
        qualifier = qualifier & QualifiedTypeAST::StorageMask;
        return (qualifier == QualifiedTypeAST::In
                || qualifier == QualifiedTypeAST::Out
                || qualifier == QualifiedTypeAST::Uniform
                || qualifier == QualifiedTypeAST::Buffer
                || qualifier == (QualifiedTypeAST::Centroid | QualifiedTypeAST::In)
                || qualifier == (QualifiedTypeAST::Centroid | QualifiedTypeAST::Out)
                || qualifier == (QualifiedTypeAST::Patch | QualifiedTypeAST::In)
                || qualifier == (QualifiedTypeAST::Patch | QualifiedTypeAST::Out)
                || qualifier == (QualifiedTypeAST::Sample | QualifiedTypeAST::In)
                || qualifier == (QualifiedTypeAST::Sample | QualifiedTypeAST::Out));
    }

    template <typename T>
    T *makeAstNode()
    {
        T *node = new (_engine->pool()) T ();
        setLocationFromToken(node, yyloc);
        return node;
    }

    template <typename T, typename A1>
    T *makeAstNode(A1 a1)
    {
        T *node = new (_engine->pool()) T (a1);
        const DiagnosticMessage::Location &location = locationFromToken(yyloc);
        node->lineno = location.line;
        node->position = location.position;
        node->length = location.length;
        return node;
    }

    template <typename T, typename A1, typename A2>
    T *makeAstNode(A1 a1, A2 a2)
    {
        T *node = new (_engine->pool()) T (a1, a2);
        const DiagnosticMessage::Location &location = locationFromToken(yyloc);
        node->lineno = location.line;
        node->position = location.position;
        node->length = location.length;
        return node;
    }

    template <typename T, typename A1, typename A2, typename A3>
    T *makeAstNode(A1 a1, A2 a2, A3 a3)
    {
        T *node = new (_engine->pool()) T (a1, a2, a3);
        setLocationFromToken(node, yyloc);
        return node;
    }

    template <typename T, typename A1, typename A2, typename A3, typename A4>
    T *makeAstNode(A1 a1, A2 a2, A3 a3, A4 a4)
    {
        T *node = new (_engine->pool()) T (a1, a2, a3, a4);
        setLocationFromToken(node, yyloc);
        return node;
    }

    TypeAST *makeBasicType(int token)
    {
        TypeAST *type = new (_engine->pool()) BasicTypeAST(token, spell[token]);
        setLocationFromToken(type, yyloc);
        return type;
    }

private:
    DiagnosticMessage::Location locationFromToken(int index) const
    {
        const Token &token = index > -1 ? tokenAt(index) : Token();
        return DiagnosticMessage::Location{token.line + 1, token.position, token.length};
    }

    void setLocationFromToken(AST *node, int index) const
    {
        const DiagnosticMessage::Location &location = locationFromToken(index);
        node->lineno = location.line;
        node->position = location.position;
        node->length = location.length;
    }

    void setLocationFromTokens(AST *node, int index, int endIndex)
    {
        const Token &token = tokenAt(index);
        node->lineno = token.line + 1;
        node->position = token.position;
        node->length = tokenAt(endIndex).end() - node->position;
    }

    Engine *_engine;
    int _tos;
    int _index;
    int yyloc;
    int yytoken;
    int yyrecovering;
    bool _recovered;
    Token _startToken;
    std::vector<int> _stateStack;
    std::vector<int> _locationStack;
    std::vector<Value> _symStack;
    std::vector<Token> _tokens;
};

} // namespace GLSL
