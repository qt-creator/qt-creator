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

#ifndef GLSLAST_H
#define GLSLAST_H

#include "glsl.h"
#include "glslmemorypool.h"
#include <qstring.h>

namespace GLSL {

class AST;
class TranslationUnitAST;
class ExpressionAST;
class IdentifierExpressionAST;
class LiteralExpressionAST;
class BinaryExpressionAST;
class UnaryExpressionAST;
class TernaryExpressionAST;
class AssignmentExpressionAST;
class MemberAccessExpressionAST;
class FunctionCallExpressionAST;
class FunctionIdentifierAST;
class DeclarationExpressionAST;
class StatementAST;
class ExpressionStatementAST;
class CompoundStatementAST;
class IfStatementAST;
class WhileStatementAST;
class DoStatementAST;
class ForStatementAST;
class JumpStatementAST;
class ReturnStatementAST;
class SwitchStatementAST;
class CaseLabelStatementAST;
class DeclarationStatementAST;
class TypeAST;
class BasicTypeAST;
class NamedTypeAST;
class ArrayTypeAST;
class StructTypeAST;
class LayoutQualifierAST;
class QualifiedTypeAST;
class DeclarationAST;
class PrecisionDeclarationAST;
class ParameterDeclarationAST;
class VariableDeclarationAST;
class TypeDeclarationAST;
class TypeAndVariableDeclarationAST;
class InvariantDeclarationAST;
class InitDeclarationAST;
class FunctionDeclarationAST;
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
        Kind_LayoutQualifier,
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

    virtual TranslationUnitAST *asTranslationUnit() { return 0; }

    virtual ExpressionAST *asExpression() { return 0; }
    virtual IdentifierExpressionAST *asIdentifierExpression() { return 0; }
    virtual LiteralExpressionAST *asLiteralExpression() { return 0; }
    virtual BinaryExpressionAST *asBinaryExpression() { return 0; }
    virtual UnaryExpressionAST *asUnaryExpression() { return 0; }
    virtual TernaryExpressionAST *asTernaryExpression() { return 0; }
    virtual AssignmentExpressionAST *asAssignmentExpression() { return 0; }
    virtual MemberAccessExpressionAST *asMemberAccessExpression() { return 0; }
    virtual FunctionCallExpressionAST *asFunctionCallExpression() { return 0; }
    virtual FunctionIdentifierAST *asFunctionIdentifier() { return 0; }
    virtual DeclarationExpressionAST *asDeclarationExpression() { return 0; }

    virtual StatementAST *asStatement() { return 0; }
    virtual ExpressionStatementAST *asExpressionStatement() { return 0; }
    virtual CompoundStatementAST *asCompoundStatement() { return 0; }
    virtual IfStatementAST *asIfStatement() { return 0; }
    virtual WhileStatementAST *asWhileStatement() { return 0; }
    virtual DoStatementAST *asDoStatement() { return 0; }
    virtual ForStatementAST *asForStatement() { return 0; }
    virtual JumpStatementAST *asJumpStatement() { return 0; }
    virtual ReturnStatementAST *asReturnStatement() { return 0; }
    virtual SwitchStatementAST *asSwitchStatement() { return 0; }
    virtual CaseLabelStatementAST *asCaseLabelStatement() { return 0; }
    virtual DeclarationStatementAST *asDeclarationStatement() { return 0; }

    virtual TypeAST *asType() { return 0; }
    virtual BasicTypeAST *asBasicType() { return 0; }
    virtual NamedTypeAST *asNamedType() { return 0; }
    virtual ArrayTypeAST *asArrayType() { return 0; }
    virtual StructTypeAST *asStructType() { return 0; }
    virtual QualifiedTypeAST *asQualifiedType() { return 0; }
    virtual LayoutQualifierAST *asLayoutQualifier() { return 0; }

    virtual DeclarationAST *asDeclaration() { return 0; }
    virtual PrecisionDeclarationAST *asPrecisionDeclaration() { return 0; }
    virtual ParameterDeclarationAST *asParameterDeclaration() { return 0; }
    virtual VariableDeclarationAST *asVariableDeclaration() { return 0; }
    virtual TypeDeclarationAST *asTypeDeclaration() { return 0; }
    virtual TypeAndVariableDeclarationAST *asTypeAndVariableDeclaration() { return 0; }
    virtual InvariantDeclarationAST *asInvariantDeclaration() { return 0; }
    virtual InitDeclarationAST *asInitDeclaration() { return 0; }
    virtual FunctionDeclarationAST *asFunctionDeclaration() { return 0; }

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

class GLSL_EXPORT TranslationUnitAST: public AST
{
public:
    TranslationUnitAST(List<DeclarationAST *> *declarations)
        : AST(Kind_TranslationUnit), declarations(finish(declarations)) {}

