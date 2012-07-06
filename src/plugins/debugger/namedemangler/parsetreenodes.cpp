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

#include <cctype>
#include <cstring>

#define PEEK() (parseState()->peek())
#define ADVANCE() (parseState()->advance())

#define PARSE_RULE_AND_ADD_RESULT_AS_CHILD(nodeType) \
    do { \
        parseRule<nodeType>(parseState()); \
        DEMANGLER_ASSERT(parseState()->stackElementCount() > 0); \
        DEMANGLER_ASSERT(dynamic_cast<nodeType *>(parseState()->stackTop())); \
        addChild(parseState()->popFromStack()); \
    } while (0)

#define CHILD_AT(obj, index) obj->childAt(index, Q_FUNC_INFO, __FILE__, __LINE__)
#define MY_CHILD_AT(index) CHILD_AT(this, index)
#define CHILD_TO_BYTEARRAY(index) MY_CHILD_AT(index)->toByteArray()

namespace Debugger {
namespace Internal {

template<int base> static int getNonNegativeNumber(GlobalParseState *parseState)
{
    ParseTreeNode::parseRule<NonNegativeNumberNode<base> >(parseState);
    NonNegativeNumberNode<base> * const numberNode
            = DEMANGLER_CAST(NonNegativeNumberNode<base>, parseState->popFromStack());
    const int value = static_cast<int>(numberNode->number());
    delete numberNode;
    return value;
}


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

/*
 * <array-type> ::= A <number> _ <type>
 *              ::= A [<expression>] _ <type>
 */
void ArrayTypeNode::parse()
{
    if (!ArrayTypeNode::mangledRepresentationStartsWith(ADVANCE()))
        throw ParseException(QString::fromLatin1("Invalid array-type"));

    const char next = PEEK();
    if (NonNegativeNumberNode<10>::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumberNode<10>);
    else if (ExpressionNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);

    if (ADVANCE() != '_')
        throw ParseException(QString::fromLatin1("Invalid array-type"));

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
}

QByteArray ArrayTypeNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(1) + '[' + CHILD_TO_BYTEARRAY(0) + ']';
}


bool BareFunctionTypeNode::mangledRepresentationStartsWith(char c)
{
    return TypeNode::mangledRepresentationStartsWith(c);
}

/* <bare-function-type> ::= <type>+ */
void BareFunctionTypeNode::parse()
{
   /*
    * The following is verbatim from the spec:
    * Whether the mangling of a function type includes the return type depends on the context
    * and the nature of the function. The rules for deciding whether the return type is included
    * are:
    *     (1) Template functions (names or types) have return types encoded, with the exceptions
    *         listed below.
    *     (2) Function types not appearing as part of a function name mangling, e.g. parameters,
    *         pointer types, etc., have return type encoded, with the exceptions listed below.
    *     (3) Non-template function names do not have return types encoded.
    * The exceptions mentioned in (1) and (2) above, for which the return type is never included,
    * are constructors, destructors and conversion operator functions, e.g. operator int.
    */
    const EncodingNode * const encodingNode = dynamic_cast<EncodingNode *>(parseState()
            ->stackElementAt(parseState()->stackElementCount() - 2));
    if (encodingNode) { // Case 1: Function name.
        const NameNode * const nameNode = DEMANGLER_CAST(NameNode, CHILD_AT(encodingNode, 0));
        m_hasReturnType = nameNode->isTemplate()
                && !nameNode->isConstructorOrDestructorOrConversionOperator();
    } else {            // Case 2: function type.
        // TODO: What do the exceptions look like for this case?
        m_hasReturnType = true;
    }

    do
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    while (TypeNode::mangledRepresentationStartsWith(PEEK()));
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

/*
 * <builtin-type> ::= v # void
 *                ::= w  # wchar_t
 *                ::= b  # bool
 *                ::= c  # char
 *                ::= a  # signed char
 *                ::= h  # unsigned char
 *                ::= s  # short
 *                ::= t  # unsigned short
 *                ::= i  # int
 *                ::= j  # unsigned int
 *                ::= l  # long
 *                ::= m  # unsigned long
 *                ::= x  # long long, __int64
 *                ::= y  # unsigned long long, __int64
 *                ::= n  # __int128
 *                ::= o  # unsigned __int128
 *                ::= f  # float
 *                ::= d  # double
 *                ::= e  # long double, __float80
 *                ::= g  # __float128
 *                ::= z  # ellipsis
 *                ::= Dd # IEEE 754r decimal floating point (64 bits)
 *                ::= De # IEEE 754r decimal floating point (128 bits)
 *                ::= Df # IEEE 754r decimal floating point (32 bits)
 *                ::= Dh # IEEE 754r half-precision floating point (16 bits)
 *                ::= Di # char32_t
 *                ::= Ds # char16_t
 *                ::= u <source-name>    # vendor extended type
 */
void BuiltinTypeNode::parse()
{
    const char next = ADVANCE();
    if (next == 'u') {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SourceNameNode);
    } else {
        // TODO: This seems silly. Why not integrate the type into this node and have
        // an additional one "VendorType" that indicates that there's a child node?
        PredefinedBuiltinTypeNode * const fixedTypeNode = new PredefinedBuiltinTypeNode;
        addChild(fixedTypeNode);

        switch (next) {
        case 'v': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::VoidType; break;
        case 'w': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::WCharType; break;
        case 'b': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::BoolType; break;
        case 'c': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::PlainCharType; break;
        case 'a': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::SignedCharType; break;
        case 'h': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::UnsignedCharType; break;
        case 's': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::SignedShortType; break;
        case 't': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::UnsignedShortType; break;
        case 'i': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::SignedIntType; break;
        case 'j': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::UnsignedIntType; break;
        case 'l': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::SignedLongType; break;
        case 'm': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::UnsignedLongType; break;
        case 'x': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::SignedLongLongType; break;
        case 'y': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::UnsignedLongLongType; break;
        case 'n': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::SignedInt128Type; break;
        case 'o': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::UnsignedInt128Type; break;
        case 'f': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::FloatType; break;
        case 'd': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::DoubleType; break;
        case 'e': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::LongDoubleType; break;
        case 'g': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::Float128Type; break;
        case 'z': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::EllipsisType; break;
        case 'D':
            switch (ADVANCE()) {
            case 'd':
                fixedTypeNode->m_type = PredefinedBuiltinTypeNode::DecimalFloatingType64;
                break;
            case 'e':
                fixedTypeNode->m_type = PredefinedBuiltinTypeNode::DecimalFloatingType128;
                break;
            case 'f':
                fixedTypeNode->m_type = PredefinedBuiltinTypeNode::DecimalFloatingType32;
                break;
            case 'h':
                fixedTypeNode->m_type = PredefinedBuiltinTypeNode::DecimalFloatingType16; break;
            case 'i': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::Char32Type; break;
            case 's': fixedTypeNode->m_type = PredefinedBuiltinTypeNode::Char16Type; break;
            default: throw ParseException(QString::fromLatin1("Invalid built-in type"));
            }
            break;
        default:
            DEMANGLER_ASSERT(false);
        }
    }
}

QByteArray BuiltinTypeNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(0);
}


