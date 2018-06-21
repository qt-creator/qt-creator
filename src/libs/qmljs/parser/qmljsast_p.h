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

#pragma once

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

#include "qmljsastvisitor_p.h"
#include "qmljsglobal_p.h"
#include "qmljsmemorypool_p.h"

#include <QtCore/qstring.h>

QT_QML_BEGIN_NAMESPACE

#define QMLJS_DECLARE_AST_NODE(name) \
  enum { K = Kind_##name };

namespace QSOperator // ### rename
{

enum Op {
    Add,
    And,
    InplaceAnd,
    Assign,
    BitAnd,
    BitOr,
    BitXor,
    InplaceSub,
    Div,
    InplaceDiv,
    Equal,
    Ge,
    Gt,
    In,
    InplaceAdd,
    InstanceOf,
    Le,
    LShift,
    InplaceLeftShift,
    Lt,
    Mod,
    InplaceMod,
    Mul,
    InplaceMul,
    NotEqual,
    Or,
    InplaceOr,
    RShift,
    InplaceRightShift,
    StrictEqual,
    StrictNotEqual,
    Sub,
    URShift,
    InplaceURightShift,
    InplaceXor
};

} // namespace QSOperator

namespace QmlJS {

namespace AST {

template <typename T1, typename T2>
T1 cast(T2 *ast)
{
    if (ast && ast->kind == static_cast<T1>(0)->K)
        return static_cast<T1>(ast);

    return 0;
}

class QML_PARSER_EXPORT Node: public Managed
{
public:
    enum Kind {
        Kind_Undefined,

        Kind_ArgumentList,
        Kind_ArrayLiteral,
        Kind_ArrayMemberExpression,
        Kind_BinaryExpression,
        Kind_Block,
        Kind_BreakStatement,
        Kind_CallExpression,
        Kind_CaseBlock,
        Kind_CaseClause,
        Kind_CaseClauses,
        Kind_Catch,
        Kind_ConditionalExpression,
        Kind_ContinueStatement,
        Kind_DebuggerStatement,
        Kind_DefaultClause,
        Kind_DeleteExpression,
        Kind_DoWhileStatement,
        Kind_ElementList,
        Kind_Elision,
        Kind_EmptyStatement,
        Kind_Expression,
        Kind_ExpressionStatement,
        Kind_FalseLiteral,
        Kind_FieldMemberExpression,
        Kind_Finally,
        Kind_ForEachStatement,
        Kind_ForStatement,
        Kind_FormalParameterList,
        Kind_FunctionBody,
        Kind_FunctionDeclaration,
        Kind_FunctionExpression,
        Kind_FunctionSourceElement,
        Kind_IdentifierExpression,
        Kind_IdentifierPropertyName,
        Kind_IfStatement,
        Kind_LabelledStatement,
        Kind_LocalForEachStatement,
        Kind_LocalForStatement,
        Kind_NewExpression,
        Kind_NewMemberExpression,
        Kind_NotExpression,
        Kind_NullExpression,
        Kind_NumericLiteral,
        Kind_NumericLiteralPropertyName,
        Kind_ObjectLiteral,
        Kind_PostDecrementExpression,
        Kind_PostIncrementExpression,
        Kind_PreDecrementExpression,
        Kind_PreIncrementExpression,
        Kind_Program,
        Kind_PropertyAssignmentList,
        Kind_PropertyGetterSetter,
        Kind_PropertyName,
        Kind_PropertyNameAndValue,
        Kind_RegExpLiteral,
        Kind_ReturnStatement,
        Kind_SourceElement,
        Kind_SourceElements,
        Kind_StatementList,
        Kind_StatementSourceElement,
        Kind_StringLiteral,
        Kind_StringLiteralPropertyName,
        Kind_SwitchStatement,
        Kind_ThisExpression,
        Kind_ThrowStatement,
        Kind_TildeExpression,
        Kind_TrueLiteral,
        Kind_TryStatement,
        Kind_TypeOfExpression,
        Kind_UnaryMinusExpression,
        Kind_UnaryPlusExpression,
        Kind_VariableDeclaration,
        Kind_VariableDeclarationList,
        Kind_VariableStatement,
        Kind_VoidExpression,
        Kind_WhileStatement,
        Kind_WithStatement,
        Kind_NestedExpression,

        Kind_UiArrayBinding,
        Kind_UiImport,
        Kind_UiObjectBinding,
        Kind_UiObjectDefinition,
        Kind_UiObjectInitializer,
        Kind_UiObjectMemberList,
        Kind_UiArrayMemberList,
        Kind_UiPragma,
        Kind_UiProgram,
        Kind_UiParameterList,
        Kind_UiPublicMember,
        Kind_UiQualifiedId,
        Kind_UiQualifiedPragmaId,
        Kind_UiScriptBinding,
        Kind_UiSourceElement,
        Kind_UiHeaderItemList,
        Kind_UiEnumDeclaration,
        Kind_UiEnumMemberList
    };

    inline Node()
        : kind(Kind_Undefined) {}

    // NOTE: node destructors are never called,
    //       instead we block free the memory
    //       (see the NodePool class)
    virtual ~Node() {}

    virtual ExpressionNode *expressionCast();
    virtual BinaryExpression *binaryExpressionCast();
    virtual Statement *statementCast();
    virtual UiObjectMember *uiObjectMemberCast();

    void accept(Visitor *visitor);
    static void accept(Node *node, Visitor *visitor);

    inline static void acceptChild(Node *node, Visitor *visitor)
    { accept(node, visitor); } // ### remove

    virtual void accept0(Visitor *visitor) = 0;
    virtual SourceLocation firstSourceLocation() const = 0;
    virtual SourceLocation lastSourceLocation() const = 0;

// attributes
    int kind;
};

class QML_PARSER_EXPORT ExpressionNode: public Node
{
public:
    ExpressionNode() {}

    ExpressionNode *expressionCast() override;
};

class QML_PARSER_EXPORT Statement: public Node
{
public:
    Statement() {}

    Statement *statementCast() override;
};

class QML_PARSER_EXPORT NestedExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(NestedExpression)

    NestedExpression(ExpressionNode *expression)
        : expression(expression)
    { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return lparenToken; }

    SourceLocation lastSourceLocation() const override
    { return rparenToken; }

// attributes
    ExpressionNode *expression;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT ThisExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(ThisExpression)

    ThisExpression() { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return thisToken; }

    SourceLocation lastSourceLocation() const override
    { return thisToken; }

// attributes
    SourceLocation thisToken;
};

class QML_PARSER_EXPORT IdentifierExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(IdentifierExpression)

    IdentifierExpression(const QStringRef &n):
        name (n) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return identifierToken; }

// attributes
    QStringRef name;
    SourceLocation identifierToken;
};

