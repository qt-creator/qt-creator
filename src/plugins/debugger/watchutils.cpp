/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "watchutils.h"

#include <QtCore/QDebug>
#include <QtCore/QTime>
#include <QtCore/QStringList>

namespace Debugger {
namespace Internal {

QString dotEscape(QString str)
{
    const QChar dot = QLatin1Char('.');
    str.replace(QLatin1Char(' '), dot);
    str.replace(QLatin1Char('\\'), dot);
    str.replace(QLatin1Char('/'), dot);
    return str;
}

QString currentTime()
{
    return QTime::currentTime().toString(QLatin1String("hh:mm:ss.zzz"));
}

bool isSkippableFunction(const QString &funcName, const QString &fileName)
{
    if (fileName.endsWith(QLatin1String("kernel/qobject.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/moc_qobject.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/qmetaobject.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String(".moc")))
        return true;

    if (funcName.endsWith("::qt_metacall"))
        return true;

    return false;
}

bool isLeavableFunction(const QString &funcName, const QString &fileName)
{
    if (funcName.endsWith(QLatin1String("QObjectPrivate::setCurrentSender")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/qmetaobject.cpp"))
            && funcName.endsWith(QLatin1String("QMetaObject::methodOffset")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/qobject.h")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/qobject.cpp"))
            && funcName.endsWith(QLatin1String("QObjectConnectionListVector::at")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/qobject.cpp"))
            && funcName.endsWith(QLatin1String("~QObject")))
        return true;
    if (fileName.endsWith(QLatin1String("thread/qmutex.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("thread/qthread.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("thread/qthread_unix.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("thread/qmutex.h")))
        return true;
    if (fileName.contains(QLatin1String("thread/qbasicatomic")))
        return true;
    if (fileName.contains(QLatin1String("thread/qorderedmutexlocker_p")))
        return true;
    if (fileName.contains(QLatin1String("arch/qatomic")))
        return true;
    if (fileName.endsWith(QLatin1String("tools/qvector.h")))
        return true;
    if (fileName.endsWith(QLatin1String("tools/qlist.h")))
        return true;
    if (fileName.endsWith(QLatin1String("tools/qhash.h")))
        return true;
    if (fileName.endsWith(QLatin1String("tools/qmap.h")))
        return true;
    if (fileName.endsWith(QLatin1String("tools/qstring.h")))
        return true;
    if (fileName.endsWith(QLatin1String("global/qglobal.h")))
        return true;

    return false;
}

bool hasLetterOrNumber(const QString &exp)
{
    const QChar underscore = QLatin1Char('_');
    for (int i = exp.size(); --i >= 0; )
        if (exp.at(i).isLetterOrNumber() || exp.at(i) == underscore)
            return true;
    return false;
}

bool hasSideEffects(const QString &exp)
{
    // FIXME: complete?
    return exp.contains(QLatin1String("-="))
        || exp.contains(QLatin1String("+="))
        || exp.contains(QLatin1String("/="))
        || exp.contains(QLatin1String("*="))
        || exp.contains(QLatin1String("&="))
        || exp.contains(QLatin1String("|="))
        || exp.contains(QLatin1String("^="))
        || exp.contains(QLatin1String("--"))
        || exp.contains(QLatin1String("++"));
}

bool isKeyWord(const QString &exp)
{
    // FIXME: incomplete
    return exp == QLatin1String("class")
        || exp == QLatin1String("const")
        || exp == QLatin1String("do")
        || exp == QLatin1String("if")
        || exp == QLatin1String("return")
        || exp == QLatin1String("struct")
        || exp == QLatin1String("template")
        || exp == QLatin1String("void")
        || exp == QLatin1String("volatile")
        || exp == QLatin1String("while");
}

bool isPointerType(const QString &type)
{
    return type.endsWith(QLatin1Char('*')) || type.endsWith(QLatin1String("* const"));
}

bool isAccessSpecifier(const QString &str)
{
    static const QStringList items =
        QStringList() << QLatin1String("private") << QLatin1String("protected") << QLatin1String("public");
    return items.contains(str);
}

bool startsWithDigit(const QString &str)
{
    return !str.isEmpty() && str.at(0).isDigit();
}

QString stripPointerType(QString type)
{
    if (type.endsWith(QLatin1Char('*')))
        type.chop(1);
    if (type.endsWith(QLatin1String("* const")))
        type.chop(7);
    if (type.endsWith(QLatin1Char(' ')))
        type.chop(1);
    return type;
}

QString gdbQuoteTypes(const QString &type)
{
    // gdb does not understand sizeof(Core::IFile*).
    // "sizeof('Core::IFile*')" is also not acceptable,
    // it needs to be "sizeof('Core::IFile'*)"
    //
    // We never will have a perfect solution here (even if we had a full blown
    // C++ parser as we do not have information on what is a type and what is
    // a variable name. So "a<b>::c" could either be two comparisons of values
    // 'a', 'b' and '::c', or a nested type 'c' in a template 'a<b>'. We
    // assume here it is the latter.
    //return type;

    // (*('myns::QPointer<myns::QObject>*'*)0x684060)" is not acceptable
    // (*('myns::QPointer<myns::QObject>'**)0x684060)" is acceptable
    if (isPointerType(type))
        return gdbQuoteTypes(stripPointerType(type)) + QLatin1Char('*');

    QString accu;
    QString result;
    int templateLevel = 0;

    const QChar colon = QLatin1Char(':');
    const QChar singleQuote = QLatin1Char('\'');
    const QChar lessThan = QLatin1Char('<');
    const QChar greaterThan = QLatin1Char('>');
    for (int i = 0; i != type.size(); ++i) {
        const QChar c = type.at(i);
        if (c.isLetterOrNumber() || c == QLatin1Char('_') || c == colon || c == QLatin1Char(' ')) {
            accu += c;
        } else if (c == lessThan) {
            ++templateLevel;
            accu += c;
        } else if (c == greaterThan) {
            --templateLevel;
            accu += c;
        } else if (templateLevel > 0) {
            accu += c;
        } else {
            if (accu.contains(colon) || accu.contains(lessThan))
                result += singleQuote + accu + singleQuote;
            else
                result += accu;
            accu.clear();
            result += c;
        }
    }
    if (accu.contains(colon) || accu.contains(lessThan))
        result += singleQuote + accu + singleQuote;
    else
        result += accu;
    //qDebug() << "GDB_QUOTING" << type << " TO " << result;

    return result;
}

bool extractTemplate(const QString &type, QString *tmplate, QString *inner)
{
    // Input "Template<Inner1,Inner2,...>::Foo" will return "Template::Foo" in
    // 'tmplate' and "Inner1@Inner2@..." etc in 'inner'. Result indicates
    // whether parsing was successful
    int level = 0;
    bool skipSpace = false;

    for (int i = 0; i != type.size(); ++i) {
        const QChar c = type.at(i);
        if (c == QLatin1Char(' ') && skipSpace) {
            skipSpace = false;
        } else if (c == QLatin1Char('<')) {
            *(level == 0 ? tmplate : inner) += c;
            ++level;
        } else if (c == QLatin1Char('>')) {
            --level;
            *(level == 0 ? tmplate : inner) += c;
        } else if (c == QLatin1Char(',')) {
            *inner += (level == 1) ? QLatin1Char('@') : QLatin1Char(',');
            skipSpace = true;
        } else {
            *(level == 0 ? tmplate : inner) += c;
        }
    }
    *tmplate = tmplate->trimmed();
    *tmplate = tmplate->remove(QLatin1String("<>"));
    *inner = inner->trimmed();
    //qDebug() << "EXTRACT TEMPLATE: " << *tmplate << *inner << " FROM " << type;
    return !inner->isEmpty();
}

QString extractTypeFromPTypeOutput(const QString &str)
{
    int pos0 = str.indexOf(QLatin1Char('='));
    int pos1 = str.indexOf(QLatin1Char('{'));
    int pos2 = str.lastIndexOf(QLatin1Char('}'));
    QString res = str;
    if (pos0 != -1 && pos1 != -1 && pos2 != -1)
        res = str.mid(pos0 + 2, pos1 - 1 - pos0)
            + QLatin1String(" ... ") + str.right(str.size() - pos2);
    return res.simplified();
}

bool isIntOrFloatType(const QString &type)
{
    static const QStringList types = QStringList()
        << QLatin1String("char") << QLatin1String("int") << QLatin1String("short")
        << QLatin1String("float") << QLatin1String("double") << QLatin1String("long")
        << QLatin1String("bool") << QLatin1String("signed char") << QLatin1String("unsigned")
        << QLatin1String("unsigned char")
        << QLatin1String("unsigned int") << QLatin1String("unsigned long")
        << QLatin1String("long long")  << QLatin1String("unsigned long long");
    return types.contains(type);
}

QString sizeofTypeExpression(const QString &type)
{
    if (type.endsWith(QLatin1Char('*')))
        return QLatin1String("sizeof(void*)");
    if (type.endsWith(QLatin1Char('>')))
        return QLatin1String("sizeof(") + type + QLatin1Char(')');
    return QLatin1String("sizeof(") + gdbQuoteTypes(type) + QLatin1Char(')');
}

/* Parse 'query' (1) protocol response of the custom dumpers:
 * "'dumpers=["QByteArray","QDateTime",..."std::basic_string",],
 * qtversion=["4","5","1"],namespace="""' */

bool parseQueryDumperOutput(const QByteArray &a, QStringList *types, QString *qtVersion, QString *qtNamespace)
{
    types->clear();
    qtVersion->clear();
    qtNamespace->clear();
    const QChar equals = QLatin1Char('=');
    const QChar doubleQuote = QLatin1Char('"');
    const QChar openingBracket = QLatin1Char('[');
    const QChar closingBracket = QLatin1Char(']');
    const QString dumperKey = QLatin1String("dumpers");
    const QString versionKey = QLatin1String("qtversion");
    const QString namespaceKey = QLatin1String("namespace");

    const QString s = QString::fromLatin1(a);
    const int size = a.size();
    for (int pos = 0; pos < size; ) {
        // split into keyword / value pairs
        const int equalsPos = s.indexOf(equals, pos);
        if (equalsPos == -1)
            break;
        const QStringRef keyword = s.midRef(pos, equalsPos - pos);
        // Array or flat value. Cut out value without delimiters
        int valuePos = equalsPos + 1;
        const QChar endChar = s.at(valuePos) == doubleQuote ? doubleQuote : closingBracket;
        valuePos++;
        int endValuePos = s.indexOf(endChar, valuePos);
        if (endValuePos == -1)
            return false;
        QString value = s.mid(valuePos, endValuePos - valuePos);
        pos = endValuePos;
        // Evaluate
        if (keyword == namespaceKey) {
            *qtNamespace = value;
        } else {
            if (keyword == versionKey) {
                *qtVersion = value.remove(doubleQuote).replace(QLatin1Char(','), QLatin1Char('.'));
            } else {
                if (keyword == dumperKey) {
                    *types = value.remove(doubleQuote).split(QLatin1Char(','));
                }
            }
        }
        // find next keyword
        while (pos < size && !s.at(pos).isLetterOrNumber())
            pos++;
    }
    return true;
}

}
}