bool CallOffsetNode::mangledRepresentationStartsWith(char c)
{
    return c == 'h' || c == 'v';
}

/*
 * <call-offset> ::= h <nv-offset> _
 *               ::= v <v-offset> _
 */
void CallOffsetNode::parse()
{
    switch (ADVANCE()) {
    case 'h': PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NvOffsetNode); break;
    case 'v': PARSE_RULE_AND_ADD_RESULT_AS_CHILD(VOffsetNode); break;
    default: DEMANGLER_ASSERT(false);
    }
    if (ADVANCE() != '_')
        throw ParseException(QString::fromLatin1("Invalid call-offset"));
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
     return NonNegativeNumberNode<10>::mangledRepresentationStartsWith(c)
             || c == 'N' || c == 'D' || c == 'Z';
}

/* <class-enum-type> ::= <name> */
void ClassEnumTypeNode::parse()
{
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NameNode);
}

QByteArray ClassEnumTypeNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(0);
}


bool DiscriminatorNode::mangledRepresentationStartsWith(char c)
{
    return c == '_';
}

/* <discriminator> := _ <non-negative-number> */
void DiscriminatorNode::parse()
{
    if (ADVANCE() != '_')
        throw ParseException(QString::fromLatin1("Invalid discriminator"));
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumberNode<10>);
}

QByteArray DiscriminatorNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(0);
}


bool CtorDtorNameNode::mangledRepresentationStartsWith(char c)
{
    return c == 'C' || c == 'D';
}

/*
 * <ctor-dtor-name> ::= C1      # complete object constructor
 *                  ::= C2      # base object constructor
 *                  ::= C3      # complete object allocating constructor
 *                  ::= D0      # deleting destructor
 *                  ::= D1      # complete object destructor
 *                  ::= D2      # base object destructor
 */
