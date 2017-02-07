/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "googletest.h"

#define private public
#include <projectexplorer/projectmacro.h>
#undef private

namespace {

using ProjectExplorer::MacroType;
using ProjectExplorer::Macro;
using ProjectExplorer::Macros;

MATCHER_P3(IsMacro, key, value, type,
           std::string(negation ? "isn't" : "is")
           + " key "+ PrintToString(key)
           + ", value " + PrintToString(value)
           + " and type " + PrintToString(type))
{
    return arg.key == key && arg.value == value && arg.type == type;
}

TEST(Macro, SplitLines)
{
    QByteArray text = "#define Foo 42\n\n#define Bar\n#define HoHoHo Bar\n// foo";

    auto textLines = Macro::splitLines(text);

    ASSERT_THAT(textLines, ElementsAre("#define Foo 42", "#define Bar", "#define HoHoHo Bar", "// foo"));
}

TEST(Macro, RemoveCarriageReturn)
{
    QByteArray text = "#define Foo 42\r\n#define Bar\n";

    auto textLines = Macro::splitLines(text);

    ASSERT_THAT(textLines, ElementsAre("#define Foo 42", "#define Bar"));
}

TEST(Macro, TokenizeNullLine)
{
    QByteArray line;

    auto tokens = Macro::tokenizeLine(line);

    ASSERT_THAT(tokens, IsEmpty());
}

TEST(Macro, TokenizeEmptyLine)
{
    QByteArray line = "";

    auto tokens = Macro::tokenizeLine(line);

    ASSERT_THAT(tokens, IsEmpty());
}

TEST(Macro, TokenizeSpaces)
{
    QByteArray line = "   ";

    auto tokens = Macro::tokenizeLine(line);

    ASSERT_THAT(tokens, IsEmpty());
}

TEST(Macro, TokenizeOneEntry)
{
    QByteArray line = "//blah";

    auto tokens = Macro::tokenizeLine(line);

    ASSERT_THAT(tokens, IsEmpty());
}

TEST(Macro, TokenizeThreeEntries)
{
    QByteArray line = "#define Foo 42";

    auto tokens = Macro::tokenizeLine(line);

    ASSERT_THAT(tokens, ElementsAre("#define", "Foo", "42"));
}

TEST(Macro, TokenizeManyEntries)
{
    QByteArray line = "#define Foo unsigned long long int";

    auto tokens = Macro::tokenizeLine(line);

    ASSERT_THAT(tokens, ElementsAre("#define", "Foo", "unsigned long long int"));
}

TEST(Macro, TokenizeWithMutipleSpaces)
{
    QByteArray line = "#define  Foo 42";

    auto tokens = Macro::tokenizeLine(line);

    ASSERT_THAT(tokens, ElementsAre("#define", "Foo", "42"));
}

TEST(Macro, TokenizeLines)
{
    QList<QByteArray> lines = {"#define Foo 42",
                              "#define Bar Ho",
                              "// this is a comment",
                              " "};

    auto tokensLines = Macro::tokenizeLines(lines);

    ASSERT_THAT(tokensLines,
                ElementsAre(ElementsAre("#define", "Foo", "42"),
                            ElementsAre("#define", "Bar", "Ho"),
                            ElementsAre("//", "this", "is a comment"),
                            IsEmpty()));
}

TEST(Macro, SpacesBeforEntries)
{
    QByteArray line = "    #define Foo";

    auto strippedLine = Macro::removeNonsemanticSpaces(line);

    ASSERT_THAT(strippedLine, "#define Foo");
}

TEST(Macro, SpacesAfterEntries)
{
    QByteArray line = "#define Foo    ";

    auto strippedLine = Macro::removeNonsemanticSpaces(line);

    ASSERT_THAT(strippedLine, "#define Foo");
}

TEST(Macro, ManySpacesInbetweenEntries)
{
    QByteArray line = "#define \t  Foo  42";

    auto strippedLine = Macro::removeNonsemanticSpaces(line);

    ASSERT_THAT(strippedLine, "#define Foo 42");
}

TEST(Macro, EmptyString)
{
    QByteArray line = "#define \t  Foo \"\"";

    auto strippedLine = Macro::removeNonsemanticSpaces(line);

    ASSERT_THAT(strippedLine, "#define Foo \"\"");
}

TEST(Macro, StringWithQuotes)
{
    QByteArray line = "#define \t  Foo \"\\\"string\\\"\"";

    auto strippedLine = Macro::removeNonsemanticSpaces(line);

    ASSERT_THAT(strippedLine, "#define Foo \"\\\"string\\\"\"");
}

TEST(Macro, DISABLED_StringConcatenation)
{
    QByteArray line = "#define \t  Foo \"a\"     \"b\"";

    auto strippedLine = Macro::removeNonsemanticSpaces(line);

    ASSERT_THAT(strippedLine, "#define Foo \"a\" \"b\"");
}

TEST(Macro, DISABLED_TokenConcatenation)
{
    QByteArray line = "#define \t  Foo \"a\"  ##   c";

    auto strippedLine = Macro::removeNonsemanticSpaces(line);

    ASSERT_THAT(strippedLine, "#define Foo \"a\" ## c");
}

TEST(Macro, StringWithQuotesAndSpaces)
{
    QByteArray line = "#define \t  Foo \"\\\"string   \\\"\"";

    auto strippedLine = Macro::removeNonsemanticSpaces(line);

    ASSERT_THAT(strippedLine.toStdString(), "#define Foo \"\\\"string   \\\"\"");
}

TEST(Macro, SpacesAferHashEntries)
{
    QByteArray line = "#    define Foo 42";

    auto strippedLine = Macro::removeNonsemanticSpaces(line);

    ASSERT_THAT(strippedLine, "#define Foo 42");
}

TEST(Macro, SpacesInStringEntries)
{
    QByteArray line = "#define   Foo  \"some  text   with spaces\"";

    auto strippedLine = Macro::removeNonsemanticSpaces(line);

    ASSERT_THAT(strippedLine, "#define Foo \"some  text   with spaces\"");
}

TEST(Macro, EmptyTokensToMacro)
{
    QList<QByteArray> tokens;

    auto macro = Macro::tokensToMacro(tokens);

    ASSERT_THAT(macro, IsMacro("", "", MacroType::Invalid));
}

TEST(Macro, DefineTwoTokensToMacro)
{
    QList<QByteArray> tokens = {"#define", "Foo"};

    auto macro = Macro::tokensToMacro(tokens);

    ASSERT_THAT(macro, IsMacro("Foo", "", MacroType::Define));
}

TEST(Macro, DefineThreeTokensToMacro)
{
    QList<QByteArray> tokens = {"#define", "Foo", "42"};

    auto macro = Macro::tokensToMacro(tokens);

    ASSERT_THAT(macro, IsMacro("Foo", "42", MacroType::Define));
}

TEST(Macro, DoNotParseNonDefines)
{
    QList<QByteArray> tokens = {"//", "this", "is a comment"};

    auto macro = Macro::tokensToMacro(tokens);

    ASSERT_THAT(macro, IsMacro("", "", MacroType::Invalid));
}

TEST(Macro, TokensLinesToMacros)
{
    QList<QList<QByteArray>> tokensLines = {{"#define", "Foo", "42"},
                                            {"#define", "Bar", "Ho"},
                                            {"//", "this", "is", "a", "comment"},
                                            {}};

    auto macros = Macro::tokensLinesToMacros(tokensLines);

    ASSERT_THAT(macros, ElementsAre(IsMacro("Foo", "42", MacroType::Define),
                                    IsMacro("Bar", "Ho", MacroType::Define)));
}


TEST(Macro, TextToMacros)
{
    QByteArray text = {"#define Foo 42\n"
                       "#define Bar Ho\n"
                       "// this is a comment\n"
                       " "};

    auto macros = Macro::toMacros(text);

    ASSERT_THAT(macros, ElementsAre(IsMacro("Foo", "42", MacroType::Define),
                                    IsMacro("Bar", "Ho", MacroType::Define)));
}

TEST(Macro, InvalidToText)
{
    Macro macro;

    auto text = macro.toByteArray();

    ASSERT_THAT(text, QByteArray());
}

TEST(Macro, DefineToText)
{
    Macro macro{"Foo", "Bar", MacroType::Define};

    auto text = macro.toByteArray();

    ASSERT_THAT(text, "#define Foo Bar");
}

TEST(Macro, DefineWithSpacesToText)
{
    Macro macro{"LongInt", "long long int", MacroType::Define};

    auto text = macro.toByteArray();

    ASSERT_THAT(text, "#define LongInt long long int");
}

TEST(Macro, UndefineToText)
{
    Macro macro{"Foo", "Bar", MacroType::Undefine};

    auto text = macro.toByteArray();

    ASSERT_THAT(text, "#undef Foo");
}

TEST(Macro, MacrosToText)
{
    Macros macros{{"LongInt", "long long int"}, {"Foo", "Bar"}, {}};

    auto text = Macro::toByteArray(macros);

    ASSERT_THAT(text, "#define LongInt long long int\n#define Foo Bar\n");
}

TEST(Macro, MacrosVectorToText)
{
    Macros macros{{"LongInt", "long long int"}, {"Foo", "Bar"}, {}, {"Foo", MacroType::Undefine}};

    auto text = Macro::toByteArray(macros);

    ASSERT_THAT(text, "#define LongInt long long int\n#define Foo Bar\n#undef Foo\n");
}

TEST(Macro, EmptyKeyValueToMacro)
{
    QString text;

    auto macro = Macro::fromKeyValue(text);

    ASSERT_THAT(macro, IsMacro("", "", MacroType::Invalid));
}

TEST(Macro, KeyToMacro)
{
    QString text = "Foo";

    auto macro = Macro::fromKeyValue(text);

    ASSERT_THAT(macro, IsMacro("Foo", "1", MacroType::Define));
}

TEST(Macro, KeyValueToMacro)
{
    QString text = "Foo=\"Some=Text\"";

    auto macro = Macro::fromKeyValue(text);

    ASSERT_THAT(macro, IsMacro("Foo", "\"Some=Text\"", MacroType::Define));
}

TEST(Macro, InvalidMacroToKeyValue)
{
    Macro macro;
    QByteArray prefix = "-D";

    auto keyValue = macro.toKeyValue(prefix);

    ASSERT_THAT(keyValue, "");
}

TEST(Macro, DefineKeyOnlyMacroToKeyValue)
{
    Macro macro{"Foo"};
    QByteArray prefix = "-D";

    auto keyValue = macro.toKeyValue(prefix);

    ASSERT_THAT(keyValue, "-DFoo");
}

TEST(Macro, DefineMacroToKeyValue)
{
    Macro macro{"Foo", "Bar"};
    QByteArray prefix = "-D";

    auto keyValue = macro.toKeyValue(prefix);

    ASSERT_THAT(keyValue, "-DFoo=Bar");
}
}

