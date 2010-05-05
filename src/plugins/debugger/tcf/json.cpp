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

#include "json.h"

#include <utils/qtcassert.h>

#include <QtCore/QByteArray>
#include <QtCore/QTextStream>

#include <ctype.h>

//#define DEBUG_JASON
#ifdef DEBUG_JASON
#define JDEBUG(s) qDebug() << s
#else
#define JDEBUG(s)
#endif

namespace Debugger {
namespace Internal {

static void skipSpaces(const char *&from, const char *to)
{
    while (from != to && isspace(*from))
        ++from;
}

QTextStream &operator<<(QTextStream &os, const JsonValue &mi)
{
    return os << mi.toString();
}

void JsonValue::parsePair(const char *&from, const char *to)
{
    skipSpaces(from, to);
    JDEBUG("parsePair: " << QByteArray(from, to - from));
    m_name = parseCString(from, to);
    skipSpaces(from, to);
    while (from < to && *from != ':') {
        JDEBUG("not a colon" << *from);
        ++from;
    }
    ++from;
    parseValue(from, to);
    skipSpaces(from, to);
}

QByteArray JsonValue::parseNumber(const char *&from, const char *to)
{
    QByteArray result;
    while (from < to && *from >= '0' && *from <= '9')
        result.append(*from++);
    return result;
}

QByteArray JsonValue::parseCString(const char *&from, const char *to)
{
    QByteArray result;
    JDEBUG("parseCString: " << QByteArray(from, to - from));
    if (*from != '"') {
        qDebug() << "JSON Parse Error, double quote expected";
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
                qDebug() << "JSON Parse Error, unterminated backslash escape";
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
                            qDebug() << "JSON Parse Error, unrecognized backslash escape";
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

    JDEBUG("parseCString, got " << result);
    return result;
}

void JsonValue::parseValue(const char *&from, const char *to)
{
    JDEBUG("parseValue: " << QByteArray(from, to - from));
    switch (*from) {
        case '{':
            parseObject(from, to);
            break;
        case '[':
            parseArray(from, to);
            break;
        case '"':
            m_type = String;
            m_data = parseCString(from, to);
            break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            m_type = Number;
            m_data = parseNumber(from, to);
        default:
            break;
    }
}

void JsonValue::parseObject(const char *&from, const char *to)
{
    JDEBUG("parseObject: " << QByteArray(from, to - from));
    QTC_ASSERT(*from == '{', /**/);
    ++from;
    m_type = Object;
    while (from < to) {
        if (*from == '}') {
            ++from;
            break;
        }
        JsonValue child;
        child.parsePair(from, to);
        if (!child.isValid())
            return;
        m_children += child;
        if (*from == ',')
            ++from;
    }
}

void JsonValue::parseArray(const char *&from, const char *to)
{
    JDEBUG("parseArray: " << QByteArray(from, to - from));
    QTC_ASSERT(*from == '[', /**/);
    ++from;
    m_type = Array;
    while (from < to) {
        if (*from == ']') {
            ++from;
            break;
        }
        JsonValue child;
        child.parseValue(from, to);
        if (child.isValid())
            m_children += child;
        if (*from == ',')
            ++from;
    }
}

void JsonValue::setStreamOutput(const QByteArray &name, const QByteArray &content)
{
    if (content.isEmpty())
        return;
    JsonValue child;
    child.m_type = String;
    child.m_name = name;
    child.m_data = content;
    m_children += child;
    if (m_type == Invalid)
        m_type = Object;
}

static QByteArray ind(int indent)
{
    return QByteArray(2 * indent, ' ');
}

void JsonValue::dumpChildren(QByteArray * str, bool multiline, int indent) const
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

QString JsonValue::escapeCString(const QString &ba)
{
    return escapeCStringTpl<MyString, ushort>(static_cast<const MyString &>(ba));
}

QByteArray JsonValue::escapeCString(const QByteArray &ba)
{
    return escapeCStringTpl<QByteArray, uchar>(ba);
}

QByteArray JsonValue::toString(bool multiline, int indent) const
{
    QByteArray result;
    switch (m_type) {
        case Invalid:
            if (multiline)
                result += ind(indent) + "Invalid\n";
            else
                result += "Invalid";
            break;
        case String:
            if (!m_name.isEmpty())
                result += m_name + "=";
            result += '"' + escapeCString(m_data) + '"';
            break;
        case Number:
            if (!m_name.isEmpty())
                result += '"' + m_name + "\":";
            result += m_data;
            break;
        case Object:
            if (!m_name.isEmpty())
                result += m_name + '=';
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
        case Array:
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

void JsonValue::fromString(const QByteArray &ba)
{
    const char *from = ba.constBegin();
    const char *to = ba.constEnd();
    parseValue(from, to);
}

JsonValue JsonValue::findChild(const char *name) const
{
    for (int i = 0; i < m_children.size(); ++i)
        if (m_children.at(i).m_name == name)
            return m_children.at(i);
    return JsonValue();
}

} // namespace Internal
} // namespace Debugger