void CtorDtorNameNode::parse()
{
    switch (ADVANCE()) {
    case 'C':
        switch (ADVANCE()) {
        case '1': case '2': case '3': m_isDestructor = false; break;
        default: throw ParseException(QString::fromLatin1("Invalid ctor-dtor-name"));
        }
        break;
    case 'D':
        switch (ADVANCE()) {
        case '0': case '1': case '2': m_isDestructor = true; break;
        default: throw ParseException(QString::fromLatin1("Invalid ctor-dtor-name"));
        }
        break;
    default:
        throw ParseException(QString::fromLatin1("Invalid ctor-dtor-name"));
    }

    m_representation = parseState()->substitutionAt(parseState()->substitutionCount() - 1);
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

/*  <CV-qualifiers> ::= [r] [V] [K] 	# restrict (C99), volatile, const */
void CvQualifiersNode::parse()
{
    m_hasConst = false;
    m_hasVolatile = false;

    while (true) {
        if (PEEK() == 'V') {
            if (hasQualifiers())
                throw ParseException(QLatin1String("Invalid qualifiers: unexpected 'volatile'"));
            m_hasVolatile = true;
            ADVANCE();
        } else if (PEEK() == 'K') {
            if (m_hasConst)
                throw ParseException(QLatin1String("Invalid qualifiers: 'const' appears twice"));
            m_hasConst = true;
            ADVANCE();
        } else {
            break;
        }
    }
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

/*
 * <encoding> ::= <name> <bare-function-type>
 *            ::= <name>
 *            ::= <special-name>
 */
void EncodingNode::parse()
{
    const char next = PEEK();
    if (NameNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NameNode);
        if (BareFunctionTypeNode::mangledRepresentationStartsWith(PEEK()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(BareFunctionTypeNode);
        parseState()->addSubstitution(this);
        parseState()->clearTemplateParams();
        parseState()->setIsConversionOperator(false);
    } else if (SpecialNameNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SpecialNameNode);
    } else {
        throw ParseException(QString::fromLatin1("Invalid encoding"));
    }
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
    if (funcNode->hasReturnType())
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

/*
 * <expression> ::= <operator-name> <expression>
 *              ::= <operator-name> <expression> <expression>
 *              ::= <operator-name> <expression> <expression> <expression>
 *              ::= cl <expression>* E          # call
 *              ::= cv <type> expression        # conversion with one argument
 *              ::= cv <type> _ <expression>* E # conversion with a different number of arguments
 *              ::= st <type>                   # sizeof (a type)
 *              ::= at <type>                      # alignof (a type)
 *              ::= <template-param>
 *              ::= <function-param>
 *              ::= sr <type> <unqualified-name>                   # dependent name
 *              ::= sr <type> <unqualified-name> <template-args>   # dependent template-id
 *              ::= sZ <template-param>                            # size of a parameter pack
 *              ::= <expr-primary>
 *
 * Note that the grammar is missing the definition of <function-param>. This
 * has not been a problem in the test cases so far.
 * TODO: <function-param> is now defined and should therefore be supported
 */
void ExpressionNode::parse()
{
    m_type = OtherType;

   /*
    * Some of the terminals in the productions of <expression>
    * also appear in the productions of <operator-name>. We assume the direct
    * productions to have higher precedence and check them first to prevent
    * them being parsed by parseOperatorName().
    */
    QByteArray str = parseState()->readAhead(2);
    if (str == "cl") {
        parseState()->advance(2);
        while (ExpressionNode::mangledRepresentationStartsWith(PEEK()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
        if (ADVANCE() != 'E')
            throw ParseException(QString::fromLatin1("Invalid expression"));
    } else if (str == "cv") {
        m_type = ConversionType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
        if (PEEK() == '_') {
            ADVANCE();
            while (ExpressionNode::mangledRepresentationStartsWith(PEEK()))
                PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
            if (ADVANCE() != 'E')
                throw ParseException(QString::fromLatin1("Invalid expression"));
        } else {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
        }
    } else if (str == "st") {
        m_type = SizeofType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    } else if (str == "at") {
        m_type = AlignofType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    } else if (str == "sr") { // TODO: Which syntax to use here?
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnqualifiedNameNode);
        if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
    } else if (str == "sZ") {
        m_type = ParameterPackSizeType;
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateParamNode);
    } else {
        const char next = PEEK();
        if (OperatorNameNode::mangledRepresentationStartsWith(next)) {
            m_type = OperatorType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(OperatorNameNode);
            OperatorNameNode * const opNode
                    = DEMANGLER_CAST(OperatorNameNode, MY_CHILD_AT(childCount() - 1));

            int expressionCount;
            switch (opNode->type()) {
            case OperatorNameNode::TernaryType:
                expressionCount = 3;
                break;
            case OperatorNameNode::ArrayNewType:
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
            case OperatorNameNode::ArrowStarType:
            case OperatorNameNode::ArrowType:
            case OperatorNameNode::IndexType:
                expressionCount = 2;
                break;
            default:
                expressionCount = 1;
            }

            for (int i = 0; i < expressionCount; ++i)
                PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
        } else if (TemplateParamNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateParamNode);

#if 0
        } else if (FunctionParamNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(FunctionParamNode);
#endif
        } else if (ExprPrimaryNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExprPrimaryNode);
        } else {
            throw ParseException(QString::fromLatin1("Invalid expression"));
        }
    }
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
        switch (opNode->type()) {
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

/*
 * <operator-name> ::= nw       # new
 *                 ::= na        # new[]
 *                 ::= dl        # delete
 *                 ::= da        # delete[]
 *                 ::= ps        # + (unary)
 *                 ::= ng        # - (unary)
 *                 ::= ad        # & (unary)
 *                 ::= de        # * (unary)
 *                 ::= co        # ~
 *                 ::= pl        # +
 *                 ::= mi        # -
 *                 ::= ml        # *
 *                 ::= dv        # /
 *                 ::= rm        # %
 *                 ::= an        # &
 *                 ::= or        # |
 *                 ::= eo        # ^
 *                 ::= aS        # =
 *                 ::= pL        # +=
 *                 ::= mI        # -=
 *                 ::= mL        # *=
 *                 ::= dV        # /=
 *                 ::= rM        # %=
 *                 ::= aN        # &=
 *                 ::= oR        # |=
 *                 ::= eO        # ^=
 *                 ::= ls        # <<
 *                 ::= rs        # >>
 *                 ::= lS        # <<=
 *                 ::= rS        # >>=
 *                 ::= eq        # ==
 *                 ::= ne        # !=
 *                 ::= lt        # <
 *                 ::= gt        # >
 *                 ::= le        # <=
 *                 ::= ge        # >=
 *                 ::= nt        # !
 *                 ::= aa        # &&
 *                 ::= oo        # ||
 *                 ::= pp        # ++
 *                 ::= mm        # --
 *                 ::= cm        # ,
 *                 ::= pm        # ->*
 *                 ::= pt        # ->
 *                 ::= cl        # ()
 *                 ::= ix        # []
 *                 ::= qu        # ?
 *                 ::= st        # sizeof (a type)
 *                 ::= sz        # sizeof (an expression)
 *                 ::= at        # alignof (a type)
 *                 ::= az        # alignof (an expression)
 *                 ::= cv <type> # (cast)
 *                 ::= v <digit> <source-name>   # vendor extended operator
 */
void OperatorNameNode::parse()
{
    if (PEEK() == 'v') {
        m_type = VendorType;
        ADVANCE();
        const int digit = ADVANCE();
        if (!std::isdigit(digit))
            throw ParseException(QString::fromLatin1("Invalid digit"));
        // Throw away digit for now; we don't know what to do with it anyway.
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SourceNameNode);
    } else {
        const QByteArray id = parseState()->readAhead(2);
        parseState()->advance(2);
        if (id == "cv") {
            parseState()->setIsConversionOperator(true);
            m_type = CastType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
        } else if (id == "nw") {
            m_type = NewType;
        } else if (id == "na") {
            m_type = ArrayNewType;
        } else if (id == "dl") {
            m_type = DeleteType;
        } else if (id == "da") {
            m_type = ArrayDeleteType;
        } else if (id == "ps") {
            m_type = UnaryPlusType;
        } else if (id == "ng") {
            m_type = UnaryMinusType;
        } else if (id == "ad") {
            m_type = UnaryAmpersandType;
        } else if (id == "de") {
            m_type = UnaryStarType;
        } else if (id == "co") {
            m_type = BitwiseNotType;
        } else if (id == "pl") {
            m_type = BinaryPlusType;
        } else if (id == "mi") {
            m_type = BinaryMinusType;
        } else if (id == "ml") {
            m_type = MultType;
        } else if (id == "dv") {
            m_type = DivType;
        } else if (id == "rm") {
            m_type = ModuloType;
        } else if (id == "an") {
            m_type = BitwiseAndType;
        } else if (id == "or") {
            m_type = BitwiseOrType;
        } else if (id == "eo") {
            m_type = XorType;
        } else if (id == "aS") {
            m_type = AssignType;
        } else if (id == "pL") {
            m_type = IncrementAndAssignType;
        } else if (id == "mI") {
            m_type = DecrementAndAssignType;
        } else if (id == "mL") {
            m_type = MultAndAssignType;
        } else if (id == "dV") {
            m_type = DivAndAssignType;
        } else if (id == "rM") {
            m_type = ModuloAndAssignType;
        } else if (id == "aN") {
            m_type = BitwiseAndAndAssignType;
        } else if (id == "oR") {
            m_type = BitwiseOrAndAssignType;
        } else if (id == "eO") {
            m_type = XorAndAssignType;
        } else if (id == "ls") {
            m_type = LeftShiftType;
        } else if (id == "rs") {
            m_type = RightShiftType;
        } else if (id == "lS") {
            m_type = LeftShiftAndAssignType;
        } else if (id == "rS") {
            m_type = RightShiftAndAssignType;
        } else if (id == "eq") {
            m_type = EqualsType;
        } else if (id == "ne") {
            m_type = NotEqualsType;
        } else if (id == "lt") {
            m_type = LessType;
        } else if (id == "gt") {
            m_type = GreaterType;
        } else if (id == "le") {
            m_type = LessEqualType;
        } else if (id == "ge") {
            m_type = GreaterEqualType;
        } else if (id == "nt") {
            m_type = LogicalNotType;
        } else if (id == "aa") {
            m_type = LogicalAndType;
        } else if (id == "oo") {
            m_type = LogicalOrType;
        } else if (id == "pp") {
            m_type = IncrementType;
        } else if (id == "mm") {
            m_type = DecrementType;
        } else if (id == "cm") {
            m_type = CommaType;
        } else if (id == "pm") {
            m_type = ArrowStarType;
        } else if (id == "pt") {
            m_type = ArrowType;
        } else if (id == "cl") {
            m_type = CallType;
        } else if (id == "ix") {
            m_type = IndexType;
        } else if (id == "qu") {
            m_type = TernaryType;
        } else if (id == "st") {
            m_type = SizeofTypeType;
        } else if (id == "sz") {
            m_type = SizeofExprType;
        } else if (id == "at") {
            m_type = AlignofTypeType;
        } else if (id == "az") {
            m_type = AlignofExprType;
        } else {
            throw ParseException(QString::fromLocal8Bit("Invalid operator encoding '%1'")
                    .arg(QString::fromLocal8Bit(id)));
        }
    }
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

/*
 * <expr-primary> ::= L <type> <number> E            # integer literal
 *                ::= L <type> <float> E             # floating literal
 *                ::= L <mangled-name> E             # external name
 */
 // TODO: This has been updated in the spec. Needs to be adapted. (nullptr etc.)
void ExprPrimaryNode::parse()
{
    if (!ExprPrimaryNode::mangledRepresentationStartsWith(ADVANCE()))
        throw ParseException(QString::fromLatin1("Invalid primary expression"));
    const char next = PEEK();
    if (TypeNode::mangledRepresentationStartsWith(next)) {
        const ParseTreeNode * const topLevelTypeNode = parseRule<TypeNode>(parseState());
        BuiltinTypeNode * const typeNode = topLevelTypeNode->childCount() == 0
                ? 0 : dynamic_cast<BuiltinTypeNode * >(CHILD_AT(topLevelTypeNode, 0));
        if (!typeNode)
            throw ParseException(QLatin1String("Invalid type in expr-primary"));
        PredefinedBuiltinTypeNode * const predefTypeNode
                = dynamic_cast<PredefinedBuiltinTypeNode *>(CHILD_AT(typeNode, 0));
        if (!predefTypeNode)
            throw ParseException(QLatin1String("Invalid type in expr-primary"));

        // TODO: Of which type can a literal actually be?
        switch (predefTypeNode->m_type) {
        case PredefinedBuiltinTypeNode::SignedIntType:
        case PredefinedBuiltinTypeNode::UnsignedIntType:
        case PredefinedBuiltinTypeNode::UnsignedLongType:
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NumberNode);
            break;
        case PredefinedBuiltinTypeNode::FloatType: case PredefinedBuiltinTypeNode::DoubleType:
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(FloatValueNode);
            break;
        default:
            throw ParseException(QString::fromLatin1("Invalid type in expr-primary"));
        }
        delete parseState()->popFromStack(); // No need to keep the type node in the tree.
    } else if (MangledNameNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(MangledNameNode);
    } else {
        throw ParseException(QString::fromLatin1("Invalid expr-primary"));
    }
    if (ADVANCE() != 'E')
        throw ParseException(QString::fromLatin1("Invalid expr-primary"));
}

QByteArray ExprPrimaryNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(0);
}

bool FunctionTypeNode::mangledRepresentationStartsWith(char c)
{
    return c == 'F';
}

/* <function-type> ::= F [Y] <bare-function-type> E */
void FunctionTypeNode::parse()
{
    if (!mangledRepresentationStartsWith(ADVANCE()))
        throw ParseException(QString::fromLatin1("Invalid function type"));

    if (PEEK() == 'Y') {
        ADVANCE();
        m_isExternC = true;
    } else {
        m_isExternC = false;
    }

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(BareFunctionTypeNode);
    if (ADVANCE() != 'E')
        throw ParseException(QString::fromLatin1("Invalid function type"));
}

QByteArray FunctionTypeNode::toByteArray() const
{
    return QByteArray(); // Not enough knowledge here to generate a string representation.
}


bool LocalNameNode::mangledRepresentationStartsWith(char c)
{
    return c == 'Z';
}

/*
 * <local-name> := Z <encoding> E <name> [<discriminator>]
 *              := Z <encoding> E s [<discriminator>]
 *
 * Note that <name> can start with 's', so we need to do read-ahead.
 */
void LocalNameNode::parse()
{
    if (!mangledRepresentationStartsWith(ADVANCE()))
        throw ParseException(QString::fromLatin1("Invalid local-name"));

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(EncodingNode);

    if (ADVANCE() != 'E')
        throw ParseException(QString::fromLatin1("Invalid local-name"));

    QByteArray str = parseState()->readAhead(2);
    char next = PEEK();
    if (str == "sp" || str == "sr" || str == "st" || str == "sz" || str == "sZ"
            || (next != 's' && NameNode::mangledRepresentationStartsWith(next))) {
        m_isStringLiteral = false;
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NameNode);
    } else if (next == 's') {
        m_isStringLiteral = true;
        ADVANCE();
    } else {
        throw ParseException(QString::fromLatin1("Invalid local-name"));
    }
    if (DiscriminatorNode::mangledRepresentationStartsWith(PEEK()))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(DiscriminatorNode);
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

/*
 * Grammar: http://www.codesourcery.com/public/cxx-abi/abi.html#mangling
 * The grammar as given there is not LL(k), so a number of transformations
 * were necessary, which we will document at the respective parsing function.
 * <mangled-name> ::= _Z <encoding>
 */
void MangledNameNode::parse()
{
    parseState()->advance(2);
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(EncodingNode);
}

QByteArray MangledNameNode::toByteArray() const
{
    return pasteAllChildren();
}


bool SourceNameNode::mangledRepresentationStartsWith(char c)
{
    return strchr("123456789", c);
}

/* <source-name> ::= <number> <identifier> */
void SourceNameNode::parse()
{
    const int idLen = getNonNegativeNumber<10>(parseState());
    m_name = parseState()->readAhead(idLen);
    parseState()->advance(idLen);
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
    return opNode && opNode->type() == OperatorNameNode::CastType;
}

/*
 * <unqualified-name> ::= <operator-name>
 *                    ::= <ctor-dtor-name>
 *                    ::= <source-name>
 */
void UnqualifiedNameNode::parse()
{
    const char next = PEEK();
    if (OperatorNameNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(OperatorNameNode);
    else if (CtorDtorNameNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CtorDtorNameNode);
    else if (SourceNameNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SourceNameNode);
    else
        throw ParseException(QString::fromLatin1("Invalid unqualified-name"));
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

/*
 * <unscoped-name> ::= <unqualified-name>
 *                 ::= St <unqualified-name>   # ::std::
 */
void UnscopedNameNode::parse()
{
    if (parseState()->readAhead(2) == "St") {
        m_inStdNamespace = true;
        parseState()->advance(2);
    } else {
        m_inStdNamespace = false;
    }

    if (!UnqualifiedNameNode::mangledRepresentationStartsWith(PEEK()))
        throw ParseException(QString::fromLatin1("Invalid unscoped-name"));

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnqualifiedNameNode);
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

/*
 * <nested-name> ::= N [<CV-qualifiers>] <prefix> <unqualified-name> E
 *               ::= N [<CV-qualifiers>] <template-prefix> <template-args> E
 * <template-prefix> ::= <prefix> <unqualified-name>
 *                   ::= <template-param>
 *                   ::= <substitution>
 *
 * The <template-prefix> rule leads to an indirect recursion with <prefix>, so
 * we integrate it into <nested-name>:
 * <nested-name> ::= N [<CV-qualifiers>] <prefix> <unqualified-name>
 *                   [<template-args>] E
 *               ::= N [<CV-qualifiers>] <template-param> <template-args> E
 *               ::= N [<CV-qualifiers>] <substitution> <template-args> E
 *
 * The occurrence of <prefix> in the first expansion makes this rule
 * completely unmanageable, because <prefix>'s first and follow sets are
 * not distinct and it also shares elements of its first set with
 * <template-param> and <substitution>. However, <prefix> can expand
 * to both the non-terminals it is followed by as well as the two competing
 * non-terminal sequences in the other rules, so we can just write:
 * <nested-name> ::= N [<CV-qualifiers>] <prefix> E
 *
 * That's not all, though: Both <operator-name> and <cv-qualifiers> can start
 * with an 'r', so we have to do a two-character-look-ahead for that case.
 */
void NestedNameNode::parse()
{
    if (!mangledRepresentationStartsWith(ADVANCE()))
        throw ParseException(QString::fromLatin1("Invalid nested-name"));

    if (CvQualifiersNode::mangledRepresentationStartsWith(PEEK()) && parseState()->peek(1) != 'm'
            && parseState()->peek(1) != 'M' && parseState()->peek(1) != 's'
            && parseState()->peek(1) != 'S') {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CvQualifiersNode);
    }

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(PrefixNode);

    if (ADVANCE() != 'E')
        throw ParseException(QString::fromLatin1("Invalid nested-name"));
}


bool SubstitutionNode::mangledRepresentationStartsWith(char c)
{
    return c == 'S';
}

/*
 * <substitution> ::= S <seq-id> _ # 36-bit number
 *                ::= S_
 *                ::= St # ::std::
 *                ::= Sa # ::std::allocator
 *                ::= Sb # ::std::basic_string
 *                ::= Ss # ::std::basic_string < char,
 *                      			 ::std::char_traits<char>,
 *                      			 ::std::allocator<char> >
 *                ::= Si # ::std::basic_istream<char,  std::char_traits<char> >
 *                ::= So # ::std::basic_ostream<char,  std::char_traits<char> >
 *                ::= Sd # ::std::basic_iostream<char, std::char_traits<char> >
 */
void SubstitutionNode::parse()
{
    if (!mangledRepresentationStartsWith(ADVANCE()))
        throw ParseException(QString::fromLatin1("Invalid substitution"));

    if (NonNegativeNumberNode<36>::mangledRepresentationStartsWith(PEEK())) {
        const int substIndex = getNonNegativeNumber<36>(parseState()) + 1;
        if (substIndex >= parseState()->substitutionCount()) {
            throw ParseException(QString::fromLatin1("Invalid substitution: substitution %1 "
                "was requested, but there are only %2").
                arg(substIndex + 1).arg(parseState()->substitutionCount()));
        }
        m_type = ActualSubstitutionType;
        m_substValue = parseState()->substitutionAt(substIndex);
        if (ADVANCE() != '_')
            throw ParseException(QString::fromLatin1("Invalid substitution"));
    } else {
        switch (ADVANCE()) {
        case '_':
            if (parseState()->substitutionCount() == 0)
                throw ParseException(QString::fromLatin1("Invalid substitution: "
                    "There are no substitutions"));
            m_type = ActualSubstitutionType;
            m_substValue = parseState()->substitutionAt(0);
            break;
        case 't': m_type = StdType; break;
        case 'a': m_type = StdAllocType; break;
        case 'b': m_type = StdBasicStringType; break;
        case 's': m_type = FullStdBasicStringType; break;
        case 'i': m_type = StdBasicIStreamType; break;
        case 'o': m_type = StdBasicOStreamType; break;
        case 'd': m_type = StdBasicIoStreamType; break;
        default: throw ParseException(QString::fromLatin1("Invalid substitution"));
        }
    }
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

/* <pointer-to-member-type> ::= M <type> <type> */
void PointerToMemberTypeNode::parse()
{
    if (!mangledRepresentationStartsWith(ADVANCE()))
        throw ParseException(QString::fromLatin1("Invalid pointer-to-member-type"));

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode); // Class type.
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode); // Member type.
}

QByteArray PointerToMemberTypeNode::toByteArray() const
{
    // Gather all qualifiers first, because we have to move them to the end en bloc
    // .
    QByteArray qualRepr;
    const TypeNode *memberTypeNode = DEMANGLER_CAST(TypeNode, MY_CHILD_AT(1));
    while (memberTypeNode->type() == TypeNode::QualifiedType) {
        const CvQualifiersNode * const cvNode
                = DEMANGLER_CAST(CvQualifiersNode, CHILD_AT(memberTypeNode, 0));
        if (cvNode->hasQualifiers()) {
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
        if (functionNode->isExternC())
            repr += "extern \"C\" ";
        if (bareFunctionNode->hasReturnType())
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

/*
 * <template-param> ::= T_      # first template parameter
 *                  ::= T <non-negative-number> _
 */
void TemplateParamNode::parse()
{
    if (!mangledRepresentationStartsWith(ADVANCE()))
        throw ParseException(QString::fromLatin1("Invalid template-param"));

    if (PEEK() == '_')
        m_index = 0;
    else
        m_index = getNonNegativeNumber<10>(parseState()) + 1;
    if (ADVANCE() != '_')
        throw ParseException(QString::fromLatin1("Invalid template-param"));
    if (m_index >= parseState()->templateParamCount()) {
        if (!parseState()->isConversionOperator()) {
            throw ParseException(QString::fromLocal8Bit("Invalid template parameter index %1")
                    .arg(m_index));
        }
    } else {
        addChild(parseState()->templateParamAt(m_index));
    }
}

QByteArray TemplateParamNode::toByteArray() const
{
    return CHILD_TO_BYTEARRAY(0);
}


bool TemplateArgsNode::mangledRepresentationStartsWith(char c)
{
    return c == 'I';
}

/*
 * <template-args> ::= I <template-arg>+ E
 */
void TemplateArgsNode::parse()
{
    if (!mangledRepresentationStartsWith(ADVANCE()))
        throw ParseException(QString::fromLatin1("Invalid template args"));

    do
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgNode);
    while (TemplateArgNode::mangledRepresentationStartsWith(PEEK()));

    if (ADVANCE() != 'E')
        throw ParseException(QString::fromLatin1("Invalid template args"));
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

/*
 * <special-name> ::= TV <type>  # virtual table
 *                ::= TT <type>  # VTT structure (construction vtable index)
 *                ::= TI <type>  # typeinfo structure
 *                ::= TS <type>  # typeinfo name (null-terminated byte string)
 *                ::= GV <name>  # Guard variable for one-time initialization
 *                ::= T <call-offset> <encoding>
 *                ::= Tc <call-offset> <call-offset> <encoding>
 *                     # base is the nominal target function of thunk
 *                     # first call-offset is 'this' adjustment
 *                     # second call-offset is result adjustment
 */
void SpecialNameNode::parse()
{
    QByteArray str = parseState()->readAhead(2);
    if (str == "TV") {
        m_type = VirtualTableType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    } else if (str == "TT") {
        m_type = VttStructType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    } else if (str == "TI") {
        m_type = TypeInfoType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    } else if (str == "TS") {
        m_type = TypeInfoNameType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    } else if (str == "GV") {
        m_type = GuardVarType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NameNode);
    } else if (str == "Tc") {
        m_type = DoubleCallOffsetType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CallOffsetNode);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CallOffsetNode);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(EncodingNode);
    } else if (ADVANCE() == 'T') {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CallOffsetNode);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(EncodingNode);
    } else {
        throw ParseException(QString::fromLatin1("Invalid special-name"));
    }
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


bool NumberNode::mangledRepresentationStartsWith(char c)
{
    return NonNegativeNumberNode<10>::mangledRepresentationStartsWith(c) || c == 'n';
}

/* <number> ::= [n] <non-negative decimal integer> */
void NumberNode::parse()
{
    const char next = PEEK();
    if (!mangledRepresentationStartsWith(next))
        throw ParseException("Invalid number");

    if (next == 'n') {
        m_isNegative = true;
        ADVANCE();
    } else {
        m_isNegative = false;
    }

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumberNode<10>);
}

