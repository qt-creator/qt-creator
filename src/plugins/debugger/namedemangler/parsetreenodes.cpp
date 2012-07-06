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
#include "parsetreenodes.h"

#include "demanglerexceptions.h"

#include <cstring>

#define MY_CHILD_AT(index) CHILD_AT(this, index)
#define CHILD_TO_BYTEARRAY(index) CHILD_AT(this, index)->toByteArray()

namespace Debugger {
namespace Internal {

ParseTreeNode::~ParseTreeNode()
{
    qDeleteAll(m_children);
}

ParseTreeNode *ParseTreeNode::childAt(int i, const QString &func, const QString &file,
        int line) const
{
    if (i < 0 || i >= m_children.count())
        throw InternalDemanglerException(func, file, line);
    return m_children.at(i);
}

QByteArray ParseTreeNode::pasteAllChildren() const
{
    QByteArray repr;
    foreach (const ParseTreeNode * const node, m_children)
        repr += node->toByteArray();
    return repr;
}


bool ArrayTypeNode::mangledRepresentationStartsWith(char c)
{
    return c == 'A';
}

QByteArray ArrayTypeNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(1) + '[' + CHILD_TO_BYTEARRAY(0) + ']';
}


bool BareFunctionTypeNode::mangledRepresentationStartsWith(char c)
{
    return TypeNode::mangledRepresentationStartsWith(c);
}

QByteArray BareFunctionTypeNode::toByteArray() const
{
    // This is only the parameter list, including parentheses. Where the return type is placed
    // must be decided at a higher level.
    QByteArray repr = "(";
    for (int i = m_hasReturnType ? 1 : 0; i < childCount(); ++i) {
        const QByteArray paramRepr = CHILD_TO_BYTEARRAY(i);
        if (paramRepr != "void")
            repr += paramRepr;
        if (i < childCount() - 1)
            repr += ", ";
    }
    return repr += ')';
}


bool BuiltinTypeNode::mangledRepresentationStartsWith(char c)
{
    return strchr("vwbcahstijlmxynofgedzDu", c);
}

QByteArray BuiltinTypeNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(0);
}


bool CallOffsetNode::mangledRepresentationStartsWith(char c)
{
    return c == 'h' || c == 'v';
}

QByteArray CallOffsetNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(0);
}


bool ClassEnumTypeNode::mangledRepresentationStartsWith(char c)
{
    /*
     * The first set of <class-enum-type> is much smaller than
     * the grammar claims.
     * firstSetClassEnumType = firstSetName;
     */
     return NonNegativeNumberNode::mangledRepresentationStartsWith(c)
             || c == 'N' || c == 'D' || c == 'Z';
}

QByteArray ClassEnumTypeNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(0);
}


bool DiscriminatorNode::mangledRepresentationStartsWith(char c)
{
    return c == '_';
}

QByteArray DiscriminatorNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(0);
}


bool CtorDtorNameNode::mangledRepresentationStartsWith(char c)
{
    return c == 'C' || c == 'D';
}

QByteArray CtorDtorNameNode::toByteArray() const
{
    QByteArray repr = m_representation;
    const int templateArgStart = repr.indexOf('<');
    if (templateArgStart != -1)
        repr.truncate(templateArgStart);
    const int prefixEnd = repr.lastIndexOf("::");
    if (prefixEnd != -1)
        repr.remove(0, prefixEnd + 2);
    if (m_isDestructor)
        repr.prepend('~');
    return repr;
}


bool CvQualifiersNode::mangledRepresentationStartsWith(char c)
{
    return c == 'K' || c == 'V' || c == 'r';
}

QByteArray CvQualifiersNode::toByteArray() const
{
    QByteArray repr;
    if (m_hasConst)
        repr = "const";
    if (m_hasVolatile) {
        if (m_hasConst)
            repr +=' ';
        repr += "volatile";
    }
    return repr;
}


bool EncodingNode::mangledRepresentationStartsWith(char c)
{
    return NameNode::mangledRepresentationStartsWith(c)
            || SpecialNameNode::mangledRepresentationStartsWith(c);
}

