
#line 215 "./glsl.g"

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
            int qualifier;
            List<LayoutQualifierAST *> *layout_list;
        } type_qualifier;
        struct {
            TypeAST *type;
            const QString *name;
        } param_declarator;
        ParameterDeclarationAST *param_declaration;
        FunctionDeclarationAST *function_declaration;
    };

    Parser(Engine *engine, const char *source, unsigned size, int variant);
    ~Parser();

    TranslationUnitAST *parse() {
        if (AST *u = parse(T_FEED_GLSL))
            return u->asTranslationUnit();
        return 0;
    }

    ExpressionAST *parseExpression() {
        if (AST *u = parse(T_FEED_EXPRESSION))
            return u->asExpression();
        return 0;
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
        return _tokens.size() - 1;
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

    void warning(int line, const QString &message)
    {
        _engine->warning(line, message);
    }

    void error(int line, const QString &message)
    {
        _engine->error(line, message);
    }

    template <typename T>
    T *makeAstNode()
    {
        T *node = new (_engine->pool()) T ();
        node->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return node;
    }

    template <typename T, typename A1>
    T *makeAstNode(A1 a1)
    {
        T *node = new (_engine->pool()) T (a1);
        node->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return node;
    }

    template <typename T, typename A1, typename A2>
    T *makeAstNode(A1 a1, A2 a2)
    {
        T *node = new (_engine->pool()) T (a1, a2);
        node->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return node;
    }

    template <typename T, typename A1, typename A2, typename A3>
    T *makeAstNode(A1 a1, A2 a2, A3 a3)
    {
        T *node = new (_engine->pool()) T (a1, a2, a3);
        node->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return node;
    }

    template <typename T, typename A1, typename A2, typename A3, typename A4>
    T *makeAstNode(A1 a1, A2 a2, A3 a3, A4 a4)
    {
        T *node = new (_engine->pool()) T (a1, a2, a3, a4);
        node->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return node;
    }

    TypeAST *makeBasicType(int token)
    {
        TypeAST *type = new (_engine->pool()) BasicTypeAST(token, spell[token]);
        type->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return type;
    }

private:
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
