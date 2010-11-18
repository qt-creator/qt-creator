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
#ifndef GLSLAST_H
#define GLSLAST_H

#include "glsl.h"
#include "glslmemorypool.h"
#include <QtCore/qstring.h>

namespace GLSL {

class AST;
class TranslationUnit;
class Expression;
class IdentifierExpression;
class LiteralExpression;
class BinaryExpression;
class UnaryExpression;
class TernaryExpression;
class AssignmentExpression;
class MemberAccessExpression;
class FunctionCallExpression;
class FunctionIdentifier;
class DeclarationExpression;
class Statement;
class ExpressionStatement;
class CompoundStatement;
class IfStatement;
class WhileStatement;
class DoStatement;
class ForStatement;
class JumpStatement;
class ReturnStatement;
class SwitchStatement;
class CaseLabelStatement;
class DeclarationStatement;
class Type;
class BasicType;
class NamedType;
class ArrayType;
class StructType;
class QualifiedType;
class Declaration;
class PrecisionDeclaration;
class ParameterDeclaration;
class VariableDeclaration;
class TypeDeclaration;
class TypeAndVariableDeclaration;
class InvariantDeclaration;
class InitDeclaration;
class FunctionDeclaration;
class Visitor;

template <typename T>
class GLSL_EXPORT List: public Managed
{
public:
    List(const T &value)
        : value(value), next(this), lineno(0) {}

    List(List *previous, const T &value)
        : value(value), lineno(0)
    {
        next = previous->next;
        previous->next = this;
    }

    List *finish()
    {
        List *head = next;
        next = 0;
        return head;
    }

    T value;
    List *next;
    int lineno;
};

// Append two lists, which are assumed to still be circular, pre-finish.
template <typename T>
List<T> *appendLists(List<T> *first, List<T> *second)
{
    if (!first)
        return second;
    else if (!second)
        return first;
    List<T> *firstHead = first->next;
    List<T> *secondHead = second->next;
    first->next = secondHead;
    second->next = firstHead;
    return second;
}

class GLSL_EXPORT AST: public Managed
{
public:
    enum Kind {
        Kind_Undefined,

        // Translation unit
        Kind_TranslationUnit,

        // Primary expressions
        Kind_Identifier,
        Kind_Literal,

        // Unary expressions
        Kind_PreIncrement,
        Kind_PostIncrement,
        Kind_PreDecrement,
        Kind_PostDecrement,
        Kind_UnaryPlus,
        Kind_UnaryMinus,
        Kind_LogicalNot,
        Kind_BitwiseNot,

        // Binary expressions
        Kind_Plus,
        Kind_Minus,
        Kind_Multiply,
        Kind_Divide,
        Kind_Modulus,
        Kind_ShiftLeft,
        Kind_ShiftRight,
        Kind_Equal,
        Kind_NotEqual,
        Kind_LessThan,
        Kind_LessEqual,
        Kind_GreaterThan,
        Kind_GreaterEqual,
        Kind_LogicalAnd,
        Kind_LogicalOr,
        Kind_LogicalXor,
        Kind_BitwiseAnd,
        Kind_BitwiseOr,
        Kind_BitwiseXor,
        Kind_Comma,
        Kind_ArrayAccess,

        // Other expressions
        Kind_Conditional,
        Kind_MemberAccess,
        Kind_FunctionCall,
        Kind_MemberFunctionCall,
        Kind_FunctionIdentifier,
        Kind_DeclarationExpression,

        // Assignment expressions
        Kind_Assign,
        Kind_AssignPlus,
        Kind_AssignMinus,
        Kind_AssignMultiply,
        Kind_AssignDivide,
        Kind_AssignModulus,
        Kind_AssignShiftLeft,
        Kind_AssignShiftRight,
        Kind_AssignAnd,
        Kind_AssignOr,
        Kind_AssignXor,

        // Statements
        Kind_ExpressionStatement,
        Kind_CompoundStatement,
        Kind_If,
        Kind_While,
        Kind_Do,
        Kind_For,
        Kind_Break,
        Kind_Continue,
        Kind_Discard,
        Kind_Return,
        Kind_ReturnExpression,
        Kind_Switch,
        Kind_CaseLabel,
        Kind_DefaultLabel,
        Kind_DeclarationStatement,

