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
#ifndef PARSETREENODES_H
#define PARSETREENODES_H

#include "globalparsestate.h"

#include <QByteArray>
#include <QList>
#include <QSet>
#include <QSharedPointer>

namespace Debugger {
namespace Internal {

class ParseTreeNode
{
public:
    typedef QSharedPointer<ParseTreeNode> Ptr;

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

    QByteArray toByteArray() const;

private:
    ArrayTypeNode(const ArrayTypeNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new ArrayTypeNode(*this)); }

    void parse();
    QByteArray description() const { return "ArrayType"; }
};

class BareFunctionTypeNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<BareFunctionTypeNode> Ptr;
    BareFunctionTypeNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    bool hasReturnType() const { return m_hasReturnType; }
    QByteArray toByteArray() const;

private:
    BareFunctionTypeNode(const BareFunctionTypeNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new BareFunctionTypeNode(*this)); }
    void parse();
    QByteArray description() const;

    bool m_hasReturnType;
};

class BuiltinTypeNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<BuiltinTypeNode> Ptr;
    BuiltinTypeNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
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
    BuiltinTypeNode(const BuiltinTypeNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new BuiltinTypeNode(*this)); }
    void parse();
    QByteArray description() const;

    Type m_type;
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
    QByteArray toByteArray() const { return QByteArray(); } // TODO: How to encode this?

private:
    NvOffsetNode(const NvOffsetNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new NvOffsetNode(*this)); }
    void parse();
    QByteArray description() const { return "NvOffset"; }
};

class VOffsetNode : public ParseTreeNode
{
public:
    VOffsetNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    QByteArray toByteArray() const { return QByteArray(); } // TODO: How to encode this?

private:
    VOffsetNode(const VOffsetNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new VOffsetNode(*this)); }
    void parse();
    QByteArray description() const { return "VOffset"; }
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
    QByteArray toByteArray() const;

private:
    CtorDtorNameNode(const CtorDtorNameNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new CtorDtorNameNode(*this)); }
    void parse();
    QByteArray description() const;

    bool m_isDestructor;
    QByteArray m_representation;
};

class CvQualifiersNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<CvQualifiersNode> Ptr;
    CvQualifiersNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    bool hasQualifiers() const { return m_hasConst || m_hasVolatile; }
    QByteArray toByteArray() const;
private:
    CvQualifiersNode(const CvQualifiersNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new CvQualifiersNode(*this)); }
    void parse();
    QByteArray description() const { return "CvQualifiers[" + toByteArray() + ']'; }

    bool m_hasConst;
    bool m_hasVolatile;
};

class EncodingNode : public ParseTreeNode
{
public:
    EncodingNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    EncodingNode(const EncodingNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new EncodingNode(*this)); }
    void parse();
    QByteArray description() const { return "Encoding"; }
};

class ExpressionNode : public ParseTreeNode
{
public:
    ExpressionNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    ExpressionNode(const ExpressionNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new ExpressionNode(*this)); }
    void parse();
    QByteArray description() const;

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
    typedef QSharedPointer<OperatorNameNode> Ptr;
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

    QByteArray toByteArray() const;

private:
    OperatorNameNode(const OperatorNameNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new OperatorNameNode(*this)); }
    void parse();
    QByteArray description() const;

    Type m_type;
};

class ExprPrimaryNode : public ParseTreeNode
{
public:
    ExprPrimaryNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

private:
    ExprPrimaryNode(const ExprPrimaryNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new ExprPrimaryNode(*this)); }
    void parse();
    QByteArray description() const;

    QByteArray m_suffix;
    bool m_isNullPtr;
};

class FunctionTypeNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<FunctionTypeNode> Ptr;
    FunctionTypeNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    bool isExternC() const { return m_isExternC; }
    QByteArray toByteArray() const;

private:
    FunctionTypeNode(const FunctionTypeNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new FunctionTypeNode(*this)); }
    void parse();
    QByteArray description() const;

    bool m_isExternC;
};

class LocalNameNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<LocalNameNode> Ptr;
    LocalNameNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;
    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
    CvQualifiersNode::Ptr cvQualifiers() const;

private:
    LocalNameNode(const LocalNameNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new LocalNameNode(*this)); }
    void parse();
    QByteArray description() const;

    bool m_isStringLiteral;
    bool m_isDefaultArg;
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
    QByteArray toByteArray() const;

private:
    NumberNode(const NumberNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new NumberNode(*this)); }
    void parse();
    QByteArray description() const;

    bool m_isNegative;
};

