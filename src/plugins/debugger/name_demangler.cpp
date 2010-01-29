/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <QtCore/QChar>
#include <QtCore/QCoreApplication>
#include <QtCore/QLatin1String>
#include <QtCore/QMap>
#include <QtCore/QRegExp>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "name_demangler.h"

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

namespace Debugger {
namespace Internal {

class NameDemanglerPrivate
{
    Q_DECLARE_TR_FUNCTIONS(NameDemanglerPrivate)
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
        Operator(const QString &code, const QString &repr)
            : code(code), repr(repr) { }
        virtual ~Operator() {}
        virtual const QString makeExpr(const QStringList &exprs) const=0;
        virtual OpType type() const=0;

        const QString code;
        QString repr;
    };

    class UnaryOperator : public Operator
    {
    public:
        UnaryOperator(const QString &code, const QString &repr)
            : Operator(code, repr) { }
        virtual const QString makeExpr(const QStringList &exprs) const
        {
            Q_ASSERT(exprs.size() == 1);
            return repr + exprs.first();
        }
        OpType type() const { return UnaryOp; }
    };

    class FunctionCallOperator : public UnaryOperator
    {
    public:
        FunctionCallOperator() : UnaryOperator(QLatin1String("cl"),
                                               QLatin1String("")) { }
        const QString makeExpr(const QStringList &exprs) const
        {
            Q_ASSERT(exprs.size() == 1);
            return QString::fromLocal8Bit("%1()").arg(exprs.first());
        }
    };

    class SizeAlignOfOperator : public UnaryOperator
    {
    public:
        SizeAlignOfOperator(const QString &code, const QString &repr)
            : UnaryOperator(code, repr) { }
        const QString makeExpr(const QStringList &exprs) const
        {
            Q_ASSERT(exprs.size() == 1);
            return QString::fromLocal8Bit("%1(%2)").
                arg(repr).arg(exprs.first());
        }
    };

    class BinaryOperator : public Operator
    {
    public:
        BinaryOperator(const QString &code, const QString &repr)
            : Operator(code, repr) { }
        virtual const QString makeExpr(const QStringList &exprs) const
        {
            Q_ASSERT(exprs.size() == 2);
            return QString::fromLocal8Bit("%1 %2 %3").
                arg(exprs.first()).arg(repr).arg(exprs.at(1));
        }
        OpType type() const { return BinaryOp; }
    };

    class ArrayNewOperator : public BinaryOperator
    {
    public:
        ArrayNewOperator() : BinaryOperator(QLatin1String("na"),
                                            QLatin1String(""))
        {
        }

        const QString makeExpr(const QStringList &exprs) const
        {
            Q_ASSERT(exprs.size() == 2);
            return QString::fromLocal8Bit("new %1[%2]").
                arg(exprs.first()).arg(exprs.at(1));
        }
    };

    class BinOpWithNoSpaces : public BinaryOperator
    {
    public:
        BinOpWithNoSpaces(const QString &code, const QString &repr)
            : BinaryOperator(code, repr) { }
        virtual const QString makeExpr(const QStringList &exprs) const
        {
            Q_ASSERT(exprs.size() == 2);
            return exprs.first() + repr + exprs.at(1);
        }
    };

    class ArrayAccessOperator : public BinaryOperator
    {
    public:
        ArrayAccessOperator() : BinaryOperator(QLatin1String("ix"),
                                               QLatin1String(""))
        {
        }

        const QString makeExpr(const QStringList &exprs) const
        {
            Q_ASSERT(exprs.size() == 2);
            return QString::fromLocal8Bit("%1[%2]").
                arg(exprs.first()).arg(exprs.at(1));
        }
    };

    class QuestionMarkOperator : public Operator
    {
    public:
        QuestionMarkOperator() : Operator(QLatin1String("qu"),
                                          QLatin1String(""))
        {
        }

        virtual const QString makeExpr(const QStringList &exprs) const
        {
            Q_ASSERT(exprs.size() == 3);
            return QString::fromLocal8Bit("%1 ? %2 : %3").
                arg(exprs.first()).arg(exprs.at(1)).arg(exprs.at(2));
        }
        OpType type() const { return TernaryOp; }
    };

    void setupFirstSets();
    void setupOps();

    QChar peek(int ahead = 0);
    QChar advance(int steps = 1);
    const QString readAhead(int charCount);

    void addSubstitution(const QString &symbol);

    /*
     * One parse function per Non-terminal.
     * The functions return their unmangled representation, except where noted.
     */
    const QString parseArrayType();

    /*
     * Returns the list of argument types, the first of which may be
     * the return type, depending on context.
     */
    const QStringList parseBareFunctionType();

    const QString parseBuiltinType();
    void parseCallOffset();
    const QString parseClassEnumType();
    const QString parseCtorDtorName();
    const QString parseCvQualifiers();
    int parseDigit();
    int parseDiscriminator();
    const QString parseEncoding();
    const QString parseExpression();
    const QString parseExprPrimary();
    double parseFloat();
    const QString parseFunctionType();
    const QString parseIdentifier(int len);
    const QString parseLocalName();
    const QString parseMangledName();
    const QString parseName();
    const QString parseNestedName();
    int parseNonNegativeNumber(int base = 10);
    int parseNumber();
    void parseNvOffset();
    const Operator &parseOperatorName();
    const QString parsePointerToMemberType();
    const QString parsePrefix();
    const QString parsePrefix2(const QString &oldPrefix);
    int parseSeqId();
    const QString parseSpecialName();
    const QString parseSourceName();
    const QString parseSubstitution();
    const QString parseTemplateArg();
    const QString parseTemplateArgs();
    const QString parseTemplateParam();
    const QString parseType();
    const QString parseUnqualifiedName();
    const QString parseUnscopedName();
    void parseVOffset();

    void insertQualifier(QString &type, const QString &qualifier);

    void error(const QString &errorSpec);

    static const QChar eoi;
    bool parseError;
    int pos;
    QString mangledName;
    QString m_errorString;
    QString m_demangledName;
    QStringList substitutions;
    QStringList templateParams;

    QMap<QString, Operator *> ops;

