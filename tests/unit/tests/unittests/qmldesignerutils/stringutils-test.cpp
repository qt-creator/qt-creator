// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <googletest.h>
#include <qmldesignerutils/stringutils.h>

namespace {

using namespace Qt::StringLiterals;

using QmlDesigner::StringUtils::find_comment_end;
using QmlDesigner::StringUtils::split_last;

TEST(StringUtils_split_last, leaf_is_empty_for_empty_input)
{
    QString text;

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(leaf, IsEmpty());
}

TEST(StringUtils_split_last, leaf_is_empty_with_ending_dot)
{
    QString text;

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(leaf, IsEmpty());
}

TEST(StringUtils_split_last, steam_is_empty_for_last_beginning_dot)
{
    QString text = ".bar";

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(steam, IsEmpty());
}

TEST(StringUtils_split_last, steam_is_empty)
{
    QString text = ".";

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(steam, IsEmpty());
}

TEST(StringUtils_split_last, leaf)
{
    QString text = "foo.foo.bar";

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(leaf, u"bar");
}

TEST(StringUtils_split_last, leaf_for_not_dot)
{
    QString text = "bar";

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(leaf, u"bar");
}

TEST(StringUtils_split_last, steam)
{
    QString text = "foo.foo.bar";

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(steam, u"foo.foo");
}

TEST(StringUtils_split_last, no_steam_for_not_dot)
{
    QString text = "bar";

    auto [steam, leaf] = split_last(text, u'.');

    ASSERT_THAT(steam, IsEmpty());
}

using ConvertFunction = QString (*)(QStringView);

struct EscapeParameters
{
    EscapeParameters(QString input, QString output, std::string name, ConvertFunction convert)
        : input{std::move(input)}
        , output{std::move(output)}
        , name{std::move(name)}
        , convert{convert}
    {}

    QString input;
    QString output;
    std::string name;
    ConvertFunction convert;
};

class escaping : public testing::TestWithParam<EscapeParameters>
{
public:
    escaping()
        : inputTerm{GetParam().input}
        , outputTerm{GetParam().output}
        , name{GetParam().name}
        , convert{GetParam().convert}
    {}

public:
    const QString &inputTerm;
    const QString &outputTerm;
    const std::string &name;
    ConvertFunction convert;
};

auto excape_printer = [](const testing::TestParamInfo<EscapeParameters> &info) {
    return info.param.name;
};

INSTANTIATE_TEST_SUITE_P(
    StringUtils,
    escaping,
    testing::Values(
        EscapeParameters(u"\\"_s, u"\\\\"_s, "escape_backslash", QmlDesigner::StringUtils::escape),
        EscapeParameters(u"\""_s, u"\\\""_s, "escape_quote", QmlDesigner::StringUtils::escape),
        EscapeParameters(u"\t"_s, u"\\t"_s, "escape_tab", QmlDesigner::StringUtils::escape),
        EscapeParameters(u"\n"_s, u"\\n"_s, "escape_new_line", QmlDesigner::StringUtils::escape),
        EscapeParameters(u"\r"_s, u"\\r"_s, "escape_carriage_return", QmlDesigner::StringUtils::escape),
        EscapeParameters(u"\\"_s, u"\\"_s, "deescape_backslash", QmlDesigner::StringUtils::deescape),
        EscapeParameters(u"\\\\"_s, u"\\"_s, "deescape_double_backslash", QmlDesigner::StringUtils::deescape),
        EscapeParameters(u"\\\""_s, u"\""_s, "deescape_quote", QmlDesigner::StringUtils::deescape),
        EscapeParameters(u"\\t"_s, u"\t"_s, "deescape_tab", QmlDesigner::StringUtils::deescape),
        EscapeParameters(u"\\n"_s, u"\n"_s, "deescape_new_line", QmlDesigner::StringUtils::deescape),
        EscapeParameters(
            u"\\r"_s, u"\r"_s, "deescape_carriage_return", QmlDesigner::StringUtils::deescape)),
    excape_printer);

TEST_P(escaping, begin)
{
    QString input = inputTerm + "foo";

    auto converted = convert(input);

    ASSERT_THAT(converted, outputTerm + "foo");
}

TEST_P(escaping, empty)
{
    QString input;

    auto converted = convert(input);

    ASSERT_THAT(converted, IsEmpty());
}

TEST_P(escaping, only)
{
    QString input = inputTerm;

    auto converted = convert(input);

    ASSERT_THAT(converted, outputTerm);
}

TEST_P(escaping, nothing)
{
    QString input = "foobar";

    auto converted = convert(input);

    ASSERT_THAT(converted, "foobar");
}

TEST_P(escaping, end)
{
    QString input = "foo" + inputTerm;

    auto converted = convert(input);

    ASSERT_THAT(converted, "foo" + outputTerm);
}

TEST_P(escaping, middle)
{
    QString input = "foo" + inputTerm + "bar";

    auto converted = convert(input);

    ASSERT_THAT(converted, "foo" + outputTerm + "bar");
}

TEST_P(escaping, multiple)
{
    QString input = "foo" + inputTerm + "bar" + inputTerm + "foo";

    auto converted = convert(input);

    ASSERT_THAT(converted, "foo" + outputTerm + "bar" + outputTerm + "foo");
}

TEST_P(escaping, multiple_in_row)
{
    if (name == "deescape_backslash") // would be double backslash
        return;
    QString input = "foo" + inputTerm + inputTerm + "foo";

    auto converted = convert(input);

    ASSERT_THAT(converted, "foo" + outputTerm + outputTerm + "foo");
}

TEST_P(escaping, skip_unicode)
{
    QString input = "\\u" + inputTerm + "foo";
    input.resize(6);

    auto converted = convert(input);

    ASSERT_THAT(converted, input);
}

TEST(StringUtils, find_comment_end_empty)
{
    QStringView input{};

    auto found = find_comment_end(input);

    ASSERT_THAT(found, input.end());
}

TEST(StringUtils, find_comment_end_line_comment)
{
    QStringView input{u"//foo\n//bar\nbar"};

    auto found = find_comment_end(input);

    QStringView text{found, input.end()};
    ASSERT_THAT(text, u"bar");
}

TEST(StringUtils, find_comment_end_block_comment)
{
    QStringView input{u"/*foo\n//bar*/foo\nbar"};

    auto found = find_comment_end(input);

    QStringView text{found, input.end()};
    ASSERT_THAT(text, u"bar");
}

TEST(StringUtils, find_comment_end_block_and_line_comment)
{
    QStringView input{u"//foo\n/*bar*/foo\nbar"};

    auto found = find_comment_end(input);

    QStringView text{found, input.end()};
    ASSERT_THAT(text, u"bar");
}

TEST(StringUtils, find_comment_end_line_comment_has_no_newline)
{
    QStringView input{u"//foobar"};

    auto found = find_comment_end(input);

    QStringView text{found, input.end()};
    ASSERT_THAT(text, input.end());
}

TEST(StringUtils, find_comment_end_block_comment_has_no_newline)
{
    QStringView input{u"//foo\n/*bar*/foo"};

    auto found = find_comment_end(input);

    QStringView text{found, input.end()};
    ASSERT_THAT(text, input.end());
}

} // namespace