QByteArray NumberNode::toByteArray() const
{
    QByteArray repr = CHILD_TO_BYTEARRAY(0);
    if (m_isNegative)
        repr.prepend('-');
    return repr;
}


template<int base> bool NonNegativeNumberNode<base>::mangledRepresentationStartsWith(char c)
{
    // Base can only be 10 or 36.
    if (base == 10)
        return strchr("0123456789", c);
    else
        return strchr("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", c);
}

template<int base> void NonNegativeNumberNode<base>::parse()
{
    QByteArray numberRepr;
    while (mangledRepresentationStartsWith(PEEK()))
        numberRepr += ADVANCE();
    if (numberRepr.count() == 0)
        throw ParseException(QString::fromLatin1("Invalid non-negative number"));
    m_number = numberRepr.toULongLong(0, base);
}

template<int base> QByteArray NonNegativeNumberNode<base>::toByteArray() const
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

/*
 * <name> ::= <nested-name>
 *        ::= <unscoped-name>
 *        ::= <unscoped-template-name> <template-args>
 *        ::= <local-name>     # See Scope Encoding below
 *
 * We can't use this rule directly, because <unscoped-template-name>
 * can expand to <unscoped-name>. We therefore integrate it directly
 * into the production for <name>:
 * <name> ::= <unscoped-name> [<template-args>]
 *        ::= <substitution> <template-args>
 *
 * Secondly, <substitution> shares an expansion ("St") with <unscoped-name>,
 * so we have to look further ahead to see which one matches.
 */