        // Types
        Kind_BasicType,
        Kind_NamedType,
        Kind_ArrayType,
        Kind_OpenArrayType,
        Kind_StructType,
        Kind_AnonymousStructType,
        Kind_StructField,
        Kind_QualifiedType,

        // Declarations
        Kind_PrecisionDeclaration,
        Kind_ParameterDeclaration,
        Kind_VariableDeclaration,
        Kind_TypeDeclaration,
        Kind_TypeAndVariableDeclaration,
        Kind_InvariantDeclaration,
        Kind_InitDeclaration,
        Kind_FunctionDeclaration
    };

    virtual TranslationUnit *asTranslationUnit() { return 0; }

    virtual Expression *asExpression() { return 0; }
    virtual IdentifierExpression *asIdentifierExpression() { return 0; }
    virtual LiteralExpression *asLiteralExpression() { return 0; }
    virtual BinaryExpression *asBinaryExpression() { return 0; }
    virtual UnaryExpression *asUnaryExpression() { return 0; }
    virtual TernaryExpression *asTernaryExpression() { return 0; }
    virtual AssignmentExpression *asAssignmentExpression() { return 0; }
    virtual MemberAccessExpression *asMemberAccessExpression() { return 0; }
    virtual FunctionCallExpression *asFunctionCallExpression() { return 0; }
    virtual FunctionIdentifier *asFunctionIdentifier() { return 0; }
    virtual DeclarationExpression *asDeclarationExpression() { return 0; }

    virtual Statement *asStatement() { return 0; }
    virtual ExpressionStatement *asExpressionStatement() { return 0; }
    virtual CompoundStatement *asCompoundStatement() { return 0; }
    virtual IfStatement *asIfStatement() { return 0; }
    virtual WhileStatement *asWhileStatement() { return 0; }
    virtual DoStatement *asDoStatement() { return 0; }
    virtual ForStatement *asForStatement() { return 0; }
    virtual JumpStatement *asJumpStatement() { return 0; }
    virtual ReturnStatement *asReturnStatement() { return 0; }
    virtual SwitchStatement *asSwitchStatement() { return 0; }
    virtual CaseLabelStatement *asCaseLabelStatement() { return 0; }
    virtual DeclarationStatement *asDeclarationStatement() { return 0; }

    virtual Type *asType() { return 0; }
    virtual BasicType *asBasicType() { return 0; }
    virtual NamedType *asNamedType() { return 0; }
    virtual ArrayType *asArrayType() { return 0; }
    virtual StructType *asStructType() { return 0; }
    virtual QualifiedType *asQualifiedType() { return 0; }

    virtual Declaration *asDeclaration() { return 0; }
    virtual PrecisionDeclaration *asPrecisionDeclaration() { return 0; }
    virtual ParameterDeclaration *asParameterDeclaration() { return 0; }
    virtual VariableDeclaration *asVariableDeclaration() { return 0; }
    virtual TypeDeclaration *asTypeDeclaration() { return 0; }
    virtual TypeAndVariableDeclaration *asTypeAndVariableDeclaration() { return 0; }
    virtual InvariantDeclaration *asInvariantDeclaration() { return 0; }
    virtual InitDeclaration *asInitDeclaration() { return 0; }
    virtual FunctionDeclaration *asFunctionDeclaration() { return 0; }

    void accept(Visitor *visitor);
    static void accept(AST *ast, Visitor *visitor);

    template <typename T>
    static void accept(List<T> *it, Visitor *visitor)
    {
        for (; it; it = it->next)
            accept(it->value, visitor);
    }

    virtual void accept0(Visitor *visitor) = 0;

protected:
    AST(Kind _kind) : kind(_kind), lineno(0) {}

    template <typename T>
    static List<T> *finish(List<T> *list)
    {
        if (! list)
            return 0;
        return list->finish(); // convert the circular list with a linked list.
    }

public: // attributes
    int kind;
    int lineno;

protected:
    ~AST() {}       // Managed types cannot be deleted.
};

class GLSL_EXPORT TranslationUnit: public AST
{
public:
    TranslationUnit(List<Declaration *> *declarations)
        : AST(Kind_TranslationUnit), declarations(finish(declarations)) {}

