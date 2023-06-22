// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../mocks/externaldependenciesmock.h"
#include "../utils/googletest.h"

#include <projectstorage/modulescanner.h>

#include <QDebug>

namespace {

QLatin1String qmlModulesPath(UNITTEST_DIR "/projectstorage/data/qml");

template<typename Matcher>
auto UrlProperty(const Matcher &matcher)
{
    return Property(&QmlDesigner::Import::url, matcher);
}

template<typename Matcher>
auto VersionProperty(const Matcher &matcher)
{
    return Property(&QmlDesigner::Import::version, matcher);
}

template<typename Matcher>
auto CorePropertiesHave(const Matcher &matcher)
{
    return AllOf(Contains(AllOf(UrlProperty("QtQuick"), matcher)),
                 Contains(AllOf(UrlProperty("QtQuick.Controls"), matcher)),
                 Contains(AllOf(UrlProperty("QtQuick3D"), matcher)),
                 Contains(AllOf(UrlProperty("QtQuick3D.Helpers"), matcher)),
                 Contains(AllOf(UrlProperty("QtQuick3D.Particles3D"), matcher)));
}

template<typename Matcher>
auto NonCorePropertiesHave(const Matcher &matcher)
{
    return Not(Contains(AllOf(UrlProperty(AnyOf(Eq(u"QtQuick"),
                                                Eq(u"QtQuick.Controls"),
                                                Eq(u"QtQuick3D"),
                                                Eq(u"QtQuick3D.Helpers"),
                                                Eq(u"QtQuick3D.Particles3D"))),
                              matcher)));
}

MATCHER(HasDuplicates, std::string(negation ? "hasn't duplicates" : "has dublicates"))
{
    auto values = arg;
    std::sort(values.begin(), values.begin());
    auto found = std::adjacent_find(values.begin(), values.end());

    return found != values.end();
}

class ModuleScanner : public testing::Test
{
protected:
    NiceMock<ExternalDependenciesMock> externalDependenciesMock;
    QmlDesigner::ModuleScanner scanner{[](QStringView moduleName) {
                                           return moduleName.endsWith(u"impl");
                                       },
                                       QmlDesigner::VersionScanning::No,
                                       externalDependenciesMock};
};

TEST_F(ModuleScanner, return_empty_optional_for_empty_path)
{
    scanner.scan(QStringList{""});

    ASSERT_THAT(scanner.modules(), IsEmpty());
}

TEST_F(ModuleScanner, return_empty_optional_for_null_string_path)
{
    scanner.scan(QStringList{QString{}});

    ASSERT_THAT(scanner.modules(), IsEmpty());
}

TEST_F(ModuleScanner, return_empty_optional_for_empty_paths)
{
    scanner.scan(QStringList{});

    ASSERT_THAT(scanner.modules(), IsEmpty());
}

TEST_F(ModuleScanner, get_qt_quick)
{
    scanner.scan(QStringList{qmlModulesPath});

    ASSERT_THAT(scanner.modules(), Contains(UrlProperty("QtQuick")));
}

TEST_F(ModuleScanner, skip_empty_modules)
{
    scanner.scan(QStringList{qmlModulesPath});

    ASSERT_THAT(scanner.modules(), Not(Contains(UrlProperty(IsEmpty()))));
}

TEST_F(ModuleScanner, use_skip_function)
{
    scanner.scan(QStringList{qmlModulesPath});

    ASSERT_THAT(scanner.modules(), Not(Contains(UrlProperty(EndsWith(QStringView{u"impl"})))));
}

TEST_F(ModuleScanner, version)
{
    QmlDesigner::ModuleScanner scanner{[](QStringView moduleName) {
                                           return moduleName.endsWith(u"impl");
                                       },
                                       QmlDesigner::VersionScanning::Yes,
                                       externalDependenciesMock};

    scanner.scan(QStringList{UNITTEST_DIR "/projectstorage/data/modulescanner"});

    ASSERT_THAT(scanner.modules(), ElementsAre(AllOf(UrlProperty("Example"), VersionProperty("1.3"))));
}

TEST_F(ModuleScanner, no_version)
{
    QmlDesigner::ModuleScanner scanner{[](QStringView moduleName) {
                                           return moduleName.endsWith(u"impl");
                                       },
                                       QmlDesigner::VersionScanning::No,
                                       externalDependenciesMock};

    scanner.scan(QStringList{UNITTEST_DIR "/projectstorage/data/modulescanner"});

    ASSERT_THAT(scanner.modules(),
                ElementsAre(AllOf(UrlProperty("Example"), VersionProperty(QString{}))));
}

TEST_F(ModuleScanner, duplicates)
{
    scanner.scan(QStringList{qmlModulesPath});

    ASSERT_THAT(scanner.modules(), Not(HasDuplicates()));
}

TEST_F(ModuleScanner, dont_add_modules_again)
{
    scanner.scan(QStringList{qmlModulesPath});

    scanner.scan(QStringList{qmlModulesPath});

    ASSERT_THAT(scanner.modules(), Not(HasDuplicates()));
}

TEST_F(ModuleScanner, set_no_version_for_qt_quick_version)
{
    scanner.scan(QStringList{qmlModulesPath});

    ASSERT_THAT(scanner.modules(), CorePropertiesHave(VersionProperty(QString{})));
}

TEST_F(ModuleScanner, set_version_for_qt_quick_version)
{
    ON_CALL(externalDependenciesMock, qtQuickVersion()).WillByDefault(Return(QString{"6.4"}));

    scanner.scan(QStringList{qmlModulesPath});

    ASSERT_THAT(scanner.modules(), CorePropertiesHave(VersionProperty(u"6.4")));
}

TEST_F(ModuleScanner, dont_set_version_for_non_qt_quick_version)
{
    ON_CALL(externalDependenciesMock, qtQuickVersion()).WillByDefault(Return(QString{"6.4"}));

    scanner.scan(QStringList{qmlModulesPath});

    ASSERT_THAT(scanner.modules(), NonCorePropertiesHave(VersionProperty(QString{})));
}

} // namespace
