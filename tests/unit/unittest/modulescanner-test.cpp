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

class ModuleScanner : public testing::Test
{
protected:
    QmlDesigner::ModuleScanner scanner{
        [](QStringView moduleName) { return moduleName.endsWith(u"impl"); }};
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

} // namespace
