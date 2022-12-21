// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "snippet.h"

#include "languageclientplugin.h"

#ifdef WITH_TESTS
#include <QtTest>
#endif

using namespace TextEditor;

namespace LanguageClient {

constexpr char dollar = '$';
constexpr char backSlash = '\\';
constexpr char underscore = '_';
constexpr char comma = ',';
constexpr char openBrace = '{';
constexpr char closeBrace = '}';
constexpr char pipe = '|';
constexpr char colon = ':';

class SnippetParseException
{
public:
    QString message;
};

void skipSpaces(QString::const_iterator &it)
{
    while (it->isSpace())
        ++it;
}

QString join(const QList<QChar> &chars)
{
    QString result;
    const QList<QChar>::const_iterator begin = chars.begin();
    const QList<QChar>::const_iterator end = chars.end();
    for (QList<QChar>::const_iterator it = begin; it < end; ++it) {
        if (it == begin)
            result += "'";
        else if (it + 1 == end)
            result += ", or '";
        else
            result += ", '";
        result += *it + "'";
    }
    return result;
}

bool checkChars(QString::const_iterator &it, const QList<QChar> &chars)
{
    if (*it == backSlash) {
        ++it;
        if (!chars.contains(*it))
            throw SnippetParseException{"expected " + join(chars) + "after escaping '\\'"};
        return false;
    }
    return chars.contains(*it);
}

void skipToEndOfTabstop(QString::const_iterator &it, const QString::const_iterator &end)
{
    while (it < end && checkChars(it, {closeBrace}))
        ++it;
}

int parseTabstopIndex(QString::const_iterator &it)
{
    int result = 0;
    while (it->isDigit()) {
        result = 10 * result + it->digitValue();
        ++it;
    }
    return result;
}

QString parseVariable(QString::const_iterator &it)
{
    // TODO: implement replacing variable with data
    QString result;
    const QString::const_iterator start = it;
    while (it->isLetter() || *it == underscore || (start != it && it->isDigit())) {
        result.append(*it);
        ++it;
    }
    return result;
}

ParsedSnippet::Part parseTabstop(QString::const_iterator &it, const QString::const_iterator &end)
{
    ParsedSnippet::Part result;
    if (*it != dollar)
        throw SnippetParseException{"Expected a '$' (tabstop)"};
    skipSpaces(++it);
    if (it->isDigit()) {
        result.variableIndex = parseTabstopIndex(it);
        if (result.variableIndex == 0)
            result.finalPart = true;
    } else if (*it == openBrace) {
        skipSpaces(++it);
        if (it->isDigit()) {
            result.variableIndex = parseTabstopIndex(it);
            if (result.variableIndex == 0)
                result.finalPart = true;
            skipSpaces(it);
            if (*it == colon) {
                ++it;
                while (it < end && !checkChars(it, {closeBrace})) {
                    result.text.append(*it);
                    ++it;
                }
            } else if (*it == pipe) {
                ++it;
                // TODO: Implement Choices for now take the first choice and use it as a placeholder
                for (; it < end && !checkChars(it, {comma, pipe, closeBrace}); ++it)
                    result.text.append(*it);
                skipToEndOfTabstop(it, end);
            }
        } else if (it->isLetter() || *it == underscore) {
            result.text = parseVariable(it);
            // TODO: implement variable transformation
            skipToEndOfTabstop(it, end);
        }
        if (*it != closeBrace)
            throw SnippetParseException{"Expected a closing curly brace"};
        ++it;
    } else if (it->isLetter() || *it == underscore) {
        result.text = parseVariable(it);
    } else {
        throw SnippetParseException{"Expected tabstop index, variable, or open curly brace"};
    }
    return result;
}

SnippetParseResult parseSnippet(const QString &snippet)
{
    ParsedSnippet result;
    ParsedSnippet::Part currentPart;

    std::optional<QString> error;
    auto it = snippet.begin();
    const auto end = snippet.end();

    while (it < end) {
        try {
            if (checkChars(it, {dollar})) {
                if (!currentPart.text.isEmpty()) {
                    if (currentPart.variableIndex != -1) {
                        throw SnippetParseException{
                            "Internal Error: expected variable index -1 in snippet part"};
                    }
                    result.parts.append(currentPart);
                    currentPart.text.clear();
                }
                const ParsedSnippet::Part &part = parseTabstop(it, end);
                while (result.variables.size() < part.variableIndex + 1)
                    result.variables.append(QList<int>());
                result.variables[part.variableIndex] << result.parts.size();
                result.parts.append(part);
            } else {
                currentPart.text.append(*it);
                ++it;
            }
        } catch (const SnippetParseException &e) {
            return SnippetParseError{e.message, snippet, int(it - snippet.begin())};
        }
    }

    if (!currentPart.text.isEmpty())
        result.parts.append(currentPart);

    return result;
}

} // namespace LanguageClient

