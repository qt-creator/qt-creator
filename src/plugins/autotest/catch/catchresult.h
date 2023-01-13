// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testresult.h"

namespace Autotest {
namespace Internal {

class CatchResult : public TestResult
{
public:
    CatchResult(const QString &id, const QString &name, int depth);

    bool isDirectParentOf(const TestResult *other, bool *needsIntermediate) const override;
    const ITestTreeItem *findTestTreeItem() const override;

private:
    int sectionDepth() const { return m_sectionDepth; }
    int m_sectionDepth = 0;
};

} // namespace Internal
} // namespace Autotest
