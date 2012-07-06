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

#include "namedemangler.h"

#include "demanglerexceptions.h"
#include "parsetreenodes.h"

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QRegExp>
#include <QStack>
#include <QString>

#include <cctype>
#include <limits>

#define PARSE_RULE_AND_ADD_RESULT_AS_CHILD(rule, parentNode) \
    do { \
        parse##rule(); \
        DEMANGLER_ASSERT(!m_parseStack.isEmpty()); \
        DEMANGLER_ASSERT(dynamic_cast<rule##Node *>(m_parseStack.top())); \
        popNodeFromStackAndAddAsChild(parentNode); \
    } while (0)

// Debugging facility.
//#define DO_TRACE
#ifdef DO_TRACE
#define FUNC_START()                                                           \
    qDebug("Function %s has started, input is at position %d.", Q_FUNC_INFO, m_pos)
#define FUNC_END()                                                             \
    qDebug("Function %s has finished, input is at position %d..", m_pos)
#else
#define FUNC_START()
#define FUNC_END()
#endif // DO_TRACE

namespace Debugger {
namespace Internal {

class NameDemanglerPrivate
{
public:
    bool demangle(const QString &mangledName);
    const QString &errorString() const { return m_errorString; }
    const QString &demangledName() const { return m_demangledName; }

private:
    char peek(int ahead = 0);
    char advance(int steps = 1);
    const QByteArray readAhead(int charCount);

    void addSubstitution(const ParseTreeNode *node);

    // One parse function per Non-terminal.
    void parseArrayType();
    void parseBareFunctionType();
    void parseBuiltinType();
    void parseCallOffset();
    void parseClassEnumType();
    void parseCtorDtorName();
    void parseCvQualifiers();
    void parseDigit();
    void parseDiscriminator();
    void parseEncoding();
    void parseExpression();
    void parseExprPrimary();
    void parseFloatValue();
    void parseFunctionType();
    void parseLocalName();
    void parseMangledName();
    void parseName();
    void parseNestedName();
    void parseNonNegativeNumber(int base = 10);
    void parseNumber(int base = 10);
    void parseNvOffset();
    void parseOperatorName();
    void parsePointerToMemberType();
    void parsePrefix();
    void parsePrefix2();
    void parseSpecialName();
    void parseSourceName();
    void parseSubstitution();
    void parseTemplateArg();
    void parseTemplateArgs();
    int parseTemplateParam();
    void parseType();
    void parseUnqualifiedName();
    void parseUnscopedName();
    void parseVOffset();

    const QByteArray getIdentifier(int len);
    int getNonNegativeNumber(int base = 10);

    template <typename T> T *allocateNodeAndAddToStack()
    {
        T * const node = new T;
        m_parseStack.push(node);
        return node;
    }

    void popNodeFromStackAndAddAsChild(ParseTreeNode *parentNode)
    {
        parentNode->addChild(m_parseStack.pop());
    }

    static const char eoi = '$';

    int m_pos;
    QByteArray m_mangledName;
    QString m_errorString;
    QString m_demangledName;
    QList<QByteArray> m_substitutions;
    QList<ParseTreeNode *> m_templateParams;
    bool m_isConversionOperator;
    QStack<ParseTreeNode *> m_parseStack;
};


bool NameDemanglerPrivate::demangle(const QString &mangledName)
{
    bool success;
    try {
        m_mangledName = mangledName.toAscii();
        m_pos = 0;
        m_isConversionOperator = false;
        m_demangledName.clear();

        if (!MangledNameNode::mangledRepresentationStartsWith(peek())) {
            m_demangledName = m_mangledName;
            return true;
        }

        parseMangledName();
        if (m_pos != m_mangledName.size())
            throw ParseException(QLatin1String("Unconsumed input"));
        if (m_parseStack.count() != 1) {
            throw ParseException(QString::fromLocal8Bit("There are %1 elements on the parse stack; "
                    "expected one.").arg(m_parseStack.count()));
        }
        m_demangledName = m_parseStack.top()->toByteArray();

        success = true;
    } catch (const ParseException &p) {
        m_errorString = QString::fromLocal8Bit("Parse error at index %1 of mangled name '%2': %3.")
                .arg(m_pos).arg(mangledName, p.error);
        success = false;
    } catch (const InternalDemanglerException &e) {
        m_errorString = QString::fromLocal8Bit("Internal demangler error at function %1, file %2, "
                "line %3").arg(e.func, e.file).arg(e.line);
        success = false;
    }

    qDeleteAll(m_parseStack);
    m_parseStack.clear();
    m_substitutions.clear();
    m_templateParams.clear();
   return success;
}

/*
 * Grammar: http://www.codesourcery.com/public/cxx-abi/abi.html#mangling
 * The grammar as given there is not LL(k), so a number of transformations
 * were necessary, which we will document at the respective parsing function.
 * <mangled-name> ::= _Z <encoding>
 */
void NameDemanglerPrivate::parseMangledName()
{
    FUNC_START();

    MangledNameNode * const node = allocateNodeAndAddToStack<MangledNameNode>();
    advance(2);
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Encoding, node);

    FUNC_END();
}

/*
 * <encoding> ::= <name> <bare-function-type>
 *            ::= <name>
 *            ::= <special-name>
 */
void NameDemanglerPrivate::parseEncoding()
{
    FUNC_START();

    EncodingNode * const encodingNode = allocateNodeAndAddToStack<EncodingNode>();
    const char next = peek();
    if (NameNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Name, encodingNode);
        if (BareFunctionTypeNode::mangledRepresentationStartsWith(peek()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(BareFunctionType, encodingNode);
        addSubstitution(encodingNode);
        m_templateParams.clear();
        m_isConversionOperator = false;
    } else if (SpecialNameNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SpecialName, encodingNode);
    } else {
        throw ParseException(QString::fromLatin1("Invalid encoding"));
    }

    FUNC_END();
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
void NameDemanglerPrivate::parseName()
{
    FUNC_START();

    NameNode * const node = allocateNodeAndAddToStack<NameNode>();

    if ((readAhead(2) == "St" && UnqualifiedNameNode::mangledRepresentationStartsWith(peek(2)))
            || UnscopedNameNode::mangledRepresentationStartsWith(peek())) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnscopedName, node);
        if (TemplateArgsNode::mangledRepresentationStartsWith(peek())) {
            addSubstitution(node);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgs, node);
        }
    } else {
        const char next = peek();
        if (NestedNameNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NestedName, node);
        } else if (SubstitutionNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Substitution, node);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgs, node);
        } else if (LocalNameNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(LocalName, node);
        } else {
            throw ParseException(QString::fromLatin1("Invalid name"));
        }
    }

    FUNC_END();
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
void NameDemanglerPrivate::parseNestedName()
{
    FUNC_START();

    if (!NestedNameNode::mangledRepresentationStartsWith(advance()))
        throw ParseException(QString::fromLatin1("Invalid nested-name"));

    NestedNameNode * const node = allocateNodeAndAddToStack<NestedNameNode>();

    if (CvQualifiersNode::mangledRepresentationStartsWith(peek()) && peek(1) != 'm'
            && peek(1) != 'M' && peek(1) != 's' && peek(1) != 'S') {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CvQualifiers, node);
    }

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Prefix, node);

    if (advance() != 'E')
        throw ParseException(QString::fromLatin1("Invalid nested-name"));

    FUNC_END();
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
void NameDemanglerPrivate::parsePrefix()
{
    FUNC_START();

    PrefixNode * const node = allocateNodeAndAddToStack<PrefixNode>();

    const char next = peek();
    if (TemplateParamNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateParam, node);
        if (TemplateArgsNode::mangledRepresentationStartsWith(peek())) {
            addSubstitution(node);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgs, node);
        }
        if (UnqualifiedNameNode::mangledRepresentationStartsWith(peek())) {
            addSubstitution(node);
            parsePrefix2(); // Pops itself to child list.
        }
    } else if (SubstitutionNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Substitution, node);
        if (TemplateArgsNode::mangledRepresentationStartsWith(peek())) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgs, node);
            if (UnqualifiedNameNode::mangledRepresentationStartsWith(peek()))
                addSubstitution(node);
        }
        parsePrefix2(); // Pops itself to child list.
    } else {
        parsePrefix2(); // Pops itself to child list.
    }

    FUNC_END();
}