void NameNode::parse()
{
    if ((parseState()->readAhead(2) == "St"
         && UnqualifiedNameNode::mangledRepresentationStartsWith(parseState()->peek(2)))
            || UnscopedNameNode::mangledRepresentationStartsWith(PEEK())) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnscopedNameNode);
        if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK())) {
            parseState()->addSubstitution(this);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
        }
    } else {
        const char next = PEEK();
        if (NestedNameNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NestedNameNode);
        } else if (SubstitutionNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SubstitutionNode);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
        } else if (LocalNameNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(LocalNameNode);
        } else {
            throw ParseException(QString::fromLatin1("Invalid name"));
        }
    }
}


bool TemplateArgNode::mangledRepresentationStartsWith(char c)
{
    return TypeNode::mangledRepresentationStartsWith(c)
            || ExprPrimaryNode::mangledRepresentationStartsWith(c)
            || c == 'X' || c == 'I' || c == 's';
}

/*
 * <template-arg> ::= <type>                    # type or template
 *                ::= X <expression> E          # expression
 *                ::= <expr-primary>            # simple expressions
 *                ::= J <template-arg>* E       # argument pack
 *                ::= sp <expression>           # pack expansion of (C++0x)
 */
void TemplateArgNode::parse()
{
    m_isTemplateArgumentPack = false;

    char next = PEEK();
    if (parseState()->readAhead(2) == "sp") {
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
    } else if (TypeNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    } else if (ExprPrimaryNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExprPrimaryNode);
    } else if (next == 'X') {
        ADVANCE();
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
        if (ADVANCE() != 'E')
            throw ParseException(QString::fromLatin1("Invalid template-arg"));
    } else if (next == 'J') {
        ADVANCE();
        while (TemplateArgNode::mangledRepresentationStartsWith(PEEK()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgNode);
        if (ADVANCE() != 'E')
            throw ParseException(QString::fromLatin1("Invalid template-arg"));
    } else {
        throw ParseException(QString::fromLatin1("Invalid template-arg"));
    }

    parseState()->addTemplateParam(this);
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

/*
 * <prefix-2> ::= <unqualified-name> [<template-args>] <prefix-2>
 *            ::=  # empty
 */
void Prefix2Node::parse()
{
    // We need to do this so we can correctly add all substitutions, which always start
    // with the representation of the prefix node.
    // Note that this breaks the invariant that a node is on the stack while it is being parsed;
    // it does not seem that this matters in practice for this particular node.
    ParseTreeNode * const prefixNode
            = parseState()->stackElementAt(parseState()->stackElementCount() - 2);
    prefixNode->addChild(this);
    parseState()->popFromStack();

    bool firstRun = true;
    while (UnqualifiedNameNode::mangledRepresentationStartsWith(PEEK())) {
        if (!firstRun)
            parseState()->addSubstitution(prefixNode);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnqualifiedNameNode);
        if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK())) {
            parseState()->addSubstitution(prefixNode);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
        }
        firstRun = false;
    }
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

