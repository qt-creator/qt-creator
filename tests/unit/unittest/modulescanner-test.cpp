// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include <projectstorage/modulescanner.h>

#include <QDebug>

namespace {

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
    QmlDesigner::ModuleScanner scanner{[](QStringView moduleName) {
                                           return moduleName.endsWith(u"impl");
                                       },
                                       QmlDesigner::VersionScanning::No};
};

TEST_F(ModuleScanner, ReturnEmptyOptionalForWrongPath)
{
    scanner.scan(QStringList{""});

    ASSERT_THAT(scanner.modules(), IsEmpty());
}

TEST_F(ModuleScanner, GetQtQuick)
{
    scanner.scan(QStringList{QT6_INSTALL_PREFIX});

    ASSERT_THAT(scanner.modules(), Contains(UrlProperty("QtQuick")));
}

TEST_F(ModuleScanner, SkipEmptyModules)
{
    scanner.scan(QStringList{QT6_INSTALL_PREFIX});

    ASSERT_THAT(scanner.modules(), Not(Contains(UrlProperty(IsEmpty()))));
}

TEST_F(ModuleScanner, UseSkipFunction)
{
    scanner.scan(QStringList{QT6_INSTALL_PREFIX});

    ASSERT_THAT(scanner.modules(), Not(Contains(UrlProperty(EndsWith(QStringView{u"impl"})))));
}

TEST_F(ModuleScanner, Version)
{
    QmlDesigner::ModuleScanner scanner{[](QStringView moduleName) {
                                           return moduleName.endsWith(u"impl");
                                       },
                                       QmlDesigner::VersionScanning::Yes};

    scanner.scan(QStringList{TESTDATA_DIR "/modulescanner"});

    ASSERT_THAT(scanner.modules(), ElementsAre(AllOf(UrlProperty("Example"), VersionProperty("1.3"))));
}

TEST_F(ModuleScanner, NoVersion)
{
    QmlDesigner::ModuleScanner scanner{[](QStringView moduleName) {
                                           return moduleName.endsWith(u"impl");
                                       },
                                       QmlDesigner::VersionScanning::No};

    scanner.scan(QStringList{TESTDATA_DIR "/modulescanner"});

    ASSERT_THAT(scanner.modules(),
                ElementsAre(AllOf(UrlProperty("Example"), VersionProperty(QString{}))));
}

TEST_F(ModuleScanner, Duplicates)
{
    scanner.scan(QStringList{QT6_INSTALL_PREFIX});

    ASSERT_THAT(scanner.modules(), Not(HasDuplicates()));
}

TEST_F(ModuleScanner, DontAddModulesAgain)
{
    scanner.scan(QStringList{QT6_INSTALL_PREFIX});

    scanner.scan(QStringList{QT6_INSTALL_PREFIX});

    ASSERT_THAT(scanner.modules(), Not(HasDuplicates()));
}

} // namespace
