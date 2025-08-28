// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "simplifytype.h"

#include <QDebug>
#include <QRegularExpression>

#include <ctype.h>

#define QTC_ASSERT_STRINGIFY_HELPER(x) #x
#define QTC_ASSERT_STRINGIFY(x) QTC_ASSERT_STRINGIFY_HELPER(x)
#define QTC_ASSERT_STRING(cond) qDebug("SOFT ASSERT: \"" cond"\" in file " __FILE__ ", line " QTC_ASSERT_STRINGIFY(__LINE__))
#define QTC_ASSERT(cond, action) if (cond) {} else { QTC_ASSERT_STRING(#cond); action; } do {} while (0)

namespace Debugger::Internal {

// Simplify complicated STL template types,
// such as 'std::basic_string<char,std::char_traits<char>,std::allocator<char> >'
// -> 'std::string' and helpers.

static QString removeExcessiveSpacesInTemplateTypes(const QString &type)
{
    const QString simplified = type.simplified();
    QString ret;
    QString::size_type start = 0;
    for (QString::size_type i = 0; i < simplified.size(); ++i) {
        const QChar chr = simplified[i];
        if (chr == '<' || chr == '>') {
            ret += simplified.mid(start, i - start).trimmed();
            ret += chr;
            start = i + 1;
        }
    }
    ret += simplified.mid(start).trimmed();

    return ret;
}

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
    rc += ">>";
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
    }
}