    // The first-sets for all non-terminals.
    QSet<QChar> firstSetArrayType;
    QSet<QChar> firstSetBareFunctionType;
    QSet<QChar> firstSetBuiltinType;
    QSet<QChar> firstSetCallOffset;
    QSet<QChar> firstSetClassEnumType;
    QSet<QChar> firstSetDiscriminator;
    QSet<QChar> firstSetCtorDtorName;
    QSet<QChar> firstSetCvQualifiers;
    QSet<QChar> firstSetEncoding;
    QSet<QChar> firstSetExpression;
    QSet<QChar> firstSetExprPrimary;
    QSet<QChar> firstSetFunctionType;
    QSet<QChar> firstSetLocalName;
    QSet<QChar> firstSetMangledName;
    QSet<QChar> firstSetName;
    QSet<QChar> firstSetNestedName;
    QSet<QChar> firstSetNonNegativeNumber;
    QSet<QChar> firstSetNumber;
    QSet<QChar> firstSetOperatorName;
    QSet<QChar> firstSetPointerToMemberType;
    QSet<QChar> firstSetPositiveNumber;
    QSet<QChar> firstSetPrefix;
    QSet<QChar> firstSetPrefix2;
    QSet<QChar> firstSetSeqId;
    QSet<QChar> firstSetSourceName;
    QSet<QChar> firstSetSpecialName;
    QSet<QChar> firstSetSubstitution;
    QSet<QChar> firstSetTemplateArg;
    QSet<QChar> firstSetTemplateArgs;
    QSet<QChar> firstSetTemplateParam;
    QSet<QChar> firstSetType;
    QSet<QChar> firstSetUnqualifiedName;
    QSet<QChar> firstSetUnscopedName;
};


const QChar NameDemanglerPrivate::eoi('$');

NameDemanglerPrivate::NameDemanglerPrivate()
{
    setupFirstSets();
    setupOps();
}

NameDemanglerPrivate::~NameDemanglerPrivate()
{
    qDeleteAll(ops);
}

bool NameDemanglerPrivate::demangle(const QString &mangledName)
{
    this->mangledName = mangledName;
    pos = 0;
    parseError = false;
    m_demangledName.clear();
    substitutions.clear();
    templateParams.clear();
    m_demangledName = parseMangledName();
    m_demangledName.replace(
        QRegExp(QLatin1String("([^a-zA-Z\\d>)])::")), QLatin1String("\\1"));
    if (m_demangledName.startsWith(QLatin1String("::")))
        m_demangledName.remove(0, 2);
    if (!parseError && pos != mangledName.size())
        error(tr("Premature end of input"));

#ifdef DO_TRACE
    qDebug("%d", substitutions.size());
    foreach (QString s, substitutions)
        qDebug(qPrintable(s));
#endif
    return !parseError;
}

/*
 * Grammar: http://www.codesourcery.com/public/cxx-abi/abi.html#mangling
 * The grammar as given there is not LL(k), so a number of transformations
 * were necessary, which we will document at the respective parsing function.
 * <mangled-name> ::= _Z <encoding>
 */
const QString NameDemanglerPrivate::parseMangledName()
{
    FUNC_START();
    QString name;
    if (readAhead(2) != QLatin1String("_Z")) {
        name = mangledName;
        advance(mangledName.size());
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
const QString NameDemanglerPrivate::parseEncoding()
{
    FUNC_START();
    Q_ASSERT((firstSetName & firstSetSpecialName).isEmpty());

    QString encoding;
    QChar next = peek();
    if (firstSetName.contains(next)) {
        encoding = parseName();
        if (!parseError && firstSetBareFunctionType.contains(peek())) {
           /*
            * A member function might have qualifiers attached to its name,
            * which must be moved all the way to the back of the declaration.
            */
            QString qualifiers;
            int qualifiersIndex = encoding.indexOf('@');
            if (qualifiersIndex != -1) {
                qualifiers = encoding.mid(qualifiersIndex + 1);
                encoding.truncate(qualifiersIndex);
            }

            const QStringList &signature = parseBareFunctionType();

            /*
             * Instantiations of function templates encode their return type,
             * normal functions do not. Also, constructors, destructors
             * and conversion operators never encode their return type.
             * TODO: The latter are probably not handled yet.
             */
            int start;
            if (encoding.endsWith('>')) { // Template instantiation.
                start = 1;
                encoding.prepend(signature.first() + QLatin1String(" "));
            } else {                              // Normal function.
                start = 0;
            }
            encoding += QLatin1String("(");
            for (int i = start; i < signature.size(); ++i) {
                if (i > start)
                    encoding += QLatin1String(", ");
                const QString &type = signature.at(i);
                if (type != QLatin1String("void"))
                    encoding += type;
            }
            encoding += QLatin1String(")");
            encoding += qualifiers;
            addSubstitution(encoding);
        } else {
            addSubstitution(encoding);
        }
        templateParams.clear();
    } else if (firstSetSpecialName.contains(next)) {
        encoding = parseSpecialName();
    } else {
        error(tr("Invalid encoding"));
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
const QString NameDemanglerPrivate::parseName()
{
    FUNC_START();
    Q_ASSERT((firstSetNestedName & firstSetUnscopedName).isEmpty());
    Q_ASSERT((firstSetNestedName & firstSetSubstitution).isEmpty());
    Q_ASSERT((firstSetNestedName & firstSetLocalName).isEmpty());
    Q_ASSERT((firstSetUnscopedName & firstSetSubstitution).size() == 1); // 'S'
    Q_ASSERT((firstSetUnscopedName & firstSetLocalName).isEmpty());
    Q_ASSERT((firstSetSubstitution & firstSetLocalName).isEmpty());

    QString name;
    if ((readAhead(2) == QLatin1String("St")
         && firstSetUnqualifiedName.contains(peek(2)))
        || firstSetUnscopedName.contains(peek())) {
        name = parseUnscopedName();
        if (!parseError && firstSetTemplateArgs.contains(peek())) {
            addSubstitution(name);
            name += parseTemplateArgs();
        }
    } else {
        QChar next = peek();
        if (firstSetNestedName.contains(next)) {
            name = parseNestedName();
        } else if (firstSetSubstitution.contains(next)) {
            name = parseSubstitution();
            if (!parseError) {
                name += parseTemplateArgs();
            }
        } else if (firstSetLocalName.contains(next)) {
            name = parseLocalName();
        } else {
            error(tr("Invalid name"));
        }
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
const QString NameDemanglerPrivate::parseNestedName()
{
    FUNC_START();
    Q_ASSERT((firstSetCvQualifiers & firstSetPrefix).size() == 1);

    QString name;
    if (advance() != 'N') {
        error(tr("Invalid nested-name"));
    } else {
        QString cvQualifiers;
        if (firstSetCvQualifiers.contains(peek()) && peek(1) != 'm'
            && peek(1) != 'M' && peek(1) != 's' && peek(1) != 'S')
            cvQualifiers = parseCvQualifiers();
        if (!parseError) {
            name = parsePrefix();
            if (!parseError && advance() != 'E')
                error(tr("Invalid nested-name"));

            /*
             * These are member function qualifiers which will have to
             * be moved to the back of the whole declaration later on,
             * so we mark them with the '@' character to ne able to easily
             * spot them.
             */
            if (!cvQualifiers.isEmpty())
                name += QLatin1String("@") + cvQualifiers;
        }
    }

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
const QString NameDemanglerPrivate::parsePrefix()
{
    FUNC_START();
    Q_ASSERT((firstSetTemplateParam & firstSetSubstitution).isEmpty());
    Q_ASSERT((firstSetTemplateArgs & firstSetPrefix2).isEmpty());
    Q_ASSERT((firstSetTemplateParam & firstSetPrefix2).isEmpty());
    Q_ASSERT((firstSetSubstitution & firstSetPrefix2).isEmpty());

    QString prefix;
    QChar next = peek();
    if (firstSetTemplateParam.contains(next)) {
        prefix = parseTemplateParam();
        if (!parseError && firstSetTemplateArgs.contains(peek())) {
            addSubstitution(prefix);
            prefix += parseTemplateArgs();
        }
        if (!parseError) {
            if (firstSetUnqualifiedName.contains(peek())) {
                addSubstitution(prefix);
                prefix = parsePrefix2(prefix);
            }
        }
    } else if (firstSetSubstitution.contains(next)) {
        prefix = parseSubstitution();
        QString templateArgs;
        if (!parseError && firstSetTemplateArgs.contains(peek())) {
            templateArgs = parseTemplateArgs();
            prefix += templateArgs;
        }
        if (!parseError) {
            if (firstSetUnqualifiedName.contains(peek()) &&
                !templateArgs.isEmpty())
                addSubstitution(prefix);
            prefix = parsePrefix2(prefix);
        }
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
const QString NameDemanglerPrivate::parsePrefix2(const QString &oldPrefix)
{
    FUNC_START();
    Q_ASSERT((firstSetTemplateArgs & firstSetPrefix2).isEmpty());

    QString prefix = oldPrefix;
    bool firstRun = true;
    while (!parseError && firstSetUnqualifiedName.contains(peek())) {
        if (!firstRun)
            addSubstitution(prefix);
        prefix += parseUnqualifiedName();
        if (!parseError) {
            if (firstSetTemplateArgs.contains(peek())) {
                addSubstitution(prefix);
                prefix += parseTemplateArgs();
            }
        }
        firstRun = false;
    }

    FUNC_END(prefix);
    return prefix;
}

/*
 * <template-args> ::= I <template-arg>+ E
 */
const QString NameDemanglerPrivate::parseTemplateArgs()
{
    FUNC_START();
    Q_ASSERT(!firstSetTemplateArg.contains('E'));

    QString args = QLatin1String("<");
    if (advance() != 'I') {
        error(tr("Invalid template args"));
    } else {
        do {
            if (args.length() > 1)
                args += QLatin1String(", ");
            args += parseTemplateArg();
        } while (!parseError && firstSetTemplateArg.contains(peek()));
        if (!parseError && advance() != 'E')
            error(tr("Invalid template args"));
    }

    args += '>';
    FUNC_END(args);
    return args;
}

/*
 * <template-param> ::= T_      # first template parameter
 *                  ::= T <non-negative-number> _
 */
const QString NameDemanglerPrivate::parseTemplateParam()
{
    FUNC_START();

    QString param;
    if (advance() != 'T') {
        error(tr("Invalid template-param"));
    } else {
        int index;
        if (peek() == '_')
            index = 0;
        else
            index = parseNonNegativeNumber() + 1;
        if (!parseError && advance() != '_')
            error(tr("Invalid template-param"));
        param = templateParams.at(index);
    }

    FUNC_END(param);
    return param;
}

/*  <CV-qualifiers> ::= [r] [V] [K] 	# restrict (C99), volatile, const */
const QString NameDemanglerPrivate::parseCvQualifiers()
{
    FUNC_START();

    QString qualifiers;
    bool volatileFound = false;
    bool constFound = false;
    while (true) {
        if (peek() == 'V') {
            if (volatileFound || constFound) {
                error(tr("Invalid qualifiers: unexpected 'volatile'"));
                break;
            } else {
                volatileFound = true;
                qualifiers += QLatin1String(" volatile");
                advance();
            }
        } else if (peek() == 'K') {
            if (constFound) {
                error(tr("Invalid qualifiers: 'const' appears twice"));
                break;
            } else {
                constFound = true;
                qualifiers += QLatin1String(" const");
                advance();
            }
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

    int startPos = pos;
    while (peek().isDigit())
        advance();
    int number;
    if (pos == startPos) {
        error(tr("Invalid non-negative number"));
        number = 0;
    } else {
        number = mangledName.mid(startPos, pos - startPos).toInt(0, base);
    }

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
const QString NameDemanglerPrivate::parseTemplateArg()
{
    FUNC_START();
    Q_ASSERT(!firstSetType.contains('X') && !firstSetType.contains('I')
             /* && !firstSetType.contains('s') */);
    Q_ASSERT((firstSetType & firstSetExprPrimary).isEmpty());
    Q_ASSERT(!firstSetExprPrimary.contains('X')
             && !firstSetExprPrimary.contains('I')
             && !firstSetExprPrimary.contains('s'));
    Q_ASSERT(!firstSetTemplateArg.contains('E'));

    QString arg;
    QChar next = peek();
    if (readAhead(2) == QLatin1String("sp")) {
        advance(2);
        arg = parseExpression();
    } else if (firstSetType.contains(next)) {
        arg = parseType();
    } else if (firstSetExprPrimary.contains(next)) {
        arg = parseExprPrimary();
    } else if (next == 'X') {
        advance();
        arg = parseExpression();
        if (!parseError && advance() != 'E')
            error(tr("Invalid template-arg"));
    } else if (next == 'I') {
        advance();
        while (!parseError && firstSetTemplateArg.contains(peek())) {
            if (!arg.isEmpty())
                arg += QLatin1String(", ");  // TODO: is this correct?
            arg += parseTemplateArg();
        }
        if (!parseError && advance() != 'E')
            error(tr("Invalid template-arg"));
    } else {
        error(tr("Invalid template-arg"));
    }

    templateParams.append(arg);
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
const QString NameDemanglerPrivate::parseExpression()
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

    QString expr;

   /*
    * Some of the terminals in the productions of <expression>
    * also appear in the productions of operator-name. We assume the direct
    * productions to have higher precedence and check them first to prevent
    * them being parsed by parseOperatorName().
    */
    QString str = readAhead(2);
    if (str == QLatin1String("cl")) {
        advance(2);
        while (!parseError && firstSetExpression.contains(peek()))
            expr += parseExpression();
        if (!parseError && advance() != 'E')
            error(tr("Invalid expression"));
    } else if (str == QLatin1String("cv")) {
        advance(2);
        expr = parseType() + QLatin1String("(");
        if (!parseError) {
            if (peek() == '_') {
                advance();
                for (int numArgs = 0;
                     !parseError && firstSetExpression.contains(peek());
                     ++numArgs) {
                    if (numArgs > 0)
                        expr += QLatin1String(", ");
                    expr += parseExpression();
                }
                if (!parseError && advance() != 'E')
                    error(tr("Invalid expression"));
            } else {
                expr += parseExpression();
            }
        }
        expr += QLatin1String(")");
    } else if (str == QLatin1String("st")) {
        advance(2);
        expr = QString::fromLocal8Bit("sizeof(%1)").arg(parseType());
    } else if (str == QLatin1String("at")) {
        advance(2);
        expr = QString::fromLocal8Bit("alignof(%1)").arg(parseType());
    } else if (str == QLatin1String("sr")) { // TODO: Which syntax to use here?
        advance(2);
        expr = parseType();
        if (!parseError)
            expr += parseUnqualifiedName();
        if (!parseError && firstSetTemplateArgs.contains(peek()))
            parseTemplateArgs();
    } else if (str == QLatin1String("sZ")) {
        expr = parseTemplateParam(); // TODO: Syntax?
    } else {
        QChar next = peek();
        if (firstSetOperatorName.contains(next)) {
            const Operator &op = parseOperatorName();
            QStringList exprs;
            if (!parseError)
                exprs.append(parseExpression());
            if (!parseError && op.type() != Operator::UnaryOp)
                exprs.append(parseExpression());
            if (!parseError && op.type() == Operator::TernaryOp) {
                exprs.append(parseExpression());
            }
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
            error(tr("Invalid expression"));
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
const QString NameDemanglerPrivate::parseExprPrimary()
{
    FUNC_START();
    Q_ASSERT((firstSetType & firstSetMangledName).isEmpty());

    QString expr;
    if (advance() != 'L') {
        error(tr("Invalid primary expression"));
    } else {
        QChar next = peek();
        if (firstSetType.contains(next)) {
            const QString type = parseType();
            if (!parseError) {
                if (true /* type just parsed indicates integer */)
                    expr += QString::number(parseNumber());
                else if (true /* type just parsed indicates float */)
                    expr += QString::number(parseFloat());
                else
                    error(tr("Invalid expr-primary"));
            }
        } else if (firstSetMangledName.contains(next)) {
            expr = parseMangledName();
        } else {
            error(tr("Invalid expr-primary"));
        }
        if (!parseError && advance() != 'E')
            error(tr("Invalid expr-primary"));
    }

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
const QString NameDemanglerPrivate::parseType()
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

    QString type;
    QString str = readAhead(2);
    if (str == QLatin1String("Dp")) {
        advance(2);
        type = parseType(); // TODO: Probably needs augmentation
    } else if (str == QLatin1String("Dt")) {
        advance(2);
        type = parseExpression();  // TODO: See above
        if (!parseError && advance() != 'E')
            error(tr("Invalid type"));
    } else if (str == QLatin1String("DT")) {
        advance(2);
        type = parseExpression(); // TODO: See above
        if (!parseError && advance() != 'E')
            error(tr("Invalid type"));
    } else {
        QChar next = peek();
        if (str == QLatin1String("Dd") || str == QLatin1String("De")
            || str == QLatin1String("Df") || str == QLatin1String("Dh")
            || str == QLatin1String("Di") || str == QLatin1String("Ds")
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
            if (!parseError && firstSetTemplateArgs.contains(peek())) {
                type += parseTemplateArgs();
                addSubstitution(type);
            }
        } else if (firstSetSubstitution.contains(next)) {
            type = parseSubstitution();
            if (!parseError && firstSetTemplateArgs.contains(peek())) {
                type += parseTemplateArgs();
                addSubstitution(type);
            }
        } else if (firstSetCvQualifiers.contains(next)) {
            const QString cvQualifiers = parseCvQualifiers();
            if (!parseError) {
                type = parseType();
                if (!parseError && !cvQualifiers.isEmpty()) {
                    insertQualifier(type, cvQualifiers);
                    addSubstitution(type);
                }
            }
        } else if (next == 'P') {
            advance();
            type = parseType();
            insertQualifier(type, QLatin1String("*"));
            addSubstitution(type);
        } else if (next == 'R') {
            advance();
            type = parseType();
            insertQualifier(type, QLatin1String("&"));
            addSubstitution(type);
        } else if (next == 'O') {
            advance();
            type = parseType() + QLatin1String("&&"); // TODO: Correct notation?
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
            if (!parseError)
                type += parseType(); // TODO: handle this correctly
        } else {
            error(tr("Invalid type"));
        }
    }

    FUNC_END(type);
    return type;
}

/* <source-name> ::= <number> <identifier> */
const QString NameDemanglerPrivate::parseSourceName()
{
    FUNC_START();

    int idLen = parseNonNegativeNumber();
    QString sourceName;
    if (!parseError)
        sourceName = parseIdentifier(idLen);

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
const QString NameDemanglerPrivate::parseBuiltinType()
{
    FUNC_START();

    // TODO: Perhaps a map would be more appropriate here.
    QString type;
    switch (advance().toAscii()) {
        case 'v':
            type = QLatin1String("void");
            break;
        case 'w':
            type = QLatin1String("wchar_t");
            break;
        case 'b':
            type = QLatin1String("bool");
            break;
        case 'c':
            type = QLatin1String("char");
            break;
        case 'a':
            type = QLatin1String("signed char");
            break;
        case 'h':
            type = QLatin1String("unsigned char");
            break;
        case 's':
            type = QLatin1String("short");
            break;
        case 't':
            type = QLatin1String("unsigned short");
            break;
        case 'i':
            type = QLatin1String("int");
            break;
        case 'j':
            type = QLatin1String("unsigned int");
            break;
        case 'l':
            type = QLatin1String("long");
            break;
        case 'm':
            type = QLatin1String("unsigned long");
            break;
        case 'x':
            type = QLatin1String("long long");
            break;
        case 'y':
            type = QLatin1String("unsigned long long");
            break;
        case 'n':
            type = QLatin1String("__int128");
            break;
        case 'o':
            type = QLatin1String("unsigned __int128");
            break;
        case 'f':
            type = QLatin1String("float");
            break;
        case 'd':
            type = QLatin1String("double");
            break;
        case 'e':
            type = QLatin1String("long double");
            break;
        case 'g':
            type = QLatin1String("__float128");
            break;
        case 'z':
            type = QLatin1String("...");
            break;
        case 'D':
            switch (advance().toAscii()) {
                case 'd':
                case 'e':
                case 'f':
                case 'h':
                    type = QLatin1String("IEEE_special_float");
                    break;
                case 'i':
                    type = QLatin1String("char32_t");
                    break;
                case 's':
                    type = QLatin1String("char16_t");
                    break;
                    break;
                default:
                    error(tr("Invalid built-in type"));
            }
            break;
        case 'u':
            type = parseSourceName();
            break;
        default:
            error(tr("Invalid builtin-type"));
    }

    FUNC_END(type);
    return type;
}

/* <function-type> ::= F [Y] <bare-function-type> E */
const QString NameDemanglerPrivate::parseFunctionType()
{
    FUNC_START();

    // TODO: Check how we get here. Do we always have a return type or not?
    QString funcType;
    bool externC = false;
    if (advance() != 'F') {
        error(tr("Invalid function type"));
    } else {
        if (peek() == 'Y') {
            advance();
            externC = true;
        }
        const QStringList &signature = parseBareFunctionType();
        if (!parseError && advance() != 'E')
            error(tr("Invalid function type"));
        if (!parseError) {
            QString returnType = signature.first();
            QString argList = QLatin1String("(");
            for (int i = 1; i < signature.size(); ++i) {
                if (i > 1)
                    argList.append(QLatin1String(", "));
                const QString &type = signature.at(i);
                if (type != QLatin1String("void"))
                    argList.append(type);
            }
            argList.append(QLatin1String(")"));
            bool retTypeIsFuncPtr = false;
            const int firstClosingParenIndex = returnType.indexOf(')');
            if (firstClosingParenIndex != -1) {
                const int firstOpeningParenIndex =
                    returnType.lastIndexOf('(', firstClosingParenIndex);
                const QChar next = returnType[firstOpeningParenIndex + 1];
                if (next == '*' || next =='&') {
                    retTypeIsFuncPtr = true;
                    funcType = returnType.left(firstOpeningParenIndex + 2)
                        + argList + returnType.mid(firstOpeningParenIndex + 2);
                }
            }
            if (!retTypeIsFuncPtr)
                funcType = QString::fromLocal8Bit("%1 %2").
                    arg(returnType).arg(argList);
        }
    }

    if (externC)
        funcType.prepend(QLatin1String("extern \"C\" "));
    FUNC_END(funcType);
    return funcType;
}

/* <bare-function-type> ::= <type>+ */
const QStringList NameDemanglerPrivate::parseBareFunctionType()
{
    FUNC_START();

    QStringList signature;
    do
        signature.append(parseType());
    while (!parseError && firstSetType.contains(peek()));

    FUNC_END(signature.join(QLatin1String(":")));
    return signature;
}

/* <class-enum-type> ::= <name> */
const QString NameDemanglerPrivate::parseClassEnumType()
{
    FUNC_START();
    const QString &name = parseName();
    FUNC_END(name);
    return name;
}

/*
 * <unqualified-name> ::= <operator-name>
 *                    ::= <ctor-dtor-name>
 *                    ::= <source-name>
 */
const QString NameDemanglerPrivate::parseUnqualifiedName()
{
    FUNC_START();
    Q_ASSERT((firstSetOperatorName & firstSetCtorDtorName).isEmpty());
    Q_ASSERT((firstSetOperatorName & firstSetSourceName).isEmpty());
    Q_ASSERT((firstSetCtorDtorName & firstSetSourceName).isEmpty());

    QString name;
    QChar next = peek();
    if (firstSetOperatorName.contains(next))
        name = QLatin1String("::operator") + parseOperatorName().repr;
    else if (firstSetCtorDtorName.contains(next))
        name = QLatin1String("::") + parseCtorDtorName();
    else if (firstSetSourceName.contains(next))
        name = QLatin1String("::") + parseSourceName();
    else
        error(tr("Invalid unqualified-name"));

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
        static const UnaryOperator vendorOp(QLatin1String("v"),
                                            QLatin1String("[unimplemented]"));
        advance();
        int numExprs = parseDigit();
        Q_UNUSED(numExprs);
        if (!parseError)
            parseSourceName();
        op = &vendorOp;
    } else {
        const QString id = readAhead(2);
        advance(2);
        if (id == QLatin1String("cv")) {
            static UnaryOperator castOp(QLatin1String("cv"),
                                        QLatin1String(""));
            QString type = parseType();
            castOp.repr = QString::fromLocal8Bit("(%1)").arg(type);
            op = &castOp;
        } else {
            op = ops.value(id);
            if (op == 0) {
                static const UnaryOperator pseudoOp(QLatin1String("invalid"),
                                                    QLatin1String("invalid"));
                op = &pseudoOp;
                error(tr("Invalid operator-name '%s'").arg(id));
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
const QString NameDemanglerPrivate::parseArrayType()
{
    FUNC_START();
    Q_ASSERT((firstSetNonNegativeNumber & firstSetExpression).isEmpty());
    Q_ASSERT(!firstSetNonNegativeNumber.contains('_'));
    Q_ASSERT(!firstSetExpression.contains('_'));

    QString type;
    if (advance() != 'A') {
        error(tr("Invalid array-type"));
    } else {
        QChar next = peek();
        QString dimension;
        if (firstSetNonNegativeNumber.contains(next)) {
            dimension = QString::number(parseNonNegativeNumber());
        } else if (firstSetExpression.contains(next)){
            dimension = parseExpression();
        }
        if (!parseError && advance() != '_')
            error(tr("Invalid array-type"));
        if (!parseError)
            type = QString::fromLocal8Bit("%1[%2]").
                arg(parseType()).arg(dimension);
    }

    FUNC_END(type);
    return type;
}

/* <pointer-to-member-type> ::= M <type> <type> */
const QString NameDemanglerPrivate::parsePointerToMemberType()
{
    FUNC_START();

    QString type;
    if (advance() != 'M') {
        error(tr("Invalid pointer-to-member-type"));
    } else {
        const QString classType = parseType();
        QString memberType;
        if (!parseError)
            memberType = parseType();
        if (!parseError) {
            if (memberType.contains(')')) { // Function?
                int parenIndex = memberType.indexOf('(');
                QString returnType = memberType.left(parenIndex);
                memberType.remove(0, parenIndex);
                type = QString::fromLocal8Bit("%1(%2::*)%3").
                    arg(returnType).arg(classType).arg(memberType);
            } else {
                type = QString::fromLocal8Bit("%1 %2::*").
                    arg(memberType).arg(classType);
            }
        }
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
const QString NameDemanglerPrivate::parseSubstitution()
{
    FUNC_START();
    Q_ASSERT(!firstSetSeqId.contains('_') && !firstSetSeqId.contains('t')
             && !firstSetSeqId.contains('a') && !firstSetSeqId.contains('b')
             && !firstSetSeqId.contains('s') && !firstSetSeqId.contains('i')
             && !firstSetSeqId.contains('o') && !firstSetSeqId.contains('d'));

    QString substitution;
    if (advance() != 'S') {
        error(tr("Invalid substitution"));
    } else if (firstSetSeqId.contains(peek())) {
        int substIndex = parseSeqId() + 1;
        if (!parseError && substIndex >= substitutions.size())
            error(tr("Invalid substitution: element %1 was requested, "
                     "but there are only %2").
                  arg(substIndex + 1).arg(substitutions.size()));
        else
            substitution = substitutions.at(substIndex);
        if (!parseError && advance() != '_')
            error(tr("Invalid substitution"));
    } else {
        switch (advance().toAscii()) {
            case '_':
                if (substitutions.isEmpty())
                    error(tr("Invalid substitution: There are no elements"));
                else
                    substitution = substitutions.first();
                break;
            case 't':
                substitution = QLatin1String("::std::");
                break;
            case 'a':
                substitution = QLatin1String("::std::allocator");
                break;
            case 'b':
                substitution = QLatin1String("::std::basic_string");
                break;
            case 's':
                substitution = QLatin1String(
                    "::std::basic_string<char, "
                    "::std::char_traits<char>, ::std::allocator<char> >");
                break;
            case 'i':
                substitution = QLatin1String(
                    "::std::basic_istream<char, std::char_traits<char> >");
                break;
            case 'o':
                substitution = QLatin1String(
                    "::std::basic_ostream<char, std::char_traits<char> >");
                break;
            case 'd':
                substitution = QLatin1String(
                    "::std::basic_iostream<char, std::char_traits<char> >");
                break;
            default:
                error(tr("Invalid substitution"));
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
const QString NameDemanglerPrivate::parseSpecialName()
{
    FUNC_START();
    Q_ASSERT(!firstSetCallOffset.contains('V')
             && !firstSetCallOffset.contains('T')
             && !firstSetCallOffset.contains('I')
             && !firstSetCallOffset.contains('S')
             && !firstSetCallOffset.contains('c'));

    QString name;
    QString str = readAhead(2);
    if (str == QLatin1String("TV")) {
        advance(2);
        name = QString::fromLocal8Bit("[virtual table of %1]").arg(parseType());
    } else if (str == QLatin1String("TT")) {
        advance(2);
        name = QString::fromLocal8Bit("[VTT struct of %1]").arg(parseType());
    } else if (str == QLatin1String("TI")) {
        advance(2);
        name = QString::fromLocal8Bit("typeid(%1)").arg(parseType());
    } else if (str == QLatin1String("TS")) {
        advance(2);
        name = QString::fromLocal8Bit("typeid(%1).name()").arg(parseType());
    } else if (str == QLatin1String("GV")) {
        advance(2);
        name = QString::fromLocal8Bit("[guard variable of %1]").
            arg(parseName());
    } else if (str == QLatin1String("Tc")) {
        advance(2);
        parseCallOffset();
        if (!parseError)
            parseCallOffset();
        if (!parseError)
            parseEncoding();
    } else if (advance() == 'T') {
        parseCallOffset();
        if (!parseError)
            parseEncoding();
    } else {
        error(tr("Invalid special-name"));
    }

    FUNC_END(name);
    return name;
}

/*
 * <unscoped-name> ::= <unqualified-name>
 *                 ::= St <unqualified-name>   # ::std::
 */
const QString NameDemanglerPrivate::parseUnscopedName()
{
    FUNC_START();
    Q_ASSERT(!firstSetUnqualifiedName.contains('S'));

    QString name;
    if (readAhead(2) == QLatin1String("St")) {
        advance(2);
        name = QLatin1String("::std") + parseUnqualifiedName();
    } else if (firstSetUnqualifiedName.contains(peek())) {
        name = parseUnqualifiedName();
    } else {
        error(tr("Invalid unqualified-name"));
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
const QString NameDemanglerPrivate::parseLocalName()
{
    FUNC_START();

    QString name;
    if (advance() != 'Z') {
        error(tr("Invalid local-name"));
    } else {
        name = parseEncoding();
        if (!parseError && advance() != 'E') {
            error(tr("Invalid local-name"));
        } else {
            QString str = readAhead(2);
            QChar next = peek();
            if (str == QLatin1String("sp") || str == QLatin1String("sr")
                || str == QLatin1String("st") || str == QLatin1String("sz")
                || str == QLatin1String("sZ")
                || (next != 's' && firstSetName.contains(next)))
                name +=  parseName();
            else if (next == 's') {
                advance();
                name += QLatin1String("::\"string literal\"");
            } else {
                error(tr("Invalid local-name"));
            }
            if (!parseError && firstSetDiscriminator.contains(peek()))
                parseDiscriminator();
        }
    }

    FUNC_END(name);
    return name;
}

/* <discriminator> := _ <non-negative-number> */
int NameDemanglerPrivate::parseDiscriminator()
{
    int index;
    if (advance() != '_') {
        error(tr("Invalid discriminator"));
        index = -1;
    } else {
        index = parseNonNegativeNumber();
    }

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
const QString NameDemanglerPrivate::parseCtorDtorName()
{
    FUNC_START();

    QString name;
    bool destructor = false;
    switch (advance().toAscii()) {
        case 'C':
            switch (advance().toAscii()) {
                case '1':
                case '2':
                case '3':
                    break;
                default:
                    error(tr("Invalid ctor-dtor-name"));
            }
            break;
        case 'D':
            switch (advance().toAscii()) {
                case '0':
                case '1':
                case '2':
                    destructor = true;
                    break;
                default:
                    error(tr("Invalid ctor-dtor-name"));
            }
            break;
        default:
            error(tr("Invalid ctor-dtor-name"));
    }

    if (!parseError) {
        name = substitutions.last();
        int templateArgsStart = name.indexOf('<');
        if (templateArgsStart != -1)
            name.remove(templateArgsStart,
                        name.indexOf('>') - templateArgsStart + 1);
        int lastComponentStart = name.lastIndexOf(QLatin1String("::"));
        if (lastComponentStart != -1)
            name.remove(0, lastComponentStart + 2);
        if (destructor)
            name.prepend('~');
    }

    FUNC_END(name);
    return name;
}

/* This will probably need the number of characters to read. */
const QString NameDemanglerPrivate::parseIdentifier(int len)
{
    FUNC_START();

    const QString id = mangledName.mid(pos, len);
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

    switch (advance().toAscii()) {
        case 'h':
            parseNvOffset();
            break;
        case 'v':
            parseVOffset();
            break;
        default:
            error(tr("Invalid call-offset"));
    }
    if (!parseError && advance() != '_')
        error(tr("Invalid call-offset"));
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
        error(tr("Invalid v-offset"));
    parseNumber();

    FUNC_END(QString());
}

int NameDemanglerPrivate::parseDigit()
{
    FUNC_START();

    int digit = advance().digitValue();
    if (digit == -1)
        error(tr("Invalid digit"));

    FUNC_END(QString::number(digit));
    return digit;
}

void NameDemanglerPrivate::error(const QString &errorSpec)
{
    parseError = true;
    m_errorString = tr("At position %1: ").arg(pos) + errorSpec;
}

QChar NameDemanglerPrivate::peek(int ahead)
{
    Q_ASSERT(pos >= 0);
    if (pos + ahead < mangledName.size()) {
        return mangledName[pos + ahead];
    } else {
        return eoi;
    }
}

QChar NameDemanglerPrivate::advance(int steps)
{
    Q_ASSERT(steps > 0);
    if (pos + steps <= mangledName.size()) {
        QChar c = mangledName[pos];
        pos += steps;
        return c;
    } else {
        pos = mangledName.size();
        parseError = true;
        return eoi;
    }
}

const QString NameDemanglerPrivate::readAhead(int charCount)
{
    QString str;
    if (pos + charCount < mangledName.size())
        str = mangledName.mid(pos, charCount);
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
    firstSetUnscopedName = firstSetUnqualifiedName | (QSet<QChar>() << 'S');
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
    foreach(QChar c, firstSetType)
        qDebug("'%c'", c.toAscii());
    qDebug("\n");
    foreach(QChar c, firstSetExprPrimary)
        qDebug("'%c'", c.toAscii());
#endif
}

void NameDemanglerPrivate::setupOps(){

    ops[QLatin1String("nw")] =
        new UnaryOperator(QLatin1String("nw"), QLatin1String("new "));
    ops[QLatin1String("na")] = new ArrayNewOperator;
    ops[QLatin1String("dl")] =
        new UnaryOperator(QLatin1String("dl"), QLatin1String("delete "));
    ops[QLatin1String("da")] =
        new UnaryOperator(QLatin1String("da"), QLatin1String("delete[] "));
    ops[QLatin1String("ps")] =
        new UnaryOperator(QLatin1String("ps"), QLatin1String("+"));
    ops[QLatin1String("ng")] =
        new UnaryOperator(QLatin1String("ng"), QLatin1String("-"));
    ops[QLatin1String("ad")] =
        new UnaryOperator(QLatin1String("ad"), QLatin1String("&"));
    ops[QLatin1String("de")] =
        new UnaryOperator(QLatin1String("de"), QLatin1String("*"));
    ops[QLatin1String("co")] =
        new UnaryOperator(QLatin1String("co"), QLatin1String("~"));
    ops[QLatin1String("pl")] =
        new BinaryOperator(QLatin1String("pl"), QLatin1String("+"));
    ops[QLatin1String("mi")] =
        new BinaryOperator(QLatin1String("mi"), QLatin1String("-"));
    ops[QLatin1String("ml")] =
        new BinaryOperator(QLatin1String("ml"), QLatin1String("*"));
    ops[QLatin1String("dv")] =
        new BinaryOperator(QLatin1String("dv"), QLatin1String("/"));
    ops[QLatin1String("rm")] =
        new BinaryOperator(QLatin1String("rm"), QLatin1String("%"));
    ops[QLatin1String("an")] =
        new BinaryOperator(QLatin1String("an"), QLatin1String("&"));
    ops[QLatin1String("or")] =
        new BinaryOperator(QLatin1String("or"), QLatin1String("|"));
    ops[QLatin1String("eo")] =
        new BinaryOperator(QLatin1String("eo"), QLatin1String("^"));
    ops[QLatin1String("aS")] =
        new BinaryOperator(QLatin1String("aS"), QLatin1String("="));
    ops[QLatin1String("pL")] =
        new BinaryOperator(QLatin1String("pl"), QLatin1String("+="));
    ops[QLatin1String("mI")] =
        new BinaryOperator(QLatin1String("mI"), QLatin1String("-="));
    ops[QLatin1String("mL")] =
        new BinaryOperator(QLatin1String("mL"), QLatin1String("*="));
    ops[QLatin1String("dV")] =
        new BinaryOperator(QLatin1String("dV"), QLatin1String("/="));
    ops[QLatin1String("rM")] =
        new BinaryOperator(QLatin1String("rM"), QLatin1String("%="));
    ops[QLatin1String("aN")] =
        new BinaryOperator(QLatin1String("aN"), QLatin1String("&="));
    ops[QLatin1String("oR")] =
        new BinaryOperator(QLatin1String("oR"), QLatin1String("|="));
    ops[QLatin1String("eO")] =
        new BinaryOperator(QLatin1String("eO"), QLatin1String("^="));
    ops[QLatin1String("ls")] =
        new BinaryOperator(QLatin1String("ls"), QLatin1String("<<"));
    ops[QLatin1String("rs")] =
        new BinaryOperator(QLatin1String("rs"), QLatin1String(">>"));
    ops[QLatin1String("lS")] =
        new BinaryOperator(QLatin1String("lS"), QLatin1String("<<="));
    ops[QLatin1String("rS")] =
        new BinaryOperator(QLatin1String("rS"), QLatin1String(">>="));
    ops[QLatin1String("eq")] =
        new BinaryOperator(QLatin1String("eq"), QLatin1String("=="));
    ops[QLatin1String("ne")] =
        new BinaryOperator(QLatin1String("ne"), QLatin1String("!="));
    ops[QLatin1String("lt")] =
        new BinaryOperator(QLatin1String("lt"), QLatin1String("<"));
    ops[QLatin1String("gt")] =
        new BinaryOperator(QLatin1String("gt"), QLatin1String(">"));
    ops[QLatin1String("le")] =
        new BinaryOperator(QLatin1String("le"), QLatin1String("<="));
    ops[QLatin1String("ge")] =
        new BinaryOperator(QLatin1String("ge"), QLatin1String(">="));
    ops[QLatin1String("nt")] =
        new UnaryOperator(QLatin1String("nt"), QLatin1String("!"));
    ops[QLatin1String("aa")] =
        new BinaryOperator(QLatin1String("aa"), QLatin1String("&&"));
    ops[QLatin1String("oo")] =
        new BinaryOperator(QLatin1String("oo"), QLatin1String("||"));
    ops[QLatin1String("pp")] =
        new UnaryOperator(QLatin1String("pp"), QLatin1String("++")); // Prefix?
    ops[QLatin1String("mm")] =
        new UnaryOperator(QLatin1String("mm"), QLatin1String("--")); // Prefix?
    ops[QLatin1String("cm")] =
        new BinaryOperator(QLatin1String("cm"), QLatin1String(","));
    ops[QLatin1String("pm")] =
        new BinOpWithNoSpaces(QLatin1String("pm"), QLatin1String("->*"));
    ops[QLatin1String("pt")] =
        new BinOpWithNoSpaces(QLatin1String("pm"), QLatin1String("->"));
    ops[QLatin1String("cl")] = new FunctionCallOperator;
    ops[QLatin1String("ix")] = new ArrayAccessOperator;
    ops[QLatin1String("qu")] = new QuestionMarkOperator;
    ops[QLatin1String("st")] =
        new SizeAlignOfOperator(QLatin1String("st"), QLatin1String("sizeof"));
    ops[QLatin1String("sz")] =
        new SizeAlignOfOperator(QLatin1String("sz"), QLatin1String("sizeof"));
    ops[QLatin1String("at")] =
        new SizeAlignOfOperator(QLatin1String("at"), QLatin1String("alignof"));
    ops[QLatin1String("az")] =
        new SizeAlignOfOperator(QLatin1String("az"), QLatin1String("alignof"));
}

void NameDemanglerPrivate::addSubstitution(const QString &symbol)
{
    if (!symbol.isEmpty() && !substitutions.contains(symbol))
        substitutions.append(symbol);
}

void NameDemanglerPrivate::insertQualifier(QString &type,
                                           const QString &qualifier)
{
    const int funcAnchor = type.indexOf(QRegExp(QLatin1String("\\([^*&]")));
    const int qualAnchor =
        type.indexOf(QRegExp(QLatin1String("(\\*|\\&|const|volatile)\\)")));
    int insertionPos;
    QString insertionString = qualifier;
    if (funcAnchor == -1) {
        if (qualAnchor == -1) {
            insertionPos = type.size();
        } else {
            insertionPos = type.indexOf(')', qualAnchor);;
        }
    } else if (qualAnchor == -1 || funcAnchor < qualAnchor) {
        if (qualifier == QLatin1String("*")
            || qualifier == QLatin1String("&")) {
            insertionPos = funcAnchor;
            insertionString = QString::fromLocal8Bit("(%1)").arg(qualifier);
        } else {
            insertionPos = type.size(); // Qualifier for pointer to member.
        }
    } else  {
        insertionPos = type.indexOf(')', qualAnchor);;
    }
    if ((insertionString == QLatin1String("*")
         || insertionString == QLatin1String("&"))
        && (type[insertionPos - 1] != '*' && type[insertionPos - 1] != '&'))
        insertionString.prepend(' ');
    type.insert(insertionPos, insertionString);
}

NameDemangler::NameDemangler() : pImpl(new NameDemanglerPrivate) { }

NameDemangler::~NameDemangler()
{
    delete pImpl;
}

bool NameDemangler::demangle(const QString &mangledName)
{
    return pImpl->demangle(mangledName);
}

const QString &NameDemangler::errorString() const
{
    return pImpl->errorString();
}

const QString &NameDemangler::demangledName() const
{
    return pImpl->demangledName();
}

} // namespace Internal
} // namespace Debugger

