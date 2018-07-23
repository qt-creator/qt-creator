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

#include "globalparsestate.h"

namespace Debugger {
namespace Internal {

class ParseTreeNode
{
public:
    using Ptr = QSharedPointer<ParseTreeNode>;

    virtual ~ParseTreeNode();
    virtual QByteArray toByteArray() const = 0;
    virtual ParseTreeNode::Ptr clone() const = 0;

    int childCount() const { return m_children.count(); }
    void addChild(ParseTreeNode::Ptr childNode) { m_children << childNode; }
    ParseTreeNode::Ptr childAt(int i, const QString &func, const QString &file, int line) const;
    QByteArray pasteAllChildren() const;

    void print(int indentation) const; // For debugging.

    template <typename T> static Ptr parseRule(GlobalParseState *parseState)
    {
        const Ptr node = Ptr(new T(parseState));
        parseState->pushToStack(node);
        parseState->stackTop()->parse();
        return node;
    }

protected:
    ParseTreeNode(GlobalParseState *parseState) : m_parseState(parseState) {}
    ParseTreeNode(const ParseTreeNode &other);
    QByteArray bool2String(bool b) const;

    GlobalParseState *parseState() const { return m_parseState; }

private:
    virtual void parse() = 0;
    virtual QByteArray description() const = 0; // For debugging.

    QList<ParseTreeNode::Ptr > m_children; // Convention: Children are inserted in parse order.
    GlobalParseState * const m_parseState;
};

class ArrayTypeNode : public ParseTreeNode
{
public:
    ArrayTypeNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}

    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const override;

private:
    ArrayTypeNode(const ArrayTypeNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new ArrayTypeNode(*this)); }

    void parse() override;
    QByteArray description() const override { return "ArrayType"; }
};

class BareFunctionTypeNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<BareFunctionTypeNode>;
    BareFunctionTypeNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    bool hasReturnType() const { return m_hasReturnType; }
    QByteArray toByteArray() const override;

private:
    BareFunctionTypeNode(const BareFunctionTypeNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new BareFunctionTypeNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    bool m_hasReturnType = false;
};

class BuiltinTypeNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<BuiltinTypeNode>;
    BuiltinTypeNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

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
    BuiltinTypeNode(const BuiltinTypeNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new BuiltinTypeNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    Type m_type; // TODO: define?
};

class CallOffsetRule
{
public:
    static bool mangledRepresentationStartsWith(char c);
    static void parse(GlobalParseState *parseState);

private:
    CallOffsetRule();
};

class NvOffsetNode : public ParseTreeNode
{
public:
    NvOffsetNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    QByteArray toByteArray() const override { return QByteArray(); } // TODO: How to encode this?

private:
    NvOffsetNode(const NvOffsetNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new NvOffsetNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "NvOffset"; }
};

class VOffsetNode : public ParseTreeNode
{
public:
    VOffsetNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    QByteArray toByteArray() const override { return QByteArray(); } // TODO: How to encode this?

private:
    VOffsetNode(const VOffsetNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new VOffsetNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "VOffset"; }
};

class ClassEnumTypeRule
{
public:
    static bool mangledRepresentationStartsWith(char c);
    static void parse(GlobalParseState *parseState);

private:
    ClassEnumTypeRule();
};

class DiscriminatorRule
{
public:
    static bool mangledRepresentationStartsWith(char c);
    static void parse(GlobalParseState *parseState);

private:
    DiscriminatorRule();
};

class CtorDtorNameNode : public ParseTreeNode
{
public:
    CtorDtorNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    CtorDtorNameNode(const CtorDtorNameNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new CtorDtorNameNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    bool m_isDestructor; // TODO: define?
    QByteArray m_representation;
};

class CvQualifiersNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<CvQualifiersNode>;
    CvQualifiersNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    bool hasQualifiers() const { return m_hasConst || m_hasVolatile; }
    QByteArray toByteArray() const override;
private:
    CvQualifiersNode(const CvQualifiersNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new CvQualifiersNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "CvQualifiers[" + toByteArray() + ']'; }

    bool m_hasConst = false;
    bool m_hasVolatile = false;
};

class EncodingNode : public ParseTreeNode
{
public:
    EncodingNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    EncodingNode(const EncodingNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new EncodingNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "Encoding"; }
};

class ExpressionNode : public ParseTreeNode
{
public:
    ExpressionNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    ExpressionNode(const ExpressionNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new ExpressionNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    enum Type {
        ConversionType, SizeofType, AlignofType, OperatorType, ParameterPackSizeType,
        NewType, ArrayNewType, DeleteType, ArrayDeleteType, PrefixIncrementType,
        PrefixDecrementType, TypeIdTypeType, TypeIdExpressionType, DynamicCastType,
        StaticCastType, ConstCastType, ReinterpretCastType, MemberAccessType,
        PointerMemberAccessType, MemberDerefType, PackExpansionType, ThrowType,
        RethrowType, OtherType
    } m_type; // TODO: define?
    bool m_globalNamespace = false;
};

class OperatorNameNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<OperatorNameNode>;
    OperatorNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
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