    virtual TranslationUnitAST *asTranslationUnit() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    List<DeclarationAST *> *declarations;
};

class GLSL_EXPORT ExpressionAST: public AST
{
protected:
    ExpressionAST(Kind _kind) : AST(_kind) {}

public:
    virtual ExpressionAST *asExpression() { return this; }
};

class GLSL_EXPORT IdentifierExpressionAST: public ExpressionAST
{
public:
    IdentifierExpressionAST(const QString *_name)
        : ExpressionAST(Kind_Identifier), name(_name) {}

    virtual IdentifierExpressionAST *asIdentifierExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    const QString *name;
};

class GLSL_EXPORT LiteralExpressionAST: public ExpressionAST
{
public:
    LiteralExpressionAST(const QString *_value)
        : ExpressionAST(Kind_Literal), value(_value) {}

    virtual LiteralExpressionAST *asLiteralExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    const QString *value;
};

class GLSL_EXPORT BinaryExpressionAST: public ExpressionAST
{
public:
    BinaryExpressionAST(Kind _kind, ExpressionAST *_left, ExpressionAST *_right)
        : ExpressionAST(_kind), left(_left), right(_right) {}

    virtual BinaryExpressionAST *asBinaryExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    ExpressionAST *left;
    ExpressionAST *right;
};

class GLSL_EXPORT UnaryExpressionAST: public ExpressionAST
{
public:
    UnaryExpressionAST(Kind _kind, ExpressionAST *_expr)
        : ExpressionAST(_kind), expr(_expr) {}

    virtual UnaryExpressionAST *asUnaryExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    ExpressionAST *expr;
};

class GLSL_EXPORT TernaryExpressionAST: public ExpressionAST
{
public:
    TernaryExpressionAST(Kind _kind, ExpressionAST *_first, ExpressionAST *_second, ExpressionAST *_third)
        : ExpressionAST(_kind), first(_first), second(_second), third(_third) {}

    virtual TernaryExpressionAST *asTernaryExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    ExpressionAST *first;
    ExpressionAST *second;
    ExpressionAST *third;
};

class GLSL_EXPORT AssignmentExpressionAST: public ExpressionAST
{
public:
    AssignmentExpressionAST(Kind _kind, ExpressionAST *_variable, ExpressionAST *_value)
        : ExpressionAST(_kind), variable(_variable), value(_value) {}

    virtual AssignmentExpressionAST *asAssignmentExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    ExpressionAST *variable;
    ExpressionAST *value;
};

class GLSL_EXPORT MemberAccessExpressionAST: public ExpressionAST
{
public:
    MemberAccessExpressionAST(ExpressionAST *_expr, const QString *_field)
        : ExpressionAST(Kind_MemberAccess), expr(_expr), field(_field) {}

    virtual MemberAccessExpressionAST *asMemberAccessExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    ExpressionAST *expr;
    const QString *field;
};

class GLSL_EXPORT FunctionCallExpressionAST: public ExpressionAST
{
public:
    FunctionCallExpressionAST(FunctionIdentifierAST *_id,
                           List<ExpressionAST *> *_arguments)
        : ExpressionAST(Kind_FunctionCall), expr(0), id(_id)
        , arguments(finish(_arguments)) {}
    FunctionCallExpressionAST(ExpressionAST *_expr, FunctionIdentifierAST *_id,
                           List<ExpressionAST *> *_arguments)
        : ExpressionAST(Kind_MemberFunctionCall), expr(_expr), id(_id)
        , arguments(finish(_arguments)) {}

    virtual FunctionCallExpressionAST *asFunctionCallExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    ExpressionAST *expr;
    FunctionIdentifierAST *id;
    List<ExpressionAST *> *arguments;
};

class GLSL_EXPORT FunctionIdentifierAST: public AST
{
public:
    FunctionIdentifierAST(const QString *_name)
        : AST(Kind_FunctionIdentifier), name(_name), type(0) {}
    FunctionIdentifierAST(TypeAST *_type)
        : AST(Kind_FunctionIdentifier), name(0), type(_type) {}

