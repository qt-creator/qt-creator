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

#include "parsetreenodes.h"

#include "demanglerexceptions.h"

#include <iostream>
#include <cctype>
#include <cstring>

#define PEEK() (parseState()->peek())
#define ADVANCE() (parseState()->advance())

#define PARSE_RULE_AND_ADD_RESULT_AS_CHILD_TO_NODE(NodeType, parseState, parentNode) \
    do { \
        ParseTreeNode::parseRule<NodeType>(parseState); \
        DEMANGLER_ASSERT(parseState->stackElementCount() > 0); \
        DEMANGLER_ASSERT(parseState->stackTop().dynamicCast<NodeType>()); \
        if (parentNode) \
            (parentNode)->addChild(parseState->popFromStack()); \
    } while (0)


#define PARSE_RULE_AND_ADD_RESULT_AS_CHILD(nodeType) \
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD_TO_NODE(nodeType, parseState(), this)
#define CHILD_AT(obj, index) obj->childAt(index, QLatin1String(Q_FUNC_INFO), QLatin1String(__FILE__), __LINE__)
#define MY_CHILD_AT(index) CHILD_AT(this, index)
#define CHILD_TO_BYTEARRAY(index) MY_CHILD_AT(index)->toByteArray()

namespace Debugger {
namespace Internal {

template<int base> static int getNonNegativeNumber(GlobalParseState *parseState)
{
    ParseTreeNode::parseRule<NonNegativeNumberNode<base> >(parseState);
    const typename NonNegativeNumberNode<base>::Ptr numberNode
            = DEMANGLER_CAST(NonNegativeNumberNode<base>, parseState->popFromStack());
    const int value = static_cast<int>(numberNode->number());
    return value;
}

ParseTreeNode::ParseTreeNode(const ParseTreeNode &other) : m_parseState(other.m_parseState)
{
    foreach (const ParseTreeNode::Ptr &child, other.m_children)
        addChild(child->clone());
}

ParseTreeNode::~ParseTreeNode()
{
}

ParseTreeNode::Ptr ParseTreeNode::childAt(int i, const QString &func, const QString &file,
        int line) const
{
    if (i < 0 || i >= m_children.count())
        throw InternalDemanglerException(func, file, line);
    return m_children.at(i);
}

QByteArray ParseTreeNode::pasteAllChildren() const
{
    QByteArray repr;
    foreach (const ParseTreeNode::Ptr &node, m_children)
        repr += node->toByteArray();
    return repr;
}

void ParseTreeNode::print(int indentation) const
{
    for (int i = 0; i < indentation; ++i)
        std::cerr << ' ';
    std::cerr << description().data() << std::endl;
    foreach (const ParseTreeNode::Ptr &n, m_children)
        n->print(indentation + 2);
}

QByteArray ParseTreeNode::bool2String(bool b) const
{
    return b ? "true" : "false";
}

bool ArrayTypeNode::mangledRepresentationStartsWith(char c)
{
    return c == 'A';
}

/*
 * <array-type> ::= A <number> _ <type>
 *              ::= A [<expression>] _ <type>
 * Note that <expression> can also start with a number, so we have to do a non-constant look-ahead.
 */
void ArrayTypeNode::parse()
{
    if (!ArrayTypeNode::mangledRepresentationStartsWith(ADVANCE()))
        throw ParseException(QString::fromLatin1("Invalid array-type"));

    bool isNumber = NonNegativeNumberNode<10>::mangledRepresentationStartsWith(PEEK());
    int i = 1;
    while (isNumber) {
        const char next = parseState()->peek(i);
        if (next == '_')
            break;
        if (!std::isdigit(parseState()->peek(i)))
            isNumber = false;
        ++i;
    }
    if (isNumber)
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumberNode<10>);
    else if (ExpressionNode::mangledRepresentationStartsWith(PEEK()))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);

    if (ADVANCE() != '_')
        throw ParseException(QString::fromLatin1("Invalid array-type"));

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
}

QByteArray ArrayTypeNode::toByteArray() const
{
    // Note: This is not used for things like "reference to array", which need to
    // combine the child nodes in a different way at a higher level.
    return CHILD_TO_BYTEARRAY(1) + '[' + CHILD_TO_BYTEARRAY(0) + ']';
}


BareFunctionTypeNode::BareFunctionTypeNode(GlobalParseState *parseState)
        : ParseTreeNode(parseState), m_hasReturnType(false)
{
}