class QML_PARSER_EXPORT NullExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(NullExpression)

    NullExpression() { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return nullToken; }

    SourceLocation lastSourceLocation() const override
    { return nullToken; }

// attributes
    SourceLocation nullToken;
};

class QML_PARSER_EXPORT TrueLiteral: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(TrueLiteral)

    TrueLiteral() { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return trueToken; }

    SourceLocation lastSourceLocation() const override
    { return trueToken; }

// attributes
    SourceLocation trueToken;
};

class QML_PARSER_EXPORT FalseLiteral: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(FalseLiteral)

    FalseLiteral() { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return falseToken; }

    SourceLocation lastSourceLocation() const override
    { return falseToken; }

// attributes
    SourceLocation falseToken;
};

class QML_PARSER_EXPORT NumericLiteral: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(NumericLiteral)

    NumericLiteral(double v):
        value(v) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return literalToken; }

    SourceLocation lastSourceLocation() const override
    { return literalToken; }

// attributes:
    double value;
    SourceLocation literalToken;
};

class QML_PARSER_EXPORT StringLiteral: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(StringLiteral)

    StringLiteral(const QStringRef &v):
        value (v) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return literalToken; }

    SourceLocation lastSourceLocation() const override
    { return literalToken; }

// attributes:
    QStringRef value;
    SourceLocation literalToken;
};

class QML_PARSER_EXPORT RegExpLiteral: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(RegExpLiteral)

    RegExpLiteral(const QStringRef &p, int f):
        pattern (p), flags (f) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return literalToken; }

    SourceLocation lastSourceLocation() const override
    { return literalToken; }

// attributes:
    QStringRef pattern;
    int flags;
    SourceLocation literalToken;
};

class QML_PARSER_EXPORT ArrayLiteral: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(ArrayLiteral)

    ArrayLiteral(Elision *e):
        elements (0), elision (e)
        { kind = K; }

    ArrayLiteral(ElementList *elts):
        elements (elts), elision (0)
        { kind = K; }

    ArrayLiteral(ElementList *elts, Elision *e):
        elements (elts), elision (e)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return lbracketToken; }

    SourceLocation lastSourceLocation() const override
    { return rbracketToken; }

// attributes
    ElementList *elements;
    Elision *elision;
    SourceLocation lbracketToken;
    SourceLocation commaToken;
    SourceLocation rbracketToken;
};

class QML_PARSER_EXPORT ObjectLiteral: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(ObjectLiteral)

    ObjectLiteral():
        properties (0) { kind = K; }

    ObjectLiteral(PropertyAssignmentList *plist):
        properties (plist) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return lbraceToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

// attributes
    PropertyAssignmentList *properties;
    SourceLocation lbraceToken;
    SourceLocation rbraceToken;
};

class QML_PARSER_EXPORT Elision: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(Elision)

    Elision():
        next (this) { kind = K; }

    Elision(Elision *previous)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return commaToken; }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() : commaToken; }

    inline Elision *finish ()
    {
        Elision *front = next;
        next = 0;
        return front;
    }

// attributes
    Elision *next;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT ElementList: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(ElementList)

    ElementList(Elision *e, ExpressionNode *expr):
        elision (e), expression (expr), next (this)
    { kind = K; }

    ElementList(ElementList *previous, Elision *e, ExpressionNode *expr):
        elision (e), expression (expr)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    inline ElementList *finish ()
    {
        ElementList *front = next;
        next = 0;
        return front;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    {
        if (elision)
            return elision->firstSourceLocation();
        return expression->firstSourceLocation();
    }

    SourceLocation lastSourceLocation() const override
    {
        if (next)
            return next->lastSourceLocation();
        return expression->lastSourceLocation();
    }

// attributes
    Elision *elision;
    ExpressionNode *expression;
    ElementList *next;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT PropertyName: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(PropertyName)

    PropertyName() { kind = K; }

    SourceLocation firstSourceLocation() const override
    { return propertyNameToken; }

    SourceLocation lastSourceLocation() const override
    { return propertyNameToken; }

    virtual QString asString() const = 0;

// attributes
    SourceLocation propertyNameToken;
};

class QML_PARSER_EXPORT PropertyAssignment: public Node
{
public:
    PropertyAssignment(PropertyName *n)
        : name(n)
    {}
// attributes
    PropertyName *name;
};

class QML_PARSER_EXPORT PropertyAssignmentList: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(PropertyAssignmentList)

    PropertyAssignmentList(PropertyAssignment *assignment)
        : assignment(assignment)
        , next(this)
    { kind = K; }

    PropertyAssignmentList(PropertyAssignmentList *previous, PropertyAssignment *assignment)
        : assignment(assignment)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    inline PropertyAssignmentList *finish ()
    {
        PropertyAssignmentList *front = next;
        next = 0;
        return front;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return assignment->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() : assignment->lastSourceLocation(); }