    virtual FunctionIdentifierAST *asFunctionIdentifier() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    const QString *name;
    TypeAST *type;
};

class GLSL_EXPORT DeclarationExpressionAST: public ExpressionAST
{
public:
    DeclarationExpressionAST(TypeAST *_type, const QString *_name,
                          ExpressionAST *_initializer)
        : ExpressionAST(Kind_DeclarationExpression), type(_type)
        , name(_name), initializer(_initializer) {}

    virtual DeclarationExpressionAST *asDeclarationExpression() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    TypeAST *type;
    const QString *name;
    ExpressionAST *initializer;
};

class GLSL_EXPORT StatementAST: public AST
{
protected:
    StatementAST(Kind _kind) : AST(_kind) {}

public:
    virtual StatementAST *asStatement() { return this; }
};

class GLSL_EXPORT ExpressionStatementAST: public StatementAST
{
public:
    ExpressionStatementAST(ExpressionAST *_expr)
        : StatementAST(Kind_ExpressionStatement), expr(_expr) {}

    virtual ExpressionStatementAST *asExpressionStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    ExpressionAST *expr;
};

class GLSL_EXPORT CompoundStatementAST: public StatementAST
{
public:
    CompoundStatementAST()
        : StatementAST(Kind_CompoundStatement), statements(0)
        , start(0), end(0), symbol(0) {}
    CompoundStatementAST(List<StatementAST *> *_statements)
        : StatementAST(Kind_CompoundStatement), statements(finish(_statements))
        , start(0), end(0), symbol(0) {}

    virtual CompoundStatementAST *asCompoundStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    List<StatementAST *> *statements;
    int start;
    int end;
    Block *symbol; // decoration
};

class GLSL_EXPORT IfStatementAST: public StatementAST
{
public:
    IfStatementAST(ExpressionAST *_condition, StatementAST *_thenClause, StatementAST *_elseClause)
        : StatementAST(Kind_If), condition(_condition)
        , thenClause(_thenClause), elseClause(_elseClause) {}

    virtual IfStatementAST *asIfStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    ExpressionAST *condition;
    StatementAST *thenClause;
    StatementAST *elseClause;
};

class GLSL_EXPORT WhileStatementAST: public StatementAST
{
public:
    WhileStatementAST(ExpressionAST *_condition, StatementAST *_body)
        : StatementAST(Kind_While), condition(_condition), body(_body) {}

    virtual WhileStatementAST *asWhileStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    ExpressionAST *condition;
    StatementAST *body;
};

class GLSL_EXPORT DoStatementAST: public StatementAST
{
public:
    DoStatementAST(StatementAST *_body, ExpressionAST *_condition)
        : StatementAST(Kind_Do), body(_body), condition(_condition) {}

    virtual DoStatementAST *asDoStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    StatementAST *body;
    ExpressionAST *condition;
};

class GLSL_EXPORT ForStatementAST: public StatementAST
{
public:
    ForStatementAST(StatementAST *_init, ExpressionAST *_condition, ExpressionAST *_increment, StatementAST *_body)
        : StatementAST(Kind_For), init(_init), condition(_condition), increment(_increment), body(_body) {}

    virtual ForStatementAST *asForStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    StatementAST *init;
    ExpressionAST *condition;
    ExpressionAST *increment;
    StatementAST *body;
};

class GLSL_EXPORT JumpStatementAST: public StatementAST
{
public:
    JumpStatementAST(Kind _kind) : StatementAST(_kind) {}

    virtual JumpStatementAST *asJumpStatement() { return this; }

    virtual void accept0(Visitor *visitor);
};

class GLSL_EXPORT ReturnStatementAST: public StatementAST
{
public:
    ReturnStatementAST() : StatementAST(Kind_Return), expr(0) {}
    ReturnStatementAST(ExpressionAST *_expr)
        : StatementAST(Kind_ReturnExpression), expr(_expr) {}

    virtual ReturnStatementAST *asReturnStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    ExpressionAST *expr;
};

class GLSL_EXPORT SwitchStatementAST: public StatementAST
{
public:
    SwitchStatementAST(ExpressionAST *_expr, StatementAST *_body)
        : StatementAST(Kind_Switch), expr(_expr), body(_body) {}

    virtual SwitchStatementAST *asSwitchStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    ExpressionAST *expr;
    StatementAST *body;
};

class GLSL_EXPORT CaseLabelStatementAST: public StatementAST
{
public:
    CaseLabelStatementAST() : StatementAST(Kind_DefaultLabel), expr(0) {}
    CaseLabelStatementAST(ExpressionAST *_expr)
        : StatementAST(Kind_CaseLabel), expr(_expr) {}