/*
 * <prefix-2> ::= <unqualified-name> [<template-args>] <prefix-2>
 *            ::=  # empty
 */
void NameDemanglerPrivate::parsePrefix2()
{
    FUNC_START();

    // We need to do this so we can correctly add all substitutions, which always start
    // with the representation of the prefix node.
    ParseTreeNode * const prefixNode = m_parseStack.top();
    Prefix2Node * const node = allocateNodeAndAddToStack<Prefix2Node>();
    prefixNode->addChild(node);

    bool firstRun = true;
    while (UnqualifiedNameNode::mangledRepresentationStartsWith(peek())) {
        if (!firstRun)
            addSubstitution(prefixNode);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnqualifiedName, node);
        if (TemplateArgsNode::mangledRepresentationStartsWith(peek())) {
            addSubstitution(prefixNode);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgs, node);
        }
        firstRun = false;
    }

    m_parseStack.pop();
    FUNC_END();
}

/*
 * <template-args> ::= I <template-arg>+ E
 */
void NameDemanglerPrivate::parseTemplateArgs()
{
    FUNC_START();

    TemplateArgsNode * const node = allocateNodeAndAddToStack<TemplateArgsNode>();

    if (!TemplateArgsNode::mangledRepresentationStartsWith(advance()))
        throw ParseException(QString::fromLatin1("Invalid template args"));

    do
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArg, node);
    while (TemplateArgNode::mangledRepresentationStartsWith(peek()));

    if (advance() != 'E')
        throw ParseException(QString::fromLatin1("Invalid template args"));

    FUNC_END();
}

/*
 * <template-param> ::= T_      # first template parameter
 *                  ::= T <non-negative-number> _
 */
