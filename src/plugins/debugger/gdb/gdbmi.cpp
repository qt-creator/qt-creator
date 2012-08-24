/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "gdbmi.h"

#include <utils/qtcassert.h>

#include <QByteArray>
#include <QDebug>
#include <QRegExp>
#include <QTextStream>

#include <ctype.h>

namespace Debugger {
namespace Internal {

void skipCommas(const char *&from, const char *to)
{
    while (*from == ',' && from != to)
        ++from;
}

QTextStream &operator<<(QTextStream &os, const GdbMi &mi)
{
    return os << mi.toString();
}

void GdbMi::parseResultOrValue(const char *&from, const char *to)
{
    while (from != to && isspace(*from))
        ++from;

    //qDebug() << "parseResultOrValue: " << QByteArray(from, to - from);
    parseValue(from, to);
    if (isValid()) {
        //qDebug() << "no valid result in " << QByteArray(from, to - from);
        return;
    }
    if (from == to || *from == '(')
        return;
    const char *ptr = from;
    while (ptr < to && *ptr != '=') {
        //qDebug() << "adding" << QChar(*ptr) << "to name";
        ++ptr;
    }
    m_name = QByteArray(from, ptr - from);
    from = ptr;
    if (from < to && *from == '=') {
        ++from;
        parseValue(from, to);
    }
}

QByteArray GdbMi::parseCString(const char *&from, const char *to)
{
    QByteArray result;
    //qDebug() << "parseCString: " << QByteArray(from, to - from);
    if (*from != '"') {
        qDebug() << "MI Parse Error, double quote expected";
        ++from; // So we don't hang
        return QByteArray();
    }
    const char *ptr = from;
    ++ptr;
    while (ptr < to) {
        if (*ptr == '"') {
            ++ptr;
            result = QByteArray(from + 1, ptr - from - 2);
            break;
        }
        if (*ptr == '\\') {
            ++ptr;
            if (ptr == to) {
                qDebug() << "MI Parse Error, unterminated backslash escape";
                from = ptr; // So we don't hang
                return QByteArray();
            }
        }
        ++ptr;
    }
    from = ptr;

    int idx = result.indexOf('\\');
    if (idx >= 0) {
        char *dst = result.data() + idx;
        const char *src = dst + 1, *end = result.data() + result.length();
        do {
            char c = *src++;
            switch (c) {
                case 'a': *dst++ = '\a'; break;
                case 'b': *dst++ = '\b'; break;
                case 'f': *dst++ = '\f'; break;
                case 'n': *dst++ = '\n'; break;
                case 'r': *dst++ = '\r'; break;
                case 't': *dst++ = '\t'; break;
                case 'v': *dst++ = '\v'; break;
                case '"': *dst++ = '"'; break;
                case '\\': *dst++ = '\\'; break;
                default:
                    {
                        int chars = 0;
                        uchar prod = 0;
                        forever {
                            if (c < '0' || c > '7') {
                                --src;
                                break;
                            }
                            prod = prod * 8 + c - '0';
                            if (++chars == 3 || src == end)
                                break;
                            c = *src++;
                        }
                        if (!chars) {
                            qDebug() << "MI Parse Error, unrecognized backslash escape";
                            return QByteArray();
                        }
                        *dst++ = prod;
                    }
            }
            while (src != end) {
                char c = *src++;
                if (c == '\\')
                    break;
                *dst++ = c;
            }
        } while (src != end);
        *dst = 0;
        result.truncate(dst - result.data());
    }

    return result;
}

void GdbMi::parseValue(const char *&from, const char *to)
{
    //qDebug() << "parseValue: " << QByteArray(from, to - from);
    switch (*from) {
        case '{':
            parseTuple(from, to);
            break;
        case '[':
            parseList(from, to);
            break;
        case '"':
            m_type = Const;
            m_data = parseCString(from, to);
            break;
        default:
            break;
    }
}


void GdbMi::parseTuple(const char *&from, const char *to)
{
    //qDebug() << "parseTuple: " << QByteArray(from, to - from);
    QTC_CHECK(*from == '{');
    ++from;
    parseTuple_helper(from, to);
}

void GdbMi::parseTuple_helper(const char *&from, const char *to)
{
    skipCommas(from, to);
    //qDebug() << "parseTuple_helper: " << QByteArray(from, to - from);
    m_type = Tuple;
    while (from < to) {
        if (*from == '}') {
            ++from;
            break;
        }
        GdbMi child;
        child.parseResultOrValue(from, to);
        //qDebug() << "\n=======\n" << qPrintable(child.toString()) << "\n========\n";
        if (!child.isValid())
            return;
        m_children += child;
        skipCommas(from, to);
    }
}

void GdbMi::parseList(const char *&from, const char *to)
{
    //qDebug() << "parseList: " << QByteArray(from, to - from);
    QTC_CHECK(*from == '[');
    ++from;
    m_type = List;
    skipCommas(from, to);
    while (from < to) {
        if (*from == ']') {
            ++from;
            break;
        }
        GdbMi child;
        child.parseResultOrValue(from, to);
        if (child.isValid())
            m_children += child;
        skipCommas(from, to);
    }
}

static QByteArray ind(int indent)
{
    return QByteArray(2 * indent, ' ');
}

void GdbMi::dumpChildren(QByteArray * str, bool multiline, int indent) const
{
    for (int i = 0; i < m_children.size(); ++i) {
        if (i != 0) {
            *str += ',';
            if (multiline)
                *str += '\n';
        }
        if (multiline)
            *str += ind(indent);
        *str += m_children.at(i).toString(multiline, indent);
    }
}

template <class String, typename CharType>
struct EscapeTraits
{
    static CharType at(const String &s, int  i)
        { return s.at(i); }
    static void appendChar(CharType c, String *s)
        { s->append(c); }
    static void appendString(const char *c, String *s)
        { s->append(c); }
};

template <>
struct EscapeTraits<QString, ushort>
{
    static ushort at(const QString &s, int  i)
        { return s.at(i).unicode(); }
    static void appendChar(ushort c, QString *s)
        { s->append(QLatin1Char(c)); }
    static void appendString(const char *c, QString *s)
        { s->append(QLatin1String(c)); }
};

template<class ST, typename CT>
inline ST escapeCStringTpl(const ST &ba)
{
    ST ret;
    ret.reserve(ba.length() * 2);
    for (int i = 0; i < ba.length(); ++i) {
        const CT c = EscapeTraits<ST, CT>::at(ba, i);
        switch (c) {
            case '\\': EscapeTraits<ST, CT>::appendString("\\\\", &ret); break;
            case '\a': EscapeTraits<ST, CT>::appendString("\\a", &ret); break;
            case '\b': EscapeTraits<ST, CT>::appendString("\\b", &ret); break;
            case '\f': EscapeTraits<ST, CT>::appendString("\\f", &ret); break;
            case '\n': EscapeTraits<ST, CT>::appendString("\\n", &ret); break;
            case '\r': EscapeTraits<ST, CT>::appendString("\\r", &ret); break;
            case '\t': EscapeTraits<ST, CT>::appendString("\\t", &ret); break;
            case '\v': EscapeTraits<ST, CT>::appendString("\\v", &ret); break;
            case '"': EscapeTraits<ST, CT>::appendString("\\\"", &ret); break;
            default:
                if (c < 32 || c == 127) {
                    EscapeTraits<ST, CT>::appendChar('\\', &ret);
                    EscapeTraits<ST, CT>::appendChar('0' + (c >> 6), &ret);
                    EscapeTraits<ST, CT>::appendChar('0' + ((c >> 3) & 7), &ret);
                    EscapeTraits<ST, CT>::appendChar('0' + (c & 7), &ret);
                } else {
                    EscapeTraits<ST, CT>::appendChar(c, &ret);
                }
        }
    }
    return ret;
}

QString GdbMi::escapeCString(const QString &ba)
{
    return escapeCStringTpl<QString, ushort>(ba);
}

QByteArray GdbMi::escapeCString(const QByteArray &ba)
{
    return escapeCStringTpl<QByteArray, uchar>(ba);
}

QByteArray GdbMi::toString(bool multiline, int indent) const
{
    QByteArray result;
    switch (m_type) {
        case Invalid:
            if (multiline)
                result += ind(indent) + "Invalid\n";
            else
                result += "Invalid";
            break;
        case Const:
            if (!m_name.isEmpty())
                result += m_name + '=';
            result += '"' + escapeCString(m_data) + '"';
            break;
        case Tuple:
            if (!m_name.isEmpty())
                result += m_name + '=';
            if (multiline) {
                result += "{\n";
                dumpChildren(&result, multiline, indent + 1);
                result += '\n' + ind(indent) + '}';
            } else {
                result += '{';
                dumpChildren(&result, multiline, indent + 1);
                result += '}';
            }
            break;
        case List:
            if (!m_name.isEmpty())
                result += m_name + '=';
            if (multiline) {
                result += "[\n";
                dumpChildren(&result, multiline, indent + 1);
                result += '\n' + ind(indent) + ']';
            } else {
                result += '[';
                dumpChildren(&result, multiline, indent + 1);
                result += ']';
            }
            break;
    }
    return result;
}

void GdbMi::fromString(const QByteArray &ba)
{
    const char *from = ba.constBegin();
    const char *to = ba.constEnd();
    parseResultOrValue(from, to);
}

void GdbMi::fromStringMultiple(const QByteArray &ba)
{
    const char *from = ba.constBegin();
    const char *to = ba.constEnd();
    parseTuple_helper(from, to);
}

GdbMi GdbMi::findChild(const char *name) const
{
    for (int i = 0; i < m_children.size(); ++i)
        if (m_children.at(i).m_name == name)
            return m_children.at(i);
    return GdbMi();
}

qulonglong GdbMi::toAddress() const
{
    QByteArray ba = m_data;
    if (ba.endsWith('L'))
        ba.chop(1);
    if (ba.startsWith('*') || ba.startsWith('@'))
        ba = ba.mid(1);
    return ba.toULongLong(0, 0);
}

//////////////////////////////////////////////////////////////////////////////////
//
// GdbResponse
//
//////////////////////////////////////////////////////////////////////////////////

QByteArray GdbResponse::stringFromResultClass(GdbResultClass resultClass)
{
    switch (resultClass) {
        case GdbResultDone: return "done";
        case GdbResultRunning: return "running";
        case GdbResultConnected: return "connected";
        case GdbResultError: return "error";
        case GdbResultExit: return "exit";
        default: return "unknown";
    }
}

QByteArray GdbResponse::toString() const
{
    QByteArray result;
    if (token != -1)
        result = QByteArray::number(token);
    result += '^';
    result += stringFromResultClass(resultClass);
    if (data.isValid())
        result += ',' + data.toString();
    result += '\n';
    return result;
}


//////////////////////////////////////////////////////////////////////////////////
//
// GdbResponse
//
//////////////////////////////////////////////////////////////////////////////////

void extractGdbVersion(const QString &msg,
    int *gdbVersion, int *gdbBuildVersion, bool *isMacGdb, bool *isQnxGdb)
{
    const QChar dot(QLatin1Char('.'));

    QString cleaned;
    QString build;
    bool inClean = true;
    foreach (QChar c, msg) {
        if (inClean && !cleaned.isEmpty() && c != dot && (c.isPunct() || c.isSpace()))
            inClean = false;
        if (inClean) {
            if (c.isDigit())
                cleaned.append(c);
            else if (!cleaned.isEmpty() && !cleaned.endsWith(dot))
                cleaned.append(dot);
        } else {
            if (c.isDigit())
                build.append(c);
            else if (!build.isEmpty() && !build.endsWith(dot))
                build.append(dot);
        }
    }

    *isMacGdb = msg.contains(QLatin1String("Apple version"));
    *isQnxGdb = msg.contains(QLatin1String("qnx"));

    *gdbVersion = 10000 * cleaned.section(dot, 0, 0).toInt()
                  + 100 * cleaned.section(dot, 1, 1).toInt()
                    + 1 * cleaned.section(dot, 2, 2).toInt();
    if (cleaned.count(dot) >= 3)
        *gdbBuildVersion = cleaned.section(dot, 3, 3).toInt();
    else
        *gdbBuildVersion = build.section(dot, 0, 0).toInt();

    if (*isMacGdb)
        *gdbBuildVersion = build.section(dot, 1, 1).toInt();
}

} // namespace Internal
} // namespace Debugger
