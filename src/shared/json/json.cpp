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

#include "json.h"

#ifdef TODO_USE_CREATOR
#include <utils/qtcassert.h>
#endif // TODO_USE_CREATOR

#include <QByteArray>
#include <QTextStream>
#include <QDebug>
#include <QStringList>
#include <QVariant>

#include <ctype.h>

#ifdef DEBUG_JASON
#define JDEBUG(s) qDebug() << s
#else
#define JDEBUG(s)
#endif

using namespace Json;

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
    if (from < to && *from == '-') // Leading '-'.
        result.append(*from++);
    while (from < to && *from >= '0' && *from <= '9')
        result.append(*from++);
    return result;
}

QByteArray JsonValue::parseCString(const char *&from, const char *to)
{
    QByteArray result;
    const char * const fromSaved = from;
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
                qWarning("JSON Parse Error, unterminated backslash escape in '%s'",
                         QByteArray(fromSaved, to - fromSaved).constData());
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
                case 'u':  { // 4 digit hex escape as in '\u000a'
                    if (end - src < 4) {
                        qWarning("JSON Parse Error, too few hex digits in \\u-escape in '%s' obtained from '%s'",
                                 result.constData(), QByteArray(fromSaved, to - fromSaved).constData());
                        return QByteArray();
                    }
                    bool ok;
                    const uchar prod = QByteArray(src, 4).toUInt(&ok, 16);
                    if (!ok) {
                        qWarning("JSON Parse Error, invalid hex digits in \\u-escape in '%s' obtained from '%s'",
                                  result.constData(), QByteArray(fromSaved, to - fromSaved).constData());
                         return QByteArray();
                    }
                    *dst++ = prod;
                    src += 4;
            }
                    break;
                default: { // Up to 3 decimal digits: Not sure if this is supported in JSON?
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
                            qWarning("JSON Parse Error, unrecognized backslash escape in string '%s' obtained from '%s'",
                                     result.constData(), QByteArray(fromSaved, to - fromSaved).constData());
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
        case 't':
            if (to - from >= 4 && qstrncmp(from, "true", 4) == 0) {
                m_data = QByteArray(from, 4);
                from += m_data.size();
                m_type = Boolean;
            }
            break;
        case 'f':
            if (to - from >= 5 && qstrncmp(from, "false", 5) == 0) {
                m_data = QByteArray(from, 5);
                from += m_data.size();
                m_type = Boolean;
            }
            break;
        case 'n':
            if (to - from >= 4 && qstrncmp(from, "null", 4) == 0) {
                m_data = QByteArray(from, 4);
                from += m_data.size();
                m_type = NullObject;
            }
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
        case '-':
            m_type = Number;
            m_data = parseNumber(from, to);
        default:
            break;
    }
}

void JsonValue::parseObject(const char *&from, const char *to)
{
    JDEBUG("parseObject: " << QByteArray(from, to - from));
#ifdef TODO_USE_CREATOR
    QTC_CHECK(*from == '{');
#endif
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
#ifdef TODO_USE_CREATOR
    QTC_CHECK(*from == '[');
#endif
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
                result += m_name + '=';
            result += '"' + escapeCString(m_data) + '"';
            break;
        case Number:
            if (!m_name.isEmpty())
                result += '"' + m_name + "\":";
            result += m_data;
            break;
        case Boolean:
        case NullObject:
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
                result += '\n' + ind(indent) + '}';
            } else {
                result += '{';
                dumpChildren(&result, multiline, indent + 1);
                result += '}';
            }
            break;
        case Array:
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


QVariant JsonValue::toVariant() const
{
    switch (m_type) {
    case String:
        return QString(m_data);
    case Number: {
        bool ok;
        qint64 val = QString(m_data).toLongLong(&ok);
        if (ok)
            return val;
        return QVariant();
    }
    case Object: {
        QHash<QString, QVariant> hash;
        for (int i = 0; i < m_children.size(); ++i) {
            QString name(m_children[i].name());
            QVariant val = m_children[i].toVariant();
            hash.insert(name, val);
        }
        return hash;
    }
    case Array: {
        QList<QVariant> list;
        for (int i = 0; i < m_children.size(); ++i) {
            list.append(m_children[i].toVariant());
        }
        return list;
    }
    case Boolean:
        return data() == QByteArray("true");
    case Invalid:
    case NullObject:
    default:
        return QVariant();
    }
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

void JsonInputStream::appendCString(const char *s)
{
    m_target.append('"');
    for (const char *p = s; *p; p++) {
        if (*p == '"' || *p == '\\')
            m_target.append('\\');
        m_target.append(*p);
    }
    m_target.append('"');
}

void JsonInputStream::appendString(const QString &in)
{
    if (in.isEmpty()) {
        m_target.append("\"\"");
        return;
    }

    const QChar doubleQuote('"');
    const QChar backSlash('\\');
    QString rc;
    const int inSize = in.size();
    rc.reserve(in.size() + 5);
    rc.append(doubleQuote);
    for (int i = 0; i < inSize; i++) {
        const QChar c = in.at(i);
        if (c == doubleQuote || c == backSlash)
            rc.append(backSlash);
        rc.append(c);
    }
    rc.append(doubleQuote);
    m_target.append(rc.toUtf8());
    return;
}

JsonInputStream &JsonInputStream::operator<<(const QStringList &in)
{
    m_target.append('[');
    const int count = in.size();
    for (int i = 0 ; i < count; i++) {
        if (i)
            m_target.append(',');
        appendString(in.at(i));
    }
    m_target.append(']');
    return *this;
}

JsonInputStream &JsonInputStream::operator<<(const QVector<QByteArray> &ba)
{
    m_target.append('[');
    const int count = ba.size();
    for (int i = 0 ; i < count; i++) {
        if (i)
            m_target.append(',');
        appendCString(ba.at(i).constData());
    }
    m_target.append(']');
    return *this;
}

JsonInputStream &JsonInputStream::operator<<(const QList<int> &in)
{
    m_target.append('[');
    const int count = in.size();
    for (int i = 0 ; i < count; i++) {
        if (i)
            m_target.append(',');
        m_target.append(QByteArray::number(in.at(i)));
    }
    m_target.append(']');
    return *this;
}

JsonInputStream &JsonInputStream::operator<<(bool b)
{
    m_target.append(b ? "true" : "false");
    return *this;
}


