// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestparser.h"

namespace Autotest {
namespace Internal {

class GTestParseResult : public TestParseResult
{
public:
    explicit GTestParseResult(ITestFramework *framework) : TestParseResult(framework) {}
    TestTreeItem *createTestTreeItem() const override;
    bool parameterized = false;
    bool typed = false;
    bool disabled = false;
};

class GTestParser : public CppParser
{
public:
    explicit GTestParser(ITestFramework *framework) : CppParser(framework) {}
    bool processDocument(QPromise<TestParseResultPtr> &futureInterface,
                         const Utils::FilePath &fileName) override;
};

} // namespace Internal
} // namespace Autotest
