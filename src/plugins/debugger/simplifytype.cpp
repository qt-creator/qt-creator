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
#include <QRegExp>

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
    while (1) {
        if (type.startsWith(QLatin1String("const")))
            type = type.mid(5);
        else if (type.startsWith(QLatin1Char(' ')))
            type = type.mid(1);
        else if (type.endsWith(QLatin1String("const")))
            type.chop(5);
        else if (type.endsWith(QLatin1Char(' ')))
            type.chop(1);
        else
            break;
    }
    return type;
}

static inline QRegExp stdStringRegExp(const QString &charType)
{
    QString rc = QLatin1String("basic_string<");
    rc += charType;
    rc += QLatin1String(",[ ]?std::char_traits<");
    rc += charType;
    rc += QLatin1String(">,[ ]?std::allocator<");
    rc += charType;
    rc += QLatin1String("> >");
    const QRegExp re(rc);
    QTC_ASSERT(re.isValid(), /**/);
    return re;
}

// Simplify string types in a type
// 'std::set<std::basic_string<char... > >' -> std::set<std::string>'
static inline void simplifyStdString(const QString &charType, const QString &replacement,
                                     QString *type)
{
    QRegExp stringRegexp = stdStringRegExp(charType);
    const int replacementSize = replacement.size();
    for (int pos = 0; pos < type->size(); ) {
        // Check next match
        const int matchPos = stringRegexp.indexIn(*type, pos);
        if (matchPos == -1)
            break;
        const int matchedLength = stringRegexp.matchedLength();
        type->replace(matchPos, matchedLength, replacement);
        pos = matchPos + replacementSize;
        // If we were inside an 'allocator<std::basic_string..char > >'
        // kill the following blank -> 'allocator<std::string>'
        if (pos + 1 < type->size() && type->at(pos) == QLatin1Char(' ')
                && type->at(pos + 1) == QLatin1Char('>'))
            type->remove(pos, 1);
    }
}

// Fix 'std::allocator<std::string >' -> 'std::allocator<std::string>',
// which can happen when replacing/simplifying
static inline QString fixNestedTemplates(QString s)
{
    const int size = s.size();
    if (size > 3
            && s.at(size - 1) == QLatin1Char('>')
            && s.at(size - 2) == QLatin1Char(' ')
            && s.at(size - 3) != QLatin1Char('>'))
        s.remove(size - 2, 1);
    return s;
}

