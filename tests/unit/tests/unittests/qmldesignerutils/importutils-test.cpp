// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <googletest.h>

#include <qmldesignerutils/importutils.h>

namespace {

using namespace Qt::StringLiterals;
using QmlDesigner::ImportUtils::find_import_location;

TEST(ImportUtils, begin_for_no_imports)
{
    auto content = QStringView{uR"("Item {})"};
    QStringView directory = u"foo";

    auto found = find_import_location(content, directory);

    ASSERT_THAT(found, content.begin());
}

TEST(ImportUtils, begin_for_imports_after_last_import)
{
    auto content = QStringView{u"import foo\n"
                               u"import bar\n"
                               u"\n"
                               u"Item {}\n"};
    QStringView directory = u"foo";

    auto found = find_import_location(content, directory);

    QStringView rest{*found, content.end()};
    ASSERT_THAT(rest, u"\nItem {}\n");
}

TEST(ImportUtils, skip_already_existing_directory_import_as_first_import)
{
    auto content = QStringView{u"import \"foo\"\n"
                               u"import bar\n"
                               u"\n"
                               u"Item {}\n"};
    QStringView directory = u"foo";

    auto found = find_import_location(content, directory);

    ASSERT_THAT(found, std::nullopt);
}

TEST(ImportUtils, skip_already_existing_directory_import_as_last_import)
{
    auto content = QStringView{u"import bar\n"
                               u"import \"foo\"\n"
                               u"\n"
                               u"Item {}\n"};
    QStringView directory = u"foo";

    auto found = find_import_location(content, directory);

    ASSERT_THAT(found, std::nullopt);
}

TEST(ImportUtils, dont_already_existing_directory_import_with_as)
{
    auto content = QStringView{u"import \"foo\" as foo\n"
                               u"import bar\n"
                               u"\n"
                               u"Item {}\n"};
    QStringView directory = u"foo";

    auto found = find_import_location(content, directory);

    QStringView rest{*found, content.end()};
    ASSERT_THAT(rest, u"\nItem {}\n");
}

TEST(ImportUtils, ignore_import_properties)
{
    auto content = QStringView{u"import foo\n"
                               u"import bar\n"
                               u"\n"
                               u"Item { import :}\n"};
    QStringView directory = u"foo";

    auto found = find_import_location(content, directory);

    QStringView rest{*found, content.end()};
    ASSERT_THAT(rest, u"\nItem { import :}\n");
}

TEST(ImportUtils, handle_nothing_after_import)
{
    auto content = QStringView{u"import foo\n"
                               u"import bar"};
    QStringView directory = u"foo";

    auto found = find_import_location(content, directory);

    ASSERT_THAT(found, content.end());
}

} // namespace
