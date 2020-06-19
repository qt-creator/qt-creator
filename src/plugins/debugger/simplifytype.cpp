/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "simplifytype.h"

#include <QDebug>
#include <QRegularExpression>

#include <ctype.h>

#define QTC_ASSERT_STRINGIFY_HELPER(x) #x
#define QTC_ASSERT_STRINGIFY(x) QTC_ASSERT_STRINGIFY_HELPER(x)
#define QTC_ASSERT_STRING(cond) qDebug("SOFT ASSERT: \"" cond"\" in file " __FILE__ ", line " QTC_ASSERT_STRINGIFY(__LINE__))
#define QTC_ASSERT(cond, action) if (cond) {} else { QTC_ASSERT_STRING(#cond); action; } do {} while (0)

namespace Debugger {
namespace Internal {

// Simplify complicated STL template types,
// such as 'std::basic_string<char,std::char_traits<char>,std::allocator<char> >'
// -> 'std::string' and helpers.

static QString chopConst(QString type)
{
    while (true) {
        if (type.startsWith("const"))
            type = type.mid(5);
        else if (type.startsWith(' '))
            type = type.mid(1);
        else if (type.endsWith("const"))
            type.chop(5);
        else if (type.endsWith(' '))
            type.chop(1);
        else
            break;
    }
    return type;
}

static inline QRegularExpression stdStringRegExp(const QString &charType)
{
    QString rc = "basic_string<";
    rc += charType;
    rc += ",[ ]?std::char_traits<";
    rc += charType;
    rc += ">,[ ]?std::allocator<";
    rc += charType;
    rc += "> >";
    const QRegularExpression re(rc);
    QTC_ASSERT(re.isValid(), /**/);
    return re;
}

// Simplify string types in a type
// 'std::set<std::basic_string<char... > >' -> std::set<std::string>'
static inline void simplifyStdString(const QString &charType, const QString &replacement,
                                     QString *type)
{
    QRegularExpression stringRegexp = stdStringRegExp(charType);
    const int replacementSize = replacement.size();
    for (int pos = 0; pos < type->size(); ) {
        // Check next match
        const QRegularExpressionMatch match = stringRegexp.match(*type, pos);
        if (!match.hasMatch())
            break;
        const int matchPos = match.capturedStart();
        const int matchedLength = match.capturedLength();
        type->replace(matchPos, matchedLength, replacement);
        pos = matchPos + replacementSize;
        // If we were inside an 'allocator<std::basic_string..char > >'
        // kill the following blank -> 'allocator<std::string>'
        if (pos + 1 < type->size() && type->at(pos) == ' '
                && type->at(pos + 1) == '>')
            type->remove(pos, 1);
    }
}

// Fix 'std::allocator<std::string >' -> 'std::allocator<std::string>',
// which can happen when replacing/simplifying
static inline QString fixNestedTemplates(QString s)
{
    const int size = s.size();
    if (size > 3
            && s.at(size - 1) == '>'
            && s.at(size - 2) == ' '
            && s.at(size - 3) != '>')
        s.remove(size - 2, 1);
    return s;
}

QString simplifyType(const QString &typeIn)
{
    QString type = typeIn;
    if (type.startsWith("class ")) // MSVC prepends class,struct
        type.remove(0, 6);
    if (type.startsWith("struct "))
        type.remove(0, 7);

    type.replace("short int", "short");
    type.replace("long long int", "long long");

    const bool isLibCpp = type.contains("std::__1");
    type.replace("std::__cxx11::", "std::");
    type.replace("std::__1::", "std::");
    type.replace("std::__debug::", "std::");
    QRegularExpression simpleStringRE("std::basic_string<char> ?");
    type.replace(simpleStringRE, "std::string");

    // Normalize space + ptr.
    type.replace(" *", "@");
    type.replace('*', '@');

    // Normalize char const * and const char *.
    type.replace("char const@", "const char@");

    for (int i = 0; i < 10; ++i) {
        // boost::shared_ptr<...>::element_type
        if (type.startsWith("boost::shared_ptr<")
                && type.endsWith(">::element_type"))
            type = type.mid(18, type.size() - 33);

        // std::shared_ptr<...>::element_type
        if (type.startsWith("std::shared_ptr<")
                && type.endsWith(">::element_type"))
            type = type.mid(16, type.size() - 31);

        // std::ifstream
        QRegularExpression ifstreamRE("std::basic_ifstream<char,\\s*?std::char_traits<char>\\s*?>");
        QTC_ASSERT(ifstreamRE.isValid(), return typeIn);
        const QRegularExpressionMatch match = ifstreamRE.match(type);
        if (match.hasMatch())
            type.replace(match.captured(), "std::ifstream");


        // std::__1::hash_node<int, void *>::value_type -> int
        if (isLibCpp) {
            QRegularExpression hashNodeRE("std::__hash_node<([^<>]*),\\s*void\\s*@>::value_type");
            QTC_ASSERT(hashNodeRE.isValid(), return typeIn);
            const QRegularExpressionMatch match = hashNodeRE.match(type);
            if (match.hasMatch())
                type.replace(match.captured(), match.captured(1));
        }

        // Anything with a std::allocator
        int start = type.indexOf("std::allocator<");
        if (start != -1) {
            // search for matching '>'
            int pos;
            int level = 0;
            for (pos = start + 12; pos < type.size(); ++pos) {
                int c = type.at(pos).unicode();
                if (c == '<') {
                    ++level;
                } else if (c == '>') {
                    --level;
                    if (level == 0)
                        break;
                }
            }
            const QString alloc = fixNestedTemplates(type.mid(start, pos + 1 - start).trimmed());
            const QString inner = fixNestedTemplates(alloc.mid(15, alloc.size() - 16).trimmed());

            const QString allocEsc = QRegularExpression::escape(alloc);
            const QString innerEsc = QRegularExpression::escape(inner);
            if (inner == "char") { // std::string
                simplifyStdString("char", "string", &type);
            } else if (inner == "wchar_t") { // std::wstring
                simplifyStdString("wchar_t", "wstring", &type);
            } else if (inner == "unsigned short") { // std::wstring/MSVC
                simplifyStdString("unsigned short", "wstring", &type);
            }
            // std::vector, std::deque, std::list
            QRegularExpression re1(QString::fromLatin1("(vector|list|deque)<%1, ?%2\\s*>").arg(innerEsc, allocEsc));
            QTC_ASSERT(re1.isValid(), return typeIn);
            QRegularExpressionMatch match = re1.match(type);
            if (match.hasMatch())
                type.replace(match.captured(), QString::fromLatin1("%1<%2>").arg(match.captured(1), inner));

            // std::stack
            QRegularExpression stackRE(QString::fromLatin1("stack<%1, ?std::deque<%2> >").arg(innerEsc, innerEsc));
            QTC_ASSERT(stackRE.isValid(), return typeIn);
            match = stackRE.match(type);
            if (match.hasMatch())
                type.replace(match.captured(), QString::fromLatin1("stack<%1>").arg(inner));

            // std::hash<char>
            QRegularExpression hashCharRE(QString::fromLatin1("hash<char, std::char_traits<char>, ?%1\\s*?>").arg(allocEsc));
            QTC_ASSERT(hashCharRE.isValid(), return typeIn);
            match = hashCharRE.match(type);
            if (match.hasMatch())
                type.replace(match.captured(), QString::fromLatin1("hash<char>"));

            // std::set
            QRegularExpression setRE(QString::fromLatin1("set<%1, ?std::less<%2>, ?%3\\s*?>").arg(innerEsc, innerEsc, allocEsc));
            QTC_ASSERT(setRE.isValid(), return typeIn);
            match = setRE.match(type);
            if (match.hasMatch())
                type.replace(match.captured(), QString::fromLatin1("set<%1>").arg(inner));

            // std::unordered_set
            QRegularExpression unorderedSetRE(QString::fromLatin1("unordered_(multi)?set<%1, ?std::hash<%2>, ?std::equal_to<%3>, ?%4\\s*?>")
                .arg(innerEsc, innerEsc, innerEsc, allocEsc));
            QTC_ASSERT(unorderedSetRE.isValid(), return typeIn);
            match = unorderedSetRE.match(type);
            if (match.hasMatch())
                type.replace(match.captured(), QString::fromLatin1("unordered_%1set<%2>").arg(match.captured(1), inner));

            // boost::unordered_set
            QRegularExpression boostUnorderedSetRE(QString::fromLatin1("unordered_set<%1, ?boost::hash<%2>, ?std::equal_to<%3>, ?%4\\s*?>")
                    .arg(innerEsc, innerEsc, innerEsc, allocEsc));
            QTC_ASSERT(boostUnorderedSetRE.isValid(), return typeIn);
            match = boostUnorderedSetRE.match(type);
            if (match.hasMatch())
                type.replace(match.captured(), QString::fromLatin1("unordered_set<%1>").arg(inner));

            // std::map
            if (inner.startsWith("std::pair<")) {
                // search for outermost ',', split key and value
                int pos;
                int level = 0;
                for (pos = 10; pos < inner.size(); ++pos) {
                    int c = inner.at(pos).unicode();
                    if (c == '<')
                        ++level;
                    else if (c == '>')
                        --level;
                    else if (c == ',' && level == 0)
                        break;
                }
                const QString key = chopConst(inner.mid(10, pos - 10));
                const QString keyEsc = QRegularExpression::escape(key);
                // Get value: MSVC: 'pair<a const ,b>', gcc: 'pair<const a, b>'
                if (inner.at(++pos) == ' ')
                    pos++;
                const QString value = inner.mid(pos, inner.size() - pos - 1).trimmed();
                const QString valueEsc = QRegularExpression::escape(value);
                QRegularExpression mapRE1(QString::fromLatin1("map<%1, ?%2, ?std::less<%3 ?>, ?%4\\s*?>")
                                          .arg(keyEsc, valueEsc, keyEsc, allocEsc));
                QTC_ASSERT(mapRE1.isValid(), return typeIn);
                match = mapRE1.match(type);
                if (match.hasMatch()) {
                    type.replace(match.captured(), QString::fromLatin1("map<%1, %2>").arg(key, value));
                } else {
                    QRegularExpression mapRE2(QString::fromLatin1("map<const %1, ?%2, ?std::less<const %3>, ?%4\\s*?>")
                                              .arg(keyEsc, valueEsc, keyEsc, allocEsc));
                    match = mapRE2.match(type);
                    if (match.hasMatch())
                        type.replace(match.captured(), QString::fromLatin1("map<const %1, %2>").arg(key, value));
                }
            }

            // std::unordered_map
            if (inner.startsWith("std::pair<")) {
                // search for outermost ',', split key and value
                int pos;
                int level = 0;
                for (pos = 10; pos < inner.size(); ++pos) {
                    int c = inner.at(pos).unicode();
                    if (c == '<')
                        ++level;
                    else if (c == '>')
                        --level;
                    else if (c == ',' && level == 0)
                        break;
                }
                const QString key = chopConst(inner.mid(10, pos - 10));
                const QString keyEsc = QRegularExpression::escape(key);
                // Get value: MSVC: 'pair<a const ,b>', gcc: 'pair<const a, b>'
                if (inner.at(++pos) == ' ')
                    pos++;
                const QString value = inner.mid(pos, inner.size() - pos - 1).trimmed();
                const QString valueEsc = QRegularExpression::escape(value);
                QRegularExpression mapRE1(QString::fromLatin1("unordered_(multi)?map<%1, ?%2, ?std::hash<%3 ?>, ?std::equal_to<%4 ?>, ?%5\\s*?>")
                               .arg(keyEsc, valueEsc, keyEsc, keyEsc, allocEsc));
                QTC_ASSERT(mapRE1.isValid(), return typeIn);
                match = mapRE1.match(type);
                if (match.hasMatch())
                    type.replace(match.captured(), QString::fromLatin1("unordered_%1map<%2, %3>").arg(match.captured(1), key, value));

                if (isLibCpp) {
                    QRegularExpression mapRE2(QString::fromLatin1("unordered_map<std::string, ?%1, "
                                                                  "?std::hash<char>, ?std::equal_to<std::string>, ?%2\\s*?>")
                                              .arg(valueEsc, allocEsc));
                    QTC_ASSERT(mapRE2.isValid(), return typeIn);
                    match = mapRE2.match(type);
                    if (match.hasMatch())
                        type.replace(match.captured(), QString::fromLatin1("unordered_map<std::string, %2>").arg(value));
                }
            }
        } // with std::allocator
    }
    type.replace('@', " *");
    type.replace(" >", ">");
    return type;
}

} // namespace Internal
} // namespace Debugger