QByteArray EncodingNode::toByteArray() const
{
    if (childCount() == 1)
        return CHILD_TO_BYTEARRAY(0);

    const ParseTreeNode * const nameNode = MY_CHILD_AT(0);
    const NestedNameNode * const nestedNameNode
            = dynamic_cast<NestedNameNode *>(CHILD_AT(nameNode, 0));
    const CvQualifiersNode * const cvQualifiersNode = nestedNameNode
            ? dynamic_cast<CvQualifiersNode *>(CHILD_AT(nestedNameNode, 0)) : 0;

    QByteArray repr;
    const BareFunctionTypeNode * const funcNode
            = DEMANGLER_CAST(BareFunctionTypeNode, MY_CHILD_AT(1));
    if (funcNode->m_hasReturnType)
        repr = CHILD_AT(funcNode, 0)->toByteArray() + ' ';
    if (cvQualifiersNode) {
        return repr + CHILD_AT(nestedNameNode, 1)->toByteArray() + funcNode->toByteArray() + ' '
            + cvQualifiersNode->toByteArray();
    }
    return repr + nameNode->toByteArray() + funcNode->toByteArray();
}


bool ExpressionNode::mangledRepresentationStartsWith(char c)
{
    return OperatorNameNode::mangledRepresentationStartsWith(c)
            || TemplateParamNode::mangledRepresentationStartsWith(c)
/*            || FunctionParamNode::mangledRepresentationStartsWith(c) */
            || ExprPrimaryNode::mangledRepresentationStartsWith(c)
            || c == 'c' || c == 's' || c == 'a';
}

QByteArray ExpressionNode::toByteArray() const
{
    QByteArray repr;

    switch (m_type) {
    case ConversionType:
        repr = CHILD_TO_BYTEARRAY(0) + '(';
        for (int i = 1; i < childCount(); ++i)
            repr += CHILD_TO_BYTEARRAY(i);
        repr += ')';
        break;
    case SizeofType:
        repr = "sizeof(" + CHILD_TO_BYTEARRAY(0) + ')';
        break;
    case AlignofType:
        repr = "alignof(" + CHILD_TO_BYTEARRAY(0) + ')';
        break;
    case ParameterPackSizeType:
        repr = CHILD_TO_BYTEARRAY(0); // TODO: What does this look like?
    case OperatorType: {
        const OperatorNameNode * const opNode = DEMANGLER_CAST(OperatorNameNode, MY_CHILD_AT(0));
        switch (opNode->m_type) {
        case OperatorNameNode::CallType:
            repr = CHILD_TO_BYTEARRAY(1) + opNode->toByteArray();
            break;
        case OperatorNameNode::SizeofExprType: case OperatorNameNode::AlignofExprType:
            repr = opNode->toByteArray() + '(' + CHILD_TO_BYTEARRAY(1) + ')';
            break;
        case OperatorNameNode::ArrayNewType:
            repr = "new " + CHILD_TO_BYTEARRAY(1) + '[' + CHILD_TO_BYTEARRAY(2) + ']';
            break;
        case OperatorNameNode::IndexType:
            repr = CHILD_TO_BYTEARRAY(1) + '[' + CHILD_TO_BYTEARRAY(2) + ']';
            break;
        case OperatorNameNode::TernaryType:
            repr = CHILD_TO_BYTEARRAY(1) + " ? " + CHILD_TO_BYTEARRAY(2) + " : " + CHILD_TO_BYTEARRAY(3);
            break;
        case OperatorNameNode::ArrowStarType: case OperatorNameNode::ArrowType:
            repr = CHILD_TO_BYTEARRAY(1) + opNode->toByteArray() + CHILD_TO_BYTEARRAY(2);
            break;
        case OperatorNameNode::BinaryPlusType:
        case OperatorNameNode::BinaryMinusType:
        case OperatorNameNode::MultType:
        case OperatorNameNode::DivType:
        case OperatorNameNode::ModuloType:
        case OperatorNameNode::BitwiseAndType:
        case OperatorNameNode::BitwiseOrType:
        case OperatorNameNode::XorType:
        case OperatorNameNode::AssignType:
        case OperatorNameNode::IncrementAndAssignType:
        case OperatorNameNode::DecrementAndAssignType:
        case OperatorNameNode::MultAndAssignType:
        case OperatorNameNode::DivAndAssignType:
        case OperatorNameNode::ModuloAndAssignType:
        case OperatorNameNode::BitwiseAndAndAssignType:
        case OperatorNameNode::BitwiseOrAndAssignType:
        case OperatorNameNode::XorAndAssignType:
        case OperatorNameNode::LeftShiftType:
        case OperatorNameNode::RightShiftType:
        case OperatorNameNode::LeftShiftAndAssignType:
        case OperatorNameNode::RightShiftAndAssignType:
        case OperatorNameNode::EqualsType:
        case OperatorNameNode::NotEqualsType:
        case OperatorNameNode::LessType:
        case OperatorNameNode::GreaterType:
        case OperatorNameNode::LessEqualType:
        case OperatorNameNode::GreaterEqualType:
        case OperatorNameNode::LogicalAndType:
        case OperatorNameNode::LogicalOrType:
        case OperatorNameNode::CommaType:
            repr = CHILD_TO_BYTEARRAY(1) + ' ' + opNode->toByteArray() + ' ' + CHILD_TO_BYTEARRAY(2);
            break;
        case OperatorNameNode::NewType:
        case OperatorNameNode::DeleteType:
        case OperatorNameNode::ArrayDeleteType:
            repr = opNode->toByteArray() + ' ' + CHILD_TO_BYTEARRAY(1);
            break;
        default: // Other unary Operators;
            repr = opNode->toByteArray() + CHILD_TO_BYTEARRAY(1);
        }
        break;
    }
    case OtherType:
        repr = pasteAllChildren();
    }

    return repr;
}


