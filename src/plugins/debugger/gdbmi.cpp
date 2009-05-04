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

#include "gdbmi.h"

#include <utils/qtcassert.h>

#include <QtCore/QByteArray>
#include <QtCore/QTextStream>

namespace Debugger {
namespace Internal {

QTextStream &operator<<(QTextStream &os, const GdbMi &mi)
{
    return os << mi.toString();
}

//static void skipSpaces(const char *&from, const char *to)
//{
//    while (from != to && QChar(*from).isSpace())
//        ++from;
//}


void GdbMi::parseResultOrValue(const char *&from, const char *to)
{
    //skipSpaces(from, to);
    while (from != to && QChar(*from).isSpace())
        ++from;

    //qDebug() << "parseResultOrValue: " << QByteArray(from, to - from);
    parseValue(from, to);
    if (isValid()) {
        //qDebug() << "no valid result in " << QByteArray::fromLatin1(from, to - from);
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
    //qDebug() << "parseCString: " << QByteArray::fromUtf16(from, to - from);
    if (*from != '"') {
        qDebug() << "MI Parse Error, double quote expected";
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
        if (*ptr == '\\' && ptr < to - 1)
            ++ptr;
        ++ptr;
    }

    if (result.contains('\\')) {
        if (result.contains("\\032\\032"))
            result.clear();
        else {
            result = result.replace("\\n", "\n");
            result = result.replace("\\t", "\t");
            result = result.replace("\\\"", "\"");
        }
    }

    from = ptr;
    return result;
}

void GdbMi::parseValue(const char *&from, const char *to)
{
    //qDebug() << "parseValue: " << QByteArray::fromUtf16(from, to - from);
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
    //qDebug() << "parseTuple: " << QByteArray::fromUtf16(from, to - from);
    QTC_ASSERT(*from == '{', /**/);
    ++from;
    parseTuple_helper(from, to);
}

void GdbMi::parseTuple_helper(const char *&from, const char *to)
{
    //qDebug() << "parseTuple_helper: " << QByteArray::fromUtf16(from, to - from);
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
    //qDebug() << "parseList: " << QByteArray::fromUtf16(from, to - from);
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

static QByteArray escaped(QByteArray ba)
{
    ba.replace("\"", "\\\"");
    return ba;
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
            result += "\"" + escaped(m_data) + "\"";
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
// GdbResultRecord
//
//////////////////////////////////////////////////////////////////////////////////

QByteArray stringFromResultClass(GdbResultClass resultClass)
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

QByteArray GdbResultRecord::toString() const
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