    virtual CaseLabelStatementAST *asCaseLabelStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    ExpressionAST *expr;
};

class GLSL_EXPORT DeclarationStatementAST: public StatementAST
{
public:
    DeclarationStatementAST(DeclarationAST *_decl)
        : StatementAST(Kind_DeclarationStatement), decl(_decl) {}

    virtual DeclarationStatementAST *asDeclarationStatement() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    DeclarationAST *decl;
};

class GLSL_EXPORT TypeAST: public AST
{
protected:
    TypeAST(Kind _kind) : AST(_kind) {}

public:
    enum Precision
    {
        PrecNotValid,       // Precision not valid (e.g. structs).
        PrecUnspecified,    // Precision not known, but can be validly set.
        Lowp,
        Mediump,
        Highp
    };

    virtual TypeAST *asType() { return this; }

    virtual Precision precision() const = 0;

    // Set the precision for the innermost basic type.  Returns false if it
    // is not valid to set a precision (e.g. structs).
    virtual bool setPrecision(Precision precision) = 0;
};

class GLSL_EXPORT BasicTypeAST: public TypeAST
{
public:
    // Pass the parser's token code: T_VOID, T_VEC4, etc.
    BasicTypeAST(int _token, const char *_name);

    virtual BasicTypeAST *asBasicType() { return this; }

    virtual void accept0(Visitor *visitor);

    virtual Precision precision() const;
    virtual bool setPrecision(Precision precision);

public: // attributes
    Precision prec;
    int token;
    const char *name;
};

class GLSL_EXPORT NamedTypeAST: public TypeAST
{
public:
    NamedTypeAST(const QString *_name) : TypeAST(Kind_NamedType), name(_name) {}

    virtual NamedTypeAST *asNamedType() { return this; }

    virtual void accept0(Visitor *visitor);

    virtual Precision precision() const;
    virtual bool setPrecision(Precision precision);

public: // attributes
    const QString *name;
};

class GLSL_EXPORT ArrayTypeAST: public TypeAST
{
public:
    ArrayTypeAST(TypeAST *_elementType)
        : TypeAST(Kind_OpenArrayType), elementType(_elementType), size(0) {}
    ArrayTypeAST(TypeAST *_elementType, ExpressionAST *_size)
        : TypeAST(Kind_ArrayType), elementType(_elementType), size(_size) {}

    virtual ArrayTypeAST *asArrayType() { return this; }

    virtual void accept0(Visitor *visitor);

    virtual Precision precision() const;
    virtual bool setPrecision(Precision precision);

public: // attributes
    TypeAST *elementType;
    ExpressionAST *size;
};

class GLSL_EXPORT StructTypeAST: public TypeAST
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
        Field(const QString *_name, TypeAST *_type)
            : AST(Kind_StructField), name(_name), type(_type) {}

        virtual void accept0(Visitor *visitor);

        void setInnerType(TypeAST *innerType);

        const QString *name;
        TypeAST *type;
    };

    StructTypeAST(List<Field *> *_fields)
        : TypeAST(Kind_AnonymousStructType), fields(finish(_fields)) {}
    StructTypeAST(const QString *_name, List<Field *> *_fields)
        : TypeAST(Kind_StructType), name(_name), fields(finish(_fields)) {}

    virtual StructTypeAST *asStructType() { return this; }

    virtual void accept0(Visitor *visitor);

    virtual Precision precision() const;
    virtual bool setPrecision(Precision precision);

    // Fix the inner types of a field list.  The "innerType" will
    // be copied into the "array holes" of all fields.
    static List<Field *> *fixInnerTypes(TypeAST *innerType, List<Field *> *fields);

public: // attributes
    const QString *name;
    List<Field *> *fields;
};

class GLSL_EXPORT LayoutQualifierAST: public AST
{
public:
    LayoutQualifierAST(const QString *_name, const QString *_number)
        : AST(Kind_LayoutQualifier), name(_name), number(_number) {}

    virtual LayoutQualifierAST *asLayoutQualifier() { return this; }
    virtual void accept0(Visitor *visitor);

public: // attributes
    const QString *name;
    const QString *number;
};