// attributes
    PropertyAssignment *assignment;
    PropertyAssignmentList *next;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT PropertyNameAndValue: public PropertyAssignment
{
public:
    QMLJS_DECLARE_AST_NODE(PropertyNameAndValue)

    PropertyNameAndValue(PropertyName *n, ExpressionNode *v)
        : PropertyAssignment(n), value(v)
    { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return name->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return value->lastSourceLocation(); }

// attributes
    SourceLocation colonToken;
    ExpressionNode *value;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT PropertyGetterSetter: public PropertyAssignment
{
public:
    QMLJS_DECLARE_AST_NODE(PropertyGetterSetter)

    enum Type {
        Getter,
        Setter
    };

    PropertyGetterSetter(PropertyName *n, FunctionBody *b)
        : PropertyAssignment(n), type(Getter), formals(0), functionBody (b)
    { kind = K; }

    PropertyGetterSetter(PropertyName *n, FormalParameterList *f, FunctionBody *b)
        : PropertyAssignment(n), type(Setter), formals(f), functionBody (b)
    { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return getSetToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

// attributes
    Type type;
    SourceLocation getSetToken;
    SourceLocation lparenToken;
    FormalParameterList *formals;
    SourceLocation rparenToken;
    SourceLocation lbraceToken;
    FunctionBody *functionBody;
    SourceLocation rbraceToken;
};

class QML_PARSER_EXPORT IdentifierPropertyName: public PropertyName
{
public:
    QMLJS_DECLARE_AST_NODE(IdentifierPropertyName)

    IdentifierPropertyName(const QStringRef &n):
        id (n) { kind = K; }

    void accept0(Visitor *visitor) override;

    QString asString() const override { return id.toString(); }

// attributes
    QStringRef id;
};

class QML_PARSER_EXPORT StringLiteralPropertyName: public PropertyName
{
public:
    QMLJS_DECLARE_AST_NODE(StringLiteralPropertyName)

    StringLiteralPropertyName(const QStringRef &n):
        id (n) { kind = K; }

    void accept0(Visitor *visitor) override;

    QString asString() const override { return id.toString(); }

// attributes
    QStringRef id;
};

class QML_PARSER_EXPORT NumericLiteralPropertyName: public PropertyName
{
public:
    QMLJS_DECLARE_AST_NODE(NumericLiteralPropertyName)

    NumericLiteralPropertyName(double n):
        id (n) { kind = K; }

    void accept0(Visitor *visitor) override;

    QString asString() const override { return QString::number(id, 'g', 16); }

// attributes
    double id;
};

class QML_PARSER_EXPORT ArrayMemberExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(ArrayMemberExpression)

    ArrayMemberExpression(ExpressionNode *b, ExpressionNode *e):
        base (b), expression (e)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return base->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return rbracketToken; }

// attributes
    ExpressionNode *base;
    ExpressionNode *expression;
    SourceLocation lbracketToken;
    SourceLocation rbracketToken;
};

class QML_PARSER_EXPORT FieldMemberExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(FieldMemberExpression)

    FieldMemberExpression(ExpressionNode *b, const QStringRef &n):
        base (b), name (n)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return base->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return identifierToken; }

    // attributes
    ExpressionNode *base;
    QStringRef name;
    SourceLocation dotToken;
    SourceLocation identifierToken;
};

class QML_PARSER_EXPORT NewMemberExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(NewMemberExpression)

    NewMemberExpression(ExpressionNode *b, ArgumentList *a):
        base (b), arguments (a)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return newToken; }

    SourceLocation lastSourceLocation() const override
    { return rparenToken; }

    // attributes
    ExpressionNode *base;
    ArgumentList *arguments;
    SourceLocation newToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT NewExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(NewExpression)

    NewExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return newToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation newToken;
};

class QML_PARSER_EXPORT CallExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(CallExpression)

    CallExpression(ExpressionNode *b, ArgumentList *a):
        base (b), arguments (a)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return base->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return rparenToken; }

// attributes
    ExpressionNode *base;
    ArgumentList *arguments;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT ArgumentList: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(ArgumentList)

    ArgumentList(ExpressionNode *e):
        expression (e), next (this)
        { kind = K; }

    ArgumentList(ArgumentList *previous, ExpressionNode *e):
        expression (e)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return expression->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    {
        if (next)
            return next->lastSourceLocation();
        return expression->lastSourceLocation();
    }

    inline ArgumentList *finish ()
    {
        ArgumentList *front = next;
        next = 0;
        return front;
    }

// attributes
    ExpressionNode *expression;
    ArgumentList *next;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT PostIncrementExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(PostIncrementExpression)

    PostIncrementExpression(ExpressionNode *b):
        base (b) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return base->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return incrementToken; }

// attributes
    ExpressionNode *base;
    SourceLocation incrementToken;
};

class QML_PARSER_EXPORT PostDecrementExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(PostDecrementExpression)

    PostDecrementExpression(ExpressionNode *b):
        base (b) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return base->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return decrementToken; }

// attributes
    ExpressionNode *base;
    SourceLocation decrementToken;
};

class QML_PARSER_EXPORT DeleteExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(DeleteExpression)

    DeleteExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return deleteToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation deleteToken;
};

class QML_PARSER_EXPORT VoidExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(VoidExpression)

    VoidExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return voidToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation voidToken;
};

class QML_PARSER_EXPORT TypeOfExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(TypeOfExpression)

    TypeOfExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return typeofToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation typeofToken;
};

class QML_PARSER_EXPORT PreIncrementExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(PreIncrementExpression)

    PreIncrementExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return incrementToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation incrementToken;
};

class QML_PARSER_EXPORT PreDecrementExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(PreDecrementExpression)

    PreDecrementExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return decrementToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation decrementToken;
};

