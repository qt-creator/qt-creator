// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "catchtreeitem.h"
#include "../itestparser.h"

namespace Autotest {
namespace Internal {

class CatchParseResult : public TestParseResult
{
public:
    explicit CatchParseResult(ITestFramework *framework)
        : TestParseResult(framework) {}
    TestTreeItem *createTestTreeItem() const override;
    CatchTreeItem::TestStates states;
};

class CatchTestParser : public CppParser
{
public:
    CatchTestParser(ITestFramework *framework)
        : CppParser(framework) {}
    bool processDocument(QPromise<TestParseResultPtr> &promise,
                         const Utils::FilePath &fileName) override;
};

} // namespace Internal
} // namespace Autotest
