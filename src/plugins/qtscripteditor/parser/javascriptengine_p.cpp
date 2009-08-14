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

#include "javascriptengine_p.h"
#include "javascriptnodepool_p.h"
#include "javascriptvalue.h"
#include <qnumeric.h>
#include <QHash>

QT_BEGIN_NAMESPACE

namespace JavaScript {

int Ecma::RegExp::flagFromChar(const QChar &ch)
{
    static QHash<QChar, int> flagsHash;
    if (flagsHash.isEmpty()) {
        flagsHash[QLatin1Char('g')] = Global;
        flagsHash[QLatin1Char('i')] = IgnoreCase;
        flagsHash[QLatin1Char('m')] = Multiline;
    }
    QHash<QChar, int>::const_iterator it;
    it = flagsHash.constFind(ch);
    if (it == flagsHash.constEnd())
        return 0;
    return it.value();
}


NodePool::NodePool(const QString &fileName, JavaScriptEnginePrivate *engine)
    : m_fileName(fileName), m_engine(engine)
{
}

NodePool::~NodePool()
{
}

Code *NodePool::createCompiledCode(AST::Node *, CompilationUnit &)
{
    Q_ASSERT(0);
    return 0;
}

static int toDigit(char c)
{
    if ((c >= '0') && (c <= '9'))
        return c - '0';
    else if ((c >= 'a') && (c <= 'z'))
        return 10 + c - 'a';
    else if ((c >= 'A') && (c <= 'Z'))
        return 10 + c - 'A';
    return -1;
}

qjsreal integerFromString(const char *buf, int size, int radix)
{
    if (size == 0)
        return qSNaN();

    qjsreal sign = 1.0;
    int i = 0;
    if (buf[0] == '+') {
        ++i;
    } else if (buf[0] == '-') {
        sign = -1.0;
        ++i;
    }

    if (((size-i) >= 2) && (buf[i] == '0')) {
        if (((buf[i+1] == 'x') || (buf[i+1] == 'X'))
            && (radix < 34)) {
            if ((radix != 0) && (radix != 16))
                return 0;
            radix = 16;
            i += 2;
        } else {
            if (radix == 0) {
                radix = 8;
                ++i;
            }
        }
    } else if (radix == 0) {
        radix = 10;
    }

    int j = i;
    for ( ; i < size; ++i) {
        int d = toDigit(buf[i]);
        if ((d == -1) || (d >= radix))
            break;
    }
    qjsreal result;
    if (j == i) {
        if (!qstrcmp(buf, "Infinity"))
            result = qInf();
        else
            result = qSNaN();
    } else {
        result = 0;
        qjsreal multiplier = 1;
        for (--i ; i >= j; --i, multiplier *= radix)
            result += toDigit(buf[i]) * multiplier;
    }
    result *= sign;
    return result;
}

qjsreal integerFromString(const QString &str, int radix)
{
    QByteArray ba = str.trimmed().toUtf8();
    return integerFromString(ba.constData(), ba.size(), radix);
}

} // end of namespace JavaScript

QT_END_NAMESPACE