class QML_PARSER_EXPORT UnaryPlusExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(UnaryPlusExpression)

    UnaryPlusExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return plusToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation plusToken;
};

class QML_PARSER_EXPORT UnaryMinusExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(UnaryMinusExpression)

    UnaryMinusExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return minusToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation minusToken;
};

class QML_PARSER_EXPORT TildeExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(TildeExpression)

    TildeExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return tildeToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation tildeToken;
};

class QML_PARSER_EXPORT NotExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(NotExpression)

    NotExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return notToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation notToken;
};

class QML_PARSER_EXPORT BinaryExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(BinaryExpression)

    BinaryExpression(ExpressionNode *l, int o, ExpressionNode *r):
        left (l), op (o), right (r)
        { kind = K; }

    BinaryExpression *binaryExpressionCast() override;

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return left->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return right->lastSourceLocation(); }

// attributes
    ExpressionNode *left;
    int op;
    ExpressionNode *right;
    SourceLocation operatorToken;
};

class QML_PARSER_EXPORT ConditionalExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(ConditionalExpression)

    ConditionalExpression(ExpressionNode *e, ExpressionNode *t, ExpressionNode *f):
        expression (e), ok (t), ko (f)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return expression->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return ko->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    ExpressionNode *ok;
    ExpressionNode *ko;
    SourceLocation questionToken;
    SourceLocation colonToken;
};

class QML_PARSER_EXPORT Expression: public ExpressionNode // ### rename
{
public:
    QMLJS_DECLARE_AST_NODE(Expression)

    Expression(ExpressionNode *l, ExpressionNode *r):
        left (l), right (r) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return left->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return right->lastSourceLocation(); }

// attributes
    ExpressionNode *left;
    ExpressionNode *right;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT Block: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(Block)

    Block(StatementList *slist):
        statements (slist) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return lbraceToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

    // attributes
    StatementList *statements;
    SourceLocation lbraceToken;
    SourceLocation rbraceToken;
};

class QML_PARSER_EXPORT StatementList: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(StatementList)

    StatementList(Statement *stmt):
        statement (stmt), next (this)
        { kind = K; }

    StatementList(StatementList *previous, Statement *stmt):
        statement (stmt)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return statement->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() : statement->lastSourceLocation(); }

    inline StatementList *finish ()
    {
        StatementList *front = next;
        next = 0;
        return front;
    }

// attributes
    Statement *statement;
    StatementList *next;
};

class QML_PARSER_EXPORT VariableStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(VariableStatement)

    VariableStatement(VariableDeclarationList *vlist):
        declarations (vlist)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return declarationKindToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    VariableDeclarationList *declarations;
    SourceLocation declarationKindToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT VariableDeclaration: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(VariableDeclaration)

    enum VariableScope {
        FunctionScope,
        BlockScope, // let
        ReadOnlyBlockScope // const
    };

    VariableDeclaration(const QStringRef &n, ExpressionNode *e, VariableScope s):
        name (n), expression (e), scope(s)
        { kind = K; }

    bool isLexicallyScoped() const { return scope != FunctionScope; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return expression ? expression->lastSourceLocation() : identifierToken; }

// attributes
    QStringRef name;
    ExpressionNode *expression;
    SourceLocation identifierToken;
    VariableScope scope;
};

class QML_PARSER_EXPORT VariableDeclarationList: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(VariableDeclarationList)

    VariableDeclarationList(VariableDeclaration *decl):
        declaration (decl), next (this)
        { kind = K; }

    VariableDeclarationList(VariableDeclarationList *previous, VariableDeclaration *decl):
        declaration (decl)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return declaration->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    {
        if (next)
            return next->lastSourceLocation();
        return declaration->lastSourceLocation();
    }

    inline VariableDeclarationList *finish(VariableDeclaration::VariableScope s)
    {
        VariableDeclarationList *front = next;
        next = 0;
        VariableDeclarationList *vdl;
        for (vdl = front; vdl != 0; vdl = vdl->next) {
            vdl->declaration->scope = s;
        }
        return front;
    }

// attributes
    VariableDeclaration *declaration;
    VariableDeclarationList *next;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT EmptyStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(EmptyStatement)

    EmptyStatement() { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return semicolonToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT ExpressionStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(ExpressionStatement)

    ExpressionStatement(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return expression->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    ExpressionNode *expression;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT IfStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(IfStatement)

    IfStatement(ExpressionNode *e, Statement *t, Statement *f = 0):
        expression (e), ok (t), ko (f)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return ifToken; }

    SourceLocation lastSourceLocation() const override
    {
        if (ko)
            return ko->lastSourceLocation();

        return ok->lastSourceLocation();
    }

// attributes
    ExpressionNode *expression;
    Statement *ok;
    Statement *ko;
    SourceLocation ifToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
    SourceLocation elseToken;
};