int NameDemanglerPrivate::parseTemplateParam()
{
    FUNC_START();

    if (!TemplateParamNode::mangledRepresentationStartsWith(advance()))
        throw ParseException(QString::fromLatin1("Invalid template-param"));

    TemplateParamNode * const node = allocateNodeAndAddToStack<TemplateParamNode>();

    int index;
    if (peek() == '_')
        index = 0;
    else
        index = getNonNegativeNumber() + 1;
    if (advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid template-param"));
    if (index >= m_templateParams.count()) {
        if (!m_isConversionOperator) {
            throw ParseException(QString::fromLocal8Bit("Invalid template parameter index %1")
                    .arg(index));
        }
    } else {
        node->addChild(m_templateParams.at(index));
    }

    FUNC_END();
    return index;
}

/*  <CV-qualifiers> ::= [r] [V] [K] 	# restrict (C99), volatile, const */
void NameDemanglerPrivate::parseCvQualifiers()
{
    FUNC_START();

    CvQualifiersNode * const node = allocateNodeAndAddToStack<CvQualifiersNode>();
    node->m_hasConst = false;
    node->m_hasVolatile = false;

    while (true) {
        if (peek() == 'V') {
            if (node->m_hasConst || node->m_hasVolatile)
                throw ParseException(QLatin1String("Invalid qualifiers: unexpected 'volatile'"));
            node->m_hasVolatile = true;
            advance();
        } else if (peek() == 'K') {
            if (node->m_hasConst)
                throw ParseException(QLatin1String("Invalid qualifiers: 'const' appears twice"));
            node->m_hasConst = true;
            advance();
        } else {
            break;
        }
    }

    FUNC_END();
}

/* <number> ::= [n] <non-negative decimal integer> */
void NameDemanglerPrivate::parseNumber(int base)
{
    FUNC_START();

    const char next = peek();
    if (!NumberNode::mangledRepresentationStartsWith(next, base))
        throw ParseException("Invalid number");

    NumberNode * const node = allocateNodeAndAddToStack<NumberNode>();

    if (next == 'n') {
        node->m_isNegative = true;
        advance();
    } else {
        node->m_isNegative = false;
    }

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumber, node);

    FUNC_END();
}


void NameDemanglerPrivate::parseNonNegativeNumber(int base)
{
    FUNC_START();

    NonNegativeNumberNode * const node = allocateNodeAndAddToStack<NonNegativeNumberNode>();

    int startPos = m_pos;
    while (NonNegativeNumberNode::mangledRepresentationStartsWith(peek(), base))
        advance();
    if (m_pos == startPos)
        throw ParseException(QString::fromLatin1("Invalid non-negative number"));
    node->m_number = m_mangledName.mid(startPos, m_pos - startPos).toULongLong(0, base);

    FUNC_END();
}

/*
 * Floating-point literals are encoded using a fixed-length lowercase
 * hexadecimal string corresponding to the internal representation
 * (IEEE on Itanium), high-order bytes first, without leading zeroes.
 * For example: "Lf bf800000 E" is -1.0f on Itanium.
 */
void NameDemanglerPrivate::parseFloatValue()
{
    FUNC_START();

    FloatValueNode * const node = allocateNodeAndAddToStack<FloatValueNode>();
    node->m_value = 0;

    while (FloatValueNode::mangledRepresentationStartsWith(peek())) {
        // TODO: Construct value;
        advance();
    }

    FUNC_END();
}

/*
 * <template-arg> ::= <type>                    # type or template
 *                ::= X <expression> E          # expression
 *                ::= <expr-primary>            # simple expressions
 *                ::= J <template-arg>* E       # argument pack
 *                ::= sp <expression>           # pack expansion of (C++0x)
 */
void NameDemanglerPrivate::parseTemplateArg()
{
    FUNC_START();

    TemplateArgNode * const node = allocateNodeAndAddToStack<TemplateArgNode>();
    node->m_isTemplateArgumentPack = false;

    char next = peek();
    if (readAhead(2) == "sp") {
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Expression, node);
    } else if (TypeNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
    } else if (ExprPrimaryNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExprPrimary, node);
    } else if (next == 'X') {
        advance();
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Expression, node);
        if (advance() != 'E')
            throw ParseException(QString::fromLatin1("Invalid template-arg"));
    } else if (next == 'J') {
        advance();
        while (TemplateArgNode::mangledRepresentationStartsWith(peek()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArg, node);
        if (advance() != 'E')
            throw ParseException(QString::fromLatin1("Invalid template-arg"));
    } else {
        throw ParseException(QString::fromLatin1("Invalid template-arg"));
    }

    m_templateParams.append(node);
    FUNC_END();
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
void NameDemanglerPrivate::parseExpression()
{
    FUNC_START();

    ExpressionNode * const node = allocateNodeAndAddToStack<ExpressionNode>();
    node->m_type = ExpressionNode::OtherType;

   /*
    * Some of the terminals in the productions of <expression>
    * also appear in the productions of <operator-name>. We assume the direct
    * productions to have higher precedence and check them first to prevent
    * them being parsed by parseOperatorName().
    */
    QByteArray str = readAhead(2);
    if (str == "cl") {
        advance(2);
        while (ExpressionNode::mangledRepresentationStartsWith(peek()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Expression, node);
        if (advance() != 'E')
            throw ParseException(QString::fromLatin1("Invalid expression"));
    } else if (str == "cv") {
        node->m_type = ExpressionNode::ConversionType;
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
        if (peek() == '_') {
            advance();
            while (ExpressionNode::mangledRepresentationStartsWith(peek()))
                PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Expression, node);
            if (advance() != 'E')
                throw ParseException(QString::fromLatin1("Invalid expression"));
        } else {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Expression, node);
        }
    } else if (str == "st") {
        node->m_type = ExpressionNode::SizeofType;
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
    } else if (str == "at") {
        node->m_type = ExpressionNode::AlignofType;
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
    } else if (str == "sr") { // TODO: Which syntax to use here?
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnqualifiedName, node);
        if (TemplateArgsNode::mangledRepresentationStartsWith(peek()))
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgs, node);
    } else if (str == "sZ") {
        node->m_type = ExpressionNode::ParameterPackSizeType;
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateParam, node);
    } else {
        const char next = peek();
        if (OperatorNameNode::mangledRepresentationStartsWith(next)) {
            node->m_type = ExpressionNode::OperatorType;
            parseOperatorName();
            OperatorNameNode * const opNode = DEMANGLER_CAST(OperatorNameNode, m_parseStack.pop());
            node->addChild(opNode);

            int expressionCount;
            switch (opNode->m_type) {
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
                PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Expression, node);

        } else if (TemplateParamNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateParam, node);

#if 0
        } else if (FunctionParamNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(FunctionParam, node);
#endif
        } else if (ExprPrimaryNode::mangledRepresentationStartsWith(next)) {
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ExprPrimary, node);
        } else {
            throw ParseException(QString::fromLatin1("Invalid expression"));
        }
    }

    FUNC_END();
}

