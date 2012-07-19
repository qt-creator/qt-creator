/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/
#ifndef PARSETREENODES_H
#define PARSETREENODES_H

#include "globalparsestate.h"

#include <QByteArray>
#include <QList>
#include <QSet>

namespace Debugger {
namespace Internal {

class ParseTreeNode
{
public:
    virtual ~ParseTreeNode();
    virtual QByteArray toByteArray() const = 0;

    int childCount() const { return m_children.count(); }
    void addChild(ParseTreeNode *childNode) { m_children << childNode; }
    ParseTreeNode *childAt(int i, const QString &func, const QString &file, int line) const;
    QByteArray pasteAllChildren() const;

    template <typename T> static T *parseRule(GlobalParseState *parseState)
    {
        T * const node = new T;
        node->m_parseState = parseState;
        parseState->pushToStack(node);
        parseState->stackTop()->parse();
        return node;
    }

protected:
    GlobalParseState *parseState() const { return m_parseState; }

    void clearChildList() { m_children.clear(); }

private:
    virtual void parse() = 0;

    QList<ParseTreeNode *> m_children; // Convention: Children are inserted in parse order.
    GlobalParseState *m_parseState;
};

class ArrayTypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();
};

class BareFunctionTypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    bool hasReturnType() const { return m_hasReturnType; }

    QByteArray toByteArray() const;

private:
    void parse();

    bool m_hasReturnType;
};

class BuiltinTypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    enum Type {
        VoidType, WCharType, BoolType,
        PlainCharType, SignedCharType, UnsignedCharType, SignedShortType, UnsignedShortType,
        SignedIntType, UnsignedIntType, SignedLongType, UnsignedLongType,
        SignedLongLongType, UnsignedLongLongType, SignedInt128Type, UnsignedInt128Type,
        FloatType, DoubleType, LongDoubleType, Float128Type, EllipsisType,
        DecimalFloatingType64, DecimalFloatingType128, DecimalFloatingType32,
        DecimalFloatingType16, Char32Type, Char16Type, AutoType, NullPtrType, VendorType
    };
    Type type() const { return m_type; }

private:
    void parse();

    Type m_type;
};

class CallOffsetRule
{
public:
    static bool mangledRepresentationStartsWith(char c);
    static void parse(GlobalParseState *parseState, ParseTreeNode *parentNode);

private:
    CallOffsetRule();
};

class NvOffsetNode : public ParseTreeNode
{
public:
    QByteArray toByteArray() const { return QByteArray(); } // TODO: How to encode this?

private:
    void parse();
};

class VOffsetNode : public ParseTreeNode
{
public:
    QByteArray toByteArray() const { return QByteArray(); } // TODO: How to encode this?

private:
    void parse();
};

class ClassEnumTypeRule
{
public:
    static bool mangledRepresentationStartsWith(char c);
    static void parse(GlobalParseState *parseState, ParseTreeNode *parentNode);

private:
    ClassEnumTypeRule();
};

class DiscriminatorRule
{
public:
    static bool mangledRepresentationStartsWith(char c);
    static void parse(GlobalParseState *parseState, ParseTreeNode *parentNode);

private:
    DiscriminatorRule();
};

class CtorDtorNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();

    bool m_isDestructor;
    QByteArray m_representation;
};

class CvQualifiersNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    bool hasQualifiers() const { return m_hasConst || m_hasVolatile; }

    QByteArray toByteArray() const;
private:
    void parse();

    bool m_hasVolatile;
    bool m_hasConst;
};

class EncodingNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();
};

class ExpressionNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();

    enum Type {
        ConversionType, SizeofType, AlignofType, OperatorType, ParameterPackSizeType,
        NewType, ArrayNewType, DeleteType, ArrayDeleteType, PrefixIncrementType,
        PrefixDecrementType, TypeIdTypeType, TypeIdExpressionType, DynamicCastType,
        StaticCastType, ConstCastType, ReinterpretCastType, MemberAccessType,
        PointerMemberAccessType, MemberDerefType, PackExpansionType, ThrowType,
        RethrowType, OtherType
    } m_type;
    bool m_globalNamespace;
};

class OperatorNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    enum Type {
        NewType, ArrayNewType, DeleteType, ArrayDeleteType, UnaryPlusType, UnaryMinusType,
        UnaryAmpersandType, UnaryStarType, BitwiseNotType, BinaryPlusType, BinaryMinusType,
        MultType, DivType, ModuloType, BitwiseAndType, BitwiseOrType, XorType, AssignType,
        IncrementAndAssignType, DecrementAndAssignType, MultAndAssignType, DivAndAssignType,
        ModuloAndAssignType, BitwiseAndAndAssignType, BitwiseOrAndAssignType, XorAndAssignType,
        LeftShiftType, RightShiftType, LeftShiftAndAssignType, RightShiftAndAssignType, EqualsType,
        NotEqualsType, LessType, GreaterType, LessEqualType, GreaterEqualType, LogicalNotType,
        LogicalAndType, LogicalOrType, IncrementType, DecrementType, CommaType, ArrowStarType,
        ArrowType, CallType, IndexType, TernaryType, SizeofTypeType, SizeofExprType,
        AlignofTypeType, AlignofExprType, CastType, VendorType
    };
    Type type() const { return m_type; }

    QByteArray toByteArray() const;