class QML_PARSER_EXPORT DoWhileStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(DoWhileStatement)

    DoWhileStatement(Statement *stmt, ExpressionNode *e):
        statement (stmt), expression (e)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return doToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    Statement *statement;
    ExpressionNode *expression;
    SourceLocation doToken;
    SourceLocation whileToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT WhileStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(WhileStatement)

    WhileStatement(ExpressionNode *e, Statement *stmt):
        expression (e), statement (stmt)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return whileToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    Statement *statement;
    SourceLocation whileToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT ForStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(ForStatement)

    ForStatement(ExpressionNode *i, ExpressionNode *c, ExpressionNode *e, Statement *stmt):
        initialiser (i), condition (c), expression (e), statement (stmt)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return forToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    ExpressionNode *initialiser;
    ExpressionNode *condition;
    ExpressionNode *expression;
    Statement *statement;
    SourceLocation forToken;
    SourceLocation lparenToken;
    SourceLocation firstSemicolonToken;
    SourceLocation secondSemicolonToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT LocalForStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(LocalForStatement)

    LocalForStatement(VariableDeclarationList *vlist, ExpressionNode *c, ExpressionNode *e, Statement *stmt):
        declarations (vlist), condition (c), expression (e), statement (stmt)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return forToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    VariableDeclarationList *declarations;
    ExpressionNode *condition;
    ExpressionNode *expression;
    Statement *statement;
    SourceLocation forToken;
    SourceLocation lparenToken;
    SourceLocation varToken;
    SourceLocation firstSemicolonToken;
    SourceLocation secondSemicolonToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT ForEachStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(ForEachStatement)

    ForEachStatement(ExpressionNode *i, ExpressionNode *e, Statement *stmt):
        initialiser (i), expression (e), statement (stmt)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return forToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    ExpressionNode *initialiser;
    ExpressionNode *expression;
    Statement *statement;
    SourceLocation forToken;
    SourceLocation lparenToken;
    SourceLocation inToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT LocalForEachStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(LocalForEachStatement)

    LocalForEachStatement(VariableDeclaration *v, ExpressionNode *e, Statement *stmt):
        declaration (v), expression (e), statement (stmt)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return forToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    VariableDeclaration *declaration;
    ExpressionNode *expression;
    Statement *statement;
    SourceLocation forToken;
    SourceLocation lparenToken;
    SourceLocation varToken;
    SourceLocation inToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT ContinueStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(ContinueStatement)

    ContinueStatement(const QStringRef &l = QStringRef()):
        label (l) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return continueToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    QStringRef label;
    SourceLocation continueToken;
    SourceLocation identifierToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT BreakStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(BreakStatement)

    BreakStatement(const QStringRef &l):
        label (l) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return breakToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

    // attributes
    QStringRef label;
    SourceLocation breakToken;
    SourceLocation identifierToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT ReturnStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(ReturnStatement)

    ReturnStatement(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return returnToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    ExpressionNode *expression;
    SourceLocation returnToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT WithStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(WithStatement)

    WithStatement(ExpressionNode *e, Statement *stmt):
        expression (e), statement (stmt)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return withToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    Statement *statement;
    SourceLocation withToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT CaseBlock: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(CaseBlock)

    CaseBlock(CaseClauses *c, DefaultClause *d = 0, CaseClauses *r = 0):
        clauses (c), defaultClause (d), moreClauses (r)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return lbraceToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

// attributes
    CaseClauses *clauses;
    DefaultClause *defaultClause;
    CaseClauses *moreClauses;
    SourceLocation lbraceToken;
    SourceLocation rbraceToken;
};

class QML_PARSER_EXPORT SwitchStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(SwitchStatement)

    SwitchStatement(ExpressionNode *e, CaseBlock *b):
        expression (e), block (b)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return switchToken; }

    SourceLocation lastSourceLocation() const override
    { return block->rbraceToken; }

// attributes
    ExpressionNode *expression;
    CaseBlock *block;
    SourceLocation switchToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT CaseClause: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(CaseClause)

    CaseClause(ExpressionNode *e, StatementList *slist):
        expression (e), statements (slist)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return caseToken; }

    SourceLocation lastSourceLocation() const override
    { return statements ? statements->lastSourceLocation() : colonToken; }

// attributes
    ExpressionNode *expression;
    StatementList *statements;
    SourceLocation caseToken;
    SourceLocation colonToken;
};

class QML_PARSER_EXPORT CaseClauses: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(CaseClauses)

    CaseClauses(CaseClause *c):
        clause (c), next (this)
        { kind = K; }

    CaseClauses(CaseClauses *previous, CaseClause *c):
        clause (c)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return clause->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() : clause->lastSourceLocation(); }

    inline CaseClauses *finish ()
    {
        CaseClauses *front = next;
        next = 0;
        return front;
    }

//attributes
    CaseClause *clause;
    CaseClauses *next;
};

class QML_PARSER_EXPORT DefaultClause: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(DefaultClause)

    DefaultClause(StatementList *slist):
        statements (slist)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return defaultToken; }

    SourceLocation lastSourceLocation() const override
    { return statements ? statements->lastSourceLocation() : colonToken; }

// attributes
    StatementList *statements;
    SourceLocation defaultToken;
    SourceLocation colonToken;
};

class QML_PARSER_EXPORT LabelledStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(LabelledStatement)

    LabelledStatement(const QStringRef &l, Statement *stmt):
        label (l), statement (stmt)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    QStringRef label;
    Statement *statement;
    SourceLocation identifierToken;
    SourceLocation colonToken;
};

class QML_PARSER_EXPORT ThrowStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(ThrowStatement)

    ThrowStatement(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return throwToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

    // attributes
    ExpressionNode *expression;
    SourceLocation throwToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT Catch: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(Catch)

    Catch(const QStringRef &n, Block *stmt):
        name (n), statement (stmt)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return catchToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    QStringRef name;
    Block *statement;
    SourceLocation catchToken;
    SourceLocation lparenToken;
    SourceLocation identifierToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT Finally: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(Finally)

    Finally(Block *stmt):
        statement (stmt)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return finallyToken; }

    SourceLocation lastSourceLocation() const override
    { return statement ? statement->lastSourceLocation() : finallyToken; }

// attributes
    Block *statement;
    SourceLocation finallyToken;
};

class QML_PARSER_EXPORT TryStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(TryStatement)

    TryStatement(Statement *stmt, Catch *c, Finally *f):
        statement (stmt), catchExpression (c), finallyExpression (f)
        { kind = K; }

    TryStatement(Statement *stmt, Finally *f):
        statement (stmt), catchExpression (0), finallyExpression (f)
        { kind = K; }

    TryStatement(Statement *stmt, Catch *c):
        statement (stmt), catchExpression (c), finallyExpression (0)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return tryToken; }

    SourceLocation lastSourceLocation() const override
    {
        if (finallyExpression)
            return finallyExpression->statement->rbraceToken;
        else if (catchExpression)
            return catchExpression->statement->rbraceToken;

        return statement->lastSourceLocation();
    }