static void simplifyAllocator(
    const QString &allocatorTemplateHead,
    const QString &containerTypePrefix,
    const bool isLibCpp,
    QString &type)
{
    const int allocatorTemplateHeadLength = allocatorTemplateHead.length();
    int start = type.indexOf(allocatorTemplateHead);
    if (start != -1) {
        // search for matching '>'
        int pos;
        int level = 0;
        for (pos = start + allocatorTemplateHeadLength - 1; pos < type.size(); ++pos) {
            int c = type.at(pos).unicode();
            if (c == '<') {
                ++level;
            } else if (c == '>') {
                --level;
                if (level == 0)
                    break;
            }
        }
        const QString alloc = type.mid(start, pos + 1 - start).trimmed();
        const QString inner
            = alloc.mid(allocatorTemplateHeadLength, alloc.size() - allocatorTemplateHeadLength - 1)
                  .trimmed();

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
        QRegularExpression re1(QString::fromLatin1("(vector|(forward_)?list|deque)<%1, ?%2\\s*>")
                                   .arg(innerEsc, allocEsc));
        QTC_ASSERT(re1.isValid(), return);
        QRegularExpressionMatch match = re1.match(type);
        if (match.hasMatch())
            type.replace(
                match.captured(),
                QString::fromLatin1("%1%2<%3>").arg(containerTypePrefix, match.captured(1), inner));

        // std::stack
        QRegularExpression stackRE(
            QString::fromLatin1("stack<%1, ?std::deque<%2>>").arg(innerEsc, innerEsc));
        QTC_ASSERT(stackRE.isValid(), return);
        match = stackRE.match(type);
        if (match.hasMatch())
            type.replace(
                match.captured(),
                QString::fromLatin1("%1stack<%2>").arg(containerTypePrefix, inner));

        // std::hash<char>
        QRegularExpression hashCharRE(
            QString::fromLatin1("hash<char, std::char_traits<char>, ?%1\\s*?>").arg(allocEsc));
        QTC_ASSERT(hashCharRE.isValid(), return);
        match = hashCharRE.match(type);
        if (match.hasMatch())
            type.replace(match.captured(), QString::fromLatin1("hash<char>"));

        // std::set
        QRegularExpression setRE(QString::fromLatin1("(multi)?set<%1, ?std::less<%2>, ?%3\\s*?>")
                                     .arg(innerEsc, innerEsc, allocEsc));
        QTC_ASSERT(setRE.isValid(), return);
        match = setRE.match(type);
        if (match.hasMatch())
            type.replace(
                match.captured(),
                QString::fromLatin1("%1%2set<%3>")
                    .arg(containerTypePrefix, match.captured(1), inner));

        // std::unordered_set
        QRegularExpression unorderedSetRE(
            QString::fromLatin1(
                "unordered_(multi)?set<%1, ?std::hash<%2>, ?std::equal_to<%3>, ?%4\\s*?>")
                .arg(innerEsc, innerEsc, innerEsc, allocEsc));
        QTC_ASSERT(unorderedSetRE.isValid(), return);
        match = unorderedSetRE.match(type);
        if (match.hasMatch())
            type.replace(
                match.captured(),
                QString::fromLatin1("%1unordered_%2set<%3>")
                    .arg(containerTypePrefix, match.captured(1), inner));

        // boost::unordered_set
        QRegularExpression boostUnorderedSetRE(
            QString::fromLatin1("unordered_set<%1, ?boost::hash<%2>, ?std::equal_to<%3>, ?%4\\s*?>")
                .arg(innerEsc, innerEsc, innerEsc, allocEsc));
        QTC_ASSERT(boostUnorderedSetRE.isValid(), return);
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
            QRegularExpression mapRE1(
                QString::fromLatin1("(multi)?map<%1, ?%2, ?std::less<%3 ?>, ?%4\\s*?>")
                    .arg(keyEsc, valueEsc, keyEsc, allocEsc));
            QTC_ASSERT(mapRE1.isValid(), return);
            match = mapRE1.match(type);
            if (match.hasMatch()) {
                type.replace(
                    match.captured(),
                    QString::fromLatin1("%1%2map<%3, %4>")
                        .arg(containerTypePrefix, match.captured(1), key, value));
            } else {
                QRegularExpression mapRE2(
                    QString::fromLatin1(
                        "(multi)?map<const %1, ?%2, ?std::less<const %3>, ?%4\\s*?>")
                        .arg(keyEsc, valueEsc, keyEsc, allocEsc));
                match = mapRE2.match(type);
                if (match.hasMatch())
                    type.replace(
                        match.captured(),
                        QString::fromLatin1("%1%2map<const %3, %4>")
                            .arg(containerTypePrefix, match.captured(1), key, value));
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
            QRegularExpression mapRE1(
                QString::fromLatin1("unordered_(multi)?map<%1, ?%2, ?std::hash<%3 ?>, "
                                    "?std::equal_to<%4 ?>, ?%5\\s*?>")
                    .arg(keyEsc, valueEsc, keyEsc, keyEsc, allocEsc));
            QTC_ASSERT(mapRE1.isValid(), return);
            match = mapRE1.match(type);
            if (match.hasMatch())
                type.replace(
                    match.captured(),
                    QString::fromLatin1("%1unordered_%2map<%3, %4>")
                        .arg(containerTypePrefix, match.captured(1), key, value));

            if (isLibCpp) {
                QRegularExpression mapRE2(
                    QString::fromLatin1("unordered_(multi)?map<std::string, ?%1, "
                                        "?std::hash<char>, ?std::equal_to<std::string>, ?%2\\s*?>")
                        .arg(valueEsc, allocEsc));
                QTC_ASSERT(mapRE2.isValid(), return);
                match = mapRE2.match(type);
                if (match.hasMatch())
                    type.replace(
                        match.captured(),
                        QString::fromLatin1("%1unordered_%2map<std::string, %3>")
                            .arg(containerTypePrefix, match.captured(1), value));
            }
        }
    }
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

    type = removeExcessiveSpacesInTemplateTypes(type);

    static const QRegularExpression simpleStringRE(
        "std::basic_string<char(, ?std::char_traits<char>)?(, ?std::allocator<char>)? ?>");
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
        static const QRegularExpression ifstreamRE("std::basic_ifstream<char,\\s*?std::char_traits<char>\\s*?>");
        QTC_ASSERT(ifstreamRE.isValid(), return typeIn);
        const QRegularExpressionMatch match = ifstreamRE.match(type);
        if (match.hasMatch())
            type.replace(match.captured(), "std::ifstream");


        // std::__1::hash_node<int, void *>::value_type -> int
        if (isLibCpp) {
            static const QRegularExpression hashNodeRE("std::__hash_node<([^<>]*),\\s*void\\s*@>::value_type");
            QTC_ASSERT(hashNodeRE.isValid(), return typeIn);
            const QRegularExpressionMatch match = hashNodeRE.match(type);
            if (match.hasMatch())
                type.replace(match.captured(), match.captured(1));
        }

        // Fix e.g. `std::vector<T, std::allocator<T>> -> std::vector<T>`
        simplifyAllocator("std::allocator<", "", isLibCpp, type);
        // Fix e.g. `std::vector<T, std::pmr::polynorphic_allocator<T>> -> std::pmr::vector<T>`
        simplifyAllocator("std::pmr::polymorphic_allocator<", "pmr::", isLibCpp, type);
    }
    type.replace('@', " *");
    return type;
}

} // Debugger::Internal
