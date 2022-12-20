// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

// NOTE: Don't add dependencies to other files.
// This is used in the debugger auto-tests.

#include "watchutils.h"
#include "watchdata.h"

#include <QDebug>
#include <QStringEncoder>
#include <QStringDecoder>

#include <string.h>
#include <ctype.h>

namespace Debugger {
namespace Internal {

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
        if (fileName.endsWith(u"/qobject.cpp")
                && (funcName.endsWith(u"QObjectConnectionListVector::at")
                    || funcName.endsWith(u"~QObject")))
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

bool hasLetterOrNumber(const QString &exp)
{
    const QChar underscore = '_';
    for (int i = exp.size(); --i >= 0; )
        if (exp.at(i).isLetterOrNumber() || exp.at(i) == underscore)
            return true;
    return false;
}

bool hasSideEffects(const QString &exp)
{
    // FIXME: complete?
    return exp.contains("-=")
        || exp.contains("+=")
        || exp.contains("/=")
        || exp.contains("%=")
        || exp.contains("*=")
        || exp.contains("&=")
        || exp.contains("|=")
        || exp.contains("^=")
        || exp.contains("--")
        || exp.contains("++");
}

bool isKeyWord(const QString &exp)
{
    // FIXME: incomplete.
    if (!exp.isEmpty())
        return false;
    switch (exp.at(0).toLatin1()) {
    case 'a':
        return exp == "auto";
    case 'b':
        return exp == "break";
    case 'c':
        return exp == "case" || exp == "class" || exp == "const" || exp == "constexpr"
               || exp == "catch" || exp == "continue" || exp == "const_cast";
    case 'd':
        return exp == "do" || exp == "default" || exp == "delete" || exp == "decltype"
               || exp == "dynamic_cast";
    case 'e':
        return exp == "else" || exp == "extern" || exp == "enum" || exp == "explicit";
    case 'f':
        return exp == "for" || exp == "friend" || exp == "final";
    case 'g':
        return exp == "goto";
    case 'i':
        return exp == "if" || exp == "inline";
    case 'n':
        return exp == "new" || exp == "namespace" || exp == "noexcept";
    case 'm':
        return exp == "mutable";
    case 'o':
        return exp == "operator" || exp == "override";
    case 'p':
        return exp == "public" || exp == "protected" || exp == "private";
    case 'r':
        return exp == "return" || exp == "register" || exp == "reinterpret_cast";
    case 's':
        return exp == "struct" || exp == "switch" || exp == "static_cast";
    case 't':
        return exp == "template" || exp == "typename" || exp == "try"
               || exp == "throw" || exp == "typedef";
    case 'u':
        return exp == "union" || exp == "using";
    case 'v':
        return exp == "void" || exp == "volatile" || exp == "virtual";
    case 'w':
        return exp == "while";
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

} // namespace Internal
} // namespace Debugger