// attributes
    Statement *statement;
    Catch *catchExpression;
    Finally *finallyExpression;
    SourceLocation tryToken;
};

class QML_PARSER_EXPORT FunctionExpression: public ExpressionNode
{
public:
    QMLJS_DECLARE_AST_NODE(FunctionExpression)

    FunctionExpression(const QStringRef &n, FormalParameterList *f, FunctionBody *b):
        name (n), formals (f), body (b)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return functionToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

// attributes
    QStringRef name;
    FormalParameterList *formals;
    FunctionBody *body;
    SourceLocation functionToken;
    SourceLocation identifierToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
    SourceLocation lbraceToken;
    SourceLocation rbraceToken;
};

class QML_PARSER_EXPORT FunctionDeclaration: public FunctionExpression
{
public:
    QMLJS_DECLARE_AST_NODE(FunctionDeclaration)

    FunctionDeclaration(const QStringRef &n, FormalParameterList *f, FunctionBody *b):
        FunctionExpression(n, f, b)
        { kind = K; }

    void accept0(Visitor *visitor) override;
};

class QML_PARSER_EXPORT FormalParameterList: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(FormalParameterList)

    FormalParameterList(const QStringRef &n):
        name (n), next (this)
        { kind = K; }

    FormalParameterList(FormalParameterList *previous, const QStringRef &n):
        name (n)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() : identifierToken; }

    inline FormalParameterList *finish ()
    {
        FormalParameterList *front = next;
        next = 0;
        return front;
    }

// attributes
    QStringRef name;
    FormalParameterList *next;
    SourceLocation commaToken;
    SourceLocation identifierToken;
};

class QML_PARSER_EXPORT SourceElement: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(SourceElement)

    inline SourceElement()
        { kind = K; }
};

class QML_PARSER_EXPORT SourceElements: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(SourceElements)

    SourceElements(SourceElement *elt):
        element (elt), next (this)
        { kind = K; }

    SourceElements(SourceElements *previous, SourceElement *elt):
        element (elt)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return element->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() : element->lastSourceLocation(); }

    inline SourceElements *finish ()
    {
        SourceElements *front = next;
        next = 0;
        return front;
    }

// attributes
    SourceElement *element;
    SourceElements *next;
};

class QML_PARSER_EXPORT FunctionBody: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(FunctionBody)

    FunctionBody(SourceElements *elts):
        elements (elts)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return elements ? elements->firstSourceLocation() : SourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return elements ? elements->lastSourceLocation() : SourceLocation(); }

// attributes
    SourceElements *elements;
};

class QML_PARSER_EXPORT Program: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(Program)

    Program(SourceElements *elts):
        elements (elts)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return elements ? elements->firstSourceLocation() : SourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return elements ? elements->lastSourceLocation() : SourceLocation(); }

// attributes
    SourceElements *elements;
};

class QML_PARSER_EXPORT FunctionSourceElement: public SourceElement
{
public:
    QMLJS_DECLARE_AST_NODE(FunctionSourceElement)

    FunctionSourceElement(FunctionDeclaration *f):
        declaration (f)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return declaration->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return declaration->lastSourceLocation(); }

// attributes
    FunctionDeclaration *declaration;
};

class QML_PARSER_EXPORT StatementSourceElement: public SourceElement
{
public:
    QMLJS_DECLARE_AST_NODE(StatementSourceElement)

    StatementSourceElement(Statement *stmt):
        statement (stmt)
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return statement->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    Statement *statement;
};

class QML_PARSER_EXPORT DebuggerStatement: public Statement
{
public:
    QMLJS_DECLARE_AST_NODE(DebuggerStatement)

    DebuggerStatement()
        { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return debuggerToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    SourceLocation debuggerToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT UiQualifiedId: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(UiQualifiedId)

    UiQualifiedId(const QStringRef &name)
        : next(this), name(name)
    { kind = K; }

    UiQualifiedId(UiQualifiedId *previous, const QStringRef &name)
        : name(name)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    UiQualifiedId *finish()
    {
        UiQualifiedId *head = next;
        next = 0;
        return head;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() : identifierToken; }

// attributes
    UiQualifiedId *next;
    QStringRef name;
    SourceLocation identifierToken;
};

class QML_PARSER_EXPORT UiImport: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(UiImport)

    UiImport(const QStringRef &fileName)
        : fileName(fileName), importUri(0)
    { kind = K; }

    UiImport(UiQualifiedId *uri)
        : importUri(uri)
    { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return importToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    QStringRef fileName;
    UiQualifiedId *importUri;
    QStringRef importId;
    SourceLocation importToken;
    SourceLocation fileNameToken;
    SourceLocation versionToken;
    SourceLocation asToken;
    SourceLocation importIdToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT UiObjectMember: public Node
{
public:
    SourceLocation firstSourceLocation() const override = 0;
    SourceLocation lastSourceLocation() const override = 0;

    UiObjectMember *uiObjectMemberCast() override;
};

class QML_PARSER_EXPORT UiObjectMemberList: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(UiObjectMemberList)

    UiObjectMemberList(UiObjectMember *member)
        : next(this), member(member)
    { kind = K; }

    UiObjectMemberList(UiObjectMemberList *previous, UiObjectMember *member)
        : member(member)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return member->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() : member->lastSourceLocation(); }

    UiObjectMemberList *finish()
    {
        UiObjectMemberList *head = next;
        next = 0;
        return head;
    }

// attributes
    UiObjectMemberList *next;
    UiObjectMember *member;
};