/*
 * <expr-primary> ::= L <type> <number> E            # integer literal
 *                ::= L <type> <float> E             # floating literal
 *                ::= L <mangled-name> E             # external name
 */
 // TODO: This has been updated in the spec. Needs to be adapted. (nullptr etc.)
void NameDemanglerPrivate::parseExprPrimary()
{
    FUNC_START();

    ExprPrimaryNode * const node = allocateNodeAndAddToStack<ExprPrimaryNode>();

    if (!ExprPrimaryNode::mangledRepresentationStartsWith(advance()))
        throw ParseException(QString::fromLatin1("Invalid primary expression"));
    const char next = peek();
    if (TypeNode::mangledRepresentationStartsWith(next)) {
        parseType();
        const ParseTreeNode * const topLevelTypeNode = m_parseStack.top();
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
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Number, node);
            break;
        case PredefinedBuiltinTypeNode::FloatType: case PredefinedBuiltinTypeNode::DoubleType:
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(FloatValue, node);
            break;
        default:
            throw ParseException(QString::fromLatin1("Invalid type in expr-primary"));
        }
        delete m_parseStack.pop(); // No need to keep the type node in the tree.
    } else if (MangledNameNode::mangledRepresentationStartsWith(next)) {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(MangledName, node);
    } else {
        throw ParseException(QString::fromLatin1("Invalid expr-primary"));
    }
    if (advance() != 'E')
        throw ParseException(QString::fromLatin1("Invalid expr-primary"));

    FUNC_END();
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
void NameDemanglerPrivate::parseType()
{
    FUNC_START();

    TypeNode * const node = allocateNodeAndAddToStack<TypeNode>();

    QByteArray str = readAhead(2);
    if (str == "Dp") {
        node->m_type = TypeNode::PackExpansionType;
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
    } else if (str == "Dt") {
        node->m_type = TypeNode::DeclType;
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Expression, node);
        if (advance() != 'E')
            throw ParseException(QString::fromLatin1("Invalid type"));
    } else if (str == "DT") {
        node->m_type = TypeNode::DeclType;
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Expression, node);
        if (advance() != 'E')
            throw ParseException(QString::fromLatin1("Invalid type"));
    } else {
        const char next = peek();
        if (str == "Dd" || str == "De" || str == "Df" || str == "Dh" || str == "Di" || str == "Ds"
            || (next != 'D' && BuiltinTypeNode::mangledRepresentationStartsWith(next))) {
            node->m_type = TypeNode::OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(BuiltinType, node);
        } else if (FunctionTypeNode::mangledRepresentationStartsWith(next)) {
            node->m_type = TypeNode::OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(FunctionType, node);
            addSubstitution(node);
        } else if (ClassEnumTypeNode::mangledRepresentationStartsWith(next)) {
            node->m_type = TypeNode::OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ClassEnumType, node);
            addSubstitution(node);
        } else if (ArrayTypeNode::mangledRepresentationStartsWith(next)) {
            node->m_type = TypeNode::OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(ArrayType, node);
        } else if (PointerToMemberTypeNode::mangledRepresentationStartsWith(next)) {
            node->m_type = TypeNode::OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(PointerToMemberType, node);
        } else if (TemplateParamNode::mangledRepresentationStartsWith(next)) {
            node->m_type = TypeNode::OtherType;
            const int templateIndex = parseTemplateParam();
            popNodeFromStackAndAddAsChild(node);
            // The type is now a substitution candidate, but the child node may contain a forward
            // reference, so we delay the substitution until it is resolved.
            // TODO: Check whether this is really safe, i.e. whether the following parse function
            // might indirectly expect this substitution to already exist.

            if (TemplateArgsNode::mangledRepresentationStartsWith(peek())) {
                PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgs, node);
                // Substitution delayed, see above.
            }

            // Resolve forward reference, if necessary.
            ParseTreeNode * const templateParamNode = CHILD_AT(node, 0);
            if (templateParamNode->childCount() == 0) {
                if (templateIndex >= m_templateParams.count()) {
                    throw ParseException(QString::fromLocal8Bit("Invalid template parameter "
                        "index %1 in forwarding").arg(templateIndex));
                }
                templateParamNode->addChild(m_templateParams.at(templateIndex));
            }

            // Delayed substitutions from above.
            addSubstitution(templateParamNode);
            if (node->childCount() > 1)
                addSubstitution(node);
        } else if (SubstitutionNode::mangledRepresentationStartsWith(next)) {
            node->m_type = TypeNode::OtherType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Substitution, node);
            if (TemplateArgsNode::mangledRepresentationStartsWith(peek())) {
                PARSE_RULE_AND_ADD_RESULT_AS_CHILD(TemplateArgs, node);
                addSubstitution(node);
            }
        } else if (CvQualifiersNode::mangledRepresentationStartsWith(next)) {
            node->m_type = TypeNode::QualifiedType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CvQualifiers, node);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
            const CvQualifiersNode * const cvNode
                    = DEMANGLER_CAST(CvQualifiersNode, CHILD_AT(node, 0));
            if (cvNode->m_hasConst || cvNode->m_hasVolatile)
                addSubstitution(node);
        } else if (next == 'P') {
            node->m_type = TypeNode::PointerType;
            advance();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
            addSubstitution(node);
        } else if (next == 'R') {
            node->m_type = TypeNode::ReferenceType;
            advance();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
            addSubstitution(node);
        } else if (next == 'O') {
            node->m_type = TypeNode::RValueType;
            advance();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
            addSubstitution(node);
        } else if (next == 'C') {
            node->m_type = TypeNode::OtherType;
            advance();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
            addSubstitution(node);
        } else if (next == 'G') {
            node->m_type = TypeNode::OtherType;
            advance();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
            addSubstitution(node);
        } else if (next == 'U') {
            node->m_type = TypeNode::VendorType;
            advance();
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SourceName, node);
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
        } else {
            throw ParseException(QString::fromLatin1("Invalid type"));
        }
    }

    FUNC_END();
}