BareFunctionTypeNode::BareFunctionTypeNode(const BareFunctionTypeNode &other)
        : ParseTreeNode(other), m_hasReturnType(other.hasReturnType())
{
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
    const EncodingNode::Ptr encodingNode = parseState()->stackElementAt(parseState()
            ->stackElementCount() - 2).dynamicCast<EncodingNode>();
    if (encodingNode) { // Case 1: Function name.
        const NameNode::Ptr nameNode = DEMANGLER_CAST(NameNode, CHILD_AT(encodingNode, 0));
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

QByteArray BareFunctionTypeNode::description() const
{
    QByteArray desc = "BareFunctionType";
    if (m_hasReturnType)
        desc += "[with return type]";
    else
        desc += "[without return type]";
    return desc;
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


BuiltinTypeNode::BuiltinTypeNode(const BuiltinTypeNode &other)
        : ParseTreeNode(other), m_type(other.type())
{
}

bool BuiltinTypeNode::mangledRepresentationStartsWith(char c)
{
    return std::strchr("vwbcahstijlmxynofgedzDu", c);
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
 *                ::= Da # auto (in dependent new-expressions)
 *                ::= Dn # std::nullptr_t (i.e., decltype(nullptr))
 *                ::= u <source-name>    # vendor extended type
 */
void BuiltinTypeNode::parse()
{
    const char next = ADVANCE();
    if (next == 'u') {
        m_type = VendorType;
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SourceNameNode);
        parseState()->addSubstitution(parseState()->stackTop());
        return;
    }

    switch (next) {
    case 'v': m_type = VoidType; break;
    case 'w': m_type = WCharType; break;
    case 'b': m_type = BoolType; break;
    case 'c': m_type = PlainCharType; break;
    case 'a': m_type = SignedCharType; break;
    case 'h': m_type = UnsignedCharType; break;
    case 's': m_type = SignedShortType; break;
    case 't': m_type = UnsignedShortType; break;
    case 'i': m_type = SignedIntType; break;
    case 'j': m_type = UnsignedIntType; break;
    case 'l': m_type = SignedLongType; break;
    case 'm': m_type = UnsignedLongType; break;
    case 'x': m_type = SignedLongLongType; break;
    case 'y': m_type = UnsignedLongLongType; break;
    case 'n': m_type = SignedInt128Type; break;
    case 'o': m_type = UnsignedInt128Type; break;
    case 'f': m_type = FloatType; break;
    case 'd': m_type = DoubleType; break;
    case 'e': m_type = LongDoubleType; break;
    case 'g': m_type = Float128Type; break;
    case 'z': m_type = EllipsisType; break;
    case 'D':
        switch (ADVANCE()) {
        case 'd':
            m_type = DecimalFloatingType64;
            break;
        case 'e':
            m_type = DecimalFloatingType128;
            break;
        case 'f':
            m_type = DecimalFloatingType32;
            break;
        case 'h':
            m_type = DecimalFloatingType16; break;
        case 'i': m_type = Char32Type; break;
        case 's': m_type = Char16Type; break;
        case 'a': m_type = AutoType; break;
        case 'n': m_type = NullPtrType; break;
        default: throw ParseException(QString::fromLatin1("Invalid built-in type"));
        }
        break;
    default:
        DEMANGLER_ASSERT(false);
    }
}

QByteArray BuiltinTypeNode::description() const
{
    return "BuiltinType[" + toByteArray() + ']';
}

QByteArray BuiltinTypeNode::toByteArray() const
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
    case AutoType: return "auto";
    case NullPtrType: return "std::nullptr_t";
    case VendorType: return CHILD_TO_BYTEARRAY(0);
    }

    DEMANGLER_ASSERT(false);
    return QByteArray();
}


bool CallOffsetRule::mangledRepresentationStartsWith(char c)
{
    return c == 'h' || c == 'v';
}

/*
 * <call-offset> ::= h <nv-offset> _
 *               ::= v <v-offset> _
 */
void CallOffsetRule::parse(GlobalParseState *parseState)
{
    const ParseTreeNode::Ptr parentNode = parseState->stackTop();
    switch (parseState->advance()) {
    case 'h': PARSE_RULE_AND_ADD_RESULT_AS_CHILD_TO_NODE(NvOffsetNode, parseState, parentNode); break;
    case 'v': PARSE_RULE_AND_ADD_RESULT_AS_CHILD_TO_NODE(VOffsetNode, parseState, parentNode); break;
    default: DEMANGLER_ASSERT(false);
    }
    if (parseState->advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid call-offset"));
}

bool ClassEnumTypeRule::mangledRepresentationStartsWith(char c)
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
void ClassEnumTypeRule::parse(GlobalParseState *parseState)
{
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD_TO_NODE(NameNode, parseState, parseState->stackTop());
}


bool DiscriminatorRule::mangledRepresentationStartsWith(char c)
{
    return c == '_';
}

/*
 *
 * <discriminator> := _ <non-negative number>      # when number < 10
 *                 := __ <non-negative number> _   # when number >= 10
 */
void DiscriminatorRule::parse(GlobalParseState *parseState)
{
    if (parseState->advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid discriminator"));
    bool ge10 = false;
    if (parseState->peek() == '_') {
        ge10 = true;
        parseState->advance();
    }
    const ParseTreeNode::Ptr parentNode = parseState->stackTop();
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD_TO_NODE(NonNegativeNumberNode<10>, parseState, parentNode);
    const NonNegativeNumberNode<10>::Ptr number
            = DEMANGLER_CAST(NonNegativeNumberNode<10>, CHILD_AT(parentNode, parentNode->childCount() - 1));
    if ((ge10 && number->number() < 10) || (!ge10 && number->number() >= 10))
        throw ParseException(QString::fromLatin1("Invalid discriminator"));
    if (ge10 && parseState->advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid discriminator"));
}


CtorDtorNameNode::CtorDtorNameNode(const CtorDtorNameNode &other)
        : ParseTreeNode(other),
          m_isDestructor(other.m_isDestructor),
          m_representation(other.m_representation)
{
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

    m_representation = parseState()->substitutionAt(parseState()->substitutionCount() - 1)->toByteArray();
}

QByteArray CtorDtorNameNode::description() const
{
    return "CtorDtor[isDestructor:" + bool2String(m_isDestructor)
            + ";repr=" + m_representation + ']';
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


CvQualifiersNode::CvQualifiersNode(GlobalParseState *parseState)
        : ParseTreeNode(parseState), m_hasConst(false), m_hasVolatile(false)
{
}

CvQualifiersNode::CvQualifiersNode(const CvQualifiersNode &other)
        : ParseTreeNode(other), m_hasConst(other.m_hasConst), m_hasVolatile(other.m_hasVolatile)
{
}

bool CvQualifiersNode::mangledRepresentationStartsWith(char c)
{
    return c == 'K' || c == 'V' || c == 'r';
}

/*  <CV-qualifiers> ::= [r] [V] [K] 	# restrict (C99), volatile, const */
void CvQualifiersNode::parse()
{
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
        parseState()->addSubstitution(parseState()->stackTop());
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

    const ParseTreeNode::Ptr firstChild = MY_CHILD_AT(0);
    const NameNode::Ptr nameNode = firstChild.dynamicCast<NameNode>();
    const CvQualifiersNode::Ptr cvQualifiersNode
            = nameNode ? nameNode->cvQualifiers() : CvQualifiersNode::Ptr();

    QByteArray repr;
    const BareFunctionTypeNode::Ptr funcNode = DEMANGLER_CAST(BareFunctionTypeNode, MY_CHILD_AT(1));
    if (funcNode->hasReturnType())
        repr = CHILD_AT(funcNode, 0)->toByteArray() + ' ';
    if (cvQualifiersNode && cvQualifiersNode->hasQualifiers()) {
        return repr + firstChild->toByteArray() + funcNode->toByteArray() + ' '
            + cvQualifiersNode->toByteArray();
    }
    return repr + firstChild->toByteArray() + funcNode->toByteArray();
}


ExpressionNode::ExpressionNode(GlobalParseState *parseState)
        : ParseTreeNode(parseState), m_type(OtherType), m_globalNamespace(false)
{
}

ExpressionNode::ExpressionNode(const ExpressionNode &other)
        : ParseTreeNode(other), m_type(other.m_type), m_globalNamespace(other.m_globalNamespace)
{
}

bool ExpressionNode::mangledRepresentationStartsWith(char c)
{
    return OperatorNameNode::mangledRepresentationStartsWith(c)
            || TemplateParamNode::mangledRepresentationStartsWith(c)
            || FunctionParamNode::mangledRepresentationStartsWith(c)
            || ExprPrimaryNode::mangledRepresentationStartsWith(c)
            || UnresolvedNameNode::mangledRepresentationStartsWith(c)
            || c == 'c' || c == 's' || c == 'a' || c == 'd' || c == 't';
}

/*
 * <expression> ::= <operator-name> <expression>
 *              ::= <operator-name> <expression> <expression>
 *              ::= <operator-name> <expression> <expression> <expression>
 *              ::= cl <expression>+ E                            # call
 *              ::= cv <type> expression                          # conversion with one argument
 *              ::= cv <type> _ <expression>* E # conversion with a different number of arguments
 *              ::= [gs] nw <expression>* _ <type> E              # new (expr-list) type
 *              ::= [gs] nw <expression>* _ <type> <initializer>  # new (expr-list) type (init)
 *              ::= [gs] na <expression>* _ <type> E              # new[] (expr-list) type
 *              ::= [gs] na <expression>* _ <type> <initializer>  # new[] (expr-list) type (init)
 *              ::= [gs] dl <expression>                          # delete expression
 *              ::= [gs] da <expression>                          # delete[] expression
 *              ::= pp_ <expression>                              # prefix ++
 *              ::= mm_ <expression>                              # prefix --
 *              ::= ti <type>                                     # typeid (type)
 *              ::= te <expression>                               # typeid (expression)
 *              ::= dc <type> <expression>                        # dynamic_cast<type> (expression)
 *              ::= sc <type> <expression>                        # static_cast<type> (expression)
 *              ::= cc <type> <expression>                        # const_cast<type> (expression)
 *              ::= rc <type> <expression>                        # reinterpret_cast<type> (expression)
 *              ::= st <type>                                     # sizeof (a type)
 *              ::= at <type>                                     # alignof (a type)
 *              ::= <template-param>
 *              ::= <function-param>
 *              ::= dt <expression> <unresolved-name>             # expr.name
 *              ::= pt <expression> <unresolved-name>             # expr->name
 *              ::= ds <expression> <expression>                  # expr.*expr
 *              ::= sZ <template-param>                           # size of a parameter pack
 *              ::= sp <expression>                               # pack expansion
 *              ::= sZ <function-param>                           # size of a function parameter pack
 *              ::= tw <expression>                               # throw expression
 *              ::= tr                                            # throw with no operand (rethrow)
 *              ::= <unresolved-name>                             # f(p), N::f(p), ::f(p),
 *                                                                # freestanding dependent name (e.g., T::x),
 *              ::= <expr-primary>
 */
void ExpressionNode::parse()
{
   /*
    * Some of the terminals in the productions of <expression>
    * also appear in the productions of <operator-name>. The direct
    * productions have higher precedence and are checked first to prevent
    * erroneous parsing by the operator rule.
    */
    QByteArray str = parseState()->readAhead(2);
    if (str == "cl") {
        parseState()->advance(2);
        do
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
        while (ExpressionNode::mangledRepresentationStartsWith(PEEK()));
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
    } else if (str == "nw" || str == "na"
               || parseState()->readAhead(4) == "gsnw" || parseState()->readAhead(4) == "gsna") {
        if (str == "gs") {
            m_globalNamespace = true;
            parseState()->advance(2);
        }
        m_type = parseState()->readAhead(2) == "nw" ? NewType : ArrayNewType;
        parseState()->advance(2);
        while (ExpressionNode::mangledRepresentationStartsWith(PEEK()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
        if (ADVANCE() != '_')
            throw ParseException(QLatin1String("Invalid expression"));
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
        if (PEEK() == 'E')
            ADVANCE();
        else
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(InitializerNode);
    } else if (str == "dl" || str == "da" || parseState()->readAhead(4) == "gsdl"
               || parseState()->readAhead(4) == "gsda") {
        if (str == "gs") {
            m_globalNamespace = true;
            parseState()->advance(2);
        }
        m_type = parseState()->readAhead(2) == "dl" ? DeleteType : ArrayDeleteType;
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
    } else if (parseState()->readAhead(3) == "pp_") {
        m_type = PrefixIncrementType;
        parseState()->advance(3);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
    } else if (parseState()->readAhead(3) == "mm_") {
        m_type = PrefixDecrementType;
        parseState()->advance(3);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
    } else if (str == "ti") {
        m_type = TypeIdTypeType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    } else if (str == "te") {
        m_type = TypeIdExpressionType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
    } else if (str == "dc" || str == "sc" || str == "cc" || str == "rc") {
        m_type = str == "dc" ? DynamicCastType : str == "sc" ? StaticCastType : str == "cc"
                ? ConstCastType : ReinterpretCastType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
    } else if (str == "st") {
        m_type = SizeofType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    } else if (str == "at") {
        m_type = AlignofType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    } else if (str == "sZ") {
        m_type = ParameterPackSizeType;
        parseState()->advance(2);
        if (TemplateParamNode::mangledRepresentationStartsWith(PEEK()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateParamNode);
        else
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(FunctionParamNode);
    } else if (str == "dt") {
        m_type = MemberAccessType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnresolvedNameNode);
    } else if (str == "pt") {
        m_type = PointerMemberAccessType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnresolvedNameNode);
    } else if (str == "ds") {
        m_type = MemberDerefType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
    } else if (str == "ps") {
        m_type = PackExpansionType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
    } else if (str == "tw") {
        m_type = ThrowType;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
    } else if (str == "tr") {
        m_type = RethrowType;
    } else {
        const char next = PEEK();
        if (OperatorNameNode::mangledRepresentationStartsWith(next) && str != "dn" && str != "on"
                && str != "gs" && str != "sr") {
            m_type = OperatorType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(OperatorNameNode);
            const OperatorNameNode::Ptr opNode
                    = DEMANGLER_CAST(OperatorNameNode, MY_CHILD_AT(childCount() - 1));

            int expressionCount;
            switch (opNode->type()) {
            case OperatorNameNode::TernaryType:
                expressionCount = 3;
                break;
            case OperatorNameNode::ArrayNewType: case OperatorNameNode::BinaryPlusType:
            case OperatorNameNode::BinaryMinusType: case OperatorNameNode::MultType:
            case OperatorNameNode::DivType: case OperatorNameNode::ModuloType:
            case OperatorNameNode::BitwiseAndType: case OperatorNameNode::BitwiseOrType:
            case OperatorNameNode::XorType: case OperatorNameNode::AssignType:
            case OperatorNameNode::IncrementAndAssignType:
            case OperatorNameNode::DecrementAndAssignType:
            case OperatorNameNode::MultAndAssignType: case OperatorNameNode::DivAndAssignType:
            case OperatorNameNode::ModuloAndAssignType:
            case OperatorNameNode::BitwiseAndAndAssignType:
            case OperatorNameNode::BitwiseOrAndAssignType:
            case OperatorNameNode::XorAndAssignType: case OperatorNameNode::LeftShiftType:
            case OperatorNameNode::RightShiftType: case OperatorNameNode::LeftShiftAndAssignType:
            case OperatorNameNode::RightShiftAndAssignType: case OperatorNameNode::EqualsType:
            case OperatorNameNode::NotEqualsType: case OperatorNameNode::LessType:
            case OperatorNameNode::GreaterType: case OperatorNameNode::LessEqualType:
            case OperatorNameNode::GreaterEqualType: case OperatorNameNode::LogicalAndType:
            case OperatorNameNode::LogicalOrType: case OperatorNameNode::CommaType:
            case OperatorNameNode::ArrowStarType: case OperatorNameNode::ArrowType:
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
        } else if (FunctionParamNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(FunctionParamNode);
        } else if (ExprPrimaryNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExprPrimaryNode);
        } else if (UnresolvedNameNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnresolvedNameNode);
        } else {
            throw ParseException(QString::fromLatin1("Invalid expression"));
        }
    }
}

QByteArray ExpressionNode::description() const
{
    return "Expression[global:" + bool2String(m_globalNamespace)
            + ";type:" + QByteArray::number(m_type) + ']';
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
    case NewType: case ArrayNewType: {
        if (m_globalNamespace)
            repr += "::";
        repr += "new";
        if (m_type == ArrayNewType)
            repr += "[]";
        repr += ' ';

        // TODO: I don't understand what the first expression list means. Putting it into
        // parentheses for now.
        QByteArray exprList;
        int i = 0;
        for (; i < childCount(); ++i) {
            if (!MY_CHILD_AT(i).dynamicCast<ExpressionNode>())
                break;
            if (i > 0)
                repr += ", ";
            repr += CHILD_TO_BYTEARRAY(i);
        }
        if (i > 0)
            repr.append('(').append(exprList).append(')');

        repr += CHILD_TO_BYTEARRAY(i++); // <type>
        if (i < childCount())
            repr += CHILD_TO_BYTEARRAY(i); // <initializer>
        break;
    }
    case DeleteType: case ArrayDeleteType:
        if (m_globalNamespace)
            repr += "::";
        repr += "delete";
        if (m_type == ArrayDeleteType)
            repr += "[]";
        repr.append(' ').append(CHILD_TO_BYTEARRAY(0));
        break;
    case PrefixIncrementType:
        repr.append("++").append(CHILD_TO_BYTEARRAY(0));
        break;
    case PrefixDecrementType:
        repr.append("--").append(CHILD_TO_BYTEARRAY(0));
        break;
    case TypeIdTypeType: case TypeIdExpressionType:
        repr.append("typeid(").append(CHILD_TO_BYTEARRAY(0)).append(')');
        break;
    case DynamicCastType: case StaticCastType: case ConstCastType: case ReinterpretCastType:
        if (m_type == DynamicCastType)
            repr += "dynamic";
        else if (m_type == StaticCastType)
            repr += "static";
        else if (m_type == ConstCastType)
            repr += "const";
        else
            repr += "reinterpret";
        repr.append("_cast<").append(CHILD_TO_BYTEARRAY(0)).append(">(")
                .append(CHILD_TO_BYTEARRAY(1)).append(')');
        break;
    case SizeofType:
        repr = "sizeof(" + CHILD_TO_BYTEARRAY(0) + ')';
        break;
    case AlignofType:
        repr = "alignof(" + CHILD_TO_BYTEARRAY(0) + ')';
        break;
    case MemberAccessType:
        repr.append(CHILD_TO_BYTEARRAY(0)).append('.').append(CHILD_TO_BYTEARRAY(1));
        break;
    case PointerMemberAccessType:
        repr.append(CHILD_TO_BYTEARRAY(0)).append("->").append(CHILD_TO_BYTEARRAY(1));
        break;
    case MemberDerefType:
        repr.append(CHILD_TO_BYTEARRAY(0)).append(".*").append(CHILD_TO_BYTEARRAY(1));
        break;
    case ParameterPackSizeType:
        repr = "sizeof...(" + CHILD_TO_BYTEARRAY(0) + ')';
        break;
    case PackExpansionType:
        repr = CHILD_TO_BYTEARRAY(0) + "...";
        break;
    case ThrowType:
        repr.append("throw ").append(CHILD_TO_BYTEARRAY(0));
        break;
    case RethrowType:
        repr.append("throw");
        break;
    case OperatorType: {
        const OperatorNameNode::Ptr opNode = DEMANGLER_CAST(OperatorNameNode, MY_CHILD_AT(0));
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


OperatorNameNode::OperatorNameNode(const OperatorNameNode &other)
        : ParseTreeNode(other), m_type(other.m_type)
{
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

QByteArray OperatorNameNode::description() const
{
    return "OperatorName[type:" + toByteArray() + ']';
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


ExprPrimaryNode::ExprPrimaryNode(GlobalParseState *parseState)
        : ParseTreeNode(parseState), m_isNullPtr(false)
{
}

ExprPrimaryNode::ExprPrimaryNode(const ExprPrimaryNode &other)
        : ParseTreeNode(other), m_suffix(other.m_suffix), m_isNullPtr(other.m_isNullPtr)
{
}

bool ExprPrimaryNode::mangledRepresentationStartsWith(char c)
{
    return c == 'L';
}

/*
 * <expr-primary> ::= L <type> <number> E            # integer literal
 *                ::= L <type> <float> E             # floating literal
 *                ::= L <string type>                # string literal
 *                ::= L <nullptr type> E             # nullptr literal (i.e., "LDnE")
 *                ::= L <type> <real-part float> _ <imag-part float> E   # complex floating point literal (C 2000)
 *                ::= L <mangled-name> E             # external name
 * Note that we ignore C 2000 features.
 */
void ExprPrimaryNode::parse()
{
    if (!ExprPrimaryNode::mangledRepresentationStartsWith(ADVANCE()))
        throw ParseException(QString::fromLatin1("Invalid primary expression"));
    bool needsSuffix = true;
    const char next = PEEK();
    if (TypeNode::mangledRepresentationStartsWith(next)) {
        const ParseTreeNode::Ptr topLevelTypeNode = parseRule<TypeNode>(parseState());
        const BuiltinTypeNode::Ptr typeNode = topLevelTypeNode->childCount() == 0
                ? BuiltinTypeNode::Ptr()
                : CHILD_AT(topLevelTypeNode, 0).dynamicCast<BuiltinTypeNode>();
        if (!typeNode)
            throw ParseException(QLatin1String("Invalid type in expr-primary"));

        switch (typeNode->type()) {
        case BuiltinTypeNode::UnsignedShortType:
        case BuiltinTypeNode::UnsignedIntType:
            m_suffix = "U"; // Fall-through.
        case BuiltinTypeNode::SignedShortType:
        case BuiltinTypeNode::SignedIntType:
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NumberNode);
            break;
        case BuiltinTypeNode::UnsignedLongType:
            m_suffix = "U"; // Fall-through.
        case BuiltinTypeNode::SignedLongType:
            m_suffix = "L";
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NumberNode);
            break;
        case BuiltinTypeNode::UnsignedLongLongType:
            m_suffix = "U"; // Fall-through.
        case BuiltinTypeNode::SignedLongLongType:
            m_suffix = "LL";
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NumberNode);
            break;
        case BuiltinTypeNode::FloatType:
            m_suffix = "f"; // Fall-through.
        case BuiltinTypeNode::DoubleType:
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(FloatValueNode);
            break;
        case BuiltinTypeNode::NullPtrType:
            m_isNullPtr = true;
            break;
        case BuiltinTypeNode::PlainCharType: case BuiltinTypeNode::WCharType:
        case BuiltinTypeNode::Char16Type: case BuiltinTypeNode::Char32Type:
            needsSuffix = false;
            break; // string type
        default:
            throw ParseException(QString::fromLatin1("Invalid type in expr-primary"));
        }
        parseState()->popFromStack(); // No need to keep the type node in the tree.
    } else if (MangledNameRule::mangledRepresentationStartsWith(next)) {
        MangledNameRule::parse(parseState(), parseState()->stackTop());
    } else {
        throw ParseException(QString::fromLatin1("Invalid expr-primary"));
    }
    if (needsSuffix && ADVANCE() != 'E')
        throw ParseException(QString::fromLatin1("Invalid expr-primary"));
}

QByteArray ExprPrimaryNode::description() const
{
    return "ExprPrimary[m_suffix:" + m_suffix + ";m_isNullPtr:" + bool2String(m_isNullPtr) + ']';
}

QByteArray ExprPrimaryNode::toByteArray() const
{
    if (m_isNullPtr)
        return "nullptr";
    return CHILD_TO_BYTEARRAY(0) + m_suffix;
}


FunctionTypeNode::FunctionTypeNode(GlobalParseState *parseState)
        : ParseTreeNode(parseState), m_isExternC(false)
{
}

FunctionTypeNode::FunctionTypeNode(const FunctionTypeNode &other)
        : ParseTreeNode(other), m_isExternC(other.isExternC())
{
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
    }

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(BareFunctionTypeNode);
    if (ADVANCE() != 'E')
        throw ParseException(QString::fromLatin1("Invalid function type"));
}

QByteArray FunctionTypeNode::description() const
{
    return "FunctionType[isExternC:" + bool2String(m_isExternC) + ']';
}

QByteArray FunctionTypeNode::toByteArray() const
{
    return QByteArray(); // Not enough knowledge here to generate a string representation.
}


LocalNameNode::LocalNameNode(GlobalParseState *parseState)
        : ParseTreeNode(parseState), m_isStringLiteral(false), m_isDefaultArg(false)
{
}

LocalNameNode::LocalNameNode(const LocalNameNode &other)
        : ParseTreeNode(other),
          m_isStringLiteral(other.m_isStringLiteral),
          m_isDefaultArg(other.m_isDefaultArg)
{
}

bool LocalNameNode::mangledRepresentationStartsWith(char c)
{
    return c == 'Z';
}

/*
 * <local-name> := Z <encoding> E <name> [<discriminator>]
 *              := Z <encoding> E s [<discriminator>]
 *              := Z <encoding> Ed [ <non-negative number> ] _ <name>
 *
 * Note that <name> can start with 's', so we need to do read-ahead.
 * Also, <name> can start with 'd' (via <operator-name>).
 * The last rule is for member functions with default closure type arguments.
 */
void LocalNameNode::parse()
{
    if (!mangledRepresentationStartsWith(ADVANCE()))
        throw ParseException(QString::fromLatin1("Invalid local-name"));

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(EncodingNode);

    if (ADVANCE() != 'E')
        throw ParseException(QString::fromLatin1("Invalid local-name"));

    QByteArray str = parseState()->readAhead(2);
    const char next = PEEK();
    if (next == 'd' && str != "dl" && str != "da" && str != "de" && str != "dv" && str != "dV") {
        m_isDefaultArg = true;
        ADVANCE();
        if (NonNegativeNumberNode<10>::mangledRepresentationStartsWith(PEEK()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumberNode<10>);
        if (ADVANCE() != '_')
            throw ParseException(QString::fromLatin1("Invalid local-name"));
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NameNode);
    } else if (str == "sp" || str == "sr" || str == "st" || str == "sz" || str == "sZ"
            || (next != 's' && NameNode::mangledRepresentationStartsWith(next))) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NameNode);
    } else if (next == 's') {
        m_isStringLiteral = true;
        ADVANCE();
    } else {
        throw ParseException(QString::fromLatin1("Invalid local-name"));
    }
    if (DiscriminatorRule::mangledRepresentationStartsWith(PEEK()))
        DiscriminatorRule::parse(parseState());
}

QByteArray LocalNameNode::description() const
{
    return "LocalName[isStringLiteral:" + bool2String(m_isStringLiteral) + ";isDefaultArg:"
            + bool2String(m_isDefaultArg) + ']';
}

QByteArray LocalNameNode::toByteArray() const
{
    QByteArray name;
    bool hasDiscriminator;
    if (m_isDefaultArg) {
        const ParseTreeNode::Ptr encodingNode = MY_CHILD_AT(0);
        const BareFunctionTypeNode::Ptr funcNode
                = DEMANGLER_CAST(BareFunctionTypeNode, CHILD_AT(encodingNode, 1));
        const int functionParamCount
                = funcNode->hasReturnType() ? funcNode->childCount() - 1 : funcNode->childCount();
        const NonNegativeNumberNode<10>::Ptr numberNode
                = MY_CHILD_AT(1).dynamicCast<NonNegativeNumberNode<10> >();

        // "_" means last argument, "n" means (n+1)th to last.
        // Note that c++filt in binutils 2.22 does this wrong.
        const int argNumber = functionParamCount - (numberNode ? numberNode->number() + 1 : 0);

        name = encodingNode->toByteArray();
        name.append("::{default arg#").append(QByteArray::number(argNumber)).append("}::")
                .append(MY_CHILD_AT(childCount() - 1)->toByteArray());
        hasDiscriminator = false;
    } else if (m_isStringLiteral) {
        name = CHILD_TO_BYTEARRAY(0) + "::{string literal}";
        hasDiscriminator = childCount() == 2;
    } else {
        name = CHILD_TO_BYTEARRAY(0) + "::" + CHILD_TO_BYTEARRAY(1);
        hasDiscriminator = childCount() == 3;
    }
    if (hasDiscriminator) {
        // TODO: Does this information serve any purpose? Names seem to demangle fine without printing anything here.
//        const QByteArray discriminator = MY_CHILD_AT(childCount() - 1)->toByteArray();
//        const int rawDiscriminatorValue = discriminator.toInt();
//        name += " (occurrence number " + QByteArray::number(rawDiscriminatorValue - 2) + ')';
    }
    return name;
}

bool LocalNameNode::isTemplate() const
{
    if (childCount() == 1 || MY_CHILD_AT(1).dynamicCast<NonNegativeNumberNode<10> >())
        return false;
    return DEMANGLER_CAST(NameNode, MY_CHILD_AT(1))->isTemplate();
}

bool LocalNameNode::isConstructorOrDestructorOrConversionOperator() const
{
    if (childCount() == 1 || MY_CHILD_AT(1).dynamicCast<NonNegativeNumberNode<10> >())
        return false;
    return DEMANGLER_CAST(NameNode, MY_CHILD_AT(1))->isConstructorOrDestructorOrConversionOperator();
}

CvQualifiersNode::Ptr LocalNameNode::cvQualifiers() const
{
    if (m_isDefaultArg)
        return DEMANGLER_CAST(NameNode, MY_CHILD_AT(childCount() - 1))->cvQualifiers();
    if (childCount() == 1 || MY_CHILD_AT(1).dynamicCast<NonNegativeNumberNode<10> >())
        return CvQualifiersNode::Ptr();
    return DEMANGLER_CAST(NameNode, MY_CHILD_AT(1))->cvQualifiers();
}


bool MangledNameRule::mangledRepresentationStartsWith(char c)
{
    return c == '_';
}

/*
 * Grammar: http://www.codesourcery.com/public/cxx-abi/abi.html#mangling
 * The grammar as given there is not LL(k), so a number of transformations
 * were necessary, which we will document at the respective parsing function.
 * <mangled-name> ::= _Z <encoding>
 */
void MangledNameRule::parse(GlobalParseState *parseState, const ParseTreeNode::Ptr &parentNode)
{
    parseState->advance(2);
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD_TO_NODE(EncodingNode, parseState, parentNode);
}


SourceNameNode::SourceNameNode(const SourceNameNode &other)
        : ParseTreeNode(other), m_name(other.m_name)
{
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

QByteArray SourceNameNode::description() const
{
    return "SourceName[name:" + m_name + ']';
}


bool UnqualifiedNameNode::mangledRepresentationStartsWith(char c)
{
    return OperatorNameNode::mangledRepresentationStartsWith(c)
            || CtorDtorNameNode::mangledRepresentationStartsWith(c)
            || SourceNameNode::mangledRepresentationStartsWith(c)
            || UnnamedTypeNameNode::mangledRepresentationStartsWith(c);
}

QByteArray UnqualifiedNameNode::toByteArray() const
{
    QByteArray repr;
    if (MY_CHILD_AT(0).dynamicCast<OperatorNameNode>())
        repr = "operator";
    return repr += CHILD_TO_BYTEARRAY(0);
}

bool UnqualifiedNameNode::isConstructorOrDestructorOrConversionOperator() const
{
    if (MY_CHILD_AT(0).dynamicCast<CtorDtorNameNode>())
        return true;
    const OperatorNameNode::Ptr opNode = MY_CHILD_AT(0).dynamicCast<OperatorNameNode>();
    return opNode && opNode->type() == OperatorNameNode::CastType;
}

/*
 * <unqualified-name> ::= <operator-name>
 *                    ::= <ctor-dtor-name>
 *                    ::= <source-name>
 *                    ::= <unnamed-type-name>
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
    else if (UnnamedTypeNameNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnnamedTypeNameNode);
    else
        throw ParseException(QString::fromLatin1("Invalid unqualified-name"));
}


UnscopedNameNode::UnscopedNameNode(GlobalParseState *parseState)
        : ParseTreeNode(parseState), m_inStdNamespace(false)
{
}

UnscopedNameNode::UnscopedNameNode(const UnscopedNameNode &other)
        : ParseTreeNode(other), m_inStdNamespace(other.m_inStdNamespace)
{
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
    const UnqualifiedNameNode::Ptr childNode = DEMANGLER_CAST(UnqualifiedNameNode, MY_CHILD_AT(0));
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
    }

    if (!UnqualifiedNameNode::mangledRepresentationStartsWith(PEEK()))
        throw ParseException(QString::fromLatin1("Invalid unscoped-name"));

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnqualifiedNameNode);
}

QByteArray UnscopedNameNode::description() const
{
    return "UnscopedName[isInStdNamespace:" + bool2String(m_inStdNamespace) + ']';
}


bool NestedNameNode::mangledRepresentationStartsWith(char c)
{
    return c == 'N';
}

QByteArray NestedNameNode::toByteArray() const
{
    // cv-qualifiers are not encoded here, since they only make sense at a higher level.
    if (MY_CHILD_AT(0).dynamicCast<CvQualifiersNode>())
        return CHILD_TO_BYTEARRAY(1);
    return CHILD_TO_BYTEARRAY(0);
}

bool NestedNameNode::isTemplate() const
{
    const PrefixNode::Ptr childNode = DEMANGLER_CAST(PrefixNode, MY_CHILD_AT(childCount() - 1));
    return childNode->isTemplate();
}

bool NestedNameNode::isConstructorOrDestructorOrConversionOperator() const
{
    const PrefixNode::Ptr childNode = DEMANGLER_CAST(PrefixNode, MY_CHILD_AT(childCount() - 1));
    return childNode->isConstructorOrDestructorOrConversionOperator();
}

CvQualifiersNode::Ptr NestedNameNode::cvQualifiers() const
{
    return MY_CHILD_AT(0).dynamicCast<CvQualifiersNode>();
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


SubstitutionNode::SubstitutionNode(const SubstitutionNode &other)
        : ParseTreeNode(other), m_type(other.type())
{
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
 *                ::= St <unqualified-name> # ::std::
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
        addChild(parseState()->substitutionAt(substIndex));
        if (ADVANCE() != '_')
            throw ParseException(QString::fromLatin1("Invalid substitution"));
    } else {
        switch (ADVANCE()) {
        case '_':
            if (parseState()->substitutionCount() == 0)
                throw ParseException(QString::fromLatin1("Invalid substitution: "
                    "There are no substitutions"));
            m_type = ActualSubstitutionType;
            addChild(parseState()->substitutionAt(0));
            break;
        case 't':
            m_type = StdType;
            if (UnqualifiedNameNode::mangledRepresentationStartsWith(PEEK())) {
                PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnqualifiedNameNode);
                parseState()->addSubstitution(parseState()->stackTop());
            }
            break;
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

QByteArray SubstitutionNode::description() const
{
    return "Substitution[type:" + QByteArray::number(m_type) + ']';
}

QByteArray SubstitutionNode::toByteArray() const
{
    switch (m_type) {
    case ActualSubstitutionType: return CHILD_TO_BYTEARRAY(0);
    case StdType: {
        QByteArray repr = "std";
        if (childCount() > 0)
            repr.append("::").append(CHILD_TO_BYTEARRAY(0));
        return repr;
    }
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
    // Gather all qualifiers first, because we have to move them to the end en bloc.
    QByteArray qualRepr;
    TypeNode::Ptr memberTypeNode = DEMANGLER_CAST(TypeNode, MY_CHILD_AT(1));
    while (memberTypeNode->type() == TypeNode::QualifiedType) {
        const CvQualifiersNode::Ptr cvNode
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
    const FunctionTypeNode::Ptr functionNode
            = CHILD_AT(memberTypeNode, 0).dynamicCast<FunctionTypeNode>();
    if (functionNode) {
        const BareFunctionTypeNode::Ptr bareFunctionNode
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


TemplateParamNode::TemplateParamNode(const TemplateParamNode &other)
        : ParseTreeNode(other), m_index(other.index())
{
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
        bool isConversionOperator = false;
        for (int i = parseState()->stackElementCount() - 1; i >= 0; --i) {
            const OperatorNameNode::Ptr opNode
                    = parseState()->stackElementAt(i).dynamicCast<OperatorNameNode>();
            if (opNode && opNode->type() == OperatorNameNode::CastType) {
                isConversionOperator = true;
                break;
            }
        }
        if (!isConversionOperator) {
            throw ParseException(QString::fromLocal8Bit("Invalid template parameter index %1")
                    .arg(m_index));
        }
    } else {
        addChild(parseState()->templateParamAt(m_index));
    }
}

QByteArray TemplateParamNode::description() const
{
    return "TemplateParam[index:" + QByteArray::number(m_index) + ']';
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


SpecialNameNode::SpecialNameNode(const SpecialNameNode &other)
        : ParseTreeNode(other), m_type(other.m_type)
{
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
        CallOffsetRule::parse(parseState());
        CallOffsetRule::parse(parseState());
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(EncodingNode);
    } else if (ADVANCE() == 'T') {
        m_type = SingleCallOffsetType;
        CallOffsetRule::parse(parseState());
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(EncodingNode);
    } else {
        throw ParseException(QString::fromLatin1("Invalid special-name"));
    }
}

QByteArray SpecialNameNode::description() const
{
    return "SpecialName[type:" + QByteArray::number(m_type) + ']';
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


NumberNode::NumberNode(GlobalParseState *parseState)
        : ParseTreeNode(parseState), m_isNegative(false)
{
}

NumberNode::NumberNode(const NumberNode &other)
        : ParseTreeNode(other), m_isNegative(other.m_isNegative)
{
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
        throw ParseException(QLatin1String("Invalid number"));

    if (next == 'n') {
        m_isNegative = true;
        ADVANCE();
    }

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumberNode<10>);
}

QByteArray NumberNode::description() const
{
    return "Number[isNegative:" + bool2String(m_isNegative) + ']';
}

QByteArray NumberNode::toByteArray() const
{
    QByteArray repr = CHILD_TO_BYTEARRAY(0);
    if (m_isNegative)
        repr.prepend('-');
    return repr;
}


template<int base> NonNegativeNumberNode<base>::NonNegativeNumberNode(const NonNegativeNumberNode<base> &other)
        : ParseTreeNode(other), m_number(other.m_number)
{
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

template<int base> QByteArray NonNegativeNumberNode<base>::description() const
{
    return "NonNegativeNumber[base:" + QByteArray::number(base) + ";number:"
            + QByteArray::number(m_number) + ']';
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
    if (childCount() > 1 && MY_CHILD_AT(1).dynamicCast<TemplateArgsNode>())
        return true;
    const NestedNameNode::Ptr nestedNameNode = MY_CHILD_AT(0).dynamicCast<NestedNameNode>();
    if (nestedNameNode)
        return nestedNameNode->isTemplate();
    const LocalNameNode::Ptr localNameNode = MY_CHILD_AT(0).dynamicCast<LocalNameNode>();
    if (localNameNode)
        return localNameNode->isTemplate();

    return false;
}

bool NameNode::isConstructorOrDestructorOrConversionOperator() const
{
    NestedNameNode::Ptr nestedNameNode = MY_CHILD_AT(0).dynamicCast<NestedNameNode>();
    if (nestedNameNode)
        return nestedNameNode->isConstructorOrDestructorOrConversionOperator();
    const LocalNameNode::Ptr localNameNode = MY_CHILD_AT(0).dynamicCast<LocalNameNode>();
    if (localNameNode)
        return localNameNode->isConstructorOrDestructorOrConversionOperator();

    return false;
}

CvQualifiersNode::Ptr NameNode::cvQualifiers() const
{
    const NestedNameNode::Ptr nestedNameNode = MY_CHILD_AT(0).dynamicCast<NestedNameNode>();
    if (nestedNameNode)
        return nestedNameNode->cvQualifiers();
    const LocalNameNode::Ptr localNameNode = MY_CHILD_AT(0).dynamicCast<LocalNameNode>();
    if (localNameNode)
        return localNameNode->cvQualifiers();
    return CvQualifiersNode::Ptr();
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
            parseState()->addSubstitution(parseState()->stackTop());
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


TemplateArgNode::TemplateArgNode(GlobalParseState *parseState)
        : ParseTreeNode(parseState), m_isTemplateArgumentPack(false)
{
}

TemplateArgNode::TemplateArgNode(const TemplateArgNode &other)
        : ParseTreeNode(other), m_isTemplateArgumentPack(other.m_isTemplateArgumentPack)
{
}

bool TemplateArgNode::mangledRepresentationStartsWith(char c)
{
    return TypeNode::mangledRepresentationStartsWith(c)
            || ExprPrimaryNode::mangledRepresentationStartsWith(c)
            || c == 'X' || c == 'J';
}

/*
 * <template-arg> ::= <type>                    # type or template
 *                ::= X <expression> E          # expression
 *                ::= <expr-primary>            # simple expressions
 *                ::= J <template-arg>* E       # argument pack
 */
void TemplateArgNode::parse()
{
    const char next = PEEK();
    if (TypeNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    } else if (ExprPrimaryNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExprPrimaryNode);
    } else if (next == 'X') {
        ADVANCE();
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
        if (ADVANCE() != 'E')
            throw ParseException(QString::fromLatin1("Invalid template-arg"));
    } else if (next == 'J') {
        m_isTemplateArgumentPack = true;
        ADVANCE();
        while (TemplateArgNode::mangledRepresentationStartsWith(PEEK()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgNode);
        if (ADVANCE() != 'E')
            throw ParseException(QString::fromLatin1("Invalid template-arg"));
    } else {
        throw ParseException(QString::fromLatin1("Invalid template-arg"));
    }

    parseState()->addTemplateParam(parseState()->stackTop());
}

QByteArray TemplateArgNode::description() const
{
    return "TemplateArg[isPack:" + bool2String(m_isTemplateArgumentPack) + ']';
}

QByteArray TemplateArgNode::toByteArray() const
{
    if (m_isTemplateArgumentPack) {
        QByteArray repr;
        for (int i = 0; i < childCount(); ++i)
            repr.append(CHILD_TO_BYTEARRAY(i)).append(", ");
        return repr += "typename...";
    }
    return CHILD_TO_BYTEARRAY(0);
}


bool PrefixNode::mangledRepresentationStartsWith(char c)
{
    return TemplateParamNode::mangledRepresentationStartsWith(c)
            || SubstitutionNode::mangledRepresentationStartsWith(c)
            || UnqualifiedNameNode::mangledRepresentationStartsWith(c)
            || SourceNameNode::mangledRepresentationStartsWith(c)
            || DeclTypeNode::mangledRepresentationStartsWith(c);
}

bool PrefixNode::isTemplate() const
{
    return childCount() > 0 && MY_CHILD_AT(childCount() - 1).dynamicCast<TemplateArgsNode>();
}

bool PrefixNode::isConstructorOrDestructorOrConversionOperator() const
{
    for (int i = childCount() - 1; i >= 0; --i) {
        const UnqualifiedNameNode::Ptr n = MY_CHILD_AT(i).dynamicCast<UnqualifiedNameNode>();
        if (n)
            return n->isConstructorOrDestructorOrConversionOperator();
    }
    return false;
}

/*
 * <prefix> ::= <prefix> <unqualified-name>
 *          ::= <template-prefix> <template-args>
 *          ::= <template-param>
 *          ::= <decltype>
 *          ::= # empty
 *          ::= <substitution>
 *          ::= <prefix> <data-member-prefix>
 * <template-prefix> ::= <prefix> <unqualified-name>
 *                   ::= <template-param>
 *                   ::= <substitution>
 * <data-member-prefix> := <source-name> M
 *
 * We have to eliminate the left-recursion and the template-prefix rule
 * and end up with this:
 * <prefix> ::= <template-param> [<template-args>] <prefix-2>
 *          ::= <substitution> [<template-args>] <prefix-2>
 *          ::= <decltype> [<template-args>] <prefix-2>
 *          ::= <prefix-2>
 * <prefix-2> ::= <unqualified-name> [<template-args>] <prefix-2>
 *            ::= <data-member-prefix> <prefix2>
 *            ::=  # empty
 * <data-member-prefix> has overlap in its rhs with <unqualified-name>, so we cannot make it
 * a node of its own. Instead, we just check whether a source name is followed by 'M' and
 * remember that.
 */
void PrefixNode::parse()
{
    const char next = PEEK();
    bool canAddSubstitution = false;
    if (TemplateParamNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateParamNode);
        if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK())) {
            parseState()->addSubstitution(parseState()->stackTop());
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
        }
        canAddSubstitution = true;
    } else if (SubstitutionNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SubstitutionNode);
        if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK())) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
            canAddSubstitution = true;
        }
    } else if (DeclTypeNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SubstitutionNode);
        if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK())) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
            canAddSubstitution = true;
        }
    }

    while (UnqualifiedNameNode::mangledRepresentationStartsWith(PEEK())) {
        if (canAddSubstitution)
            parseState()->addSubstitution(parseState()->stackTop());
        else
            canAddSubstitution = true;
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnqualifiedNameNode);
        const bool isDataMember = CHILD_AT(MY_CHILD_AT(childCount() - 1), 0)
                .dynamicCast<SourceNameNode>() && PEEK() == 'M';
        if (isDataMember) {
            // TODO: Being a data member is apparently relevant for initializers, but what does
            // this mean for the demangled string?
            ADVANCE();
            continue;
        }
        if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK())) {
            parseState()->addSubstitution(parseState()->stackTop());
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
        }
    }
}