    QByteArray toByteArray() const override;

private:
    OperatorNameNode(const OperatorNameNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new OperatorNameNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    Type m_type = VendorType;
};

class ExprPrimaryNode : public ParseTreeNode
{
public:
    ExprPrimaryNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const override;

private:
    ExprPrimaryNode(const ExprPrimaryNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new ExprPrimaryNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    QByteArray m_suffix;
    bool m_isNullPtr = false;
};

class FunctionTypeNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<FunctionTypeNode>;
    FunctionTypeNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    bool isExternC() const { return m_isExternC; }
    QByteArray toByteArray() const override;

private:
    FunctionTypeNode(const FunctionTypeNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new FunctionTypeNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    bool m_isExternC = false;
};

class LocalNameNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<LocalNameNode>;
    LocalNameNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;
    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
    CvQualifiersNode::Ptr cvQualifiers() const;

private:
    LocalNameNode(const LocalNameNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new LocalNameNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    bool m_isStringLiteral = false;
    bool m_isDefaultArg = false;
};

class MangledNameRule
{
public:
    static bool mangledRepresentationStartsWith(char c);
    static void parse(GlobalParseState *parseState, const ParseTreeNode::Ptr &parentNode);

private:
    MangledNameRule();
};

class NumberNode : public ParseTreeNode
{
public:
    NumberNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    NumberNode(const NumberNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new NumberNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    bool m_isNegative = false;
};

class SourceNameNode : public ParseTreeNode
{
public:
    SourceNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override { return m_name; }

private:
    SourceNameNode(const SourceNameNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new SourceNameNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    QByteArray m_name;
};

class UnqualifiedNameNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<UnqualifiedNameNode>;
    UnqualifiedNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    bool isConstructorOrDestructorOrConversionOperator() const;
    QByteArray toByteArray() const override;

private:
    UnqualifiedNameNode(const UnqualifiedNameNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new UnqualifiedNameNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "UnqualifiedName"; }
};

class UnscopedNameNode : public ParseTreeNode
{
public:
    UnscopedNameNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    bool isConstructorOrDestructorOrConversionOperator() const;
    QByteArray toByteArray() const override;

private:
    UnscopedNameNode(const UnscopedNameNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new UnscopedNameNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    bool m_inStdNamespace = false;
};

class NestedNameNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<NestedNameNode>;
    NestedNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState ){}

    static bool mangledRepresentationStartsWith(char c);

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
    CvQualifiersNode::Ptr cvQualifiers() const;

    QByteArray toByteArray() const override;

private:
    NestedNameNode(const NestedNameNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new NestedNameNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "NestedName"; }
};

class SubstitutionNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<SubstitutionNode>;
    SubstitutionNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

    enum Type {
        ActualSubstitutionType, StdType, StdAllocType, StdBasicStringType, FullStdBasicStringType,
        StdBasicIStreamType, StdBasicOStreamType, StdBasicIoStreamType
    };
    Type type() const { return m_type; }

private:
    SubstitutionNode(const SubstitutionNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new SubstitutionNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    Type m_type; // TODO: define?
};

class PointerToMemberTypeNode : public ParseTreeNode
{
public:
    PointerToMemberTypeNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    PointerToMemberTypeNode(const PointerToMemberTypeNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new PointerToMemberTypeNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "PointerToMember"; }
};

class TemplateParamNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<TemplateParamNode>;

    TemplateParamNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}

    static bool mangledRepresentationStartsWith(char c);

    int index() const { return m_index; }

    QByteArray toByteArray() const override;

private:
    TemplateParamNode(const TemplateParamNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new TemplateParamNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    int m_index; // TODO: define?
};

class TemplateArgsNode : public ParseTreeNode
{
public:
    TemplateArgsNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    TemplateArgsNode(const TemplateArgsNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new TemplateArgsNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "TemplateArgs"; }
};

class SpecialNameNode : public ParseTreeNode
{
public:
    SpecialNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    SpecialNameNode(const SpecialNameNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new SpecialNameNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    enum Type {
        VirtualTableType, VttStructType, TypeInfoType, TypeInfoNameType, GuardVarType,
        SingleCallOffsetType, DoubleCallOffsetType
    } m_type; // TODO: define?
};

template<int base> class NonNegativeNumberNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<NonNegativeNumberNode<base> >;
    NonNegativeNumberNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c) {
        // Base can only be 10 or 36.
        return (c >= '0' && c <= '9') || (base == 36 && c >= 'A' && c <= 'Z');
    }
    quint64 number() const { return m_number; }
    QByteArray toByteArray() const override;

private:
    NonNegativeNumberNode(const NonNegativeNumberNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new NonNegativeNumberNode<base>(*this)); }
    void parse() override;
    QByteArray description() const override;

    quint64 m_number; // TODO: define?
};

class NameNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<NameNode>;

    NameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}

    static bool mangledRepresentationStartsWith(char c);

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
    CvQualifiersNode::Ptr cvQualifiers() const;

    QByteArray toByteArray() const override;

private:
    NameNode(const NameNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new NameNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "Name"; }
};

class TemplateArgNode : public ParseTreeNode
{
public:
    TemplateArgNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    TemplateArgNode(const TemplateArgNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new TemplateArgNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    bool m_isTemplateArgumentPack = false;
};

class PrefixNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<PrefixNode>;
    PrefixNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}

    static bool mangledRepresentationStartsWith(char c);

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;

    QByteArray toByteArray() const override;

private:
    PrefixNode(const PrefixNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new PrefixNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "Prefix"; }
};

class TypeNode : public ParseTreeNode
{
public:
    using Ptr = QSharedPointer<TypeNode>;

    TypeNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}

    static bool mangledRepresentationStartsWith(char c);

    enum Type {
        QualifiedType, PointerType, ReferenceType, RValueType, VendorType, PackExpansionType,
        SubstitutionType, OtherType
    };
    Type type() const { return m_type; }

    QByteArray toByteArray() const override;

private:
    TypeNode(const TypeNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new TypeNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    QByteArray toByteArrayQualPointerRef(const TypeNode *typeNode,
            const QByteArray &qualPtrRef) const;
    QByteArray qualPtrRefListToByteArray(const QList<const ParseTreeNode *> &nodeList) const;

    Type m_type = OtherType;
};

class FloatValueNode : public ParseTreeNode
{
public:
    FloatValueNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    FloatValueNode(const FloatValueNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new FloatValueNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    double m_value; // TODO: define?
};

class LambdaSigNode : public ParseTreeNode
{
public:
    LambdaSigNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    LambdaSigNode(const LambdaSigNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new LambdaSigNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "LambdaSig"; }
};

class ClosureTypeNameNode : public ParseTreeNode
{
public:
    ClosureTypeNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    QByteArray toByteArray() const override;

private:
    ClosureTypeNameNode(const ClosureTypeNameNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new ClosureTypeNameNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "ClosureType"; }
};

class UnnamedTypeNameNode : public ParseTreeNode
{
public:
    UnnamedTypeNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    UnnamedTypeNameNode(const UnnamedTypeNameNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new UnnamedTypeNameNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "UnnnamedType"; }
};

class DeclTypeNode : public ParseTreeNode
{
public:
    DeclTypeNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    DeclTypeNode(const DeclTypeNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new DeclTypeNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "DeclType"; }
};

class UnresolvedTypeRule
{
public:
    static bool mangledRepresentationStartsWith(char c);
    static void parse(GlobalParseState *parseState);

private:
    UnresolvedTypeRule();
};

class SimpleIdNode : public ParseTreeNode
{
public:
    SimpleIdNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    SimpleIdNode(const SimpleIdNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new SimpleIdNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "SimpleId"; }
};

class DestructorNameNode : public ParseTreeNode
{
public:
    DestructorNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    DestructorNameNode(const DestructorNameNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new DestructorNameNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "DesctuctorName"; }
};

class UnresolvedQualifierLevelRule
{
public:
    static bool mangledRepresentationStartsWith(char c);
    static void parse(GlobalParseState *parseState);

private:
    UnresolvedQualifierLevelRule();
};

class BaseUnresolvedNameNode : public ParseTreeNode
{
public:
    BaseUnresolvedNameNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    BaseUnresolvedNameNode(const BaseUnresolvedNameNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new BaseUnresolvedNameNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    bool m_isOperator = false;
};

class InitializerNode : public ParseTreeNode
{
public:
    InitializerNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    InitializerNode(const InitializerNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new InitializerNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "Initializer"; }
};

class UnresolvedNameNode : public ParseTreeNode
{
public:
    UnresolvedNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    UnresolvedNameNode(const UnresolvedNameNode &other);
    ParseTreeNode::Ptr clone() const override { return Ptr(new UnresolvedNameNode(*this)); }
    void parse() override;
    QByteArray description() const override;

    bool m_globalNamespace; // TODO: define?
};

class FunctionParamNode : public ParseTreeNode
{
public:
    FunctionParamNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const override;

private:
    FunctionParamNode(const FunctionParamNode &other) = default;
    ParseTreeNode::Ptr clone() const override { return Ptr(new FunctionParamNode(*this)); }
    void parse() override;
    QByteArray description() const override { return "FunctionParam"; }
};

} // namespace Internal
} // namespace Debugger
