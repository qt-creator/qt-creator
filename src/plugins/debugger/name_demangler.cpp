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

#include "name_demangler.h"

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QRegExp>
#include <QSet>
#include <QString>

#include <cctype>
#include <limits>

// Debugging facility.
//#define DO_TRACE
#ifdef DO_TRACE
#define FUNC_START()                                                           \
    qDebug("Function %s has started, input is at position %d.", Q_FUNC_INFO, pos)
#define FUNC_END(result)                                                       \
    qDebug("Function %s has finished, result is '%s'.", Q_FUNC_INFO, qPrintable(result))
#else
#define FUNC_START()
#define FUNC_END(result)
#endif // DO_TRACE

// The first-sets for all non-terminals.
static QSet<char> firstSetArrayType;
static QSet<char> firstSetBareFunctionType;
static QSet<char> firstSetBuiltinType;
static QSet<char> firstSetCallOffset;
static QSet<char> firstSetClassEnumType;
static QSet<char> firstSetDiscriminator;
static QSet<char> firstSetCtorDtorName;
static QSet<char> firstSetCvQualifiers;
static QSet<char> firstSetEncoding;
static QSet<char> firstSetExpression;
static QSet<char> firstSetExprPrimary;
static QSet<char> firstSetFunctionType;
static QSet<char> firstSetLocalName;
static QSet<char> firstSetMangledName;
static QSet<char> firstSetName;
static QSet<char> firstSetNestedName;
static QSet<char> firstSetNonNegativeNumber;
static QSet<char> firstSetNumber;
static QSet<char> firstSetOperatorName;
static QSet<char> firstSetPointerToMemberType;
static QSet<char> firstSetPositiveNumber;
static QSet<char> firstSetPrefix;
static QSet<char> firstSetPrefix2;
static QSet<char> firstSetSeqId;
static QSet<char> firstSetSourceName;
static QSet<char> firstSetSpecialName;
static QSet<char> firstSetSubstitution;
static QSet<char> firstSetTemplateArg;
static QSet<char> firstSetTemplateArgs;
static QSet<char> firstSetTemplateParam;
static QSet<char> firstSetType;
static QSet<char> firstSetUnqualifiedName;
static QSet<char> firstSetUnscopedName;

namespace Debugger {
namespace Internal {
class ParseException
{
public:
    ParseException(const QString &error) : error(error) {}

    const QString error;
};

class NameDemanglerPrivate
{
public:
    NameDemanglerPrivate();
    ~NameDemanglerPrivate();

    bool demangle(const QString &mangledName);
    const QString &errorString() const { return m_errorString; }
    const QString &demangledName() const { return m_demangledName; }

private:
    class Operator
    {
    public:
        enum OpType { UnaryOp, BinaryOp, TernaryOp };
        Operator(const QByteArray &code, const QByteArray &repr)
            : code(code), repr(repr) { }
        virtual ~Operator() {}
        virtual const QByteArray makeExpr(const QList<QByteArray> &exprs) const=0;
        virtual OpType type() const=0;

        const QByteArray code;
        QByteArray repr;
    };

    class UnaryOperator : public Operator
    {
    public:
        UnaryOperator(const QByteArray &code, const QByteArray &repr)
            : Operator(code, repr) { }
        virtual const QByteArray makeExpr(const QList<QByteArray> &exprs) const
        {
            Q_ASSERT(exprs.size() == 1);
            return repr + exprs.first();
        }
        OpType type() const { return UnaryOp; }
    };

    class FunctionCallOperator : public UnaryOperator
    {
    public:
        FunctionCallOperator() : UnaryOperator("cl", "") { }
        const QByteArray makeExpr(const QList<QByteArray> &exprs) const
        {
            Q_ASSERT(exprs.size() == 1);
            return exprs.first() + "()";
        }
    };

    class SizeAlignOfOperator : public UnaryOperator
    {
    public:
        SizeAlignOfOperator(const QByteArray &code, const QByteArray &repr)
            : UnaryOperator(code, repr) { }
        const QByteArray makeExpr(const QList<QByteArray> &exprs) const
        {
            Q_ASSERT(exprs.size() == 1);
            return repr + '(' + exprs.first() + ')';
        }
    };

    class BinaryOperator : public Operator
    {
    public:
        BinaryOperator(const QByteArray &code, const QByteArray &repr)
            : Operator(code, repr) { }
        virtual const QByteArray makeExpr(const QList<QByteArray> &exprs) const
        {
            Q_ASSERT(exprs.size() == 2);
            return exprs.first() + ' ' + repr + ' ' + exprs.at(1);
        }
        OpType type() const { return BinaryOp; }
    };

    class ArrayNewOperator : public BinaryOperator
    {
    public:
        ArrayNewOperator() : BinaryOperator("na", "") { }

        const QByteArray makeExpr(const QList<QByteArray> &exprs) const
        {
            Q_ASSERT(exprs.size() == 2);
            return "new " + exprs.first() + '[' + exprs.at(1) + ']';
        }
    };

    class BinOpWithNoSpaces : public BinaryOperator
    {
    public:
        BinOpWithNoSpaces(const QByteArray &code, const QByteArray &repr)
            : BinaryOperator(code, repr) { }
        virtual const QByteArray makeExpr(const QList<QByteArray> &exprs) const
        {
            Q_ASSERT(exprs.size() == 2);
            return exprs.first() + repr + exprs.at(1);
        }
    };

    class ArrayAccessOperator : public BinaryOperator
    {
    public:
        ArrayAccessOperator() : BinaryOperator("ix", "") { }

        const QByteArray makeExpr(const QList<QByteArray> &exprs) const
        {
            Q_ASSERT(exprs.size() == 2);
            return exprs.first() + '[' + exprs.at(1) + ']';
        }
    };

    class QuestionMarkOperator : public Operator
    {
    public:
        QuestionMarkOperator() : Operator("qu", "") { }

        virtual const QByteArray makeExpr(const QList<QByteArray> &exprs) const
        {
            Q_ASSERT(exprs.size() == 3);
            return exprs.first() + " ? " + exprs.at(1) + " : " + exprs.at(2);
        }
        OpType type() const { return TernaryOp; }
    };

    static void setupFirstSets();
    static void setupOps();

    char peek(int ahead = 0);
    char advance(int steps = 1);
    const QByteArray readAhead(int charCount);

    void addSubstitution(const QByteArray &symbol);

    /*
     * One parse function per Non-terminal.
     * The functions return their unmangled representation, except where noted.
     */
    const QByteArray parseArrayType();

    /*
     * Returns the list of argument types, the first of which may be
     * the return type, depending on context.
     */
    const QList<QByteArray> parseBareFunctionType();

    const QByteArray parseBuiltinType();
    void parseCallOffset();
    const QByteArray parseClassEnumType();
    const QByteArray parseCtorDtorName();
    const QByteArray parseCvQualifiers();
    int parseDigit();
    int parseDiscriminator();
    const QByteArray parseEncoding();
    const QByteArray parseExpression();
    const QByteArray parseExprPrimary();
    double parseFloat();
    const QByteArray parseFunctionType();
    const QByteArray parseIdentifier(int len);
    const QByteArray parseLocalName();
    const QByteArray parseMangledName();
    const QByteArray parseName();
    const QByteArray parseNestedName();
    int parseNonNegativeNumber(int base = 10);
    int parseNumber();
    void parseNvOffset();
    const Operator &parseOperatorName();
    const QByteArray parsePointerToMemberType();
    const QByteArray parsePrefix();
    const QByteArray parsePrefix2(const QByteArray &oldPrefix);
    int parseSeqId();
    const QByteArray parseSpecialName();
    const QByteArray parseSourceName();
    const QByteArray parseSubstitution();
    const QByteArray parseTemplateArg();
    const QByteArray parseTemplateArgs();
    const QByteArray parseTemplateParam();
    const QByteArray parseType();
    const QByteArray parseUnqualifiedName();
    const QByteArray parseUnscopedName();
    void parseVOffset();