    virtual TranslationUnit *asTranslationUnit() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    List<Declaration *> *declarations;
};

class GLSL_EXPORT Expression: public AST
{
protected:
    Expression(Kind _kind) : AST(_kind) {}

public:
    virtual Expression *asExpression() { return this; }
};

class GLSL_EXPORT IdentifierExpression: public Expression
{
public:
    IdentifierExpression(const QString *_name)
        : Expression(Kind_Identifier), name(_name) {}

    virtual IdentifierExpression *asIdentifierExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    const QString *name;
};

class GLSL_EXPORT LiteralExpression: public Expression
{
public:
    LiteralExpression(const QString *_value)
        : Expression(Kind_Literal), value(_value) {}

    virtual LiteralExpression *asLiteralExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    const QString *value;
};

class GLSL_EXPORT BinaryExpression: public Expression
{
public:
    BinaryExpression(Kind _kind, Expression *_left, Expression *_right)
        : Expression(_kind), left(_left), right(_right) {}

    virtual BinaryExpression *asBinaryExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Expression *left;
    Expression *right;
};

class GLSL_EXPORT UnaryExpression: public Expression
{
public:
    UnaryExpression(Kind _kind, Expression *_expr)
        : Expression(_kind), expr(_expr) {}

    virtual UnaryExpression *asUnaryExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Expression *expr;
};

class GLSL_EXPORT TernaryExpression: public Expression
{
public:
    TernaryExpression(Kind _kind, Expression *_first, Expression *_second, Expression *_third)
        : Expression(_kind), first(_first), second(_second), third(_third) {}

    virtual TernaryExpression *asTernaryExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Expression *first;
    Expression *second;
    Expression *third;
};

class GLSL_EXPORT AssignmentExpression: public Expression
{
public:
    AssignmentExpression(Kind _kind, Expression *_variable, Expression *_value)
        : Expression(_kind), variable(_variable), value(_value) {}

    virtual AssignmentExpression *asAssignmentExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Expression *variable;
    Expression *value;
};

class GLSL_EXPORT MemberAccessExpression: public Expression
{
public:
    MemberAccessExpression(Expression *_expr, const QString *_field)
        : Expression(Kind_MemberAccess), expr(_expr), field(_field) {}

    virtual MemberAccessExpression *asMemberAccessExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Expression *expr;
    const QString *field;
};

class GLSL_EXPORT FunctionCallExpression: public Expression
{
public:
    FunctionCallExpression(FunctionIdentifier *_id,
                           List<Expression *> *_arguments)
        : Expression(Kind_FunctionCall), expr(0), id(_id)
        , arguments(finish(_arguments)) {}
    FunctionCallExpression(Expression *_expr, FunctionIdentifier *_id,
                           List<Expression *> *_arguments)
        : Expression(Kind_MemberFunctionCall), expr(_expr), id(_id)
        , arguments(finish(_arguments)) {}

    virtual FunctionCallExpression *asFunctionCallExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Expression *expr;
    FunctionIdentifier *id;
    List<Expression *> *arguments;
};

class GLSL_EXPORT FunctionIdentifier: public AST
{
public:
    FunctionIdentifier(const QString *_name)
        : AST(Kind_FunctionIdentifier), name(_name), type(0) {}
    FunctionIdentifier(Type *_type)
        : AST(Kind_FunctionIdentifier), name(0), type(_type) {}

    virtual FunctionIdentifier *asFunctionIdentifier() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    const QString *name;
    Type *type;
};

class GLSL_EXPORT DeclarationExpression: public Expression
{
public:
    DeclarationExpression(Type *_type, const QString *_name,
                          Expression *_initializer)
        : Expression(Kind_DeclarationExpression), type(_type)
        , name(_name), initializer(_initializer) {}

    virtual DeclarationExpression *asDeclarationExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Type *type;
    const QString *name;
    Expression *initializer;
};

class GLSL_EXPORT Statement: public AST
{
protected:
    Statement(Kind _kind) : AST(_kind) {}

public:
    virtual Statement *asStatement() { return this; }
};

class GLSL_EXPORT ExpressionStatement: public Statement
{
public:
    ExpressionStatement(Expression *_expr)
        : Statement(Kind_ExpressionStatement), expr(_expr) {}