QByteArray PrefixNode::toByteArray() const
{
    if (childCount() == 0)
        return QByteArray();
    QByteArray repr = CHILD_TO_BYTEARRAY(0);
    for (int i = 1; i < childCount(); ++i) {
        if (!MY_CHILD_AT(i).dynamicCast<TemplateArgsNode>())
            repr += "::";
        repr += CHILD_TO_BYTEARRAY(i);
    }
    return repr;
}


TypeNode::TypeNode(const TypeNode &other)
        : ParseTreeNode(other), m_type(other.type())
{
}

bool TypeNode::mangledRepresentationStartsWith(char c)
{
    return BuiltinTypeNode::mangledRepresentationStartsWith(c)
            || FunctionTypeNode::mangledRepresentationStartsWith(c)
            || ClassEnumTypeRule::mangledRepresentationStartsWith(c)
            || ArrayTypeNode::mangledRepresentationStartsWith(c)
            || PointerToMemberTypeNode::mangledRepresentationStartsWith(c)
            || TemplateParamNode::mangledRepresentationStartsWith(c)
            || SubstitutionNode::mangledRepresentationStartsWith(c)
            || CvQualifiersNode::mangledRepresentationStartsWith(c)
            || DeclTypeNode::mangledRepresentationStartsWith(c)
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
 *        ::= <decltype>
 *        ::= <substitution> # See Compression below
 *        ::= <CV-qualifiers> <type>
 *        ::= P <type>   # pointer-to
 *        ::= R <type>   # reference-to
 *        ::= O <type>   # rvalue reference-to (C++0x)
 *        ::= C <type>   # complex pair (C 2000)
 *        ::= G <type>   # imaginary (C 2000)
 *        ::= U <source-name> <type>     # vendor extended type qualifier
 *        ::= Dp <type>          # pack expansion of (C++0x)
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
    } else {
        const char next = PEEK();
        if (str == "Dt" || str == "DT") {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(DeclTypeNode);
        } else if (str == "Dd" || str == "De" || str == "Df" || str == "Dh" || str == "Di" || str == "Ds"
            || (next != 'D' && BuiltinTypeNode::mangledRepresentationStartsWith(next))) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(BuiltinTypeNode);
        } else if (FunctionTypeNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(FunctionTypeNode);
            parseState()->addSubstitution(parseState()->stackTop());
        } else if (ClassEnumTypeRule::mangledRepresentationStartsWith(next)) {
            ClassEnumTypeRule::parse(parseState());
            parseState()->addSubstitution(parseState()->stackTop());
        } else if (ArrayTypeNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ArrayTypeNode);
            parseState()->addSubstitution(parseState()->stackTop());
        } else if (PointerToMemberTypeNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(PointerToMemberTypeNode);
            parseState()->addSubstitution(parseState()->stackTop());
        } else if (TemplateParamNode::mangledRepresentationStartsWith(next)) {
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
            const TemplateParamNode::Ptr templateParamNode
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
                parseState()->addSubstitution(parseState()->stackTop());
        } else if (SubstitutionNode::mangledRepresentationStartsWith(next)) {
            m_type = SubstitutionType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SubstitutionNode);
            if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK())) {
                PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
                parseState()->addSubstitution(parseState()->stackTop());
            }
        } else if (CvQualifiersNode::mangledRepresentationStartsWith(next)) {
            m_type = QualifiedType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CvQualifiersNode);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
            const CvQualifiersNode::Ptr cvNode = DEMANGLER_CAST(CvQualifiersNode, MY_CHILD_AT(0));
            if (cvNode->hasQualifiers())
                parseState()->addSubstitution(parseState()->stackTop());
        } else if (next == 'P') {
            m_type = PointerType;
            ADVANCE();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
            parseState()->addSubstitution(parseState()->stackTop());
        } else if (next == 'R') {
            m_type = ReferenceType;
            ADVANCE();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
            parseState()->addSubstitution(parseState()->stackTop());
        } else if (next == 'O') {
            m_type = RValueType;
            ADVANCE();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
            parseState()->addSubstitution(parseState()->stackTop());
        } else if (next == 'C') {
            ADVANCE();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
            parseState()->addSubstitution(parseState()->stackTop());
        } else if (next == 'G') {
            ADVANCE();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
            parseState()->addSubstitution(parseState()->stackTop());
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

QByteArray TypeNode::description() const
{
    return "Type[type:" + QByteArray::number(m_type) + ']';
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
    Type lastType = TypeNode::OtherType;
    while (!leafType) {
        Type currentType = currentNode->m_type;
        switch (currentType) {
        case QualifiedType: {
            const CvQualifiersNode::Ptr cvNode
                    = DEMANGLER_CAST(CvQualifiersNode, CHILD_AT(currentNode, 0));
            if (cvNode->hasQualifiers())
                qualPtrRefList << cvNode.data();
            currentNode = DEMANGLER_CAST(TypeNode, CHILD_AT(currentNode, 1)).data();
            break;
        }
        case PointerType: case ReferenceType: case RValueType:
            /*
             * The Standard says (8.3.2/6) that nested references collapse according
             * to the following rules:
             *     (1) Reference to reference -> reference
             *     (2) Reference to rvalue -> reference
             *     (3) Rvalue to reference -> reference
             *     (4) Rvalue to Rvalue -> Rvalue
             */
            if (currentType == ReferenceType
                    && (lastType == ReferenceType || lastType == RValueType)) { // (1) and (3)
                qualPtrRefList.removeLast();
                qualPtrRefList << currentNode;
            } else if (currentType == RValueType
                       && (lastType == ReferenceType || lastType == RValueType)) { // (2) and (4)
                // Ignore current element.
                currentType = lastType;
            } else {
                qualPtrRefList << currentNode;
            }
            currentNode = DEMANGLER_CAST(TypeNode, CHILD_AT(currentNode, 0)).data();
            break;
        default: {
            ParseTreeNode::Ptr childNode = CHILD_AT(currentNode, 0);
            while (true) {
                if (childCount() != 1)
                    break;
                SubstitutionNode::Ptr substNode = childNode.dynamicCast<SubstitutionNode>();
                if (substNode && substNode->type() == SubstitutionNode::ActualSubstitutionType) {
                    childNode = CHILD_AT(childNode, 0);
                } else if (childNode.dynamicCast<TemplateParamNode>()) {
                    childNode = CHILD_AT(childNode, 0);
                    if (childNode.dynamicCast<TemplateArgNode>())
                        childNode = CHILD_AT(childNode, 0);
                } else {
                    break;
                }
            }
            if (childNode != CHILD_AT(currentNode, 0)) {
                const TypeNode::Ptr nextCurrent = childNode.dynamicCast<TypeNode>();
                if (nextCurrent) {
                    currentNode = nextCurrent.data();
                    continue;
                }
            }

            leafType = true;
            break;
        }
        }
        lastType = currentType;
    }

    if (qualPtrRefList.isEmpty()) {
        switch (currentNode->m_type) {
        case PackExpansionType: return CHILD_TO_BYTEARRAY(0) + "...";
        case VendorType: return pasteAllChildren();
        case OtherType: return pasteAllChildren();

        // Can happen if qualifier node does not actually have qualifiers, e.g. in <function-param>.
        default: return pasteAllChildren();
        }
    }

    return toByteArrayQualPointerRef(currentNode, qualPtrRefListToByteArray(qualPtrRefList));
}

QByteArray TypeNode::toByteArrayQualPointerRef(const TypeNode *typeNode,
        const QByteArray &qualPtrRef) const
{
    const FunctionTypeNode::Ptr functionNode
            = CHILD_AT(typeNode, 0).dynamicCast<FunctionTypeNode>();
    if (functionNode) {
        const BareFunctionTypeNode::Ptr bareFunctionNode
                = DEMANGLER_CAST(BareFunctionTypeNode, CHILD_AT(functionNode, 0));
        QByteArray repr;
        if (functionNode->isExternC())
            repr += "extern \"C\" ";
        if (bareFunctionNode->hasReturnType())
            repr += CHILD_AT(bareFunctionNode, 0)->toByteArray() + ' ';
        return repr += '(' + qualPtrRef + ')' + bareFunctionNode->toByteArray();
    }

    const ArrayTypeNode::Ptr arrayNode = CHILD_AT(typeNode, 0).dynamicCast<ArrayTypeNode>();
    if (arrayNode) {
        return CHILD_AT(arrayNode, 1)->toByteArray() + " (" + qualPtrRef + ")["
                + CHILD_AT(arrayNode, 0)->toByteArray() + ']';
    }

    if (CHILD_AT(typeNode, 0).dynamicCast<PointerToMemberTypeNode>())
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
                break;
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


FloatValueNode::FloatValueNode(const FloatValueNode &other)
        : ParseTreeNode(other), m_value(other.m_value)
{
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

QByteArray FloatValueNode::description() const
{
    return "FloatValue[value:" + QByteArray::number(m_value) + ']';
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


bool LambdaSigNode::mangledRepresentationStartsWith(char c)
{
    return TypeNode::mangledRepresentationStartsWith(c);
}

// <lambda-sig> ::= <type>+  # Parameter types or "v" if the lambda has no parameters
void LambdaSigNode::parse()
{
    do
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TypeNode);
    while (TypeNode::mangledRepresentationStartsWith(PEEK()));
}

QByteArray LambdaSigNode::toByteArray() const
{
    QByteArray repr = "lambda(";
    for (int i = 0; i < childCount(); ++i) {
        const QByteArray paramRepr = CHILD_TO_BYTEARRAY(i);
        if (paramRepr != "void")
            repr += paramRepr;
        if (i < childCount() - 1)
            repr += ", ";
    }
    return repr += ')';
}


// <closure-type-name> ::= Ul <lambda-sig> E [ <nonnegative number> ] _
void ClosureTypeNameNode::parse()
{
    if (parseState()->readAhead(2) != "Ul")
        throw ParseException(QLatin1String("Invalid closure-type-name"));
    parseState()->advance(2);
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(LambdaSigNode);
    if (ADVANCE() != 'E')
        throw ParseException(QLatin1String("invalid closure-type-name"));
    if (NonNegativeNumberNode<10>::mangledRepresentationStartsWith(PEEK()))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumberNode<10>);
    if (ADVANCE() != '_')
        throw ParseException(QLatin1String("Invalid closure-type-name"));
}

QByteArray ClosureTypeNameNode::toByteArray() const
{
    QByteArray repr = CHILD_TO_BYTEARRAY(0) + '#';
    quint64 number;
    if (childCount() == 2) {
        const NonNegativeNumberNode<10>::Ptr numberNode
                = DEMANGLER_CAST(NonNegativeNumberNode<10>, MY_CHILD_AT(1));
        number = numberNode->number() + 2;
    } else {
        number = 1;
    }
    return repr += QByteArray::number(number);
}


bool UnnamedTypeNameNode::mangledRepresentationStartsWith(char c)
{
    return c == 'U';
}

/*
 *  <unnamed-type-name> ::= Ut [ <nonnegative number> ] _
 *                      ::= <closure-type-name>
 */
void UnnamedTypeNameNode::parse()
{
    if (parseState()->readAhead(2) == "Ut") {
        parseState()->advance(2);
        if (NonNegativeNumberNode<10>::mangledRepresentationStartsWith(PEEK()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumberNode<10>);
        if (ADVANCE() != '_')
            throw ParseException(QLatin1String("Invalid unnamed-type-node"));
    } else {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ClosureTypeNameNode);
    }
}

QByteArray UnnamedTypeNameNode::toByteArray() const
{
    QByteArray repr(1, '{');
    if (childCount() == 0) {
        repr += "unnamed type#1";
    } else {
        const NonNegativeNumberNode<10>::Ptr numberNode
                = MY_CHILD_AT(0).dynamicCast<NonNegativeNumberNode<10> >();
        if (numberNode)
            repr += "unnamed type#" + QByteArray::number(numberNode->number() + 2);
        else
            repr += CHILD_TO_BYTEARRAY(0);
    }
    return repr += '}';
}


bool DeclTypeNode::mangledRepresentationStartsWith(char c)
{
    return c == 'D';
}

/*
 * <decltype> ::= Dt <expression> E  # decltype of an id-expression or class member access (C++0x)
 *            ::= DT <expression> E  # decltype of an expression (C++0x)
 */
void DeclTypeNode::parse()
{
    const QByteArray start = parseState()->readAhead(2);
    if (start != "DT" && start != "Dt")
        throw ParseException(QLatin1String("Invalid decltype"));
    parseState()->advance(2);
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
    if (ADVANCE() != 'E')
        throw ParseException(QString::fromLatin1("Invalid type"));
}

QByteArray DeclTypeNode::toByteArray() const
{
    return "decltype(" + CHILD_TO_BYTEARRAY(0) + ')';
}


bool UnresolvedTypeRule::mangledRepresentationStartsWith(char c)
{
    return TemplateParamNode::mangledRepresentationStartsWith(c)
            || DeclTypeNode::mangledRepresentationStartsWith(c)
            || SubstitutionNode::mangledRepresentationStartsWith(c);
}

/*
 * <unresolved-type> ::= <template-param>
 *                   ::= <decltype>
 *                   ::= <substitution>
 */
void UnresolvedTypeRule::parse(GlobalParseState *parseState)
{
    const char next = parseState->peek();
    const ParseTreeNode::Ptr parentNode = parseState->stackTop();
    if (TemplateParamNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD_TO_NODE(TemplateParamNode, parseState, parentNode);
    else if (DeclTypeNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD_TO_NODE(DeclTypeNode, parseState, parentNode);
    else if (SubstitutionNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD_TO_NODE(SubstitutionNode, parseState, parentNode);
    else
        throw ParseException(QLatin1String("Invalid unresolved-type"));
}


bool SimpleIdNode::mangledRepresentationStartsWith(char c)
{
    return SourceNameNode::mangledRepresentationStartsWith(c);
}

// <simple-id> ::= <source-name> [ <template-args> ]
void SimpleIdNode::parse()
{
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SourceNameNode);
    if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK()))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
}

QByteArray SimpleIdNode::toByteArray() const
{
    return pasteAllChildren();
}


bool DestructorNameNode::mangledRepresentationStartsWith(char c)
{
    return UnresolvedTypeRule::mangledRepresentationStartsWith(c)
            || SimpleIdNode::mangledRepresentationStartsWith(c);
}

/*
 * <destructor-name> ::= <unresolved-type>       # e.g., ~T or ~decltype(f())
 *                   ::= <simple-id>             # e.g., ~A<2*N>
 */
void DestructorNameNode::parse()
{
    const char next = PEEK();
    if (UnresolvedTypeRule::mangledRepresentationStartsWith(next))
        UnresolvedTypeRule::parse(parseState());
    else if (SimpleIdNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SimpleIdNode);
    else
        throw ParseException(QLatin1String("Invalid destructor-name"));
}

QByteArray DestructorNameNode::toByteArray() const
{
    return '~' + CHILD_TO_BYTEARRAY(0);
}


bool UnresolvedQualifierLevelRule::mangledRepresentationStartsWith(char c)
{
    return SimpleIdNode::mangledRepresentationStartsWith(c);
}

// <unresolved-qualifier-level> ::= <simple-id>
 void UnresolvedQualifierLevelRule::parse(GlobalParseState *parseState)
{
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD_TO_NODE(SimpleIdNode, parseState, parseState->stackTop());
}


BaseUnresolvedNameNode::BaseUnresolvedNameNode(GlobalParseState *parseState)
        : ParseTreeNode(parseState), m_isOperator(false)
{
}

BaseUnresolvedNameNode::BaseUnresolvedNameNode(const BaseUnresolvedNameNode &other)
        : ParseTreeNode(other), m_isOperator(other.m_isOperator)
{
}

bool BaseUnresolvedNameNode::mangledRepresentationStartsWith(char c)
{
    return SimpleIdNode::mangledRepresentationStartsWith(c) || c == 'o' || c == 'd';
}

/*
 * <base-unresolved-name> ::= <simple-id>                         # unresolved name
 *                        ::= on <operator-name>                  # unresolved operator-function-id
 *                        ::= on <operator-name> <template-args>  # unresolved operator template-id
 *                        ::= dn <destructor-name>                # destructor or pseudo-destructor;
 *                                                                # e.g. ~X or ~X<N-1>
 */
void BaseUnresolvedNameNode::parse()
{
    if (SimpleIdNode::mangledRepresentationStartsWith(PEEK())) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SimpleIdNode);
    } else if (parseState()->readAhead(2) == "on") {
        m_isOperator = true;
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(OperatorNameNode);
        if (TemplateArgsNode::mangledRepresentationStartsWith(PEEK()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgsNode);
    } else if (parseState()->readAhead(2) == "dn") {
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(DestructorNameNode);
    } else {
        throw ParseException(QLatin1String("Invalid <base-unresolved-name>"));
    }
}

QByteArray BaseUnresolvedNameNode::description() const
{
    return "BaseUnresolvedName[isOperator:" + bool2String(m_isOperator) + ']';
}

QByteArray BaseUnresolvedNameNode::toByteArray() const
{
    QByteArray repr;
    if (m_isOperator)
        repr += "operator";
    return repr += pasteAllChildren();
}


bool InitializerNode::mangledRepresentationStartsWith(char c)
{
    return c == 'p';
}

// <initializer> ::= pi <expression>* E          # parenthesized initialization
void InitializerNode::parse()
{
    if (parseState()->readAhead(2) != "pi")
        throw ParseException(QLatin1String("Invalid initializer"));
    parseState()->advance(2);
    while (ExpressionNode::mangledRepresentationStartsWith(PEEK()))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExpressionNode);
    if (ADVANCE() != 'E')
        throw ParseException(QLatin1String("Invalid initializer"));
}