class QML_PARSER_EXPORT UiQualifiedPragmaId: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(UiQualifiedPragmaId)

    UiQualifiedPragmaId(const QStringRef &name)
        : next(this), name(name)
    { kind = K; }

    UiQualifiedPragmaId(UiQualifiedPragmaId *previous, const QStringRef &name)
        : name(name)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    UiQualifiedPragmaId *finish()
    {
        UiQualifiedPragmaId *head = next;
        next = 0;
        return head;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() : identifierToken; }

// attributes
    UiQualifiedPragmaId *next;
    QStringRef name;
    SourceLocation identifierToken;
};

class QML_PARSER_EXPORT UiPragma: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(UiPragma)

    UiPragma(UiQualifiedPragmaId *type)
        : pragmaType(type)
    { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return pragmaToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    UiQualifiedPragmaId *pragmaType;
    SourceLocation pragmaToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT UiHeaderItemList: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(UiHeaderItemList)

    UiHeaderItemList(UiImport *import)
        : headerItem(import), next(this)
    { kind = K; }

    UiHeaderItemList(UiPragma *pragma)
        : headerItem(pragma), next(this)
    { kind = K; }

    UiHeaderItemList(UiHeaderItemList *previous, UiImport *import)
        : headerItem(import)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    UiHeaderItemList(UiHeaderItemList *previous, UiPragma *pragma)
        : headerItem(pragma)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    UiHeaderItemList *finish()
    {
        UiHeaderItemList *head = next;
        next = 0;
        return head;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return headerItem->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() : headerItem->lastSourceLocation(); }

// attributes
    Node *headerItem;
    UiHeaderItemList *next;
};

class QML_PARSER_EXPORT UiProgram: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(UiProgram)

    UiProgram(UiHeaderItemList *headers, UiObjectMemberList *members)
        : headers(headers), members(members)
    { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    {
        if (headers)
            return headers->firstSourceLocation();
        else if (members)
            return members->firstSourceLocation();
        return SourceLocation();
    }

    SourceLocation lastSourceLocation() const override
    {
        if (members)
            return members->lastSourceLocation();
        else if (headers)
            return headers->lastSourceLocation();
        return SourceLocation();
    }

// attributes
    UiHeaderItemList *headers;
    UiObjectMemberList *members;
};

class QML_PARSER_EXPORT UiArrayMemberList: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(UiArrayMemberList)

    UiArrayMemberList(UiObjectMember *member)
        : next(this), member(member)
    { kind = K; }

    UiArrayMemberList(UiArrayMemberList *previous, UiObjectMember *member)
        : member(member)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return member->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() : member->lastSourceLocation(); }

    UiArrayMemberList *finish()
    {
        UiArrayMemberList *head = next;
        next = 0;
        return head;
    }

// attributes
    UiArrayMemberList *next;
    UiObjectMember *member;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT UiObjectInitializer: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(UiObjectInitializer)

    UiObjectInitializer(UiObjectMemberList *members)
        : members(members)
    { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return lbraceToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

// attributes
    SourceLocation lbraceToken;
    UiObjectMemberList *members;
    SourceLocation rbraceToken;
};

class QML_PARSER_EXPORT UiParameterList: public Node
{
public:
    QMLJS_DECLARE_AST_NODE(UiParameterList)

    UiParameterList(UiQualifiedId *t, const QStringRef &n):
        type (t), name (n), next (this)
        { kind = K; }

    UiParameterList(UiParameterList *previous, UiQualifiedId *t, const QStringRef &n):
        type (t), name (n)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(Visitor *) override;

    SourceLocation firstSourceLocation() const override
    { return propertyTypeToken; }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() : identifierToken; }

    inline UiParameterList *finish ()
    {
        UiParameterList *front = next;
        next = 0;
        return front;
    }

// attributes
    UiQualifiedId *type;
    QStringRef name;
    UiParameterList *next;
    SourceLocation commaToken;
    SourceLocation propertyTypeToken;
    SourceLocation identifierToken;
};

class QML_PARSER_EXPORT UiPublicMember: public UiObjectMember
{
public:
    QMLJS_DECLARE_AST_NODE(UiPublicMember)

    UiPublicMember(UiQualifiedId *memberType,
                   const QStringRef &name)
        : type(Property), memberType(memberType), name(name), statement(0), binding(0), isDefaultMember(false), isReadonlyMember(false), parameters(0)
    { kind = K; }