bool OperatorNameNode::mangledRepresentationStartsWith(char c)
{
    return strchr("ndpacmroelgiqsv", c);
}

QByteArray OperatorNameNode::toByteArray() const
{
    switch (m_type) {
    case NewType: return "new";
    case ArrayNewType: return "new[]";
    case DeleteType: return "delete";
    case ArrayDeleteType: return "delete[]";
    case UnaryPlusType: case BinaryPlusType: return "+";
    case UnaryMinusType: case BinaryMinusType: return "-";
    case UnaryAmpersandType: case BitwiseAndType: return "&";
    case UnaryStarType: case MultType: return "*";
    case BitwiseNotType: return "~";
    case DivType: return "/";
    case ModuloType: return "%";
    case BitwiseOrType: return "|";
    case XorType: return "^";
    case AssignType: return "=";
    case IncrementAndAssignType: return "+=";
    case DecrementAndAssignType: return "-=";
    case MultAndAssignType: return "*=";
    case DivAndAssignType: return "/=";
    case ModuloAndAssignType: return "%=";
    case BitwiseAndAndAssignType: return "&=";
    case BitwiseOrAndAssignType: return "|=";
    case XorAndAssignType: return "^=";
    case LeftShiftType: return "<<";
    case RightShiftType: return ">>";
    case LeftShiftAndAssignType: return "<<=";
    case RightShiftAndAssignType: return ">>=";
    case EqualsType: return "==";
    case NotEqualsType: return "!=";
    case LessType: return "<";
    case GreaterType: return ">";
    case LessEqualType: return "<=";
    case GreaterEqualType: return ">=";
    case LogicalNotType: return "!";
    case LogicalAndType: return "&&";
    case LogicalOrType: return "||";
    case IncrementType: return "++";
    case DecrementType: return "--";
    case CommaType: return ",";
    case ArrowStarType: return "->*";
    case ArrowType: return "->";
    case CallType: return "()";
    case IndexType: return "[]";
    case TernaryType: return "?";
    case SizeofTypeType: case SizeofExprType: return "sizeof";
    case AlignofTypeType: case AlignofExprType: return "alignof";
    case CastType: return ' ' + CHILD_TO_BYTEARRAY(0);
    case VendorType: return "[vendor extended operator]";
    }

    DEMANGLER_ASSERT(false);
    return QByteArray();
}