class SourceNameNode : public ParseTreeNode
{
public:
    SourceNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const { return m_name; }

private:
    SourceNameNode(const SourceNameNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new SourceNameNode(*this)); }
    void parse();
    QByteArray description() const;

    QByteArray m_name;
};

class UnqualifiedNameNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<UnqualifiedNameNode> Ptr;
    UnqualifiedNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    bool isConstructorOrDestructorOrConversionOperator() const;
    QByteArray toByteArray() const;

private:
    UnqualifiedNameNode(const UnqualifiedNameNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new UnqualifiedNameNode(*this)); }
    void parse();
    QByteArray description() const { return "UnqualifiedName"; }
};

class UnscopedNameNode : public ParseTreeNode
{
public:
    UnscopedNameNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    bool isConstructorOrDestructorOrConversionOperator() const;
    QByteArray toByteArray() const;

private:
    UnscopedNameNode(const UnscopedNameNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new UnscopedNameNode(*this)); }
    void parse();
    QByteArray description() const;

    bool m_inStdNamespace;
};

class NestedNameNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<NestedNameNode> Ptr;
    NestedNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState ){}

    static bool mangledRepresentationStartsWith(char c);

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
    CvQualifiersNode::Ptr cvQualifiers() const;

    QByteArray toByteArray() const;

private:
    NestedNameNode(const NestedNameNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new NestedNameNode(*this)); }
    void parse();
    QByteArray description() const { return "NestedName"; }
};

class SubstitutionNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<SubstitutionNode> Ptr;
    SubstitutionNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

    enum Type {
        ActualSubstitutionType, StdType, StdAllocType, StdBasicStringType, FullStdBasicStringType,
        StdBasicIStreamType, StdBasicOStreamType, StdBasicIoStreamType
    };
    Type type() const { return m_type; }

private:
    SubstitutionNode(const SubstitutionNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new SubstitutionNode(*this)); }
    void parse();
    QByteArray description() const;

    Type m_type;
};

class PointerToMemberTypeNode : public ParseTreeNode
{
public:
    PointerToMemberTypeNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    PointerToMemberTypeNode(const PointerToMemberTypeNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new PointerToMemberTypeNode(*this)); }
    void parse();
    QByteArray description() const { return "PointerToMember"; }
};

class TemplateParamNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<TemplateParamNode> Ptr;

    TemplateParamNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}

    static bool mangledRepresentationStartsWith(char c);

    int index() const { return m_index; }

    QByteArray toByteArray() const;

private:
    TemplateParamNode(const TemplateParamNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new TemplateParamNode(*this)); }
    void parse();
    QByteArray description() const;

    int m_index;
};

class TemplateArgsNode : public ParseTreeNode
{
public:
    TemplateArgsNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    TemplateArgsNode(const TemplateArgsNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new TemplateArgsNode(*this)); }
    void parse();
    QByteArray description() const { return "TemplateArgs"; }
};

class SpecialNameNode : public ParseTreeNode
{
public:
    SpecialNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    SpecialNameNode(const SpecialNameNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new SpecialNameNode(*this)); }
    void parse();
    QByteArray description() const;

    enum Type {
        VirtualTableType, VttStructType, TypeInfoType, TypeInfoNameType, GuardVarType,
        SingleCallOffsetType, DoubleCallOffsetType
    } m_type;
};

template<int base> class NonNegativeNumberNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<NonNegativeNumberNode<base> > Ptr;
    NonNegativeNumberNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c) {
        // Base can only be 10 or 36.
        return (c >= '0' && c <= '9') || (base == 36 && c >= 'A' && c <= 'Z');
    }
    quint64 number() const { return m_number; }
    QByteArray toByteArray() const;

private:
    NonNegativeNumberNode(const NonNegativeNumberNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new NonNegativeNumberNode<base>(*this)); }
    void parse();
    QByteArray description() const;

    quint64 m_number;
};

class NameNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<NameNode> Ptr;

    NameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}

    static bool mangledRepresentationStartsWith(char c);

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
    CvQualifiersNode::Ptr cvQualifiers() const;

    QByteArray toByteArray() const;

private:
    NameNode(const NameNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new NameNode(*this)); }
    void parse();
    QByteArray description() const { return "Name"; }
};

class TemplateArgNode : public ParseTreeNode
{
public:
    TemplateArgNode(GlobalParseState *parseState);
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    TemplateArgNode(const TemplateArgNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new TemplateArgNode(*this)); }
    void parse();
    QByteArray description() const;

    bool m_isTemplateArgumentPack;
};

class PrefixNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<PrefixNode> Ptr;
    PrefixNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}

    static bool mangledRepresentationStartsWith(char c);

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;

    QByteArray toByteArray() const;

private:
    PrefixNode(const PrefixNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new PrefixNode(*this)); }
    void parse();
    QByteArray description() const { return "Prefix"; }
};

class TypeNode : public ParseTreeNode
{
public:
    typedef QSharedPointer<TypeNode> Ptr;

    TypeNode(GlobalParseState *parseState) : ParseTreeNode(parseState), m_type(OtherType) {}

    static bool mangledRepresentationStartsWith(char c);

    enum Type {
        QualifiedType, PointerType, ReferenceType, RValueType, VendorType, PackExpansionType,
        SubstitutionType, OtherType
    };
    Type type() const { return m_type; }

    QByteArray toByteArray() const;

private:
    TypeNode(const TypeNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new TypeNode(*this)); }
    void parse();
    QByteArray description() const;

    QByteArray toByteArrayQualPointerRef(const TypeNode *typeNode,
            const QByteArray &qualPtrRef) const;
    QByteArray qualPtrRefListToByteArray(const QList<const ParseTreeNode *> &nodeList) const;

    Type m_type;
};

class FloatValueNode : public ParseTreeNode
{
public:
    FloatValueNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    FloatValueNode(const FloatValueNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new FloatValueNode(*this)); }
    void parse();
    QByteArray description() const;

    double m_value;
};

class LambdaSigNode : public ParseTreeNode
{
public:
    LambdaSigNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    LambdaSigNode(const LambdaSigNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new LambdaSigNode(*this)); }
    void parse();
    QByteArray description() const { return "LambdaSig"; }
};

class ClosureTypeNameNode : public ParseTreeNode
{
public:
    ClosureTypeNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    QByteArray toByteArray() const;

private:
    ClosureTypeNameNode(const ClosureTypeNameNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new ClosureTypeNameNode(*this)); }
    void parse();
    QByteArray description() const { return "ClosureType"; }
};

class UnnamedTypeNameNode : public ParseTreeNode
{
public:
    UnnamedTypeNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    UnnamedTypeNameNode(const UnnamedTypeNameNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new UnnamedTypeNameNode(*this)); }
    void parse();
    QByteArray description() const { return "UnnnamedType"; }
};

class DeclTypeNode : public ParseTreeNode
{
public:
    DeclTypeNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    DeclTypeNode(const DeclTypeNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new DeclTypeNode(*this)); }
    void parse();
    QByteArray description() const { return "DeclType"; }
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
    QByteArray toByteArray() const;

private:
    SimpleIdNode(const SimpleIdNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new SimpleIdNode(*this)); }
    void parse();
    QByteArray description() const { return "SimpleId"; }
};

class DestructorNameNode : public ParseTreeNode
{
public:
    DestructorNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    DestructorNameNode(const DestructorNameNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new DestructorNameNode(*this)); }
    void parse();
    QByteArray description() const { return "DesctuctorName"; }
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
    QByteArray toByteArray() const;

private:
    BaseUnresolvedNameNode(const BaseUnresolvedNameNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new BaseUnresolvedNameNode(*this)); }
    void parse();
    QByteArray description() const;

    bool m_isOperator;
};

class InitializerNode : public ParseTreeNode
{
public:
    InitializerNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    InitializerNode(const InitializerNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new InitializerNode(*this)); }
    void parse();
    QByteArray description() const { return "Initializer"; }
};

class UnresolvedNameNode : public ParseTreeNode
{
public:
    UnresolvedNameNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    UnresolvedNameNode(const UnresolvedNameNode &other);
    ParseTreeNode::Ptr clone() const { return Ptr(new UnresolvedNameNode(*this)); }
    void parse();
    QByteArray description() const;

    bool m_globalNamespace;
};

class FunctionParamNode : public ParseTreeNode
{
public:
    FunctionParamNode(GlobalParseState *parseState) : ParseTreeNode(parseState) {}
    static bool mangledRepresentationStartsWith(char c);
    QByteArray toByteArray() const;

private:
    FunctionParamNode(const FunctionParamNode &other) : ParseTreeNode(other) {}
    ParseTreeNode::Ptr clone() const { return Ptr(new FunctionParamNode(*this)); }
    void parse();
    QByteArray description() const { return "FunctionParam"; }
};

} // namespace Internal
} // namespace Debugger

#endif // PARSETREENODES_H