QString simplifyType(const QString &typeIn)
{
    QString type = typeIn;
    if (type.startsWith(QLatin1String("class "))) // MSVC prepends class,struct
        type.remove(0, 6);
    if (type.startsWith(QLatin1String("struct ")))
        type.remove(0, 7);

    const bool isLibCpp = type.contains(QLatin1String("std::__1"));
    type.replace(QLatin1String("std::__cxx11::"), QLatin1String("std::"));
    type.replace(QLatin1String("std::__1::"), QLatin1String("std::"));
    type.replace(QLatin1String("std::__debug::"), QLatin1String("std::"));
    QRegExp simpleStringRE(QString::fromLatin1("std::basic_string<char> ?"));
    type.replace(simpleStringRE, QLatin1String("std::string"));

    // Normalize space + ptr.
    type.replace(QLatin1String(" *"), QLatin1String("@"));
    type.replace(QLatin1Char('*'), QLatin1Char('@'));

    // Normalize char const * and const char *.
    type.replace(QLatin1String("char const@"), QLatin1String("const char@"));

    for (int i = 0; i < 10; ++i) {
        // boost::shared_ptr<...>::element_type
        if (type.startsWith(QLatin1String("boost::shared_ptr<"))
                && type.endsWith(QLatin1String(">::element_type")))
            type = type.mid(18, type.size() - 33);

        // std::shared_ptr<...>::element_type
        if (type.startsWith(QLatin1String("std::shared_ptr<"))
                && type.endsWith(QLatin1String(">::element_type")))
            type = type.mid(16, type.size() - 31);

        // std::ifstream
        QRegExp ifstreamRE(QLatin1String("std::basic_ifstream<char,\\s*std::char_traits<char>\\s*>"));
        ifstreamRE.setMinimal(true);
        QTC_ASSERT(ifstreamRE.isValid(), return typeIn);
        if (ifstreamRE.indexIn(type) != -1)
            type.replace(ifstreamRE.cap(0), QLatin1String("std::ifstream"));


        // std::__1::hash_node<int, void *>::value_type -> int
        if (isLibCpp) {
            //QRegExp hashNodeRE(QLatin1String("std::__hash_node<([^<>]*),\\s*void\\s*@>::value_type"));
            QRegExp hashNodeRE(QLatin1String("std::__hash_node<([^<>]*),\\s*void\\s*@>::value_type"));
            QTC_ASSERT(hashNodeRE.isValid(), return typeIn);
            if (hashNodeRE.indexIn(type) != -1)
                type.replace(hashNodeRE.cap(0), hashNodeRE.cap(1));
        }

        // Anything with a std::allocator
        int start = type.indexOf(QLatin1String("std::allocator<"));
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

            const QString allocEsc = QRegExp::escape(alloc);
            const QString innerEsc = QRegExp::escape(inner);
            if (inner == QLatin1String("char")) { // std::string
                simplifyStdString(QLatin1String("char"), QLatin1String("string"), &type);
            } else if (inner == QLatin1String("wchar_t")) { // std::wstring
                simplifyStdString(QLatin1String("wchar_t"), QLatin1String("wstring"), &type);
            } else if (inner == QLatin1String("unsigned short")) { // std::wstring/MSVC
                simplifyStdString(QLatin1String("unsigned short"), QLatin1String("wstring"), &type);
            }
            // std::vector, std::deque, std::list
            QRegExp re1(QString::fromLatin1("(vector|list|deque)<%1, ?%2\\s*>").arg(innerEsc, allocEsc));
            QTC_ASSERT(re1.isValid(), return typeIn);
            if (re1.indexIn(type) != -1)
                type.replace(re1.cap(0), QString::fromLatin1("%1<%2>").arg(re1.cap(1), inner));

            // std::stack
            QRegExp stackRE(QString::fromLatin1("stack<%1, ?std::deque<%2> >").arg(innerEsc, innerEsc));
            stackRE.setMinimal(true);
            QTC_ASSERT(stackRE.isValid(), return typeIn);
            if (stackRE.indexIn(type) != -1)
                type.replace(stackRE.cap(0), QString::fromLatin1("stack<%1>").arg(inner));

            // std::hash<char>
            QRegExp hashCharRE(QString::fromLatin1("hash<char, std::char_traits<char>, ?%1\\s*>").arg(allocEsc));
            hashCharRE.setMinimal(true);
            QTC_ASSERT(hashCharRE.isValid(), return typeIn);
            if (hashCharRE.indexIn(type) != -1)
                type.replace(hashCharRE.cap(0), QString::fromLatin1("hash<char>"));

            // std::set
            QRegExp setRE(QString::fromLatin1("set<%1, ?std::less<%2>, ?%3\\s*>").arg(innerEsc, innerEsc, allocEsc));
            setRE.setMinimal(true);
            QTC_ASSERT(setRE.isValid(), return typeIn);
            if (setRE.indexIn(type) != -1)
                type.replace(setRE.cap(0), QString::fromLatin1("set<%1>").arg(inner));

            // std::unordered_set
            QRegExp unorderedSetRE(QString::fromLatin1("unordered_(multi)?set<%1, ?std::hash<%2>, ?std::equal_to<%3>, ?%4\\s*>")
                .arg(innerEsc, innerEsc, innerEsc, allocEsc));
            unorderedSetRE.setMinimal(true);
            QTC_ASSERT(unorderedSetRE.isValid(), return typeIn);
            if (unorderedSetRE.indexIn(type) != -1)
                type.replace(unorderedSetRE.cap(0), QString::fromLatin1("unordered_%1set<%2>").arg(unorderedSetRE.cap(1), inner));

            // boost::unordered_set
            QRegExp boostUnorderedSetRE(QString::fromLatin1("unordered_set<%1, ?boost::hash<%2>, ?std::equal_to<%3>, ?%4\\s*>")
                    .arg(innerEsc, innerEsc, innerEsc, allocEsc));
            boostUnorderedSetRE.setMinimal(true);
            QTC_ASSERT(boostUnorderedSetRE.isValid(), return typeIn);
            if (boostUnorderedSetRE.indexIn(type) != -1)
                type.replace(boostUnorderedSetRE.cap(0), QString::fromLatin1("unordered_set<%1>").arg(inner));

            // std::map
            if (inner.startsWith(QLatin1String("std::pair<"))) {
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
                const QString keyEsc = QRegExp::escape(key);
                // Get value: MSVC: 'pair<a const ,b>', gcc: 'pair<const a, b>'
                if (inner.at(++pos) == QLatin1Char(' '))
                    pos++;
                const QString value = inner.mid(pos, inner.size() - pos - 1).trimmed();
                const QString valueEsc = QRegExp::escape(value);
                QRegExp mapRE1(QString::fromLatin1("map<%1, ?%2, ?std::less<%3 ?>, ?%4\\s*>")
                               .arg(keyEsc, valueEsc, keyEsc, allocEsc));
                mapRE1.setMinimal(true);
                QTC_ASSERT(mapRE1.isValid(), return typeIn);
                if (mapRE1.indexIn(type) != -1) {
                    type.replace(mapRE1.cap(0), QString::fromLatin1("map<%1, %2>").arg(key, value));
                } else {
                    QRegExp mapRE2(QString::fromLatin1("map<const %1, ?%2, ?std::less<const %3>, ?%4\\s*>")
                                   .arg(keyEsc, valueEsc, keyEsc, allocEsc));
                    mapRE2.setMinimal(true);
                    if (mapRE2.indexIn(type) != -1)
                        type.replace(mapRE2.cap(0), QString::fromLatin1("map<const %1, %2>").arg(key, value));
                }
            }

            // std::unordered_map
            if (inner.startsWith(QLatin1String("std::pair<"))) {
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
                const QString keyEsc = QRegExp::escape(key);
                // Get value: MSVC: 'pair<a const ,b>', gcc: 'pair<const a, b>'
                if (inner.at(++pos) == QLatin1Char(' '))
                    pos++;
                const QString value = inner.mid(pos, inner.size() - pos - 1).trimmed();
                const QString valueEsc = QRegExp::escape(value);
                QRegExp mapRE1(QString::fromLatin1("unordered_(multi)?map<%1, ?%2, ?std::hash<%3 ?>, ?std::equal_to<%4 ?>, ?%5\\s*>")
                               .arg(keyEsc, valueEsc, keyEsc, keyEsc, allocEsc));
                mapRE1.setMinimal(true);
                QTC_ASSERT(mapRE1.isValid(), return typeIn);
                if (mapRE1.indexIn(type) != -1)
                    type.replace(mapRE1.cap(0), QString::fromLatin1("unordered_%1map<%2, %3>").arg(mapRE1.cap(1), key, value));

                if (isLibCpp) {
                    QRegExp mapRE2(QString::fromLatin1("unordered_map<std::string, ?%1, "
                                    "?std::hash<char>, ?std::equal_to<std::string>, ?%2\\s*>")
                                   .arg(valueEsc, allocEsc));
                    mapRE2.setMinimal(true);
                    QTC_ASSERT(mapRE2.isValid(), return typeIn);
                    if (mapRE2.indexIn(type) != -1)
                        type.replace(mapRE2.cap(0), QString::fromLatin1("unordered_map<std::string, %2>").arg(value));
                }
            }
        } // with std::allocator
    }
    type.replace(QLatin1Char('@'), QLatin1String(" *"));
    type.replace(QLatin1String(" >"), QLatin1String(">"));
    return type;
}

} // namespace Internal
} // namespace Debugger