/*
 * <prefix> ::= <prefix> <unqualified-name>
 *          ::= <template-prefix> <template-args>
 *          ::= <template-param>
 *          ::= # empty
 *          ::= <substitution>
 *
 * We have to eliminate the left-recursion and the template-prefix rule
 * and end up with this:
 * <prefix> ::= <template-param> [<template-args>] <prefix-2>
 *          ::= <substitution> [<template-args>] <prefix-2>
 *          ::= <prefix-2>
 */
void PrefixNode::parse()
{
    const char next = PEEK();
    if (TemplateParamNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateParamNode);
        if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK())) {
            parseState()->addSubstitution(this);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
        }
        if (UnqualifiedNameNode::mangledRepresentationStartsWith(PEEK())) {
            parseState()->addSubstitution(this);
            parseRule<Prefix2Node>(parseState()); // Pops itself to child list.
        }
    } else if (SubstitutionNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SubstitutionNode);
        if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK())) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
            if (UnqualifiedNameNode::mangledRepresentationStartsWith(PEEK()))
                parseState()->addSubstitution(this);
        }
        parseRule<Prefix2Node>(parseState()); // Pops itself to child list.
    } else {
        parseRule<Prefix2Node>(parseState()); // Pops itself to child list.
    }
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

/*
 * <type> ::= <builtin-type>
 *        ::= <function-type>
 *        ::= <class-enum-type>
 *        ::= <array-type>
 *        ::= <pointer-to-member-type>
 *        ::= <template-param>
 *        ::= <template-template-param> <template-args>
 *        ::= <substitution> # See Compression below
 *        ::= <CV-qualifiers> <type>
 *        ::= P <type>   # pointer-to
 *        ::= R <type>   # reference-to
 *        ::= O <type>   # rvalue reference-to (C++0x)
 *        ::= C <type>   # complex pair (C 2000)
 *        ::= G <type>   # imaginary (C 2000)
 *        ::= U <source-name> <type>     # vendor extended type qualifier
 *        ::= Dp <type>          # pack expansion of (C++0x)
 *        ::= Dt <expression> E  # decltype of an id-expression or class member access (C++0x)
 *        ::= DT <expression> E  # decltype of an expression (C++0x)
 *
 * Because <template-template-param> can expand to <template-param>, we have to
 * do a slight transformation: We get rid of <template-template-param> and
 * integrate its rhs into <type>'s rhs. This leads to the following
 * identical prefixes:
 * <type> ::= <template-param>
 *        ::= <template-param> <template-args>
 *        ::= <substitution>
 *        ::= <substitution> <template-args>
 *
 * Also, the first set of <builtin-type> has some overlap with
 * direct productions of <type>, so these have to be worked around as well.
 */