/* <source-name> ::= <number> <identifier> */
void NameDemanglerPrivate::parseSourceName()
{
    FUNC_START();

    SourceNameNode * const node = allocateNodeAndAddToStack<SourceNameNode>();

    const int idLen = getNonNegativeNumber();
    node->m_name = getIdentifier(idLen);

    FUNC_END();
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
void NameDemanglerPrivate::parseBuiltinType()
{
    FUNC_START();

    BuiltinTypeNode * const typeNode = allocateNodeAndAddToStack<BuiltinTypeNode>();
    const char next = advance();
    if (next == 'u') {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SourceName, typeNode);
    } else {
        PredefinedBuiltinTypeNode * const fixedTypeNode = new PredefinedBuiltinTypeNode;
        typeNode->addChild(fixedTypeNode);

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
            switch (advance()) {
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

    FUNC_END();
}

/* <function-type> ::= F [Y] <bare-function-type> E */
void NameDemanglerPrivate::parseFunctionType()
{
    FUNC_START();

    if (!FunctionTypeNode::mangledRepresentationStartsWith(advance()))
        throw ParseException(QString::fromLatin1("Invalid function type"));

    FunctionTypeNode * const node = allocateNodeAndAddToStack<FunctionTypeNode>();

    if (peek() == 'Y') {
        advance();
        node->m_isExternC = true;
    } else {
        node->m_isExternC = false;
    }

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(BareFunctionType, node);
    if (advance() != 'E')
        throw ParseException(QString::fromLatin1("Invalid function type"));
    FUNC_END();
}

/* <bare-function-type> ::= <type>+ */
void NameDemanglerPrivate::parseBareFunctionType()
{
    FUNC_START();

    BareFunctionTypeNode * const node = allocateNodeAndAddToStack<BareFunctionTypeNode>();

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
    const EncodingNode * const encodingNode
            = dynamic_cast<EncodingNode *>(m_parseStack.at(m_parseStack.count() - 2));
    if (encodingNode) { // Case 1: Function name.
        const NameNode * const nameNode = DEMANGLER_CAST(NameNode, CHILD_AT(encodingNode, 0));
        node->m_hasReturnType = nameNode->isTemplate()
                && !nameNode->isConstructorOrDestructorOrConversionOperator();
    } else {            // Case 2: function type.
        // TODO: What do the exceptions look like for this case?
        node->m_hasReturnType = true;
    }

    do
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
    while (TypeNode::mangledRepresentationStartsWith(peek()));


    FUNC_END();
}

/* <class-enum-type> ::= <name> */
void NameDemanglerPrivate::parseClassEnumType()
{
    FUNC_START();

    ClassEnumTypeNode * const node = allocateNodeAndAddToStack<ClassEnumTypeNode>();
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Name, node);

    FUNC_END();
}

/*
 * <unqualified-name> ::= <operator-name>
 *                    ::= <ctor-dtor-name>
 *                    ::= <source-name>
 */
void NameDemanglerPrivate::parseUnqualifiedName()
{
    FUNC_START();

    UnqualifiedNameNode * const node = allocateNodeAndAddToStack<UnqualifiedNameNode>();

    const char next = peek();
    if (OperatorNameNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(OperatorName, node);
    else if (CtorDtorNameNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CtorDtorName, node);
    else if (SourceNameNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SourceName, node);
    else
        throw ParseException(QString::fromLatin1("Invalid unqualified-name"));

    FUNC_END();
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
void NameDemanglerPrivate::parseOperatorName()
{
    FUNC_START();

    OperatorNameNode * const node = allocateNodeAndAddToStack<OperatorNameNode>();

    if (peek() == 'v') {
        node->m_type = OperatorNameNode::VendorType;
        advance();
        parseDigit();
        popNodeFromStackAndAddAsChild(node);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(SourceName, node);
    } else {
        const QByteArray id = readAhead(2);
        advance(2);
        if (id == "cv") {
            m_isConversionOperator = true;
            node->m_type = OperatorNameNode::CastType;
            PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
        } else if (id == "nw") {
            node->m_type = OperatorNameNode::NewType;
        } else if (id == "na") {
            node->m_type = OperatorNameNode::ArrayNewType;
        } else if (id == "dl") {
            node->m_type = OperatorNameNode::DeleteType;
        } else if (id == "da") {
            node->m_type = OperatorNameNode::ArrayDeleteType;
        } else if (id == "ps") {
            node->m_type = OperatorNameNode::UnaryPlusType;
        } else if (id == "ng") {
            node->m_type = OperatorNameNode::UnaryMinusType;
        } else if (id == "ad") {
            node->m_type = OperatorNameNode::UnaryAmpersandType;
        } else if (id == "de") {
            node->m_type = OperatorNameNode::UnaryStarType;
        } else if (id == "co") {
            node->m_type = OperatorNameNode::BitwiseNotType;
        } else if (id == "pl") {
            node->m_type = OperatorNameNode::BinaryPlusType;
        } else if (id == "mi") {
            node->m_type = OperatorNameNode::BinaryMinusType;
        } else if (id == "ml") {
            node->m_type = OperatorNameNode::MultType;
        } else if (id == "dv") {
            node->m_type = OperatorNameNode::DivType;
        } else if (id == "rm") {
            node->m_type = OperatorNameNode::ModuloType;
        } else if (id == "an") {
            node->m_type = OperatorNameNode::BitwiseAndType;
        } else if (id == "or") {
            node->m_type = OperatorNameNode::BitwiseOrType;
        } else if (id == "eo") {
            node->m_type = OperatorNameNode::XorType;
        } else if (id == "aS") {
            node->m_type = OperatorNameNode::AssignType;
        } else if (id == "pL") {
            node->m_type = OperatorNameNode::IncrementAndAssignType;
        } else if (id == "mI") {
            node->m_type = OperatorNameNode::DecrementAndAssignType;
        } else if (id == "mL") {
            node->m_type = OperatorNameNode::MultAndAssignType;
        } else if (id == "dV") {
            node->m_type = OperatorNameNode::DivAndAssignType;
        } else if (id == "rM") {
            node->m_type = OperatorNameNode::ModuloAndAssignType;
        } else if (id == "aN") {
            node->m_type = OperatorNameNode::BitwiseAndAndAssignType;
        } else if (id == "oR") {
            node->m_type = OperatorNameNode::BitwiseOrAndAssignType;
        } else if (id == "eO") {
            node->m_type = OperatorNameNode::XorAndAssignType;
        } else if (id == "ls") {
            node->m_type = OperatorNameNode::LeftShiftType;
        } else if (id == "rs") {
            node->m_type = OperatorNameNode::RightShiftType;
        } else if (id == "lS") {
            node->m_type = OperatorNameNode::LeftShiftAndAssignType;
        } else if (id == "rS") {
            node->m_type = OperatorNameNode::RightShiftAndAssignType;
        } else if (id == "eq") {
            node->m_type = OperatorNameNode::EqualsType;
        } else if (id == "ne") {
            node->m_type = OperatorNameNode::NotEqualsType;
        } else if (id == "lt") {
            node->m_type = OperatorNameNode::LessType;
        } else if (id == "gt") {
            node->m_type = OperatorNameNode::GreaterType;
        } else if (id == "le") {
            node->m_type = OperatorNameNode::LessEqualType;
        } else if (id == "ge") {
            node->m_type = OperatorNameNode::GreaterEqualType;
        } else if (id == "nt") {
            node->m_type = OperatorNameNode::LogicalNotType;
        } else if (id == "aa") {
            node->m_type = OperatorNameNode::LogicalAndType;
        } else if (id == "oo") {
            node->m_type = OperatorNameNode::LogicalOrType;
        } else if (id == "pp") {
            node->m_type = OperatorNameNode::IncrementType;
        } else if (id == "mm") {
            node->m_type = OperatorNameNode::DecrementType;
        } else if (id == "cm") {
            node->m_type = OperatorNameNode::CommaType;
        } else if (id == "pm") {
            node->m_type = OperatorNameNode::ArrowStarType;
        } else if (id == "pt") {
            node->m_type = OperatorNameNode::ArrowType;
        } else if (id == "cl") {
            node->m_type = OperatorNameNode::CallType;
        } else if (id == "ix") {
            node->m_type = OperatorNameNode::IndexType;
        } else if (id == "qu") {
            node->m_type = OperatorNameNode::TernaryType;
        } else if (id == "st") {
            node->m_type = OperatorNameNode::SizeofTypeType;
        } else if (id == "sz") {
            node->m_type = OperatorNameNode::SizeofExprType;
        } else if (id == "at") {
            node->m_type = OperatorNameNode::AlignofTypeType;
        } else if (id == "az") {
            node->m_type = OperatorNameNode::AlignofExprType;
        } else {
            throw ParseException(QString::fromLocal8Bit("Invalid operator encoding '%1'")
                    .arg(QString::fromLocal8Bit(id)));
        }
    }

    FUNC_END();
}

/*
 * <array-type> ::= A <number> _ <type>
 *              ::= A [<expression>] _ <type>
 */
void NameDemanglerPrivate::parseArrayType()
{
    FUNC_START();

    if (!ArrayTypeNode::mangledRepresentationStartsWith(advance()))
        throw ParseException(QString::fromLatin1("Invalid array-type"));

    ArrayTypeNode * const node = allocateNodeAndAddToStack<ArrayTypeNode>();

    const char next = peek();
    if (NonNegativeNumberNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumber, node);
    else if (ExpressionNode::mangledRepresentationStartsWith(next))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Expression, node);

    if (advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid array-type"));

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);

    FUNC_END();
}

/* <pointer-to-member-type> ::= M <type> <type> */
void NameDemanglerPrivate::parsePointerToMemberType()
{
    FUNC_START();

    if (!PointerToMemberTypeNode::mangledRepresentationStartsWith(advance()))
        throw ParseException(QString::fromLatin1("Invalid pointer-to-member-type"));

    PointerToMemberTypeNode * const node = allocateNodeAndAddToStack<PointerToMemberTypeNode>();

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node); // Class type.
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node); // Member type.

    FUNC_END();
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
void NameDemanglerPrivate::parseSubstitution()
{
    FUNC_START();

    if (!SubstitutionNode::mangledRepresentationStartsWith(advance()))
        throw ParseException(QString::fromLatin1("Invalid substitution"));

    SubstitutionNode * const node = allocateNodeAndAddToStack<SubstitutionNode>();

    if (NonNegativeNumberNode::mangledRepresentationStartsWith(peek(), 36)) {
        const int substIndex = getNonNegativeNumber(36) + 1;
        if (substIndex >= m_substitutions.size()) {
            throw ParseException(QString::fromLatin1("Invalid substitution: substitution %1 "
                "was requested, but there are only %2").
                arg(substIndex + 1).arg(m_substitutions.size()));
        }
        node->m_type = SubstitutionNode::ActualSubstitutionType;
        node->m_substValue = m_substitutions.at(substIndex);
        if (advance() != '_')
            throw ParseException(QString::fromLatin1("Invalid substitution"));
    } else {
        switch (advance()) {
        case '_':
            if (m_substitutions.isEmpty())
                throw ParseException(QString::fromLatin1("Invalid substitution: "
                    "There are no substitutions"));
            node->m_type = SubstitutionNode::ActualSubstitutionType;
            node->m_substValue = m_substitutions.first();
            break;
        case 't':
            node->m_type = SubstitutionNode::StdType;
            break;
        case 'a':
            node->m_type = SubstitutionNode::StdAllocType;
            break;
        case 'b':
            node->m_type = SubstitutionNode::StdBasicStringType;
            break;
        case 's':
            node->m_type = SubstitutionNode::FullStdBasicStringType;
            break;
        case 'i':
            node->m_type = SubstitutionNode::StdBasicIStreamType;
            break;
        case 'o':
            node->m_type = SubstitutionNode::StdBasicOStreamType;
            break;
        case 'd':
        node->m_type = SubstitutionNode::StdBasicIoStreamType;
            break;
        default:
            throw ParseException(QString::fromLatin1("Invalid substitution"));
        }
    }

    FUNC_END();
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
void NameDemanglerPrivate::parseSpecialName()
{
    FUNC_START();

    SpecialNameNode * const node = allocateNodeAndAddToStack<SpecialNameNode>();

    QByteArray str = readAhead(2);
    if (str == "TV") {
        node->m_type = SpecialNameNode::VirtualTableType;
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
    } else if (str == "TT") {
        node->m_type = SpecialNameNode::VttStructType;
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
    } else if (str == "TI") {
        node->m_type = SpecialNameNode::TypeInfoType;
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
    } else if (str == "TS") {
        node->m_type = SpecialNameNode::TypeInfoNameType;
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Type, node);
    } else if (str == "GV") {
        node->m_type = SpecialNameNode::GuardVarType;
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Name, node);
    } else if (str == "Tc") {
        node->m_type = SpecialNameNode::DoubleCallOffsetType;
        advance(2);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CallOffset, node);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CallOffset, node);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Encoding, node);
    } else if (advance() == 'T') {
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(CallOffset, node);
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Encoding, node);
    } else {
        throw ParseException(QString::fromLatin1("Invalid special-name"));
    }

    FUNC_END();
}