QByteArray PredefinedBuiltinTypeNode::toByteArray() const
{
    switch (m_type) {
    case VoidType: return "void";
    case WCharType: return "wchar_t";
    case BoolType: return "bool";
    case PlainCharType: return "char";
    case SignedCharType: return "signed char";
    case UnsignedCharType: return "unsigned char";
    case SignedShortType: return "signed short";
    case UnsignedShortType: return "unsigned short";
    case SignedIntType: return "int";
    case UnsignedIntType: return "unsigned int";
    case SignedLongType: return "long";
    case UnsignedLongType: return "unsigned long";
    case SignedLongLongType: return "long long";
    case UnsignedLongLongType: return "unsigned long long";
    case SignedInt128Type: return "__int128";
    case UnsignedInt128Type: return "unsigned __int128";
    case FloatType: return "float";
    case DoubleType: return "double";
    case LongDoubleType: return "long double";
    case Float128Type: return "__float128";
    case EllipsisType: return "...";
    case DecimalFloatingType16: return "[IEEE 754r half-precision floating point]";
    case DecimalFloatingType32: return "[IEEE 754r decimal floating point (32 bits)]";
    case DecimalFloatingType64: return "[IEEE 754r decimal floating point (64 bits)]";
    case DecimalFloatingType128: return "[IEEE 754r decimal floating point (128 bits)]";
    case Char32Type: return "char32_t";
    case Char16Type: return "char16_t";
    }

    DEMANGLER_ASSERT(false);
    return QByteArray();
}


bool ExprPrimaryNode::mangledRepresentationStartsWith(char c)
{
    return c == 'L';
}

QByteArray ExprPrimaryNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(0);
}

bool FunctionTypeNode::mangledRepresentationStartsWith(char c)
{
    return c == 'F';
}

QByteArray FunctionTypeNode::toByteArray() const
{
    return QByteArray(); // Not enough knowledge here to generate a string representation.
}


bool LocalNameNode::mangledRepresentationStartsWith(char c)
{
    return c == 'Z';
}

QByteArray LocalNameNode::toByteArray() const
{
    QByteArray name;
    bool hasDiscriminator;
    if (m_isStringLiteral) {
        name = CHILD_TO_BYTEARRAY(0) + "::[string literal]";
        hasDiscriminator = childCount() == 2;
    } else {
        name = CHILD_TO_BYTEARRAY(0) + "::" + CHILD_TO_BYTEARRAY(1);
        hasDiscriminator = childCount() == 3;
    }
    if (hasDiscriminator) {
        const QByteArray discriminator = MY_CHILD_AT(childCount() - 1)->toByteArray();
        const int rawDiscriminatorValue = discriminator.toInt();
        name += " (occurrence number " + QByteArray::number(rawDiscriminatorValue - 2) + ')';
    }
    return name;
}


bool MangledNameNode::mangledRepresentationStartsWith(char c)
{
    return c == '_';
}

QByteArray MangledNameNode::toByteArray() const
{
    return pasteAllChildren();
}


bool SourceNameNode::mangledRepresentationStartsWith(char c)
{
    return strchr("123456789", c);
}


bool UnqualifiedNameNode::mangledRepresentationStartsWith(char c)
{
    return OperatorNameNode::mangledRepresentationStartsWith(c)
            || CtorDtorNameNode::mangledRepresentationStartsWith(c)
            || SourceNameNode::mangledRepresentationStartsWith(c);
}

QByteArray UnqualifiedNameNode::toByteArray() const
{
    QByteArray repr;
    if (dynamic_cast<OperatorNameNode *>(MY_CHILD_AT(0)))
        repr = "operator";
    return repr += CHILD_TO_BYTEARRAY(0);
}

bool UnqualifiedNameNode::isConstructorOrDestructorOrConversionOperator() const
{
    if (dynamic_cast<CtorDtorNameNode *>(MY_CHILD_AT(0)))
        return true;
    const OperatorNameNode * const opNode = dynamic_cast<OperatorNameNode *>(MY_CHILD_AT(0));
    return opNode && opNode->m_type == OperatorNameNode::CastType;
}


bool UnscopedNameNode::mangledRepresentationStartsWith(char c)
{
    return UnqualifiedNameNode::mangledRepresentationStartsWith(c) || c == 'S';
}

