// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testresult.h"

namespace Autotest {
namespace Internal {

class BoostTestTreeItem;

class BoostTestResult : public TestResult
{
public:
    BoostTestResult(const QString &id, const QString &name, const Utils::FilePath &projectFile,
                    const QString &testCaseName = {}, const QString &testSuiteName = {});

    bool isDirectParentOf(const TestResult *other, bool *needsIntermediate) const override;
    const ITestTreeItem * findTestTreeItem() const override;
private:
    bool matches(const BoostTestTreeItem *item) const;

    Utils::FilePath m_projectFile;
    QString m_testCaseName;
    QString m_testSuiteName;
};

} // namespace Internal
} // namespace Autotest