    UiPublicMember(UiQualifiedId *memberType,
                   const QStringRef &name,
                   Statement *statement)
        : type(Property), memberType(memberType), name(name), statement(statement), binding(0), isDefaultMember(false), isReadonlyMember(false), parameters(0)
    { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    {
      if (defaultToken.isValid())
        return defaultToken;
      else if (readonlyToken.isValid())
          return readonlyToken;

      return propertyToken;
    }

    SourceLocation lastSourceLocation() const override
    {
      if (binding)
          return binding->lastSourceLocation();
      if (statement)
          return statement->lastSourceLocation();

      return semicolonToken;
    }

    QStringRef memberTypeName() const
    {
        return memberType ? memberType->name : QStringRef();
    }

    bool isValid() const
    {
        return memberType && !memberType->name.isNull() && !memberType->name.isEmpty();
    }

// attributes
    enum { Signal, Property } type;
    QStringRef typeModifier;
    UiQualifiedId *memberType;
    QStringRef name;
    Statement *statement; // initialized with a JS expression
    UiObjectMember *binding; // initialized with a QML object or array.
    bool isDefaultMember;
    bool isReadonlyMember;
    UiParameterList *parameters;
    SourceLocation defaultToken;
    SourceLocation readonlyToken;
    SourceLocation propertyToken;
    SourceLocation typeModifierToken;
    SourceLocation typeToken;
    SourceLocation identifierToken;
    SourceLocation colonToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT UiObjectDefinition: public UiObjectMember
{
public:
    QMLJS_DECLARE_AST_NODE(UiObjectDefinition)

    UiObjectDefinition(UiQualifiedId *qualifiedTypeNameId,
                       UiObjectInitializer *initializer)
        : qualifiedTypeNameId(qualifiedTypeNameId), initializer(initializer)
    { kind = K; }

    void accept0(Visitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return qualifiedTypeNameId->identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return initializer->rbraceToken; }

// attributes
    UiQualifiedId *qualifiedTypeNameId;
    UiObjectInitializer *initializer;
};

class QML_PARSER_EXPORT UiSourceElement: public UiObjectMember
{
public:
    QMLJS_DECLARE_AST_NODE(UiSourceElement)

    UiSourceElement(Node *sourceElement)
        : sourceElement(sourceElement)
    { kind = K; }

    SourceLocation firstSourceLocation() const override
    {
      if (FunctionDeclaration *funDecl = cast<FunctionDeclaration *>(sourceElement))
        return funDecl->firstSourceLocation();
      else if (VariableStatement *varStmt = cast<VariableStatement *>(sourceElement))
        return varStmt->firstSourceLocation();

      return SourceLocation();
    }

    SourceLocation lastSourceLocation() const override
    {
      if (FunctionDeclaration *funDecl = cast<FunctionDeclaration *>(sourceElement))
        return funDecl->lastSourceLocation();
      else if (VariableStatement *varStmt = cast<VariableStatement *>(sourceElement))
        return varStmt->lastSourceLocation();

      return SourceLocation();
    }

    void accept0(Visitor *visitor) override;


// attributes
    Node *sourceElement;
};

class QML_PARSER_EXPORT UiObjectBinding: public UiObjectMember
{
public:
    QMLJS_DECLARE_AST_NODE(UiObjectBinding)

    UiObjectBinding(UiQualifiedId *qualifiedId,
                    UiQualifiedId *qualifiedTypeNameId,
                    UiObjectInitializer *initializer)
        : qualifiedId(qualifiedId),
          qualifiedTypeNameId(qualifiedTypeNameId),
          initializer(initializer),
          hasOnToken(false)
    { kind = K; }

    SourceLocation firstSourceLocation() const override
    {
        if (hasOnToken && qualifiedTypeNameId)
            return qualifiedTypeNameId->identifierToken;

        return qualifiedId->identifierToken;
    }

    SourceLocation lastSourceLocation() const override
    { return initializer->rbraceToken; }

    void accept0(Visitor *visitor) override;


// attributes
    UiQualifiedId *qualifiedId;
    UiQualifiedId *qualifiedTypeNameId;
    UiObjectInitializer *initializer;
    SourceLocation colonToken;
    bool hasOnToken;
};

class QML_PARSER_EXPORT UiScriptBinding: public UiObjectMember
{
public:
    QMLJS_DECLARE_AST_NODE(UiScriptBinding)

    UiScriptBinding(UiQualifiedId *qualifiedId,
                    Statement *statement)
        : qualifiedId(qualifiedId),
          statement(statement)
    { kind = K; }

    SourceLocation firstSourceLocation() const override
    { return qualifiedId->identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

    void accept0(Visitor *visitor) override;

// attributes
    UiQualifiedId *qualifiedId;
    Statement *statement;
    SourceLocation colonToken;
};

class QML_PARSER_EXPORT UiArrayBinding: public UiObjectMember
{
public:
    QMLJS_DECLARE_AST_NODE(UiArrayBinding)

    UiArrayBinding(UiQualifiedId *qualifiedId,
                   UiArrayMemberList *members)
        : qualifiedId(qualifiedId),
          members(members)
    { kind = K; }

    SourceLocation firstSourceLocation() const override
    { return qualifiedId->identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return rbracketToken; }

    void accept0(Visitor *visitor) override;

// attributes
    UiQualifiedId *qualifiedId;
    UiArrayMemberList *members;
    SourceLocation colonToken;
    SourceLocation lbracketToken;
    SourceLocation rbracketToken;
};

class QML_PARSER_EXPORT UiEnumMemberList: public Node
{
    QMLJS_DECLARE_AST_NODE(UiEnumMemberList)
public:
    UiEnumMemberList(const QStringRef &member, double v = 0.0)
        : next(this), member(member), value(v)
    { kind = K; }

    UiEnumMemberList(UiEnumMemberList *previous, const QStringRef &member)
        : member(member)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
        value = previous->value + 1;
    }

    UiEnumMemberList(UiEnumMemberList *previous, const QStringRef &member, double v)
        : member(member), value(v)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    SourceLocation firstSourceLocation() const override
    { return memberToken; }

    SourceLocation lastSourceLocation() const override
    { return next ? next->lastSourceLocation() :
                    valueToken.isValid() ? valueToken : memberToken; }

    void accept0(Visitor *visitor) override;

    UiEnumMemberList *finish()
    {
        UiEnumMemberList *head = next;
        next = 0;
        return head;
    }

// attributes
    UiEnumMemberList *next;
    QStringRef member;
    double value;
    SourceLocation memberToken;
    SourceLocation valueToken;
};

class QML_PARSER_EXPORT UiEnumDeclaration: public UiObjectMember
{
public:
    QMLJS_DECLARE_AST_NODE(UiEnumDeclaration)

    UiEnumDeclaration(const QStringRef &name,
                      UiEnumMemberList *members)
        : name(name)
        , members(members)
    { kind = K; }

    SourceLocation firstSourceLocation() const override
    { return enumToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

    void accept0(Visitor *visitor) override;

// attributes
    SourceLocation enumToken;
    SourceLocation rbraceToken;
    QStringRef name;
    UiEnumMemberList *members;
};

} } // namespace AST



QT_QML_END_NAMESPACE

