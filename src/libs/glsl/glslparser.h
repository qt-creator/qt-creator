
#line 214 "./glsl.g"

/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
        Declaration *declaration;
        List<Declaration *> *declaration_list;
        Expression *expression;
        List<Expression *> *expression_list;
        Statement *statement;
        List<Statement *> *statement_list;
        Type *type;
        StructType::Field *field;
        List<StructType::Field *> *field_list;
        TranslationUnit *translation_unit;
        FunctionIdentifier *function_identifier;
        AST::Kind kind;
        Type::Precision precision;
        struct {
            Statement *thenClause;
            Statement *elseClause;
        } ifstmt;
        struct {
            Expression *condition;
            Expression *increment;
        } forstmt;
        struct {
            FunctionIdentifier *id;
            List<Expression *> *arguments;
        } function;
        int qualifier;
        LayoutQualifier *layout;
        List<LayoutQualifier *> *layout_list;
        struct {
            int qualifier;
            List<LayoutQualifier *> *layout_list;
        } type_qualifier;
        struct {
            Type *type;
            const QString *name;
        } param_declarator;
        ParameterDeclaration *param_declaration;
        FunctionDeclaration *function_declaration;
    };

    Parser(Engine *engine, const char *source, unsigned size, int variant);
    ~Parser();

    TranslationUnit *parse();

private:
    // 1-based
    Value &sym(int n) { return _symStack[_tos + n - 1]; }
    AST *&ast(int n) { return _symStack[_tos + n - 1].ast; }
    const QString *&string(int n) { return _symStack[_tos + n - 1].string; }
    Expression *&expression(int n) { return _symStack[_tos + n - 1].expression; }
    Statement *&statement(int n) { return _symStack[_tos + n - 1].statement; }
    Type *&type(int n) { return _symStack[_tos + n - 1].type; }
    FunctionDeclaration *&function(int n) { return _symStack[_tos + n - 1].function_declaration; }

    inline int consumeToken() { return _index++; }
    inline const Token &tokenAt(int index) const { return _tokens.at(index); }
    inline int tokenKind(int index) const { return _tokens.at(index).kind; }
    void reduce(int ruleno);

    void warning(int line, const QString &message)
    {
        DiagnosticMessage m;
        m.setKind(DiagnosticMessage::Warning);
        m.setLine(line);
        m.setMessage(message);
        _engine->addDiagnosticMessage(m);
    }

    void error(int line, const QString &message)
    {
        DiagnosticMessage m;
        m.setKind(DiagnosticMessage::Error);
        m.setLine(line);
        m.setMessage(message);
        _engine->addDiagnosticMessage(m);
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

    Type *makeBasicType(int token, BasicType::Category category)
    {
        Type *type = new (_engine->pool()) BasicType(token, spell[token], category);
        type->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return type;
    }

private:
    Engine *_engine;
    int _tos;
    int _index;
    int yyloc;
    std::vector<int> _stateStack;
    std::vector<int> _locationStack;
    std::vector<Value> _symStack;
    std::vector<Token> _tokens;
};

} // end of namespace GLSL
