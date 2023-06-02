// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestparser.h"
#include "boosttesttreeitem.h"

namespace Autotest {
namespace Internal {

class BoostTestParseResult : public TestParseResult
{
public:
    explicit BoostTestParseResult(ITestFramework *framework) : TestParseResult(framework) {}
    TestTreeItem *createTestTreeItem() const override;
    // TODO special attributes/states (labeled, timeout,...?)
    BoostTestTreeItem::TestStates state = BoostTestTreeItem::Enabled;
};

class BoostTestParser : public CppParser
{
public:
    explicit BoostTestParser(ITestFramework *framework) : CppParser(framework) {}
    bool processDocument(QPromise<TestParseResultPtr> &promise,
                         const Utils::FilePath &fileName) override;
};

} // namespace Internal
} // namespace Autotest