QByteArray InitializerNode::toByteArray() const
{
    QByteArray repr = "(";
    for (int i = 0; i < childCount(); ++i) {
        repr += CHILD_TO_BYTEARRAY(i);
        if (i < childCount() - 1)
            repr += ", ";
    }
    return repr += ')';
}


UnresolvedNameNode::UnresolvedNameNode(const UnresolvedNameNode &other)
        : ParseTreeNode(other), m_globalNamespace(other.m_globalNamespace)
{
}

bool UnresolvedNameNode::mangledRepresentationStartsWith(char c)
{
    return BaseUnresolvedNameNode::mangledRepresentationStartsWith(c) || c == 'g' || c == 's';
}

/*
 * <unresolved-name> ::= [gs] <base-unresolved-name>                     # x or (with "gs") ::x
 *                   ::= sr <unresolved-type> <base-unresolved-name>     # T::x / decltype(p)::x
 *                   ::= srN <unresolved-type> <unresolved-qualifier-level>+ E <base-unresolved-name>
 *                                                    # T::N::x /decltype(p)::N::x
 *                   ::= [gs] sr <unresolved-qualifier-level>+ E <base-unresolved-name>
 *                                                    # A::x, N::y, A<T>::z; "gs" means leading "::"
 */
void UnresolvedNameNode::parse()
{
    if (parseState()->readAhead(2) == "gs") {
        m_globalNamespace = true;
        parseState()->advance(2);
    } else {
        m_globalNamespace = false;
    }
    if (parseState()->readAhead(2) == "sr") {
        parseState()->advance(2);
        if (PEEK() == 'N') {
            ADVANCE();
            UnresolvedTypeRule::parse(parseState());
            do
                UnresolvedQualifierLevelRule::parse(parseState());
            while (UnresolvedQualifierLevelRule::mangledRepresentationStartsWith(PEEK()));
            if (ADVANCE() != 'E')
                throw ParseException(QLatin1String("Invalid unresolved-name"));
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(BaseUnresolvedNameNode);
        } else if (UnresolvedTypeRule::mangledRepresentationStartsWith(PEEK())) {
            if (m_globalNamespace)
                throw ParseException(QLatin1String("Invalid unresolved-name"));
            UnresolvedTypeRule::parse(parseState());
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(BaseUnresolvedNameNode);
        } else {
            if (!UnresolvedQualifierLevelRule::mangledRepresentationStartsWith(PEEK()))
                throw ParseException(QLatin1String("Invalid unresolved-name"));
            while (UnresolvedQualifierLevelRule::mangledRepresentationStartsWith(PEEK()))
                UnresolvedQualifierLevelRule::parse(parseState());
            if (ADVANCE() != 'E')
                throw ParseException(QLatin1String("Invalid unresolved-name"));
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(BaseUnresolvedNameNode);
        }
    } else {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(BaseUnresolvedNameNode);
    }
}

