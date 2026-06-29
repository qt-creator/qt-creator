// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestframework.h"

#include "../qtest/qttestframework.h"

namespace Autotest {
namespace QuickTest::Constants {

inline constexpr char FRAMEWORK_ID[]                = "AutoTest.Framework.QtQuickTest";

} // namespace QuickTest::Constants

namespace Internal {

class QuickTestFramework : public QtTestFramework
{
public:
    QuickTestFramework();

protected:
    ITestParser *createTestParser() override;
    ITestTreeItem *createRootNode() override;
};

QuickTestFramework &theQuickTestFramework();

} // namespace Internal
} // namespace Autotest
