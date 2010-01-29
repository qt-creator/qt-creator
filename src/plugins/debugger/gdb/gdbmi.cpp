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

#include "gdbmi.h"

#include <utils/qtcassert.h>

#include <QtCore/QByteArray>
#include <QtCore/QTextStream>

#include <ctype.h>

namespace Debugger {
namespace Internal {

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
    QTC_ASSERT(*from == '{', /**/);
    ++from;
    parseTuple_helper(from, to);
}

void GdbMi::parseTuple_helper(const char *&from, const char *to)
{
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
        if (*from == ',')
            ++from;
    }
}

void GdbMi::parseList(const char *&from, const char *to)
{
    //qDebug() << "parseList: " << QByteArray(from, to - from);
    QTC_ASSERT(*from == '[', /**/);
    ++from;
    m_type = List;
    while (from < to) {
        if (*from == ']') {
            ++from;
            break;
        }
        GdbMi child;
        child.parseResultOrValue(from, to);
        if (child.isValid())
            m_children += child;
        if (*from == ',')
            ++from;
    }
}

void GdbMi::setStreamOutput(const QByteArray &name, const QByteArray &content)
{
    if (content.isEmpty())
        return;
    GdbMi child;
    child.m_type = Const;
    child.m_name = name;
    child.m_data = content;
    m_children += child;
    if (m_type == Invalid)
        m_type = Tuple;
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

class MyString : public QString {
public:
    ushort at(int i) const { return constData()[i].unicode(); }
};

template<class ST, typename CT>
inline ST escapeCStringTpl(const ST &ba)
{
    ST ret;
    ret.reserve(ba.length() * 2);
    for (int i = 0; i < ba.length(); ++i) {
        CT c = ba.at(i);
        switch (c) {
            case '\\': ret += "\\\\"; break;
            case '\a': ret += "\\a"; break;
            case '\b': ret += "\\b"; break;
            case '\f': ret += "\\f"; break;
            case '\n': ret += "\\n"; break;
            case '\r': ret += "\\r"; break;
            case '\t': ret += "\\t"; break;
            case '\v': ret += "\\v"; break;
            case '"': ret += "\\\""; break;
            default:
                if (c < 32 || c == 127) {
                    ret += '\\';
                    ret += '0' + (c >> 6);
                    ret += '0' + ((c >> 3) & 7);
                    ret += '0' + (c & 7);
                } else {
                    ret += c;
                }
        }
    }
    return ret;
}

QString GdbMi::escapeCString(const QString &ba)
{
    return escapeCStringTpl<MyString, ushort>(static_cast<const MyString &>(ba));
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
                result += m_name + "=";
            result += "\"" + escapeCString(m_data) + "\"";
            break;
        case Tuple:
            if (!m_name.isEmpty())
                result += m_name + "=";
            if (multiline) {
                result += "{\n";
                dumpChildren(&result, multiline, indent + 1);
                result += '\n' + ind(indent) + "}";
            } else {
                result += "{";
                dumpChildren(&result, multiline, indent + 1);
                result += "}";
            }
            break;
        case List:
            if (!m_name.isEmpty())
                result += m_name + "=";
            if (multiline) {
                result += "[\n";
                dumpChildren(&result, multiline, indent + 1);
                result += '\n' + ind(indent) + "]";
            } else {
                result += "[";
                dumpChildren(&result, multiline, indent + 1);
                result += "]";
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

GdbMi GdbMi::findChild(const char *name) const
{
    for (int i = 0; i < m_children.size(); ++i)
        if (m_children.at(i).m_name == name)
            return m_children.at(i);
    return GdbMi();
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
};

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

} // namespace Internal
} // namespace Debugger
