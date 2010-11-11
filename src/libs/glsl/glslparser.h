
#line 212 "./glsl.g"

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
#include <vector>
#include <stack>

namespace GLSL {

class GLSL_EXPORT Parser: public GLSLParserTable
{
public:
    union Value {
        void *ptr;
        const std::string *string;
        AST *ast;
        List<AST *> *ast_list;
        Declaration *declaration;
        List<Declaration *> *declaration_list;
        Expression *expression;
        List<Expression *> *expression_list;
        TranslationUnit *translation_unit;
        // ### ast nodes...
    };

    Parser(Engine *engine, const char *source, unsigned size, int variant);
    ~Parser();

    TranslationUnit *parse();

private:
    // 1-based
    Value &sym(int n) { return _symStack[_tos + n - 1]; }
    AST *&ast(int n) { return _symStack[_tos + n - 1].ast; }
    const std::string *&string(int n) { return _symStack[_tos + n - 1].string; }

    inline int consumeToken() { return _index++; }
    inline const Token &tokenAt(int index) const { return _tokens.at(index); }
    inline int tokenKind(int index) const { return _tokens.at(index).kind; }
    void reduce(int ruleno);

private:
    Engine *_engine;
    int _tos;
    int _index;
    std::vector<int> _stateStack;
    std::vector<int> _locationStack;
    std::vector<Value> _symStack;
    std::vector<Token> _tokens;
};

} // end of namespace GLSL