/*
 * <unscoped-name> ::= <unqualified-name>
 *                 ::= St <unqualified-name>   # ::std::
 */
void NameDemanglerPrivate::parseUnscopedName()
{
    FUNC_START();

    UnscopedNameNode * const node = allocateNodeAndAddToStack<UnscopedNameNode>();

    if (readAhead(2) == "St") {
        node->m_inStdNamespace = true;
        advance(2);
    } else {
        node->m_inStdNamespace = false;
    }

    if (!UnqualifiedNameNode::mangledRepresentationStartsWith(peek()))
        throw ParseException(QString::fromLatin1("Invalid unscoped-name"));

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(UnqualifiedName, node);

    FUNC_END();
}


/*
 * <local-name> := Z <encoding> E <name> [<discriminator>]
 *              := Z <encoding> E s [<discriminator>]
 *
 * Note that <name> can start with 's', so we need to do read-ahead.
 */
void NameDemanglerPrivate::parseLocalName()
{
    FUNC_START();

    if (!LocalNameNode::mangledRepresentationStartsWith(advance()))
        throw ParseException(QString::fromLatin1("Invalid local-name"));

    LocalNameNode * const node = allocateNodeAndAddToStack<LocalNameNode>();

    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Encoding, node);

    if (advance() != 'E')
        throw ParseException(QString::fromLatin1("Invalid local-name"));

    QByteArray str = readAhead(2);
    char next = peek();
    if (str == "sp" || str == "sr" || str == "st" || str == "sz" || str == "sZ"
            || (next != 's' && NameNode::mangledRepresentationStartsWith(next))) {
        node->m_isStringLiteral = false;
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Name, node);
    } else if (next == 's') {
        node->m_isStringLiteral = true;
        advance();
    } else {
        throw ParseException(QString::fromLatin1("Invalid local-name"));
    }
    if (DiscriminatorNode::mangledRepresentationStartsWith(peek()))
        PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Discriminator, node);

    FUNC_END();
}