QByteArray UnscopedNameNode::toByteArray() const
{
    QByteArray name = CHILD_TO_BYTEARRAY(0);
    if (m_inStdNamespace)
        name.prepend("std::");
    return name;
}

bool UnscopedNameNode::isConstructorOrDestructorOrConversionOperator() const
{
    const UnqualifiedNameNode * const childNode
            = DEMANGLER_CAST(UnqualifiedNameNode, MY_CHILD_AT(0));
    return childNode->isConstructorOrDestructorOrConversionOperator();
}


bool NestedNameNode::mangledRepresentationStartsWith(char c)
{
    return c == 'N';
}

QByteArray NestedNameNode::toByteArray() const
{
    // This the valid representation only if no cv-qualifiers are present.
    // In that case (only possible for member functions), a higher-level object must
    // create the string representation.
    return CHILD_TO_BYTEARRAY(0);
}

bool NestedNameNode::isTemplate() const
{
    const PrefixNode * const childNode = DEMANGLER_CAST(PrefixNode, MY_CHILD_AT(childCount() - 1));
    return childNode->isTemplate();
}

bool NestedNameNode::isConstructorOrDestructorOrConversionOperator() const
{
    const PrefixNode * const childNode = DEMANGLER_CAST(PrefixNode, MY_CHILD_AT(childCount() - 1));
    return childNode->isConstructorOrDestructorOrConversionOperator();
}


bool SubstitutionNode::mangledRepresentationStartsWith(char c)
{
    return c == 'S';
}

QByteArray SubstitutionNode::toByteArray() const
{
    switch (m_type) {
    case ActualSubstitutionType: return m_substValue;
    case StdType: return "std";
    case StdAllocType: return "std::allocator";
    case StdBasicStringType: return "std::basic_string";
    case FullStdBasicStringType: return "std::basic_string<char, std::char_traits<char>, "
            "std::allocator<char> >";
    case StdBasicIStreamType: return "std::basic_istream<char, std::char_traits<char> >";
    case StdBasicOStreamType: return "std::basic_ostream<char, std::char_traits<char> >";
    case StdBasicIoStreamType: return "std::basic_iostream<char, std::char_traits<char> >";
    }

    DEMANGLER_ASSERT(false);
    return QByteArray();
}


bool PointerToMemberTypeNode::mangledRepresentationStartsWith(char c)
{
    return c == 'M';
}

QByteArray PointerToMemberTypeNode::toByteArray() const
{
    // Gather all qualifiers first, because we have to move them to the end en bloc
    // .
    QByteArray qualRepr;
    const TypeNode *memberTypeNode = DEMANGLER_CAST(TypeNode, MY_CHILD_AT(1));
    while (memberTypeNode->m_type == TypeNode::QualifiedType) {
        const CvQualifiersNode * const cvNode
                = DEMANGLER_CAST(CvQualifiersNode, CHILD_AT(memberTypeNode, 0));
        if (cvNode->m_hasConst || cvNode->m_hasVolatile) {
            if (!qualRepr.isEmpty())
                qualRepr += ' ';
            qualRepr += cvNode->toByteArray();
        }
        memberTypeNode = DEMANGLER_CAST(TypeNode, CHILD_AT(memberTypeNode, 1));
    }

    QByteArray repr;
    const QByteArray classTypeRepr = CHILD_TO_BYTEARRAY(0);
    const FunctionTypeNode * const functionNode
            = dynamic_cast<const FunctionTypeNode *>(CHILD_AT(memberTypeNode, 0));
    if (functionNode) {
        const BareFunctionTypeNode * const bareFunctionNode
                = DEMANGLER_CAST(BareFunctionTypeNode, CHILD_AT(functionNode, 0));
        if (functionNode->m_isExternC)
            repr += "extern \"C\" ";
        if (bareFunctionNode->m_hasReturnType)
            repr += CHILD_AT(bareFunctionNode, 0)->toByteArray() + ' ';
        repr += '(' + classTypeRepr + "::*)" + bareFunctionNode->toByteArray();
        if (!qualRepr.isEmpty())
            repr += ' ' + qualRepr;
    } else {
        repr = memberTypeNode->toByteArray() + ' ' + classTypeRepr + "::";
        if (!qualRepr.isEmpty())
            repr += qualRepr + ' ';
        repr += '*';
    }
    return repr;
}