class GLSL_EXPORT QualifiedTypeAST: public TypeAST
{
public:
    QualifiedTypeAST(int _qualifiers, TypeAST *_type, List<LayoutQualifierAST *> *_layout_list)
        : TypeAST(Kind_QualifiedType), qualifiers(_qualifiers), type(_type)
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
        Invariant           = 0x00010000,
        Struct              = 0x00020000
    };

    virtual QualifiedTypeAST *asQualifiedType() { return this; }

    virtual void accept0(Visitor *visitor);

    virtual Precision precision() const { return type->precision(); }
    virtual bool setPrecision(Precision precision) { return type->setPrecision(precision); }

public: // attributes
    int qualifiers;
    TypeAST *type;
    List<LayoutQualifierAST *> *layout_list;
};

class GLSL_EXPORT DeclarationAST: public AST
{
protected:
    DeclarationAST(Kind _kind) : AST(_kind) {}

public:
    virtual DeclarationAST *asDeclaration() { return this; }
};

class GLSL_EXPORT PrecisionDeclarationAST: public DeclarationAST
{
public:
    PrecisionDeclarationAST(TypeAST::Precision _precision, TypeAST *_type)
        : DeclarationAST(Kind_PrecisionDeclaration)
        , precision(_precision), type(_type) {}

    virtual PrecisionDeclarationAST *asPrecisionDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    TypeAST::Precision precision;
    TypeAST *type;
};

class GLSL_EXPORT ParameterDeclarationAST: public DeclarationAST
{
public:
    enum Qualifier
    {
        In,
        Out,
        InOut
    };
    ParameterDeclarationAST(TypeAST *_type, Qualifier _qualifier,
                         const QString *_name)
        : DeclarationAST(Kind_ParameterDeclaration), type(_type)
        , qualifier(_qualifier), name(_name) {}

    virtual ParameterDeclarationAST *asParameterDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    TypeAST *type;
    Qualifier qualifier;
    const QString *name;
};

class GLSL_EXPORT VariableDeclarationAST: public DeclarationAST
{
public:
    VariableDeclarationAST(TypeAST *_type, const QString *_name,
                        ExpressionAST *_initializer = 0)
        : DeclarationAST(Kind_VariableDeclaration), type(_type)
        , name(_name), initializer(_initializer) {}

    virtual VariableDeclarationAST *asVariableDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

    static TypeAST *declarationType(List<DeclarationAST *> *decls);

public: // attributes
    TypeAST *type;
    const QString *name;
    ExpressionAST *initializer;
};

class GLSL_EXPORT TypeDeclarationAST: public DeclarationAST
{
public:
    TypeDeclarationAST(TypeAST *_type)
        : DeclarationAST(Kind_TypeDeclaration), type(_type) {}

    virtual TypeDeclarationAST *asTypeDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    TypeAST *type;
};

class GLSL_EXPORT TypeAndVariableDeclarationAST: public DeclarationAST
{
public:
    TypeAndVariableDeclarationAST(TypeDeclarationAST *_typeDecl,
                               VariableDeclarationAST *_varDecl)
        : DeclarationAST(Kind_TypeAndVariableDeclaration)
        , typeDecl(_typeDecl), varDecl(_varDecl) {}

    virtual TypeAndVariableDeclarationAST *asTypeAndVariableDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    TypeDeclarationAST *typeDecl;
    VariableDeclarationAST *varDecl;
};

class GLSL_EXPORT InvariantDeclarationAST: public DeclarationAST
{
public:
    InvariantDeclarationAST(const QString *_name)
        : DeclarationAST(Kind_InvariantDeclaration), name(_name) {}

    virtual InvariantDeclarationAST *asInvariantDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    const QString *name;
};

class GLSL_EXPORT InitDeclarationAST: public DeclarationAST
{
public:
    InitDeclarationAST(List<DeclarationAST *> *_decls)
        : DeclarationAST(Kind_InitDeclaration), decls(finish(_decls)) {}

    virtual InitDeclarationAST *asInitDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

public: // attributes
    List<DeclarationAST *> *decls;
};

class GLSL_EXPORT FunctionDeclarationAST : public DeclarationAST
{
public:
    FunctionDeclarationAST(TypeAST *_returnType, const QString *_name)
        : DeclarationAST(Kind_FunctionDeclaration), returnType(_returnType)
        , name(_name), params(0), body(0) {}

    virtual FunctionDeclarationAST *asFunctionDeclaration() { return this; }

    virtual void accept0(Visitor *visitor);

    void finishParams() { params = finish(params); }

    bool isPrototype() const { return body == 0; }

public: // attributes
    TypeAST *returnType;
    const QString *name;
    List<ParameterDeclarationAST *> *params;
    StatementAST *body;
};

} // namespace GLSL

#endif // GLSLAST_H