/* <discriminator> := _ <non-negative-number> */
void NameDemanglerPrivate::parseDiscriminator()
{
    FUNC_START();

    if (advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid discriminator"));

    DiscriminatorNode * const node = allocateNodeAndAddToStack<DiscriminatorNode>();
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NonNegativeNumber, node);

    FUNC_END();
}

/*
 * <ctor-dtor-name> ::= C1      # complete object constructor
 *                  ::= C2      # base object constructor
 *                  ::= C3      # complete object allocating constructor
 *                  ::= D0      # deleting destructor
 *                  ::= D1      # complete object destructor
 *                  ::= D2      # base object destructor
 */
void NameDemanglerPrivate::parseCtorDtorName()
{
    FUNC_START();

    CtorDtorNameNode * const node = allocateNodeAndAddToStack<CtorDtorNameNode>();

    switch (advance()) {
    case 'C':
        switch (advance()) {
        case '1': case '2': case '3': node->m_isDestructor = false; break;
        default: throw ParseException(QString::fromLatin1("Invalid ctor-dtor-name"));
        }
        break;
    case 'D':
        switch (advance()) {
        case '0': case '1': case '2': node->m_isDestructor = true; break;
        default: throw ParseException(QString::fromLatin1("Invalid ctor-dtor-name"));
        }
        break;
    default:
        throw ParseException(QString::fromLatin1("Invalid ctor-dtor-name"));
    }

    node->m_representation = m_substitutions.last();

    FUNC_END();
}