    virtual ExpressionStatement *asExpressionStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Expression *expr;
};

class GLSL_EXPORT CompoundStatement: public Statement
{
public:
    CompoundStatement()
        : Statement(Kind_CompoundStatement), statements(0) {}
    CompoundStatement(List<Statement *> *_statements)
        : Statement(Kind_CompoundStatement), statements(finish(_statements)) {}

    virtual CompoundStatement *asCompoundStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    List<Statement *> *statements;
};

class GLSL_EXPORT IfStatement: public Statement
{
public:
    IfStatement(Expression *_condition, Statement *_thenClause, Statement *_elseClause)
        : Statement(Kind_If), condition(_condition)
        , thenClause(_thenClause), elseClause(_elseClause) {}

    virtual IfStatement *asIfStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Expression *condition;
    Statement *thenClause;
    Statement *elseClause;
};

class GLSL_EXPORT WhileStatement: public Statement
{
public:
    WhileStatement(Expression *_condition, Statement *_body)
        : Statement(Kind_While), condition(_condition), body(_body) {}

    virtual WhileStatement *asWhileStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Expression *condition;
    Statement *body;
};

class GLSL_EXPORT DoStatement: public Statement
{
public:
    DoStatement(Statement *_body, Expression *_condition)
        : Statement(Kind_Do), body(_body), condition(_condition) {}

    virtual DoStatement *asDoStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Statement *body;
    Expression *condition;
};

class GLSL_EXPORT ForStatement: public Statement
{
public:
    ForStatement(Statement *_init, Expression *_condition, Expression *_increment, Statement *_body)
        : Statement(Kind_For), init(_init), condition(_condition), increment(_increment), body(_body) {}

    virtual ForStatement *asForStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Statement *init;
    Expression *condition;
    Expression *increment;
    Statement *body;
};

class GLSL_EXPORT JumpStatement: public Statement
{
public:
    JumpStatement(Kind _kind) : Statement(_kind) {}

    virtual JumpStatement *asJumpStatement() { return this; }

    virtual void accept0(Visitor *visitor);
};

class GLSL_EXPORT ReturnStatement: public Statement
{
public:
    ReturnStatement() : Statement(Kind_Return), expr(0) {}
    ReturnStatement(Expression *_expr)
        : Statement(Kind_ReturnExpression), expr(_expr) {}

    virtual ReturnStatement *asReturnStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Expression *expr;
};

class GLSL_EXPORT SwitchStatement: public Statement
{
public:
    SwitchStatement(Expression *_expr, Statement *_body)
        : Statement(Kind_Switch), expr(_expr), body(_body) {}

    virtual SwitchStatement *asSwitchStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Expression *expr;
    Statement *body;
};

class GLSL_EXPORT CaseLabelStatement: public Statement
{
public:
    CaseLabelStatement() : Statement(Kind_DefaultLabel), expr(0) {}
    CaseLabelStatement(Expression *_expr)
        : Statement(Kind_CaseLabel), expr(_expr) {}

    virtual CaseLabelStatement *asCaseLabelStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Expression *expr;
};

class GLSL_EXPORT DeclarationStatement: public Statement
{
public:
    DeclarationStatement(List<Declaration *> *_decls)
        : Statement(Kind_DeclarationStatement), decls(finish(_decls)) {}

    virtual DeclarationStatement *asDeclarationStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    List<Declaration *> *decls;
};

class GLSL_EXPORT Type: public AST
{
protected:
    Type(Kind _kind) : AST(_kind) {}

public:
    enum Precision
    {
        PrecNotValid,       // Precision not valid (e.g. structs).
        PrecUnspecified,    // Precision not known, but can be validly set.
        Lowp,
        Mediump,
        Highp
    };

    enum Category
    {
        Void,
        Primitive,
        Vector2,
        Vector3,
        Vector4,
        Matrix,
        Sampler1D,
        Sampler2D,
        Sampler3D,
        SamplerCube,
        Sampler1DShadow,
        Sampler2DShadow,
        SamplerCubeShadow,
        Sampler1DArray,
        Sampler2DArray,
        SamplerCubeArray,
        Sampler1DArrayShadow,
        Sampler2DArrayShadow,
        SamplerCubeArrayShadow,
        Sampler2DRect,
        Sampler2DRectShadow,
        Sampler2DMS,
        Sampler2DMSArray,
        SamplerBuffer,
        Array,
        Struct
    };