QByteArray UnresolvedNameNode::description() const
{
    return "UnresolvedName[globalNamespace:" + bool2String(m_globalNamespace) + ']';
}

QByteArray UnresolvedNameNode::toByteArray() const
{
    QByteArray repr;
    if (m_globalNamespace)
        repr += "::";
    for (int i = 0; i < childCount(); ++i) {
        repr += CHILD_TO_BYTEARRAY(i);
        if (i < childCount() - 1)
            repr += "::";
    }
    return repr;
}


bool FunctionParamNode::mangledRepresentationStartsWith(char c)
{
    return c == 'f';
}

/*
 * <function-param> ::= fp <top-level CV-qualifiers> _                                     # L == 0, first parameter
 *                  ::= fp <top-level CV-qualifiers> <parameter-2 non-negative number> _   # L == 0, second and later parameters
 *                  ::= fL <L-1 non-negative number> p <top-level CV-qualifiers> _         # L > 0, first parameter
 *                  ::= fL <L-1 non-negative number> p <top-level CV-qualifiers> <parameter-2 non-negative number> _   # L > 0, second and later parameters
 */
void FunctionParamNode::parse()
{
    if (parseState()->readAhead(2) == "fp") {
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CvQualifiersNode);
        if (NonNegativeNumberNode<10>::mangledRepresentationStartsWith(PEEK()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumberNode<10>);
        if (ADVANCE() != '_')
            throw ParseException(QLatin1String("Invalid function-param"));
    } else if (parseState()->readAhead(2) == "fL") {
        parseState()->advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumberNode<10>);
        if (ADVANCE() != 'p')
            throw ParseException(QLatin1String("Invalid function-param"));
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CvQualifiersNode);
        if (NonNegativeNumberNode<10>::mangledRepresentationStartsWith(PEEK()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumberNode<10>);
        if (ADVANCE() != '_')
            throw ParseException(QLatin1String("Invalid function-param"));
    } else {
        throw ParseException(QLatin1String("Invalid function-param"));
    }
}

QByteArray FunctionParamNode::toByteArray() const
{
    // We ignore L for now.
    const NonNegativeNumberNode<10>::Ptr numberNode
            = MY_CHILD_AT(childCount() - 1).dynamicCast<NonNegativeNumberNode<10> >();
    const int paramNumber = numberNode ? numberNode->number() + 2 : 1;
    const int cvIndex = MY_CHILD_AT(0).dynamicCast<CvQualifiersNode>() ? 0 : 1;
    const CvQualifiersNode::Ptr cvNode = DEMANGLER_CAST(CvQualifiersNode, MY_CHILD_AT(cvIndex));
    QByteArray repr = "{param#" + QByteArray::number(paramNumber);
    if (cvNode->hasQualifiers())
        repr.append(' ').append(cvNode->toByteArray());
    repr += '}';
    return repr;
}

} // namespace Internal
} // namespace Debugger