TemplateParamNode::~TemplateParamNode()
{
    clearChildList(); // Child node is deleted elsewhere.
}

bool TemplateParamNode::mangledRepresentationStartsWith(char c)
{
    return c == 'T';
}

QByteArray TemplateParamNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(0);
}


bool TemplateArgsNode::mangledRepresentationStartsWith(char c)
{
    return c == 'I';
}

QByteArray TemplateArgsNode::toByteArray() const
{
    QByteArray repr = "<";
    for (int i = 0; i < childCount(); ++i) {
        repr += CHILD_TO_BYTEARRAY(i);
        if (i < childCount() - 1)
            repr += ", ";
    }
    return repr += '>';
}


bool SpecialNameNode::mangledRepresentationStartsWith(char c)
{
    return c == 'T' || c == 'G';
}

QByteArray SpecialNameNode::toByteArray() const
{
    switch (m_type) {
    case VirtualTableType:
        return "[virtual table of " + CHILD_TO_BYTEARRAY(0) + ']';
    case VttStructType:
        return "[VTT struct of " + CHILD_TO_BYTEARRAY(0) + ']';
    case TypeInfoType:
        return "typeid(" + CHILD_TO_BYTEARRAY(0) + ')';
    case TypeInfoNameType:
        return "typeid(" + CHILD_TO_BYTEARRAY(0) + ").name()";
    case GuardVarType:
        return "[guard variable of " + CHILD_TO_BYTEARRAY(0) + ']';
    case SingleCallOffsetType:
        return "[offset:" + CHILD_TO_BYTEARRAY(0) + ']' + CHILD_TO_BYTEARRAY(1);
    case DoubleCallOffsetType:
        return "[this-adjustment:" + CHILD_TO_BYTEARRAY(0) + "][result-adjustment:"
                + CHILD_TO_BYTEARRAY(1) + ']' + CHILD_TO_BYTEARRAY(2);
    }

    DEMANGLER_ASSERT(false);
    return QByteArray();
}


bool NumberNode::mangledRepresentationStartsWith(char c, int base)
{
    return NonNegativeNumberNode::mangledRepresentationStartsWith(c, base) || c == 'n';
}

QByteArray NumberNode::toByteArray() const
{
    QByteArray repr = CHILD_TO_BYTEARRAY(0);
    if (m_isNegative)
        repr.prepend('-');
    return repr;
}


bool NonNegativeNumberNode::mangledRepresentationStartsWith(char c, int base)
{
    // Base can only be 10 or 36.
    if (base == 10)
        return strchr("0123456789", c);
    else
        return strchr("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", c);
}

QByteArray NonNegativeNumberNode::toByteArray() const
{
    return QByteArray::number(m_number);
}


bool NameNode::mangledRepresentationStartsWith(char c)
{
    return NestedNameNode::mangledRepresentationStartsWith(c)
            || UnscopedNameNode::mangledRepresentationStartsWith(c)
            || SubstitutionNode::mangledRepresentationStartsWith(c)
            || LocalNameNode::mangledRepresentationStartsWith(c);
}

QByteArray NameNode::toByteArray() const
{
    return pasteAllChildren();
}

bool NameNode::isTemplate() const
{
    if (childCount() > 1 && dynamic_cast<TemplateArgsNode *>(MY_CHILD_AT(1)))
        return true;
    const NestedNameNode * const nestedNameNode = dynamic_cast<NestedNameNode *>(MY_CHILD_AT(0));
    if (nestedNameNode)
        return nestedNameNode->isTemplate();

    // TODO: Is <local-name> relevant?
    return false;
}

bool NameNode::isConstructorOrDestructorOrConversionOperator() const
{
    const NestedNameNode * const nestedNameNode = dynamic_cast<NestedNameNode *>(MY_CHILD_AT(0));
    if (nestedNameNode)
        return nestedNameNode->isConstructorOrDestructorOrConversionOperator();

    // TODO: Is <local-name> relevant?
    return false;
}