    virtual Type *asType() { return this; }

    virtual Precision precision() const = 0;

    // Set the precision for the innermost basic type.  Returns false if it
    // is not valid to set a precision (e.g. structs).
    virtual bool setPrecision(Precision precision) = 0;

    virtual Category category() const = 0;
};

class GLSL_EXPORT BasicType: public Type
{
public:
    // Pass the parser's token code: T_VOID, T_VEC4, etc.
    BasicType(int _token, const char *_name, Category _category);

    virtual BasicType *asBasicType() { return this; }

    virtual void accept0(Visitor *visitor);

    virtual Precision precision() const;
    virtual bool setPrecision(Precision precision);

    virtual Category category() const { return categ; }

public: // attributes
    Precision prec;
    int token;
    const char *name;
    Category categ;
};

class GLSL_EXPORT NamedType: public Type
{
public:
    NamedType(const QString *_name) : Type(Kind_NamedType), name(_name) {}

    virtual NamedType *asNamedType() { return this; }

    virtual void accept0(Visitor *visitor);

    virtual Precision precision() const;
    virtual bool setPrecision(Precision precision);

    virtual Category category() const { return Struct; }

public: // attributes
    const QString *name;
};

class GLSL_EXPORT ArrayType: public Type
{
public:
    ArrayType(Type *_elementType)
        : Type(Kind_OpenArrayType), elementType(_elementType), size(0) {}
    ArrayType(Type *_elementType, Expression *_size)
        : Type(Kind_ArrayType), elementType(_elementType), size(_size) {}

    virtual ArrayType *asArrayType() { return this; }

    virtual void accept0(Visitor *visitor);

    virtual Precision precision() const;
    virtual bool setPrecision(Precision precision);

    virtual Category category() const { return Array; }

public: // attributes
    Type *elementType;
    Expression *size;
};

class GLSL_EXPORT StructType: public Type
{
public:
    class Field: public AST
    {
    public:
        Field(const QString *_name)
            : AST(Kind_StructField), name(_name), type(0) {}

        // Takes the outer shell of an array type with the innermost
        // element type set to null.  The fixInnerTypes() method will
        // set the innermost element type to a meaningful value.
        Field(const QString *_name, Type *_type)
            : AST(Kind_StructField), name(_name), type(_type) {}

        virtual void accept0(Visitor *visitor);

        void setInnerType(Type *innerType);

        const QString *name;
        Type *type;
    };

    StructType(List<Field *> *_fields)
        : Type(Kind_AnonymousStructType), fields(finish(_fields)) {}
    StructType(const QString *_name, List<Field *> *_fields)
        : Type(Kind_StructType), name(_name), fields(finish(_fields)) {}

    virtual StructType *asStructType() { return this; }

    virtual void accept0(Visitor *visitor);

    virtual Precision precision() const;
    virtual bool setPrecision(Precision precision);

    // Fix the inner types of a field list.  The "innerType" will
    // be copied into the "array holes" of all fields.
    static List<Field *> *fixInnerTypes(Type *innerType, List<Field *> *fields);

    virtual Category category() const { return Struct; }

public: // attributes
    const QString *name;
    List<Field *> *fields;
};

class GLSL_EXPORT LayoutQualifier
{
public:
    LayoutQualifier(const QString *_name, const QString *_number)
        : name(_name), number(_number), lineno(0) {}

public: // attributes
    const QString *name;
    const QString *number;
    int lineno;
};

class GLSL_EXPORT QualifiedType: public Type
{
public:
    QualifiedType(int _qualifiers, Type *_type, List<LayoutQualifier *> *_layout_list)
        : Type(Kind_QualifiedType), qualifiers(_qualifiers), type(_type)
        , layout_list(finish(_layout_list)) {}

