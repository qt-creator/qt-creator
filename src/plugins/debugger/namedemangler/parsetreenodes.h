/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#ifndef PARSETREENODES_H
#define PARSETREENODES_H

#include <QByteArray>
#include <QList>
#include <QSet>

#define CHILD_AT(obj, index) obj->childAt(index, Q_FUNC_INFO, __FILE__, __LINE__)

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

protected:
    void clearChildList() { m_children.clear(); }

private:
    QList<ParseTreeNode *> m_children; // Convention: Children are inserted in parse order.
};

class ArrayTypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;
};

class BareFunctionTypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool m_hasReturnType;
};

class BuiltinTypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;
};

class PredefinedBuiltinTypeNode : public ParseTreeNode
{
public:
    QByteArray toByteArray() const;

    enum Type {
        VoidType, WCharType, BoolType,
        PlainCharType, SignedCharType, UnsignedCharType, SignedShortType, UnsignedShortType,
        SignedIntType, UnsignedIntType, SignedLongType, UnsignedLongType,
        SignedLongLongType, UnsignedLongLongType, SignedInt128Type, UnsignedInt128Type,
        FloatType, DoubleType, LongDoubleType, Float128Type, EllipsisType,
        DecimalFloatingType64, DecimalFloatingType128, DecimalFloatingType32,
        DecimalFloatingType16, Char32Type, Char16Type
    } m_type;
};

class CallOffsetNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;
};

class NvOffsetNode : public ParseTreeNode
{
public:
    QByteArray toByteArray() const { return QByteArray(); } // TODO: How to encode this?
};

class VOffsetNode : public ParseTreeNode
{
public:
    QByteArray toByteArray() const { return QByteArray(); } // TODO: How to encode this?
};

class ClassEnumTypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;
};

class DiscriminatorNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;
};

class CtorDtorNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool m_isDestructor;
    QByteArray m_representation;
};

class CvQualifiersNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool m_hasVolatile;
    bool m_hasConst;
};

class EncodingNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;
};

class ExpressionNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    enum Type {
        ConversionType, SizeofType, AlignofType, OperatorType, OtherType, ParameterPackSizeType
    } m_type;
};

class OperatorNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

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
    } m_type;

};

class ExprPrimaryNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;
};

class FunctionTypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool m_isExternC;
};

class LocalNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool m_isStringLiteral;
};

class MangledNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;
};

class NumberNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c, int base = 10);

    QByteArray toByteArray() const;

    bool m_isNegative;
};

class SourceNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const { return m_name; }

    QByteArray m_name;
};

class UnqualifiedNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool isConstructorOrDestructorOrConversionOperator() const;
};

class UnscopedNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool isConstructorOrDestructorOrConversionOperator() const;

    bool m_inStdNamespace;
};

class NestedNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
};

class SubstitutionNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

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
};

class TemplateParamNode : public ParseTreeNode
{
public:
    ~TemplateParamNode();

    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;
};

class TemplateArgsNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;
};

class SpecialNameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    enum Type {
        VirtualTableType, VttStructType, TypeInfoType, TypeInfoNameType, GuardVarType,
        SingleCallOffsetType, DoubleCallOffsetType
    } m_type;
};

class NonNegativeNumberNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c, int base = 10);

    QByteArray toByteArray() const;

    quint64 m_number;
};

class NameNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
};

class TemplateArgNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool m_isTemplateArgumentPack;
};

class Prefix2Node : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
};

class PrefixNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    bool isTemplate() const;
    bool isConstructorOrDestructorOrConversionOperator() const;
};

class TypeNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    QByteArray toByteArrayQualPointerRef(const TypeNode *typeNode,
            const QByteArray &qualPtrRef) const;
    QByteArray qualPtrRefListToByteArray(const QList<const ParseTreeNode *> &nodeList) const;

    enum Type {
        QualifiedType, PointerType, ReferenceType, RValueType, VendorType, PackExpansionType,
        DeclType, OtherType
    } m_type;
};

class FloatValueNode : public ParseTreeNode
{
public:
    static bool mangledRepresentationStartsWith(char c);

    QByteArray toByteArray() const;

    double m_value;
};

} // namespace Internal
} // namespace Debugger

#endif // PARSETREENODES_H