    void insertQualifier(QByteArray &type, const QByteArray &qualifier);

    static const char eoi;
    static bool staticInitializationsDone;
    static QMap<QByteArray, Operator *> ops;

    int m_pos;
    QByteArray m_mangledName;
    QString m_errorString;
    QString m_demangledName;
    QList<QByteArray> m_substitutions;
    QList<QByteArray> m_templateParams;
};


const char NameDemanglerPrivate::eoi('$');
bool NameDemanglerPrivate::staticInitializationsDone = false;
QMap<QByteArray, NameDemanglerPrivate::Operator *> NameDemanglerPrivate::ops;

NameDemanglerPrivate::NameDemanglerPrivate()
{
    // Not thread safe. We can easily add a mutex if that ever becomes a requirement.
    if (staticInitializationsDone)
        return;

    setupFirstSets();
    setupOps();
    staticInitializationsDone = true;
}

NameDemanglerPrivate::~NameDemanglerPrivate()
{
    qDeleteAll(ops);
}

bool NameDemanglerPrivate::demangle(const QString &mangledName)
{
    try {
        m_mangledName = mangledName.toAscii();
        m_pos = 0;
        m_demangledName.clear();
        m_substitutions.clear();
        m_templateParams.clear();
        m_demangledName = parseMangledName();
        m_demangledName.replace(
                    QRegExp(QLatin1String("([^a-zA-Z\\d>)])::")), QLatin1String("\\1"));
        if (m_demangledName.startsWith(QLatin1String("::")))
            m_demangledName.remove(0, 2);
        if (m_pos != m_mangledName.size())
            throw ParseException(QLatin1String("Unconsumed input"));
#ifdef DO_TRACE
        qDebug("%d", substitutions.size());
        foreach (const QByteArray &s, substitutions)
            qDebug(qPrintable(s));
#endif
        return true;
    } catch (const ParseException &p) {
        m_errorString = QString::fromLocal8Bit("Parse error at index %1 of mangled name '%2': %3.")
                .arg(m_pos).arg(mangledName, p.error);
        return false;
    }
}

/*
 * Grammar: http://www.codesourcery.com/public/cxx-abi/abi.html#mangling
 * The grammar as given there is not LL(k), so a number of transformations
 * were necessary, which we will document at the respective parsing function.
 * <mangled-name> ::= _Z <encoding>
 */
const QByteArray NameDemanglerPrivate::parseMangledName()
{
    FUNC_START();
    QByteArray name;
    if (readAhead(2) != "_Z") {
        name = m_mangledName;
        advance(m_mangledName.size());
    } else {
        advance(2);
        name = parseEncoding();
    }
    FUNC_END(name);
    return name;
}

/*
 * <encoding> ::= <name> <bare-function-type>
 *            ::= <name>
 *            ::= <special-name
 */
const QByteArray NameDemanglerPrivate::parseEncoding()
{
    FUNC_START();
    Q_ASSERT((firstSetName & firstSetSpecialName).isEmpty());

    QByteArray encoding;
    char next = peek();
    if (firstSetName.contains(next)) {
        encoding = parseName();
        if (firstSetBareFunctionType.contains(peek())) {
           /*
            * A member function might have qualifiers attached to its name,
            * which must be moved all the way to the back of the declaration.
            */
            QByteArray qualifiers;
            int qualifiersIndex = encoding.indexOf('@');
            if (qualifiersIndex != -1) {
                qualifiers = encoding.mid(qualifiersIndex + 1);
                encoding.truncate(qualifiersIndex);
            }

            const QList<QByteArray> &signature = parseBareFunctionType();

            /*
             * Instantiations of function templates encode their return type,
             * normal functions do not. Also, constructors, destructors
             * and conversion operators never encode their return type.
             * TODO: The latter are probably not handled yet.
             */
            int start;
            if (encoding.endsWith('>')) { // Template instantiation.
                start = 1;
                encoding.prepend(signature.first() + ' ');
            } else {                              // Normal function.
                start = 0;
            }
            encoding += '(';
            for (int i = start; i < signature.size(); ++i) {
                if (i > start)
                    encoding += ", ";
                const QByteArray &type = signature.at(i);
                if (type != "void")
                    encoding += type;
            }
            encoding += ')';
            encoding += qualifiers;
            addSubstitution(encoding);
        } else {
            addSubstitution(encoding);
        }
        m_templateParams.clear();
    } else if (firstSetSpecialName.contains(next)) {
        encoding = parseSpecialName();
    } else {
        throw ParseException(QString::fromLatin1("Invalid encoding"));
    }

    FUNC_END(encoding);
    return encoding;
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
const QByteArray NameDemanglerPrivate::parseName()
{
    FUNC_START();
    Q_ASSERT((firstSetNestedName & firstSetUnscopedName).isEmpty());
    Q_ASSERT((firstSetNestedName & firstSetSubstitution).isEmpty());
    Q_ASSERT((firstSetNestedName & firstSetLocalName).isEmpty());
    Q_ASSERT((firstSetUnscopedName & firstSetSubstitution).size() == 1); // 'S'
    Q_ASSERT((firstSetUnscopedName & firstSetLocalName).isEmpty());
    Q_ASSERT((firstSetSubstitution & firstSetLocalName).isEmpty());

    QByteArray name;
    if ((readAhead(2) == "St" && firstSetUnqualifiedName.contains(peek(2)))
        || firstSetUnscopedName.contains(peek())) {
        name = parseUnscopedName();
        if (firstSetTemplateArgs.contains(peek())) {
            addSubstitution(name);
            name += parseTemplateArgs();
        }
    } else {
        const char next = peek();
        if (firstSetNestedName.contains(next))
            name = parseNestedName();
        else if (firstSetSubstitution.contains(next))
            name = parseSubstitution() + parseTemplateArgs();
        else if (firstSetLocalName.contains(next))
            name = parseLocalName();
        else
            throw ParseException(QString::fromLatin1("Invalid name"));
    }

    FUNC_END(name);
    return name;
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
const QByteArray NameDemanglerPrivate::parseNestedName()
{
    FUNC_START();
    Q_ASSERT((firstSetCvQualifiers & firstSetPrefix).size() == 1);

    QByteArray name;
    if (advance() != 'N')
        throw ParseException(QString::fromLatin1("Invalid nested-name"));

    QByteArray cvQualifiers;
    if (firstSetCvQualifiers.contains(peek()) && peek(1) != 'm'
            && peek(1) != 'M' && peek(1) != 's' && peek(1) != 'S') {
        cvQualifiers = parseCvQualifiers();
    }
    name = parsePrefix();
    if (advance() != 'E')
        throw ParseException(QString::fromLatin1("Invalid nested-name"));

    /*
     * These are member function qualifiers which will have to
     * be moved to the back of the whole declaration later on,
     * so we mark them with the '@' character to ne able to easily
     * spot them.
     */
    if (!cvQualifiers.isEmpty())
        name += '@' + cvQualifiers;

    FUNC_END(name);
    return name;
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
const QByteArray NameDemanglerPrivate::parsePrefix()
{
    FUNC_START();
    Q_ASSERT((firstSetTemplateParam & firstSetSubstitution).isEmpty());
    Q_ASSERT((firstSetTemplateArgs & firstSetPrefix2).isEmpty());
    Q_ASSERT((firstSetTemplateParam & firstSetPrefix2).isEmpty());
    Q_ASSERT((firstSetSubstitution & firstSetPrefix2).isEmpty());

    QByteArray prefix;
    char next = peek();
    if (firstSetTemplateParam.contains(next)) {
        prefix = parseTemplateParam();
        if (firstSetTemplateArgs.contains(peek())) {
            addSubstitution(prefix);
            prefix += parseTemplateArgs();
        }
        if (firstSetUnqualifiedName.contains(peek())) {
            addSubstitution(prefix);
            prefix = parsePrefix2(prefix);
        }
    } else if (firstSetSubstitution.contains(next)) {
        prefix = parseSubstitution();
        QByteArray templateArgs;
        if (firstSetTemplateArgs.contains(peek())) {
            templateArgs = parseTemplateArgs();
            prefix += templateArgs;
        }
        if (firstSetUnqualifiedName.contains(peek()) && !templateArgs.isEmpty())
            addSubstitution(prefix);
        prefix = parsePrefix2(prefix);
    } else {
        prefix = parsePrefix2(prefix);
    }

    FUNC_END(prefix);
    return prefix;
}

/*
 * <prefix-2> ::= <unqualified-name> [<template-args>] <prefix-2>
 *            ::=  # empty
 */
const QByteArray NameDemanglerPrivate::parsePrefix2(const QByteArray &oldPrefix)
{
    FUNC_START();
    Q_ASSERT((firstSetTemplateArgs & firstSetPrefix2).isEmpty());

    QByteArray prefix = oldPrefix;
    bool firstRun = true;
    while (firstSetUnqualifiedName.contains(peek())) {
        if (!firstRun)
            addSubstitution(prefix);
        prefix += parseUnqualifiedName();
        if (firstSetTemplateArgs.contains(peek())) {
            addSubstitution(prefix);
            prefix += parseTemplateArgs();
        }
        firstRun = false;
    }

    FUNC_END(prefix);
    return prefix;
}

/*
 * <template-args> ::= I <template-arg>+ E
 */
const QByteArray NameDemanglerPrivate::parseTemplateArgs()
{
    FUNC_START();
    Q_ASSERT(!firstSetTemplateArg.contains('E'));

    QByteArray args = "<";
    if (advance() != 'I')
        throw ParseException(QString::fromLatin1("Invalid template args"));

    do {
        if (args.length() > 1)
            args += ", ";
        args += parseTemplateArg();
    } while (firstSetTemplateArg.contains(peek()));

    if (advance() != 'E')
        throw ParseException(QString::fromLatin1("Invalid template args"));

    args += '>';
    FUNC_END(args);
    return args;
}

/*
 * <template-param> ::= T_      # first template parameter
 *                  ::= T <non-negative-number> _
 */
const QByteArray NameDemanglerPrivate::parseTemplateParam()
{
    FUNC_START();

    QByteArray param;
    if (advance() != 'T')
        throw ParseException(QString::fromLatin1("Invalid template-param"));

    int index;
    if (peek() == '_')
        index = 0;
    else
        index = parseNonNegativeNumber() + 1;
    if (advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid template-param"));
    param = m_templateParams.at(index);

    FUNC_END(param);
    return param;
}

/*  <CV-qualifiers> ::= [r] [V] [K] 	# restrict (C99), volatile, const */
const QByteArray NameDemanglerPrivate::parseCvQualifiers()
{
    FUNC_START();

    QByteArray qualifiers;
    bool volatileFound = false;
    bool constFound = false;
    while (true) {
        if (peek() == 'V') {
            if (volatileFound || constFound)
                throw ParseException(QLatin1String("Invalid qualifiers: unexpected 'volatile'"));
            volatileFound = true;
            qualifiers += " volatile";
            advance();
        } else if (peek() == 'K') {
            if (constFound)
                throw ParseException(QLatin1String("Invalid qualifiers: 'const' appears twice"));
            constFound = true;
            qualifiers += " const";
            advance();
        } else {
            break;
        }
    }

    FUNC_END(qualifiers);
    return qualifiers;
}

int NameDemanglerPrivate::parseNumber()
{
    FUNC_START();

    bool negative = false;
    if (peek() == 'n') {
        negative = true;
        advance();
    }
    int val = parseNonNegativeNumber();
    int number = negative ? -val : val;

    FUNC_END(QString::number(number));
    return number;
}


int NameDemanglerPrivate::parseNonNegativeNumber(int base)
{
    FUNC_START();

    int startPos = m_pos;
    while (std::isdigit(peek()))
        advance();
    if (m_pos == startPos)
        throw ParseException(QString::fromLatin1("Invalid non-negative number"));
    const int number = m_mangledName.mid(startPos, m_pos - startPos).toInt(0, base);

    FUNC_END(QString::number(number));
    return number;
}

/*
 * Floating-point literals are encoded using a fixed-length lowercase
 * hexadecimal string corresponding to the internal representation
 * (IEEE on Itanium), high-order bytes first, without leading zeroes.
 * For example: "Lf bf800000 E" is -1.0f on Itanium.
 */
double NameDemanglerPrivate::parseFloat()
{
    FUNC_START();

    // TODO: Implementation!
    Q_ASSERT(0);

    FUNC_END(QString());
    return 0.0;
}

/*
 * <template-arg> ::= <type>                    # type or template
 *                ::= X <expression> E          # expression
 *                ::= <expr-primary>            # simple expressions
 *                ::= I <template-arg>* E       # argument pack
 *                ::= sp <expression>           # pack expansion of (C++0x)
 */
const QByteArray NameDemanglerPrivate::parseTemplateArg()
{
    FUNC_START();
    Q_ASSERT(!firstSetType.contains('X') && !firstSetType.contains('I')
             /* && !firstSetType.contains('s') */);
    Q_ASSERT((firstSetType & firstSetExprPrimary).isEmpty());
    Q_ASSERT(!firstSetExprPrimary.contains('X')
             && !firstSetExprPrimary.contains('I')
             && !firstSetExprPrimary.contains('s'));
    Q_ASSERT(!firstSetTemplateArg.contains('E'));

    QByteArray arg;
    char next = peek();
    if (readAhead(2) == "sp") {
        advance(2);
        arg = parseExpression();
    } else if (firstSetType.contains(next)) {
        arg = parseType();
    } else if (firstSetExprPrimary.contains(next)) {
        arg = parseExprPrimary();
    } else if (next == 'X') {
        advance();
        arg = parseExpression();
        if (advance() != 'E')
            throw ParseException(QString::fromLatin1("Invalid template-arg"));
    } else if (next == 'I') {
        advance();
        while (firstSetTemplateArg.contains(peek())) {
            if (!arg.isEmpty())
                arg += ", ";  // TODO: is this correct?
            arg += parseTemplateArg();
        }
        if (advance() != 'E')
            throw ParseException(QString::fromLatin1("Invalid template-arg"));
    } else {
        throw ParseException(QString::fromLatin1("Invalid template-arg"));
    }

    m_templateParams.append(arg);
    FUNC_END(arg);
    return arg;
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
 */
const QByteArray NameDemanglerPrivate::parseExpression()
{
    FUNC_START();
    Q_ASSERT((firstSetOperatorName & firstSetTemplateParam).isEmpty());
//    Q_ASSERT((firstSetOperatorName & firstSetFunctionParam).isEmpty());
    Q_ASSERT((firstSetOperatorName & firstSetExprPrimary).isEmpty());
//    Q_ASSERT((firstSetTemplateParam & firstSetFunctionParam).isEmpty());
    Q_ASSERT((firstSetTemplateParam & firstSetExprPrimary).isEmpty());
    Q_ASSERT(!firstSetTemplateParam.contains('c')
             && !firstSetTemplateParam.contains('s')
             && !firstSetTemplateParam.contains('a'));
//    Q_ASSERT((firstSetFunctionParam & firstSetExprPrimary).isEmpty());
/*
    Q_ASSERT(!firstSetFunctionParam.contains('c')
             && !firstSetFunctionParam.contains('s')
             && !firstSetFunctionParam.contains('a'));
*/
    Q_ASSERT(!firstSetExprPrimary.contains('c')
             && !firstSetExprPrimary.contains('s')
             && !firstSetExprPrimary.contains('a'));
    Q_ASSERT(!firstSetExpression.contains('E'));
    Q_ASSERT(!firstSetExpression.contains('_'));

    QByteArray expr;

   /*
    * Some of the terminals in the productions of <expression>
    * also appear in the productions of operator-name. We assume the direct
    * productions to have higher precedence and check them first to prevent
    * them being parsed by parseOperatorName().
    */
    QByteArray str = readAhead(2);
    if (str == "cl") {
        advance(2);
        while (firstSetExpression.contains(peek()))
            expr += parseExpression();
        if (advance() != 'E')
            throw ParseException(QString::fromLatin1("Invalid expression"));
    } else if (str == "cv") {
        advance(2);
        expr = parseType() + '(';
        if (peek() == '_') {
            advance();
            for (int numArgs = 0; firstSetExpression.contains(peek()); ++numArgs) {
                if (numArgs > 0)
                    expr += ", ";
                expr += parseExpression();
            }
            if (advance() != 'E')
                throw ParseException(QString::fromLatin1("Invalid expression"));
        } else {
            expr += parseExpression();
        }
        expr += ')';
    } else if (str == "st") {
        advance(2);
        expr = "sizeof(" + parseType() + ')';
    } else if (str == "at") {
        advance(2);
        expr = "alignof(" + parseType() + ')';
    } else if (str == "sr") { // TODO: Which syntax to use here?
        advance(2);
        expr = parseType() + parseUnqualifiedName();
        if (firstSetTemplateArgs.contains(peek()))
            parseTemplateArgs();
    } else if (str == "sZ") {
        expr = parseTemplateParam(); // TODO: Syntax?
    } else {
        char next = peek();
        if (firstSetOperatorName.contains(next)) {
            const Operator &op = parseOperatorName();
            QList<QByteArray> exprs;
            exprs.append(parseExpression());
            if (op.type() != Operator::UnaryOp)
                exprs.append(parseExpression());
            if (op.type() == Operator::TernaryOp)
                exprs.append(parseExpression());
            expr = op.makeExpr(exprs);
        } else if (firstSetTemplateParam.contains(next)) {
            expr = parseTemplateParam();
#if 0
        } else if (firstSetFunctionParam.contains(next)) {
            expr = parseFunctionParam();
#endif
        } else if (firstSetExprPrimary.contains(next)) {
            expr = parseExprPrimary();
        } else {
            throw ParseException(QString::fromLatin1("Invalid expression"));
        }
    }

    FUNC_END(expr);
    return expr;
}

/*
 * <expr-primary> ::= L <type> <number> E            # integer literal
 *                ::= L <type> <float> E             # floating literal
 *                ::= L <mangled-name> E             # external name
 */
const QByteArray NameDemanglerPrivate::parseExprPrimary()
{
    FUNC_START();
    Q_ASSERT((firstSetType & firstSetMangledName).isEmpty());

    QByteArray expr;
    if (advance() != 'L')
        throw ParseException(QString::fromLatin1("Invalid primary expression"));
    const char next = peek();
    if (firstSetType.contains(next)) {
        const QByteArray type = parseType();
        if (true /* type just parsed indicates integer */)
            expr += QByteArray::number(parseNumber());
        else if (true /* type just parsed indicates float */)
            expr += QByteArray::number(parseFloat());
        else
            throw ParseException(QString::fromLatin1("Invalid expr-primary"));
    } else if (firstSetMangledName.contains(next)) {
        expr = parseMangledName();
    } else {
        throw ParseException(QString::fromLatin1("Invalid expr-primary"));
    }
    if (advance() != 'E')
        throw ParseException(QString::fromLatin1("Invalid expr-primary"));

    FUNC_END(expr);
    return expr;
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
const QByteArray NameDemanglerPrivate::parseType()
{
    FUNC_START();
    Q_ASSERT((firstSetBuiltinType & firstSetFunctionType).isEmpty());
    Q_ASSERT((firstSetBuiltinType & firstSetClassEnumType).size() == 1);
    Q_ASSERT((firstSetBuiltinType & firstSetArrayType).isEmpty());
    Q_ASSERT((firstSetBuiltinType & firstSetPointerToMemberType).isEmpty());
    Q_ASSERT((firstSetBuiltinType & firstSetTemplateParam).isEmpty());
    Q_ASSERT((firstSetBuiltinType & firstSetSubstitution).isEmpty());
    Q_ASSERT((firstSetBuiltinType & firstSetCvQualifiers).isEmpty());
    Q_ASSERT(!firstSetBuiltinType.contains('P')
             && !firstSetBuiltinType.contains('R')
             && !firstSetBuiltinType.contains('O')
             && !firstSetBuiltinType.contains('C')
             && !firstSetBuiltinType.contains('G')
             && !firstSetBuiltinType.contains('U'));
    Q_ASSERT((firstSetFunctionType & firstSetClassEnumType).isEmpty());
    Q_ASSERT((firstSetFunctionType & firstSetArrayType).isEmpty());
    Q_ASSERT((firstSetFunctionType & firstSetPointerToMemberType).isEmpty());
    Q_ASSERT((firstSetFunctionType & firstSetTemplateParam).isEmpty());
    Q_ASSERT((firstSetFunctionType & firstSetSubstitution).isEmpty());
    Q_ASSERT((firstSetFunctionType & firstSetCvQualifiers).isEmpty());
    Q_ASSERT(!firstSetFunctionType.contains('P')
             && !firstSetFunctionType.contains('R')
             && !firstSetFunctionType.contains('O')
             && !firstSetFunctionType.contains('C')
             && !firstSetFunctionType.contains('G')
             && !firstSetFunctionType.contains('U')
             && !firstSetFunctionType.contains('D'));
    Q_ASSERT((firstSetClassEnumType & firstSetArrayType).isEmpty());
    Q_ASSERT((firstSetClassEnumType & firstSetPointerToMemberType).isEmpty());
    Q_ASSERT((firstSetClassEnumType & firstSetTemplateParam).isEmpty());
    Q_ASSERT((firstSetClassEnumType & firstSetSubstitution).isEmpty());
    Q_ASSERT((firstSetClassEnumType & firstSetCvQualifiers).isEmpty());
    Q_ASSERT(!firstSetClassEnumType.contains('P')
             && !firstSetClassEnumType.contains('R')
             && !firstSetClassEnumType.contains('O')
             && !firstSetClassEnumType.contains('C')
             && !firstSetClassEnumType.contains('G')
             && !firstSetClassEnumType.contains('U')
             /* && !firstSetClassEnumType.contains('D') */);
    Q_ASSERT((firstSetArrayType & firstSetPointerToMemberType).isEmpty());
    Q_ASSERT((firstSetArrayType & firstSetTemplateParam).isEmpty());
    Q_ASSERT((firstSetArrayType & firstSetSubstitution).isEmpty());
    Q_ASSERT((firstSetArrayType & firstSetCvQualifiers).isEmpty());
    Q_ASSERT(!firstSetArrayType.contains('P')
             && !firstSetArrayType.contains('R')
             && !firstSetArrayType.contains('O')
             && !firstSetArrayType.contains('C')
             && !firstSetArrayType.contains('G')
             && !firstSetArrayType.contains('U')
             && !firstSetArrayType.contains('D'));
    Q_ASSERT((firstSetPointerToMemberType & firstSetTemplateParam).isEmpty());
    Q_ASSERT((firstSetPointerToMemberType & firstSetSubstitution).isEmpty());
    Q_ASSERT((firstSetPointerToMemberType & firstSetCvQualifiers).isEmpty());
    Q_ASSERT(!firstSetPointerToMemberType.contains('P')
             && !firstSetPointerToMemberType.contains('R')
             && !firstSetPointerToMemberType.contains('O')
             && !firstSetPointerToMemberType.contains('C')
             && !firstSetPointerToMemberType.contains('G')
             && !firstSetPointerToMemberType.contains('U')
             && !firstSetPointerToMemberType.contains('D'));
    Q_ASSERT((firstSetTemplateParam & firstSetSubstitution).isEmpty());
    Q_ASSERT((firstSetTemplateParam & firstSetCvQualifiers).isEmpty());
    Q_ASSERT(!firstSetTemplateParam.contains('P')
             && !firstSetTemplateParam.contains('R')
             && !firstSetTemplateParam.contains('O')
             && !firstSetTemplateParam.contains('C')
             && !firstSetTemplateParam.contains('G')
             && !firstSetTemplateParam.contains('U')
             && !firstSetTemplateParam.contains('D'));
    Q_ASSERT((firstSetSubstitution & firstSetCvQualifiers).isEmpty());
    Q_ASSERT(!firstSetSubstitution.contains('P')
             && !firstSetSubstitution.contains('R')
             && !firstSetSubstitution.contains('O')
             && !firstSetSubstitution.contains('C')
             && !firstSetSubstitution.contains('G')
             && !firstSetSubstitution.contains('U')
             && !firstSetSubstitution.contains('D'));
    Q_ASSERT(!firstSetCvQualifiers.contains('P')
             && !firstSetCvQualifiers.contains('R')
             && !firstSetCvQualifiers.contains('O')
             && !firstSetCvQualifiers.contains('C')
             && !firstSetCvQualifiers.contains('G')
             && !firstSetCvQualifiers.contains('U')
             && !firstSetCvQualifiers.contains('D'));

    QByteArray type;
    QByteArray str = readAhead(2);
    if (str == "Dp") {
        advance(2);
        type = parseType(); // TODO: Probably needs augmentation
    } else if (str == "Dt") {
        advance(2);
        type = parseExpression();  // TODO: See above
        if (advance() != 'E')
            throw ParseException(QString::fromLatin1("Invalid type"));
    } else if (str == "DT") {
        advance(2);
        type = parseExpression(); // TODO: See above
        if (advance() != 'E')
            throw ParseException(QString::fromLatin1("Invalid type"));
    } else {
        char next = peek();
        if (str == "Dd" || str == "De" || str == "Df" || str == "Dh" || str == "Di" || str == "Ds"
            || (next != 'D' && firstSetBuiltinType.contains(next))) {
            type = parseBuiltinType();
        } else if (firstSetFunctionType.contains(next)) {
            type = parseFunctionType();
            addSubstitution(type);
        } else if (firstSetClassEnumType.contains(next)) {
            type = parseClassEnumType();
            addSubstitution(type);
        } else if (firstSetArrayType.contains(next)) {
            type = parseArrayType();
        } else if (firstSetPointerToMemberType.contains(next)) {
            type = parsePointerToMemberType();
        } else if (firstSetTemplateParam.contains(next)) {
            type = parseTemplateParam();
            addSubstitution(type);
            if (firstSetTemplateArgs.contains(peek())) {
                type += parseTemplateArgs();
                addSubstitution(type);
            }
        } else if (firstSetSubstitution.contains(next)) {
            type = parseSubstitution();
            if (firstSetTemplateArgs.contains(peek())) {
                type += parseTemplateArgs();
                addSubstitution(type);
            }
        } else if (firstSetCvQualifiers.contains(next)) {
            const QByteArray cvQualifiers = parseCvQualifiers();
            type = parseType();
            if (!cvQualifiers.isEmpty()) {
                insertQualifier(type, cvQualifiers);
                addSubstitution(type);
            }
        } else if (next == 'P') {
            advance();
            type = parseType();
            insertQualifier(type, "*");
            addSubstitution(type);
        } else if (next == 'R') {
            advance();
            type = parseType();
            insertQualifier(type, "&");
            addSubstitution(type);
        } else if (next == 'O') {
            advance();
            type = parseType() + "&&"; // TODO: Correct notation?
            addSubstitution(type);
        } else if (next == 'C') {
            advance();
            type = parseType(); // TODO: What to do append here?
            addSubstitution(type);
        } else if (next == 'G') {
            advance();
            type = parseType(); // TODO: see above
            addSubstitution(type);
        } else if (next == 'U') {
            advance();
            type = parseSourceName();
            type += parseType(); // TODO: handle this correctly
        } else {
            throw ParseException(QString::fromLatin1("Invalid type"));
        }
    }

    FUNC_END(type);
    return type;
}

/* <source-name> ::= <number> <identifier> */
const QByteArray NameDemanglerPrivate::parseSourceName()
{
    FUNC_START();

    const int idLen = parseNonNegativeNumber();
    const QByteArray sourceName = parseIdentifier(idLen);

    FUNC_END(sourceName);
    return sourceName;
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
const QByteArray NameDemanglerPrivate::parseBuiltinType()
{
    FUNC_START();

    QByteArray type;
    switch (advance()) {
    case 'v': type = "void"; break;
    case 'w': type = "wchar_t"; break;
    case 'b': type = "bool"; break;
    case 'c': type = "char"; break;
    case 'a': type = "signed char"; break;
    case 'h': type = "unsigned char"; break;
    case 's': type = "short"; break;
    case 't': type = "unsigned short"; break;
    case 'i': type = "int"; break;
    case 'j': type = "unsigned int"; break;
    case 'l': type = "long"; break;
    case 'm': type = "unsigned long"; break;
    case 'x': type = "long long"; break;
    case 'y': type = "unsigned long long"; break;
    case 'n': type = "__int128"; break;
    case 'o': type = "unsigned __int128"; break;
    case 'f': type = "float"; break;
    case 'd': type = "double"; break;
    case 'e': type = "long double"; break;
    case 'g': type = "__float128"; break;
    case 'z': type = "..."; break;
    case 'u': type = parseSourceName(); break;
    case 'D':
        switch (advance()) {
        case 'd': case 'e': case 'f': case 'h': type = "IEEE_special_float"; break;
        case 'i': type = "char32_t"; break;
        case 's': type = "char16_t"; break;
        default: throw ParseException(QString::fromLatin1("Invalid built-in type"));
        }
        break;
    default:
        throw ParseException(QString::fromLatin1("Invalid builtin-type"));
    }

    FUNC_END(type);
    return type;
}

/* <function-type> ::= F [Y] <bare-function-type> E */
const QByteArray NameDemanglerPrivate::parseFunctionType()
{
    FUNC_START();

    // TODO: Check how we get here. Do we always have a return type or not?
    if (advance() != 'F')
        throw ParseException(QString::fromLatin1("Invalid function type"));

    bool externC = false;
    if (peek() == 'Y') {
        advance();
        externC = true;
    }

    QByteArray funcType;
    const QList<QByteArray> &signature = parseBareFunctionType();
    if (advance() != 'E')
        throw ParseException(QString::fromLatin1("Invalid function type"));
    QByteArray returnType = signature.first();
    QByteArray argList = "(";
    for (int i = 1; i < signature.size(); ++i) {
        if (i > 1)
            argList.append(", ");
        const QByteArray &type = signature.at(i);
        if (type != "void")
            argList.append(type);
    }
    argList.append(')');
    bool retTypeIsFuncPtr = false;
    const int firstClosingParenIndex = returnType.indexOf(')');
    if (firstClosingParenIndex != -1) {
        const int firstOpeningParenIndex =
                returnType.lastIndexOf('(', firstClosingParenIndex);
        const char next = returnType[firstOpeningParenIndex + 1];
        if (next == '*' || next =='&') {
            retTypeIsFuncPtr = true;
            funcType = returnType.left(firstOpeningParenIndex + 2)
                    + argList + returnType.mid(firstOpeningParenIndex + 2);
        }
    }
    if (!retTypeIsFuncPtr)
        funcType = returnType + ' ' + argList;

    if (externC)
        funcType.prepend("extern \"C\" ");
    FUNC_END(funcType);
    return funcType;
}

/* <bare-function-type> ::= <type>+ */
const QList<QByteArray> NameDemanglerPrivate::parseBareFunctionType()
{
    FUNC_START();

    QList<QByteArray> signature;
    do
        signature.append(parseType());
    while (firstSetType.contains(peek()));

    FUNC_END(signature.join(':'));
    return signature;
}

/* <class-enum-type> ::= <name> */
const QByteArray NameDemanglerPrivate::parseClassEnumType()
{
    FUNC_START();
    const QByteArray &name = parseName();
    FUNC_END(name);
    return name;
}

/*
 * <unqualified-name> ::= <operator-name>
 *                    ::= <ctor-dtor-name>
 *                    ::= <source-name>
 */
const QByteArray NameDemanglerPrivate::parseUnqualifiedName()
{
    FUNC_START();
    Q_ASSERT((firstSetOperatorName & firstSetCtorDtorName).isEmpty());
    Q_ASSERT((firstSetOperatorName & firstSetSourceName).isEmpty());
    Q_ASSERT((firstSetCtorDtorName & firstSetSourceName).isEmpty());

    QByteArray name;
    char next = peek();
    if (firstSetOperatorName.contains(next))
        name = "::operator" + parseOperatorName().repr;
    else if (firstSetCtorDtorName.contains(next))
        name = "::" + parseCtorDtorName();
    else if (firstSetSourceName.contains(next))
        name = "::" + parseSourceName();
    else
        throw ParseException(QString::fromLatin1("Invalid unqualified-name"));

    FUNC_END(name);
    return name;
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
const NameDemanglerPrivate::Operator &NameDemanglerPrivate::parseOperatorName()
{
    FUNC_START();

    const Operator *op;
    if (peek() == 'v') {
        // TODO: Implement vendor-extended operators.
        static const UnaryOperator vendorOp("v",
                                            "[unimplemented]");
        advance();
        int numExprs = parseDigit();
        Q_UNUSED(numExprs);
        parseSourceName();
        op = &vendorOp;
    } else {
        const QByteArray id = readAhead(2);
        advance(2);
        if (id == "cv") {
            static UnaryOperator castOp("cv", "");
            QByteArray type = parseType();
            castOp.repr = '(' + type + ')';
            op = &castOp;
        } else {
            op = ops.value(id);
            if (op == 0) {
                static const UnaryOperator pseudoOp("invalid", "invalid");
                op = &pseudoOp;
                throw ParseException(QString::fromLatin1("Invalid operator-name '%s'")
                        .arg(QString::fromAscii(id)));
            }
        }
    }

    FUNC_END(op->repr);
    return *op;
}

/*
 * <array-type> ::= A <number> _ <type>
 *              ::= A [<expression>] _ <type>
 */
const QByteArray NameDemanglerPrivate::parseArrayType()
{
    FUNC_START();
    Q_ASSERT((firstSetNonNegativeNumber & firstSetExpression).isEmpty());
    Q_ASSERT(!firstSetNonNegativeNumber.contains('_'));
    Q_ASSERT(!firstSetExpression.contains('_'));

    QByteArray type;
    if (advance() != 'A')
        throw ParseException(QString::fromLatin1("Invalid array-type"));

    const char next = peek();
    QByteArray dimension;
    if (firstSetNonNegativeNumber.contains(next)) {
        dimension = QByteArray::number(parseNonNegativeNumber());
    } else if (firstSetExpression.contains(next)){
        dimension = parseExpression();
    }
    if (advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid array-type"));
    type = parseType() + '[' + dimension + ']';

    FUNC_END(type);
    return type;
}

/* <pointer-to-member-type> ::= M <type> <type> */
const QByteArray NameDemanglerPrivate::parsePointerToMemberType()
{
    FUNC_START();

    QByteArray type;
    if (advance() != 'M')
        throw ParseException(QString::fromLatin1("Invalid pointer-to-member-type"));

    const QByteArray classType = parseType();
    QByteArray memberType = parseType();
    if (memberType.contains(')')) { // Function?
        const int parenIndex = memberType.indexOf('(');
        QByteArray returnType = memberType.left(parenIndex);
        memberType.remove(0, parenIndex);
        type = returnType + '(' + classType + "::*)" + memberType;
    } else {
        type = memberType + ' ' + classType + "::*";
    }

    FUNC_END(type);
    return type;
}

/*
 * <substitution> ::= S <seq-id> _
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
const QByteArray NameDemanglerPrivate::parseSubstitution()
{
    FUNC_START();
    Q_ASSERT(!firstSetSeqId.contains('_') && !firstSetSeqId.contains('t')
             && !firstSetSeqId.contains('a') && !firstSetSeqId.contains('b')
             && !firstSetSeqId.contains('s') && !firstSetSeqId.contains('i')
             && !firstSetSeqId.contains('o') && !firstSetSeqId.contains('d'));

    QByteArray substitution;
    if (advance() != 'S')
        throw ParseException(QString::fromLatin1("Invalid substitution"));

    if (firstSetSeqId.contains(peek())) {
        int substIndex = parseSeqId() + 1;
        if (substIndex >= m_substitutions.size()) {
            throw ParseException(QString::fromLatin1("Invalid substitution: element %1 was requested, "
                     "but there are only %2").
                  arg(substIndex + 1).arg(m_substitutions.size()));
        }
        substitution = m_substitutions.at(substIndex);
        if (advance() != '_')
            throw ParseException(QString::fromLatin1("Invalid substitution"));
    } else {
        switch (advance()) {
        case '_':
            if (m_substitutions.isEmpty())
                throw ParseException(QString::fromLatin1("Invalid substitution: There are no elements"));
            substitution = m_substitutions.first();
            break;
        case 't':
            substitution = "::std::";
            break;
        case 'a':
            substitution = "::std::allocator";
            break;
        case 'b':
            substitution = "::std::basic_string";
            break;
        case 's':
            substitution = "::std::basic_string<char, ::std::char_traits<char>, "
                    "::std::allocator<char> >";
            break;
        case 'i':
            substitution = "::std::basic_istream<char, std::char_traits<char> >";
            break;
        case 'o':
            substitution = "::std::basic_ostream<char, std::char_traits<char> >";
            break;
        case 'd':
            substitution = "::std::basic_iostream<char, std::char_traits<char> >";
            break;
        default:
            throw ParseException(QString::fromLatin1("Invalid substitution"));
        }
    }

    FUNC_END(substitution);
    return substitution;
}


/*
 * The <seq-id> is a sequence number in base 36, using digits
 * and upper case letters.
 */
int NameDemanglerPrivate::parseSeqId()
{
    FUNC_START();
    int seqId = parseNonNegativeNumber(36);
    FUNC_END(QString::number(seqId));
    return seqId;
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
const QByteArray NameDemanglerPrivate::parseSpecialName()
{
    FUNC_START();
    Q_ASSERT(!firstSetCallOffset.contains('V')
             && !firstSetCallOffset.contains('T')
             && !firstSetCallOffset.contains('I')
             && !firstSetCallOffset.contains('S')
             && !firstSetCallOffset.contains('c'));

    QByteArray name;
    QByteArray str = readAhead(2);
    if (str == "TV") {
        advance(2);
        name = "[virtual table of " + parseType() + ']';
    } else if (str == "TT") {
        advance(2);
        name = "[VTT struct of " + parseType() + ']';
    } else if (str == "TI") {
        advance(2);
        name = "typeid(" + parseType() + ')';
    } else if (str == "TS") {
        advance(2);
        name = "typeid(" + parseType() + ").name()";
    } else if (str == "GV") {
        advance(2);
        name = "[guard variable of " + parseName() + ']';
    } else if (str == "Tc") {
        advance(2);
        parseCallOffset();
        parseCallOffset();
        parseEncoding();
    } else if (advance() == 'T') {
        parseCallOffset();
        parseEncoding();
    } else {
        throw ParseException(QString::fromLatin1("Invalid special-name"));
    }

    FUNC_END(name);
    return name;
}

/*
 * <unscoped-name> ::= <unqualified-name>
 *                 ::= St <unqualified-name>   # ::std::
 */
const QByteArray NameDemanglerPrivate::parseUnscopedName()
{
    FUNC_START();
    Q_ASSERT(!firstSetUnqualifiedName.contains('S'));

    QByteArray name;
    if (readAhead(2) == "St") {
        advance(2);
        name = "::std" + parseUnqualifiedName();
    } else if (firstSetUnqualifiedName.contains(peek())) {
        name = parseUnqualifiedName();
    } else {
        throw ParseException(QString::fromLatin1("Invalid unqualified-name"));
    }

    FUNC_END(name);
    return name;
}


/*
 * <local-name> := Z <encoding> E <name> [<discriminator>]
 *              := Z <encoding> E s [<discriminator>]
 *
 * Note that <name> can start with 's', so we need to to read-ahead.
 */
const QByteArray NameDemanglerPrivate::parseLocalName()
{
    FUNC_START();

    QByteArray name;
    if (advance() != 'Z')
        throw ParseException(QString::fromLatin1("Invalid local-name"));

    name = parseEncoding();
    if (advance() != 'E')
        throw ParseException(QString::fromLatin1("Invalid local-name"));

    QByteArray str = readAhead(2);
    char next = peek();
    if (str == "sp" || str == "sr" || str == "st" || str == "sz" || str == "sZ"
            || (next != 's' && firstSetName.contains(next)))
        name +=  parseName();
    else if (next == 's') {
        advance();
        name += "::\"string literal\"";
    } else {
        throw ParseException(QString::fromLatin1("Invalid local-name"));
    }
    if (firstSetDiscriminator.contains(peek()))
        parseDiscriminator();

    FUNC_END(name);
    return name;
}

/* <discriminator> := _ <non-negative-number> */
int NameDemanglerPrivate::parseDiscriminator()
{
    if (advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid discriminator"));

    const int index = parseNonNegativeNumber();

    FUNC_END(QString::number(index));
    return index;
}

/*
 * <ctor-dtor-name> ::= C1      # complete object constructor
 *                  ::= C2      # base object constructor
 *                  ::= C3      # complete object allocating constructor
 *                  ::= D0      # deleting destructor
 *                  ::= D1      # complete object destructor
 *                  ::= D2      # base object destructor
 */
const QByteArray NameDemanglerPrivate::parseCtorDtorName()
{
    FUNC_START();

    QByteArray name;
    bool destructor = false;
    switch (advance()) {
    case 'C':
        switch (advance()) {
        case '1': case '2': case '3': break;
        default: throw ParseException(QString::fromLatin1("Invalid ctor-dtor-name"));
        }
        break;
    case 'D':
        switch (advance()) {
        case '0': case '1': case '2': destructor = true; break;
        default: throw ParseException(QString::fromLatin1("Invalid ctor-dtor-name"));
        }
        break;
    default:
        throw ParseException(QString::fromLatin1("Invalid ctor-dtor-name"));
    }

    name = m_substitutions.last();
    int templateArgsStart = name.indexOf('<');
    if (templateArgsStart != -1)
        name.remove(templateArgsStart,
                    name.indexOf('>') - templateArgsStart + 1);
    int lastComponentStart = name.lastIndexOf("::");
    if (lastComponentStart != -1)
        name.remove(0, lastComponentStart + 2);
    if (destructor)
        name.prepend('~');

    FUNC_END(name);
    return name;
}

/* This will probably need the number of characters to read. */
const QByteArray NameDemanglerPrivate::parseIdentifier(int len)
{
    FUNC_START();

    const QByteArray id = m_mangledName.mid(m_pos, len);
    advance(len);

    FUNC_END(id);
    return id;
}

/*
 * <call-offset> ::= h <nv-offset> _
 *               ::= v <v-offset> _
 */
void NameDemanglerPrivate::parseCallOffset()
{
    FUNC_START();

    switch (advance()) {
    case 'h': parseNvOffset(); break;
    case 'v': parseVOffset(); break;
    default: throw ParseException(QString::fromLatin1("Invalid call-offset"));
    }
    if (advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid call-offset"));
    FUNC_END(QString());
}

/* <nv-offset> ::= <number> # non-virtual base override */
void NameDemanglerPrivate::parseNvOffset()
{
    FUNC_START();
    parseNumber();
    FUNC_END(QString());
}

/*
 * <v-offset> ::= <number> _ <number>
 *    # virtual base override, with vcall offset
 */
void NameDemanglerPrivate::parseVOffset()
{
    FUNC_START();

    parseNumber();
    if (advance() != '_')
        throw ParseException(QString::fromLatin1("Invalid v-offset"));
    parseNumber();

    FUNC_END(QString());
}

int NameDemanglerPrivate::parseDigit()
{
    FUNC_START();

    int digit = advance();
    if (!std::isdigit(digit))
        throw ParseException(QString::fromLatin1("Invalid digit"));
    digit -= 0x30;

    FUNC_END(QString::number(digit));
    return digit;
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

void NameDemanglerPrivate::setupFirstSets()
{
    firstSetMangledName << '_';
    firstSetNestedName << 'N';
    firstSetFunctionType << 'F';
    firstSetArrayType << 'A';
    firstSetPointerToMemberType << 'M';
    firstSetTemplateParam << 'T';
    firstSetSubstitution << 'S';
    firstSetLocalName << 'Z';
    firstSetTemplateArgs << 'I';
    firstSetExprPrimary << 'L';
    firstSetDiscriminator << '_';
    firstSetSpecialName << 'T' << 'G';
    firstSetCtorDtorName << 'C' << 'D';
    firstSetCallOffset << 'h' << 'v';
    firstSetCvQualifiers << 'K' << 'V' << 'r';
    firstSetOperatorName << 'n' << 'd' << 'p' << 'a' << 'c' << 'm' << 'r'
                         << 'o' << 'e' << 'l' << 'g' << 'i' << 'q' << 's'
                         << 'v';
    firstSetBuiltinType << 'v' << 'w' << 'b' << 'c' << 'a' << 'h' << 's' << 't'
                        << 'i' << 'j' << 'l' << 'm' << 'x' << 'y' << 'n' << 'o'
                        << 'f' << 'g' << 'e' << 'd' << 'z' << 'D' << 'u';
    firstSetPositiveNumber
        << '1' << '2' << '3' << '4' << '5' << '6' << '7' << '8' << '9';
    (firstSetNonNegativeNumber += firstSetPositiveNumber) << '0';
    (firstSetNumber += firstSetPositiveNumber) << 'n';
    firstSetSourceName = firstSetPositiveNumber;
    firstSetSeqId += firstSetNonNegativeNumber;
    for (char c = 'A'; c != 'Z'; ++c)
        firstSetSeqId << c;
    (((firstSetExpression += firstSetOperatorName) += firstSetTemplateParam)
     /* += firstSetFunctionParam */ += firstSetExprPrimary)
        << 'c' << 's' << 'a';
    firstSetUnqualifiedName = firstSetOperatorName | firstSetCtorDtorName
        | firstSetSourceName;
    firstSetPrefix2 = firstSetUnqualifiedName;
    firstSetPrefix = firstSetTemplateParam | firstSetSubstitution
        | firstSetPrefix2;
    firstSetUnscopedName = firstSetUnqualifiedName | (QSet<char>() << 'S');
    firstSetName = firstSetNestedName | firstSetUnscopedName
        | firstSetSubstitution | firstSetLocalName;

    /*
     * The first set of <class-enum-type> is much smaller than
     * the grammar claims.
     * firstSetClassEnumType = firstSetName;
     */
    (firstSetClassEnumType += firstSetNonNegativeNumber) << 'N' << 'D' << 'Z';

    firstSetEncoding = firstSetName | firstSetSpecialName;
    ((((((((firstSetType += firstSetBuiltinType) += firstSetFunctionType)
         += firstSetClassEnumType) += firstSetArrayType)
       += firstSetPointerToMemberType) += firstSetTemplateParam)
      += firstSetSubstitution) += firstSetCvQualifiers) << 'P' << 'R'
        << 'O' << 'C' << 'G' << 'U' << 'D';
    firstSetBareFunctionType = firstSetType;
    ((firstSetTemplateArg += firstSetType) += firstSetExprPrimary)
      << 'X' << 'I' << 's';

#if 0
    foreach (char c, firstSetType)
        qDebug("'%c'", c.toAscii());
    qDebug("\n");
    foreach (char c, firstSetExprPrimary)
        qDebug("'%c'", c.toAscii());
#endif
}

void NameDemanglerPrivate::setupOps()
{
    ops["nw"] = new UnaryOperator("nw", "new ");
    ops["na"] = new ArrayNewOperator;
    ops["dl"] = new UnaryOperator("dl", "delete ");
    ops["da"] = new UnaryOperator("da", "delete[] ");
    ops["ps"] = new UnaryOperator("ps", "+");
    ops["ng"] = new UnaryOperator("ng", "-");
    ops["ad"] = new UnaryOperator("ad", "&");
    ops["de"] = new UnaryOperator("de", "*");
    ops["co"] = new UnaryOperator("co", "~");
    ops["pl"] = new BinaryOperator("pl", "+");
    ops["mi"] = new BinaryOperator("mi", "-");
    ops["ml"] = new BinaryOperator("ml", "*");
    ops["dv"] = new BinaryOperator("dv", "/");
    ops["rm"] = new BinaryOperator("rm", "%");
    ops["an"] = new BinaryOperator("an", "&");
    ops["or"] = new BinaryOperator("or", "|");
    ops["eo"] = new BinaryOperator("eo", "^");
    ops["aS"] = new BinaryOperator("aS", "=");
    ops["pL"] = new BinaryOperator("pl", "+=");
    ops["mI"] = new BinaryOperator("mI", "-=");
    ops["mL"] = new BinaryOperator("mL", "*=");
    ops["dV"] = new BinaryOperator("dV", "/=");
    ops["rM"] = new BinaryOperator("rM", "%=");
    ops["aN"] = new BinaryOperator("aN", "&=");
    ops["oR"] = new BinaryOperator("oR", "|=");
    ops["eO"] = new BinaryOperator("eO", "^=");
    ops["ls"] = new BinaryOperator("ls", "<<");
    ops["rs"] = new BinaryOperator("rs", ">>");
    ops["lS"] = new BinaryOperator("lS", "<<=");
    ops["rS"] = new BinaryOperator("rS", ">>=");
    ops["eq"] = new BinaryOperator("eq", "==");
    ops["ne"] = new BinaryOperator("ne", "!=");
    ops["lt"] = new BinaryOperator("lt", "<");
    ops["gt"] = new BinaryOperator("gt", ">");
    ops["le"] = new BinaryOperator("le", "<=");
    ops["ge"] = new BinaryOperator("ge", ">=");
    ops["nt"] = new UnaryOperator("nt", "!");
    ops["aa"] = new BinaryOperator("aa", "&&");
    ops["oo"] = new BinaryOperator("oo", "||");
    ops["pp"] = new UnaryOperator("pp", "++"); // Prefix?
    ops["mm"] = new UnaryOperator("mm", "--"); // Prefix?
    ops["cm"] = new BinaryOperator("cm", ",");
    ops["pm"] = new BinOpWithNoSpaces("pm", "->*");
    ops["pt"] = new BinOpWithNoSpaces("pm", "->");
    ops["cl"] = new FunctionCallOperator;
    ops["ix"] = new ArrayAccessOperator;
    ops["qu"] = new QuestionMarkOperator;
    ops["st"] = new SizeAlignOfOperator("st", "sizeof");
    ops["sz"] = new SizeAlignOfOperator("sz", "sizeof");
    ops["at"] = new SizeAlignOfOperator("at", "alignof");
    ops["az"] = new SizeAlignOfOperator("az", "alignof");
}

void NameDemanglerPrivate::addSubstitution(const QByteArray &symbol)
{
    if (!symbol.isEmpty() && !m_substitutions.contains(symbol))
        m_substitutions.append(symbol);
}

void NameDemanglerPrivate::insertQualifier(QByteArray &type, const QByteArray &qualifier)
{
    int funcAnchor = -1;
    while (true) {
         funcAnchor = type.indexOf('(', funcAnchor + 1);
         if (funcAnchor == -1)
             break;
         if (funcAnchor == type.count() - 1) {
             funcAnchor = -1;
             break;
         }
         const int next = type.at(funcAnchor + 1);
         if (next != '*' && next != '&')
             break;
    }

    int qualAnchorCandidates[4];
    qualAnchorCandidates[0] = type.indexOf("*)");
    qualAnchorCandidates[1] = type.indexOf("&)");
    qualAnchorCandidates[2] = type.indexOf("const)");
    qualAnchorCandidates[3] = type.indexOf("volatile)");
    int qualAnchor = -1;
    for (std::size_t i = 0 ; i < sizeof qualAnchorCandidates/sizeof *qualAnchorCandidates; ++i) {
        const int candidate = qualAnchorCandidates[i];
        if (candidate == -1)
            continue;
        if (qualAnchor == -1)
            qualAnchor = candidate;
        else
            qualAnchor = qMin(qualAnchor, candidate);
    }

    int insertionPos;
    QByteArray insertionString = qualifier;
    if (funcAnchor == -1) {
        if (qualAnchor == -1) {
            insertionPos = type.size();
        } else {
            insertionPos = type.indexOf(')', qualAnchor);
        }
    } else if (qualAnchor == -1 || funcAnchor < qualAnchor) {
        if (qualifier == "*" || qualifier == "&") {
            insertionPos = funcAnchor;
            insertionString = '(' + qualifier + ')';
        } else {
            insertionPos = type.size(); // Qualifier for pointer to member.
        }
    } else  {
        insertionPos = type.indexOf(')', qualAnchor);
    }
    if ((insertionString == "*" || insertionString == "&")
        && (type[insertionPos - 1] != '*' && type[insertionPos - 1] != '&'))
        insertionString.prepend(' ');
    type.insert(insertionPos, insertionString);
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