    enum
    {
        StorageMask         = 0x000000FF,
        NoStorage           = 0x00000000,
        Const               = 0x00000001,
        Attribute           = 0x00000002,
        Varying             = 0x00000003,
        CentroidVarying     = 0x00000004,
        In                  = 0x00000005,
        Out                 = 0x00000006,
        CentroidIn          = 0x00000007,
        CentroidOut         = 0x00000008,
        PatchIn             = 0x00000009,
        PatchOut            = 0x0000000A,
        SampleIn            = 0x0000000B,
        SampleOut           = 0x0000000C,
        Uniform             = 0x0000000D,
        InterpolationMask   = 0x00000F00,
        NoInterpolation     = 0x00000000,
        Smooth              = 0x00000100,
        Flat                = 0x00000200,
        NoPerspective       = 0x00000300,
        Invariant           = 0x00010000
    };

    virtual QualifiedType *asQualifiedType() { return this; }

    virtual void accept0(Visitor *visitor);

    virtual Precision precision() const { return type->precision(); }
    virtual bool setPrecision(Precision precision) { return type->setPrecision(precision); }

    virtual Category category() const { return type->category(); }

public: // attributes
    int qualifiers;
    Type *type;
    List<LayoutQualifier *> *layout_list;
};

class GLSL_EXPORT Declaration: public AST
{
protected:
    Declaration(Kind _kind) : AST(_kind) {}

public:
    virtual Declaration *asDeclaration() { return this; }
};

class GLSL_EXPORT PrecisionDeclaration: public Declaration
{
public:
    PrecisionDeclaration(Type::Precision _precision, Type *_type)
        : Declaration(Kind_PrecisionDeclaration)
        , precision(_precision), type(_type) {}

    virtual PrecisionDeclaration *asPrecisionDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Type::Precision precision;
    Type *type;
};

class ParameterDeclaration: public Declaration
{
public:
    enum Qualifier
    {
        In,
        Out,
        InOut
    };
    ParameterDeclaration(Type *_type, Qualifier _qualifier,
                         const QString *_name)
        : Declaration(Kind_ParameterDeclaration), type(_type)
        , qualifier(_qualifier), name(_name) {}

    virtual ParameterDeclaration *asParameterDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Type *type;
    Qualifier qualifier;
    const QString *name;
};

class VariableDeclaration: public Declaration
{
public:
    VariableDeclaration(Type *_type, const QString *_name,
                        Expression *_initializer = 0)
        : Declaration(Kind_VariableDeclaration), type(_type)
        , name(_name), initializer(_initializer) {}

    virtual VariableDeclaration *asVariableDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

    static Type *declarationType(List<Declaration *> *decls);

public: // attributes
    Type *type;
    const QString *name;
    Expression *initializer;
};

class TypeDeclaration: public Declaration
{
public:
    TypeDeclaration(Type *_type)
        : Declaration(Kind_TypeDeclaration), type(_type) {}

    virtual TypeDeclaration *asTypeDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    Type *type;
};

class TypeAndVariableDeclaration: public Declaration
{
public:
    TypeAndVariableDeclaration(TypeDeclaration *_typeDecl,
                               VariableDeclaration *_varDecl)
        : Declaration(Kind_TypeAndVariableDeclaration)
        , typeDecl(_typeDecl), varDecl(_varDecl) {}

    virtual TypeAndVariableDeclaration *asTypeAndVariableDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    TypeDeclaration *typeDecl;
    VariableDeclaration *varDecl;
};

class InvariantDeclaration: public Declaration
{
public:
    InvariantDeclaration(const QString *_name)
        : Declaration(Kind_InvariantDeclaration), name(_name) {}

    virtual InvariantDeclaration *asInvariantDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    const QString *name;
};

class InitDeclaration: public Declaration
{
public:
    InitDeclaration(List<Declaration *> *_decls)
        : Declaration(Kind_InitDeclaration), decls(finish(_decls)) {}

    virtual InitDeclaration *asInitDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    List<Declaration *> *decls;
};

class FunctionDeclaration : public Declaration
{
public:
    FunctionDeclaration(Type *_returnType, const QString *_name)
        : Declaration(Kind_FunctionDeclaration), returnType(_returnType)
        , name(_name), params(0), body(0) {}

    virtual FunctionDeclaration *asFunctionDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

    void finishParams() { params = finish(params); }

    bool isPrototype() const { return body == 0; }

public: // attributes
    Type *returnType;
    const QString *name;
    List<ParameterDeclaration *> *params;
    Statement *body;
};

} // namespace GLSL

#endif // GLSLAST_H