bool TemplateArgNode::mangledRepresentationStartsWith(char c)
{
    return TypeNode::mangledRepresentationStartsWith(c)
            || ExprPrimaryNode::mangledRepresentationStartsWith(c)
            || c == 'X' || c == 'I' || c == 's';
}

QByteArray TemplateArgNode::toByteArray() const
{
    if (m_isTemplateArgumentPack) {
        QByteArray repr;
        for (int i = 0; i < childCount(); ++i) {
            if (i > 0 && i < childCount() - 1)
                repr += ", "; // TODO: Probably not the right syntax
            repr += CHILD_TO_BYTEARRAY(i);
        }
        return repr;
    }
    return CHILD_TO_BYTEARRAY(0);
}


bool Prefix2Node::mangledRepresentationStartsWith(char c)
{
    return UnqualifiedNameNode::mangledRepresentationStartsWith(c);
}

QByteArray Prefix2Node::toByteArray() const
{
    if (childCount() == 0)
        return QByteArray();
    QByteArray repr = CHILD_TO_BYTEARRAY(0);
    for (int i = 1; i < childCount(); ++i) {
        if (dynamic_cast<UnqualifiedNameNode *>(MY_CHILD_AT(i)))
            repr += "::"; // Don't show the "global namespace" indicator.
        repr += CHILD_TO_BYTEARRAY(i);
    }
    return repr;
}

bool Prefix2Node::isTemplate() const
{
    return dynamic_cast<TemplateArgsNode *>(MY_CHILD_AT(childCount() - 1));
}

bool Prefix2Node::isConstructorOrDestructorOrConversionOperator() const
{
    for (int i = childCount() - 1; i >= 0; --i) {
        const UnqualifiedNameNode * const n = dynamic_cast<UnqualifiedNameNode *>(MY_CHILD_AT(i));
        if (n)
            return n->isConstructorOrDestructorOrConversionOperator();
    }
    return false;
}


bool PrefixNode::mangledRepresentationStartsWith(char c)
{
    return TemplateParamNode::mangledRepresentationStartsWith(c)
            || SubstitutionNode::mangledRepresentationStartsWith(c)
            || Prefix2Node::mangledRepresentationStartsWith(c);
}

QByteArray PrefixNode::toByteArray() const
{
    if (childCount() == 1)
        return CHILD_TO_BYTEARRAY(0);
    if (MY_CHILD_AT(childCount() - 1)->childCount() == 0) // Empty prefix2, i.e. no symbol follows.
        return pasteAllChildren();
    if (childCount() == 2)
        return CHILD_TO_BYTEARRAY(0) + "::" + CHILD_TO_BYTEARRAY(1);
    return CHILD_TO_BYTEARRAY(0) + CHILD_TO_BYTEARRAY(1) + "::" + CHILD_TO_BYTEARRAY(2);
}

bool PrefixNode::isTemplate() const
{
    if (childCount() > 1 && dynamic_cast<TemplateArgsNode *>(CHILD_AT(this, 1)))
        return true;
    const Prefix2Node * const childNode
            = DEMANGLER_CAST(Prefix2Node, MY_CHILD_AT(childCount() - 1));
    return childNode->isTemplate();
}

bool PrefixNode::isConstructorOrDestructorOrConversionOperator() const
{
    const Prefix2Node * const childNode
            = DEMANGLER_CAST(Prefix2Node, MY_CHILD_AT(childCount() - 1));
    return childNode->isConstructorOrDestructorOrConversionOperator();
}


bool TypeNode::mangledRepresentationStartsWith(char c)
{
    return BuiltinTypeNode::mangledRepresentationStartsWith(c)
            || FunctionTypeNode::mangledRepresentationStartsWith(c)
            || ClassEnumTypeNode::mangledRepresentationStartsWith(c)
            || ArrayTypeNode::mangledRepresentationStartsWith(c)
            || PointerToMemberTypeNode::mangledRepresentationStartsWith(c)
            || TemplateParamNode::mangledRepresentationStartsWith(c)
            || SubstitutionNode::mangledRepresentationStartsWith(c)
            || CvQualifiersNode::mangledRepresentationStartsWith(c)
            || strchr("PROCGUD", c);

}