void TypeNode::parse()
{
    QByteArray str = parseState()->readAhead(2);
    if (str == "Dp") {
        m_type = PackExpansionType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    } else if (str == "Dt") {
        m_type = DeclType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
        if (ADVANCE() != 'E')
            throw ParseException(QString::fromLatin1("Invalid type"));
    } else if (str == "DT") {
        m_type = DeclType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
        if (ADVANCE() != 'E')
            throw ParseException(QString::fromLatin1("Invalid type"));
    } else {
        const char next = PEEK();
        if (str == "Dd" || str == "De" || str == "Df" || str == "Dh" || str == "Di" || str == "Ds"
            || (next != 'D' && BuiltinTypeNode::mangledRepresentationStartsWith(next))) {
            m_type = OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(BuiltinTypeNode);
        } else if (FunctionTypeNode::mangledRepresentationStartsWith(next)) {
            m_type = OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(FunctionTypeNode);
            parseState()->addSubstitution(this);
        } else if (ClassEnumTypeNode::mangledRepresentationStartsWith(next)) {
            m_type = OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ClassEnumTypeNode);
            parseState()->addSubstitution(this);
        } else if (ArrayTypeNode::mangledRepresentationStartsWith(next)) {
            m_type = OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ArrayTypeNode);
        } else if (PointerToMemberTypeNode::mangledRepresentationStartsWith(next)) {
            m_type = OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(PointerToMemberTypeNode);
        } else if (TemplateParamNode::mangledRepresentationStartsWith(next)) {
            m_type = OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateParamNode);
            // The type is now a substitution candidate, but the child node may contain a forward
            // reference, so we delay the substitution until it is resolved.
            // TODO: Check whether this is really safe, i.e. whether the following parse function
            // might indirectly expect this substitution to already exist.

            if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK())) {
                PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
                // Substitution delayed, see above.
            }

            // Resolve forward reference, if necessary.
            TemplateParamNode * const templateParamNode
                    = DEMANGLER_CAST(TemplateParamNode, MY_CHILD_AT(0));
            if (templateParamNode->childCount() == 0) {
                if (templateParamNode->index() >= parseState()->templateParamCount()) {
                    throw ParseException(QString::fromLocal8Bit("Invalid template parameter "
                        "index %1 in forwarding").arg(templateParamNode->index()));
                }
                templateParamNode->addChild(parseState()
                        ->templateParamAt(templateParamNode->index()));
            }

            // Delayed substitutions from above.
            parseState()->addSubstitution(templateParamNode);
            if (childCount() > 1)
                parseState()->addSubstitution(this);
        } else if (SubstitutionNode::mangledRepresentationStartsWith(next)) {
            m_type = OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SubstitutionNode);
            if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK())) {
                PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
                parseState()->addSubstitution(this);
            }
        } else if (CvQualifiersNode::mangledRepresentationStartsWith(next)) {
            m_type = QualifiedType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CvQualifiersNode);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
            const CvQualifiersNode * const cvNode
                    = DEMANGLER_CAST(CvQualifiersNode, MY_CHILD_AT(0));
            if (cvNode->hasQualifiers())
                parseState()->addSubstitution(this);
        } else if (next == 'P') {
            m_type = PointerType;
            ADVANCE();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
            parseState()->addSubstitution(this);
        } else if (next == 'R') {
            m_type = ReferenceType;
            ADVANCE();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
            parseState()->addSubstitution(this);
        } else if (next == 'O') {
            m_type = RValueType;
            ADVANCE();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
            parseState()->addSubstitution(this);
        } else if (next == 'C') {
            m_type = OtherType;
            ADVANCE();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
            parseState()->addSubstitution(this);
        } else if (next == 'G') {
            m_type = OtherType;
            ADVANCE();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
            parseState()->addSubstitution(this);
        } else if (next == 'U') {
            m_type = VendorType;
            ADVANCE();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SourceNameNode);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
        } else {
            throw ParseException(QString::fromLatin1("Invalid type"));
        }
    }
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
            if (cvNode->hasQualifiers())
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
        if (functionNode->isExternC())
            repr += "extern \"C\" ";
        if (bareFunctionNode->hasReturnType())
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

/*
 * Floating-point literals are encoded using a fixed-length lowercase
 * hexadecimal string corresponding to the internal representation
 * (IEEE on Itanium), high-order bytes first, without leading zeroes.
 * For example: "Lf bf800000 E" is -1.0f on Itanium.
 */
void FloatValueNode::parse()
{
    m_value = 0;
    while (mangledRepresentationStartsWith(PEEK())) {
        // TODO: Construct value;
        ADVANCE();
    }
}

QByteArray FloatValueNode::toByteArray() const
{
    return QByteArray::number(m_value);
}


/* <nv-offset> ::= <number> # non-virtual base override */
void NvOffsetNode::parse()
{
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NumberNode);
}

/*
 * <v-offset> ::= <number> _ <number>
 *    # virtual base override, with vcall offset
 */
void VOffsetNode::parse()
{
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NumberNode);
    if (ADVANCE() != '_')
        throw ParseException(QString::fromLatin1("Invalid v-offset"));
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NumberNode);
}

} // namespace Internal
} // namespace Debugger
