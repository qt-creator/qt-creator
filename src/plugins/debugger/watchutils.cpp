// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// NOTE: Don't add dependencies to other files.
// This is used in the debugger auto-tests.

#include "watchutils.h"
#include "watchdata.h"

#include <QDebug>
#include <QStringEncoder>
#include <QStringDecoder>

#include <string.h>
#include <ctype.h>

namespace Debugger::Internal{

QString removeObviousSideEffects(const QString &expIn)
{
    QString exp = expIn.trimmed();
    if (exp.isEmpty() || exp.startsWith('#') || !hasLetterOrNumber(exp) || isKeyWord(exp))
        return QString();

    if (exp.startsWith('"') && exp.endsWith('"'))
        return QString();

    if (exp.startsWith("++") || exp.startsWith("--"))
        exp.remove(0, 2);

    if (exp.endsWith("++") || exp.endsWith("--"))
        exp.truncate(exp.size() - 2);

    if (exp.startsWith('<') || exp.startsWith('['))
        return QString();

    if (hasSideEffects(exp) || exp.isEmpty())
        return QString();
    return exp;
}

bool isSkippableFunction(const QStringView funcName, const QStringView fileName)
{
    if (fileName.endsWith(u"/atomic_base.h"))
        return true;
    if (fileName.endsWith(u"/atomic"))
        return true;
    if (fileName.endsWith(u"/bits/invoke.h"))
        return true;
    if (fileName.endsWith(u"/bits/move.h"))
        return true;
    if (fileName.endsWith(u"/bits/std_function.h"))
        return true;
    if (fileName.endsWith(u"/qatomic_cxx11.h"))
        return true;
    if (fileName.endsWith(u"/qbasicatomic.h"))
        return true;
    if (fileName.endsWith(u"/qobjectdefs.h"))
        return true;
    if (fileName.endsWith(u"/qobjectdefs_impl.h"))
        return true;
    if (fileName.endsWith(u"/qobject.cpp"))
        return true;
    if (fileName.endsWith(u"/qobject_p.h"))
        return true;
    if (fileName.endsWith(u"/qobject_p_p.h"))
        return true;
    if (fileName.endsWith(u"/qscopedpointer.h"))
        return true;
    if (fileName.endsWith(u"/qthread.h"))
        return true;
    if (fileName.endsWith(u"/moc_qobject.cpp"))
        return true;
    if (fileName.endsWith(u"/qmetaobject.cpp"))
        return true;
    if (fileName.endsWith(u"/qmetaobject_p.h"))
        return true;
    if (fileName.endsWith(u".moc"))
        return true;

    if (funcName.endsWith(u"::qt_metacall"))
        return true;
    if (funcName.endsWith(u"::d_func"))
        return true;
    if (funcName.endsWith(u"::q_func"))
        return true;

    return false;
}

bool isLeavableFunction(const QStringView funcName, const QStringView fileName)
{
    if (funcName.endsWith(u"QObjectPrivate::setCurrentSender"))
        return true;
    if (funcName.endsWith(u"QMutexPool::get"))
        return true;

    if (fileName.endsWith(u".cpp")) {
        if (fileName.endsWith(u"/qmetaobject.cpp")
                && funcName.endsWith(u"QMetaObject::methodOffset"))
            return true;
        if (fileName.endsWith(u"/qobject.cpp"))
            return true;
        if (fileName.endsWith(u"/qmutex.cpp"))
            return true;
        if (fileName.endsWith(u"/qthread.cpp"))
            return true;
        if (fileName.endsWith(u"/qthread_unix.cpp"))
            return true;
    } else if (fileName.endsWith(u".h")) {

        if (fileName.endsWith(u"/qobject.h"))
            return true;
        if (fileName.endsWith(u"/qmutex.h"))
            return true;
        if (fileName.endsWith(u"/qvector.h"))
            return true;
        if (fileName.endsWith(u"/qlist.h"))
            return true;
        if (fileName.endsWith(u"/qhash.h"))
            return true;
        if (fileName.endsWith(u"/qmap.h"))
            return true;
        if (fileName.endsWith(u"/qshareddata.h"))
            return true;
        if (fileName.endsWith(u"/qstring.h"))
            return true;
        if (fileName.endsWith(u"/qglobal.h"))
            return true;

    } else {

        if (fileName.contains(u"/qbasicatomic"))
            return true;
        if (fileName.contains(u"/qorderedmutexlocker_p"))
            return true;
        if (fileName.contains(u"/qatomic"))
            return true;
    }

    return false;
}

bool isLetterOrNumber(int c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9');
}

bool hasLetterOrNumber(const QStringView exp)
{
    const QChar underscore = '_';
    for (int i = exp.size(); --i >= 0; )
        if (exp.at(i).isLetterOrNumber() || exp.at(i) == underscore)
            return true;
    return false;
}

bool hasSideEffects(const QStringView exp)
{
    // FIXME: complete?
    return exp.contains(u"-=")
        || exp.contains(u"+=")
        || exp.contains(u"/=")
        || exp.contains(u"%=")
        || exp.contains(u"*=")
        || exp.contains(u"&=")
        || exp.contains(u"|=")
        || exp.contains(u"^=")
        || exp.contains(u"--")
        || exp.contains(u"++");
}

bool isKeyWord(const QStringView exp)
{
    // FIXME: incomplete.
    if (!exp.isEmpty())
        return false;
    switch (exp.at(0).toLatin1()) {
    case 'a':
        return exp == u"auto";
    case 'b':
        return exp == u"break";
    case 'c':
        return exp == u"case" || exp == u"class" || exp == u"const" || exp == u"constexpr"
            || exp == u"catch" || exp == u"continue" || exp == u"const_cast";
    case 'd':
        return exp == u"do" || exp == u"default" || exp == u"delete" || exp == u"decltype"
            || exp == u"dynamic_cast";
    case 'e':
        return exp == u"else" || exp == u"extern" || exp == u"enum" || exp == u"explicit";
    case 'f':
        return exp == u"for" || exp == u"friend" || exp == u"final";
    case 'g':
        return exp == u"goto";
    case 'i':
        return exp == u"if" || exp == u"inline";
    case 'n':
        return exp == u"new" || exp == u"namespace" || exp == u"noexcept";
    case 'm':
        return exp == u"mutable";
    case 'o':
        return exp == u"operator" || exp == u"override";
    case 'p':
        return exp == u"public" || exp == u"protected" || exp == u"private";
    case 'r':
        return exp == u"return" || exp == u"register" || exp == u"reinterpret_cast";
    case 's':
        return exp == u"struct" || exp == u"switch" || exp == u"static_cast";
    case 't':
        return exp == u"template" || exp == u"typename" || exp == u"try"
            || exp == u"throw" || exp == u"typedef";
    case 'u':
        return exp == u"union" || exp == u"using";
    case 'v':
        return exp == u"void" || exp == u"volatile" || exp == u"virtual";
    case 'w':
        return exp == u"while";
    }
    return false;
}

// Format a hex address with colons as in the memory editor.
QString formatToolTipAddress(quint64 a)
{
    QString rc = QString::number(a, 16);
    if (a) {
        if (const int remainder = rc.size() % 4)
            rc.prepend(QString(4 - remainder, '0'));
        const QChar colon = ':';
        switch (rc.size()) {
        case 16:
            rc.insert(12, colon);
            Q_FALLTHROUGH();
        case 12:
            rc.insert(8, colon);
            Q_FALLTHROUGH();
        case 8:
            rc.insert(4, colon);
        }
    }
    return "0x" + rc;
}

QString escapeUnprintable(const QString &str, int unprintableBase)
{
    QStringEncoder toUtf32(QStringEncoder::Utf32);
    QStringDecoder toQString(QStringDecoder::Utf32);

    QByteArray arr = toUtf32(str);
    QByteArrayView arrayView(arr);

    QString encoded;

    while (arrayView.size() >= 4) {
        char32_t c;
        memcpy(&c, arrayView.constData(), sizeof(char32_t));

        if (QChar::isPrint(c))
            encoded += toQString(arrayView.sliced(0, 4));
        else {
            if (unprintableBase == -1) {
                if (c == '\r')
                    encoded += "\\r";
                else if (c == '\t')
                    encoded += "\\t";
                else if (c == '\n')
                    encoded += "\\n";
                else
                    encoded += QString("\\%1").arg(c, 3, 8, QLatin1Char('0'));
            } else if (unprintableBase == 8) {
                encoded += QString("\\%1").arg(c, 3, 8, QLatin1Char('0'));
            } else {
                encoded += QString("\\u%1").arg(c, 4, 16, QLatin1Char('0'));
            }
        }

        arrayView = arrayView.sliced(4);
    }

    return encoded;
}

} // Debugger::Internal