QByteArray TypeNode::toByteArray() const
{
    // A pure top-down approach is not possible to due to the weird function pointer syntax,
    // e.g. things like (* const &)(int) etc.
    // Instead, we have to gather all successive qualifiers, pointers and references first
    // and then apply them as a whole to whatever follows.
    // Note that "qualifier to function" is not possible here, since that is handled by
    // PointerToMemberType.
    QList<const ParseTreeNode *> qualPtrRefList;
    const TypeNode *currentNode = this;
    bool leafType = false;
    while (!leafType) {
        switch (currentNode->m_type) {
        case QualifiedType: {
            const CvQualifiersNode * const cvNode
                    = DEMANGLER_CAST(CvQualifiersNode, CHILD_AT(currentNode, 0));
            if (cvNode->m_hasConst || cvNode->m_hasVolatile)
                qualPtrRefList << cvNode;
            currentNode = DEMANGLER_CAST(TypeNode, CHILD_AT(currentNode, 1));
            break;
        }
        case PointerType: case ReferenceType: case RValueType:
            qualPtrRefList << currentNode;
            currentNode = DEMANGLER_CAST(TypeNode, CHILD_AT(currentNode, 0));
            break;
        default:
            leafType = true;
            break;
        }
    }

    if (qualPtrRefList.isEmpty()) {
        switch (currentNode->m_type) {
        case PackExpansionType: return CHILD_TO_BYTEARRAY(0); // TODO: What's the syntax?
        case DeclType: return "decltype(" + CHILD_TO_BYTEARRAY(0) + ')';
        case VendorType: return pasteAllChildren();
        case OtherType: return pasteAllChildren();
        default: DEMANGLER_ASSERT(false);
        }
    }

    return toByteArrayQualPointerRef(currentNode, qualPtrRefListToByteArray(qualPtrRefList));
}

QByteArray TypeNode::toByteArrayQualPointerRef(const TypeNode *typeNode,
        const QByteArray &qualPtrRef) const
{
    const FunctionTypeNode * const functionNode
            = dynamic_cast<FunctionTypeNode *>(CHILD_AT(typeNode, 0));
    if (functionNode) {
        const BareFunctionTypeNode * const bareFunctionNode
                = DEMANGLER_CAST(BareFunctionTypeNode, CHILD_AT(functionNode, 0));
        QByteArray repr;
        if (functionNode->m_isExternC)
            repr += "extern \"C\" ";
        if (bareFunctionNode->m_hasReturnType)
            repr += CHILD_AT(bareFunctionNode, 0)->toByteArray() + ' ';
        return repr += '(' + qualPtrRef + ')' + bareFunctionNode->toByteArray();
    }

    if (dynamic_cast<PointerToMemberTypeNode *>(CHILD_AT(typeNode, 0)))
        return typeNode->toByteArray() + qualPtrRef;

    return typeNode->toByteArray() + ' ' + qualPtrRef;
}

QByteArray TypeNode::qualPtrRefListToByteArray(const QList<const ParseTreeNode *> &nodeList) const
{
    QByteArray repr;
    foreach (const ParseTreeNode * const n, nodeList) {
        const TypeNode * const typeNode = dynamic_cast<const TypeNode *>(n);
        if (typeNode) {
            switch (typeNode->m_type) {
            case PointerType:
                if (!repr.isEmpty() && !repr.startsWith('*'))
                    repr.prepend(' ');
                repr.prepend('*');
                break;
            case ReferenceType:
                if (!repr.isEmpty())
                    repr.prepend(' ');
                repr.prepend('&');
                break;
            case RValueType:
                if (!repr.isEmpty())
                    repr.prepend(' ');
                repr.prepend("&&");
            default:
                DEMANGLER_ASSERT(false);
            }
        } else {
            if (!repr.isEmpty())
                repr.prepend(' ');
            repr.prepend(n->toByteArray());
        }
    }
    return repr;
}


bool FloatValueNode::mangledRepresentationStartsWith(char c)
{
    return strchr("0123456789abcdef", c);
}

QByteArray FloatValueNode::toByteArray() const
{
    return QByteArray::number(m_value);
}

} // namespace Internal
} // namespace Debugger