private:
    void parse();

    Type m_type;
};

class ExprPrimaryNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();

    QByteArray m_suffix;
    bool m_isNullPtr;
};

class FunctionTypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    bool isExternC() const { return m_isExternC; }

    QByteArray toByteArray() const;

private:
    void parse();

    bool m_isExternC;
};

class LocalNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
    const CvQualifiersNode *cvQualifiers() const;

private:
    void parse();

    bool m_isStringLiteral;
    bool m_isDefaultArg;
};

class MangledNameRule
{
public:
    static bool mangledRepresentationStartsWith(char c);
    static void parse(GlobalParseState *parseState, ParseTreeNode *parentNode);

private:
    MangledNameRule();
};

class NumberNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();

    bool m_isNegative;
};

class SourceNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const { return m_name; }

private:
    void parse();

    QByteArray m_name;
};

class UnqualifiedNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    bool isConstructorOrDestructorOrConversionOperator() const;

    QByteArray toByteArray() const;

private:
    void parse();
};

class UnscopedNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    bool isConstructorOrDestructorOrConversionOperator() const;

    QByteArray toByteArray() const;

private:
    void parse();

    bool m_inStdNamespace;
};

class NestedNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
    const CvQualifiersNode *cvQualifiers() const;

    QByteArray toByteArray() const;

private:
    void parse();
};

class SubstitutionNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();

    enum Type {
        ActualSubstitutionType, StdType, StdAllocType, StdBasicStringType, FullStdBasicStringType,
        StdBasicIStreamType, StdBasicOStreamType, StdBasicIoStreamType
    } m_type;
    QByteArray m_substValue;
};

class PointerToMemberTypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();
};

class TemplateParamNode : public ParseTreeNode
{
public:
    ~TemplateParamNode();

    static bool mangledRepresentationStartsWith(char c);

    int index() const { return m_index; }

    QByteArray toByteArray() const;

private:
    void parse();

    int m_index;
};

class TemplateArgsNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();
};

class SpecialNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();

    enum Type {
        VirtualTableType, VttStructType, TypeInfoType, TypeInfoNameType, GuardVarType,
        SingleCallOffsetType, DoubleCallOffsetType
    } m_type;
};

template<int base> class NonNegativeNumberNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    quint64 number() const { return m_number; }

    QByteArray toByteArray() const;

private:
    void parse();

    quint64 m_number;
};

class NameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
    const CvQualifiersNode *cvQualifiers() const;

    QByteArray toByteArray() const;

private:
    void parse();
};

class TemplateArgNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();

    bool m_isTemplateArgumentPack;
};

class Prefix2Node : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;

    QByteArray toByteArray() const;

private:
    void parse();
};

class PrefixNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;

    QByteArray toByteArray() const;

private:
    void parse();
};

class TypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    enum Type {
        QualifiedType, PointerType, ReferenceType, RValueType, VendorType, PackExpansionType,
        OtherType
    };
    Type type() const { return m_type; }

    QByteArray toByteArray() const;

private:
    void parse();

    QByteArray toByteArrayQualPointerRef(const TypeNode *typeNode,
            const QByteArray &qualPtrRef) const;
    QByteArray qualPtrRefListToByteArray(const QList<const ParseTreeNode *> &nodeList) const;

    Type m_type;
};

class FloatValueNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();

    double m_value;
};

class LambdaSigNode : public ParseTreeNode
{
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();
};

class ClosureTypeNameNode : public ParseTreeNode
{
    QByteArray toByteArray() const;

private:
    void parse();
};

class UnnamedTypeNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();
};

class DeclTypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();
};

class UnresolvedTypeRule
{
public:
    static bool mangledRepresentationStartsWith(char c);
    static void parse(GlobalParseState *parseState, ParseTreeNode *parentNode);

private:
    UnresolvedTypeRule();
};

class SimpleIdNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();
};

class DestructorNameNode : public ParseTreeNode
{
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();
};

class UnresolvedQualifierLevelRule
{
public:
    static bool mangledRepresentationStartsWith(char c);
    static void parse(GlobalParseState *parseState, ParseTreeNode *parentNode);

private:
    UnresolvedQualifierLevelRule();
};

class BaseUnresolvedNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();

    bool m_isOperator;
};

class InitializerNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();
};

class UnresolvedNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();

    bool m_globalNamespace;
};

class FunctionParamNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    void parse();
};

} // namespace Internal
} // namespace Debugger

#endif // PARSETREENODES_H