#ifdef WITH_TESTS

const char NOMANGLER_ID[] = "TextEditor::NoMangler";

struct SnippetPart
{
    SnippetPart() = default;
    explicit SnippetPart(const QString &text,
                         int index = -1,
                         const Utils::Id &manglerId = NOMANGLER_ID,
                         const QList<SnippetPart> &nested = {})
        : text(text)
        , variableIndex(index)
        , manglerId(manglerId)
        , nested(nested)
    {}
    QString text;
    int variableIndex = -1; // if variable index is >= 0 the text is interpreted as a variable
    Utils::Id manglerId;
    QList<SnippetPart> nested;
};
Q_DECLARE_METATYPE(SnippetPart);

using Parts = QList<SnippetPart>;
void LanguageClient::LanguageClientPlugin::testSnippetParsing_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("success");
    QTest::addColumn<Parts>("parts");

    QTest::newRow("no input") << QString() << true << Parts();
    QTest::newRow("empty input") << QString("") << true << Parts();

    QTest::newRow("empty tabstop") << QString("$1") << true << Parts{SnippetPart("", 1)};
    QTest::newRow("empty tabstop with braces") << QString("${1}") << true << Parts{SnippetPart("", 1)};
    QTest::newRow("double tabstop")
        << QString("$1$1") << true << Parts{SnippetPart("", 1), SnippetPart("", 1)};
    QTest::newRow("different tabstop")
        << QString("$1$2") << true << Parts{SnippetPart("", 1), SnippetPart("", 2)};
    QTest::newRow("empty tabstop") << QString("$1") << true << Parts{SnippetPart("", 1)};
    QTest::newRow("double dollar") << QString("$$1") << false << Parts();
    QTest::newRow("escaped tabstop") << QString("\\$1") << true << Parts{SnippetPart("$1")};
    QTest::newRow("escaped double tabstop")
        << QString("\\$$1") << true << Parts{SnippetPart("$"), SnippetPart("", 1)};

    QTest::newRow("placeholder") << QString("${1:foo}") << true << Parts{SnippetPart("foo", 1)};
    QTest::newRow("placeholder with text")
        << QString("text${1:foo}text") << true
        << Parts{SnippetPart("text"), SnippetPart("foo", 1), SnippetPart("text")};
    QTest::newRow("2 placeholder") << QString("${1:foo}${2:bar}") << true
                                   << Parts{SnippetPart("foo", 1), SnippetPart("bar", 2)};
    QTest::newRow("2 placeholder same tabstop")
        << QString("${1:foo}${1:bar}") << true
        << Parts{SnippetPart("foo", 1), SnippetPart("bar", 1)};
}

void LanguageClient::LanguageClientPlugin::testSnippetParsing()
{
    QFETCH(QString, input);
    QFETCH(bool, success);
    QFETCH(Parts, parts);

    SnippetParseResult result = LanguageClient::parseSnippet(input);
    QCOMPARE(std::holds_alternative<ParsedSnippet>(result), success);
    if (!success)
        return;

    ParsedSnippet snippet = std::get<ParsedSnippet>(result);

    auto rangesCompare = [&](const ParsedSnippet::Part &actual, const SnippetPart &expected) {
        QCOMPARE(actual.text, expected.text);
        QCOMPARE(actual.variableIndex, expected.variableIndex);
        auto manglerId = actual.mangler ? actual.mangler->id() : NOMANGLER_ID;
        QCOMPARE(manglerId, expected.manglerId);
    };

    QCOMPARE(snippet.parts.count(), parts.count());

    for (int i = 0; i < parts.count(); ++i)
        rangesCompare(snippet.parts.at(i), parts.at(i));
}
#endif
