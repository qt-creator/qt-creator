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

QTextStream & operator<<(QTextStream & os, const GdbMi & mi)
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

QByteArray GdbMi::toString(bool multiline, int indent) const
{
    QByteArray result;
    switch (m_type) {
    case Invalid:
        if (multiline) {
            result += ind(indent) + "Invalid\n";
        } else {
        result += "Invalid";
        }
        break;
    case Const:
        if (!m_name.isEmpty())
            result += m_name + "=";
        if (multiline) {
        result += "\"" + m_data + "\"";
        } else {
            result += "\"" + m_data + "\"";
        }
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
            result += "]";
        } else {
            result += "[";
            dumpChildren(&result, multiline, indent + 1);
            result += '\n' + ind(indent) + "]";
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

GdbMi GdbMi::findChild(const QByteArray &name) const
{
    for (int i = 0; i < m_children.size(); ++i)
        if (m_children.at(i).m_name == name)
            return m_children.at(i);
    return GdbMi();
}


GdbMi GdbMi::findChild(const QByteArray &name, const QByteArray &defaultData) const
{
    for (int i = 0; i < m_children.size(); ++i)
        if (m_children.at(i).m_name == name)
            return m_children.at(i);
    GdbMi result;
    result.m_data = defaultData;
    return result;
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


//////////////////////////////////////////////////////////////////////////////////
//
// GdbStreamOutput
//
//////////////////////////////////////////////////////////////////////////////////

#if 0

static const char test1[] =
    "1^done,stack=[frame={level=\"0\",addr=\"0x00000000004061ca\","
    "func=\"main\",file=\"test1.cpp\","
    "fullname=\"/home/apoenitz/work/test1/test1.cpp\",line=\"209\"}]\n"
    "(gdb)\n";

static const char test2[] =
    "2^done,stack=[frame={level=\"0\",addr=\"0x00002ac058675840\","
    "func=\"QApplication\",file=\"/home/apoenitz/dev/qt/src/gui/kernel/qapplication.cpp\","
    "fullname=\"/home/apoenitz/dev/qt/src/gui/kernel/qapplication.cpp\",line=\"592\"},"
    "frame={level=\"1\",addr=\"0x00000000004061e0\",func=\"main\",file=\"test1.cpp\","
    "fullname=\"/home/apoenitz/work/test1/test1.cpp\",line=\"209\"}]\n"
    "(gdb)\n";

static const char test3[] =
    "3^done,stack=[frame={level=\"0\",addr=\"0x00000000004061ca\","
    "func=\"main\",file=\"test1.cpp\","
    "fullname=\"/home/apoenitz/work/test1/test1.cpp\",line=\"209\"}]\n"
    "(gdb)\n";

static const char test4[] =
    "&\"source /home/apoenitz/dev/ide/main/bin/gdb/qt4macros\\n\"\n"
    "4^done\n"
    "(gdb)\n";


static const char test5[] =
    "1*stopped,reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"1\","
    "frame={addr=\"0x0000000000405738\",func=\"main\","
    "args=[{name=\"argc\",value=\"1\"},{name=\"argv\",value=\"0x7fff1ac78f28\"}],"
    "file=\"test1.cpp\",fullname=\"/home/apoenitz/work/test1/test1.cpp\","
    "line=\"209\"}\n"
    "(gdb)\n";

static const char test6[] =
    "{u = {u = 2048, v = 16788279, w = -689265400}, a = 1, b = -689265424, c = 11063, s = {static null = {<No data fields>}, static shared_null = {ref = {value = 2}, alloc = 0, size = 0, data = 0x6098da, clean = 0, simpletext = 0, righttoleft = 0, asciiCache = 0, capacity = 0, reserved = 0, array = {0}}, static shared_empty = {ref = {value = 1}, alloc = 0, size = 0, data = 0x2b37d84f8fba, clean = 0, simpletext = 0, righttoleft = 0, asciiCache = 0, capacity = 0, reserved = 0, array = {0}}, d = 0x6098c0, static codecForCStrings = 0x0}}";

static const char test8[] =
    "8^done,data={locals={{name=\"a\"},{name=\"w\"}}}\n"
    "(gdb)\n";

static const char test9[] =
    "9^done,data={locals=[name=\"baz\",name=\"urgs\",name=\"purgs\"]}\n"
    "(gdb)\n";


static const char test10[] =
    "16^done,name=\"urgs\",numchild=\"1\",type=\"Urgs\"\n"
    "(gdb)\n"
    "17^done,name=\"purgs\",numchild=\"1\",type=\"Urgs *\"\n"
    "(gdb)\n"
    "18^done,name=\"bar\",numchild=\"0\",type=\"int\"\n"
    "(gdb)\n"
    "19^done,name=\"z\",numchild=\"0\",type=\"int\"\n"
    "(gdb)\n";

static const char test11[] =
    "[{name=\"size\",value=\"1\",type=\"size_t\",readonly=\"true\"},"
     "{name=\"0\",value=\"one\",type=\"QByteArray\"}]";

static const char test12[] =
    "{iname=\"local.hallo\",value=\"\\\"\\\"\",type=\"QByteArray\",numchild=\"0\"}";

static struct Tester {

    Tester() {
        //test(test10);
        test2(test12);
        //test(test4);
        //apple();
        exit(0);
    }

    void test(const char* input)
    {
        //qDebug("\n<<<<\n%s\n====\n%s\n>>>>\n", input,
           //qPrintable(GdbResponse(input).toString()));
    }

    void test2(const char* input)
    {
        GdbMi mi(input);
        qDebug("\n<<<<\n%s\n====\n%s\n>>>>\n", input,
            qPrintable(mi.toString()));
    }

    void apple()
    {
        QByteArray input(test9);
/*
        qDebug() << "input: " << input;
        input = input.replace("{{","[");
        input = input.replace("},{",",");
        input = input.replace("}}","]");
        qDebug() << "input: " << input;
        GdbResponse response(input);
        qDebug() << "read: " << response.toString();
        GdbMi list = response.results[0].data.findChild("data").findChild("locals");
        QByteArrayList locals;
        foreach (const GdbMi &item, list.children())
            locals.append(item.string());
        qDebug() << "Locals (new): " << locals;
*/
    }
    void parse(const QByteArray &str)
    {
        QByteArray result;
        result += "\n ";
        int indent = 0;
        int from = 0;
        int to = str.size();
        if (str.size() && str[0] == '{' /*'}'*/) {
            ++from;
            --to;
        }
        for (int i = from; i < to; ++i) {
            if (str[i] == '{')
                result += "{\n" + QByteArray(2*++indent + 1, ' ');
            else if (str[i] == '}') {
                if (!result.isEmpty() && result[result.size() - 1] != '\n')
                    result += "\n";
                result += QByteArray(2*--indent + 1, ' ') + "}\n";
            }
            else if (str[i] == ',') {
                if (true || !result.isEmpty() && result[result.size() - 1] != '\n')
                    result += "\n";
                result += QByteArray(2*indent, ' ');
            }
            else
                result += str[i];
        }
        qDebug() << "result:\n" << result;
    }

} dummy;

#endif

} // namespace Internal
} // namespace Debugger