const QByteArray NameDemanglerPrivate::getIdentifier(int len)
{
    FUNC_START();

    const QByteArray id = m_mangledName.mid(m_pos, len);
    advance(len);

    FUNC_END();
    return id;
}

/*
 * <call-offset> ::= h <nv-offset> _
 *               ::= v <v-offset> _
 */
void NameDemanglerPrivate::parseCallOffset()
{
    FUNC_START();

    CallOffsetNode * const node = allocateNodeAndAddToStack<CallOffsetNode>();
    switch (advance()) {
    case 'h': PARSE_RULE_AND_ADD_RESULT_AS_CHILD(NvOffset, node); break;
    case 'v': PARSE_RULE_AND_ADD_RESULT_AS_CHILD(VOffset, node); break;
    default: DEMANGLER_ASSERT(false);
    }
    if (advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid call-offset"));

    FUNC_END();
}

/* <nv-offset> ::= <number> # non-virtual base override */
void NameDemanglerPrivate::parseNvOffset()
{
    FUNC_START();

    NvOffsetNode * const node = allocateNodeAndAddToStack<NvOffsetNode>();
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Number, node);

    FUNC_END();
}

/*
 * <v-offset> ::= <number> _ <number>
 *    # virtual base override, with vcall offset
 */
void NameDemanglerPrivate::parseVOffset()
{
    FUNC_START();

    VOffsetNode * const node = allocateNodeAndAddToStack<VOffsetNode>();
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Number, node);
    if (advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid v-offset"));
    PARSE_RULE_AND_ADD_RESULT_AS_CHILD(Number, node);

    FUNC_END();
}

int NameDemanglerPrivate::getNonNegativeNumber(int base)
{
    parseNonNegativeNumber(base);
    NonNegativeNumberNode * const numberNode
            = DEMANGLER_CAST(NonNegativeNumberNode, m_parseStack.pop());
    const int value = static_cast<int>(numberNode->m_number);
    delete numberNode;
    return value;
}

void NameDemanglerPrivate::parseDigit()
{
    FUNC_START();

    NonNegativeNumberNode * const node = allocateNodeAndAddToStack<NonNegativeNumberNode>();

    const int digit = advance();
    if (!std::isdigit(digit))
        throw ParseException(QString::fromLatin1("Invalid digit"));
    node->m_number = digit - 0x30;

    FUNC_END();
}

char NameDemanglerPrivate::peek(int ahead)
{
    Q_ASSERT(m_pos >= 0);

    if (m_pos + ahead < m_mangledName.size())
        return m_mangledName[m_pos + ahead];
    return eoi;
}

char NameDemanglerPrivate::advance(int steps)
{
    Q_ASSERT(steps > 0);
    if (m_pos + steps > m_mangledName.size())
        throw ParseException(QLatin1String("Unexpected end of input"));

    const char c = m_mangledName[m_pos];
    m_pos += steps;
    return c;
}

const QByteArray NameDemanglerPrivate::readAhead(int charCount)
{
    QByteArray str;
    if (m_pos + charCount < m_mangledName.size())
        str = m_mangledName.mid(m_pos, charCount);
    else
        str.fill(eoi, charCount);
    return str;
}

void NameDemanglerPrivate::addSubstitution(const ParseTreeNode *node)
{
    const QByteArray symbol = node->toByteArray();
    if (!symbol.isEmpty() && !m_substitutions.contains(symbol))
        m_substitutions.append(symbol);
}


NameDemangler::NameDemangler() : d(new NameDemanglerPrivate) { }

NameDemangler::~NameDemangler()
{
    delete d;
}

bool NameDemangler::demangle(const QString &mangledName)
{
    return d->demangle(mangledName);
}

QString NameDemangler::errorString() const
{
    return d->errorString();
}

QString NameDemangler::demangledName() const
{
    return d->demangledName();
}

} // namespace Internal
} // namespace Debugger
