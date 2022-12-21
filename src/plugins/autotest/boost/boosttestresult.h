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
    BoostTestResult(const QString &id, const Utils::FilePath &projectFile, const QString &name);
    const QString outputString(bool selected) const override;

    bool isDirectParentOf(const TestResult *other, bool *needsIntermediate) const override;
    const ITestTreeItem * findTestTreeItem() const override;
    void setTestSuite(const QString &testSuite) { m_testSuite = testSuite; }
    void setTestCase(const QString &testCase) { m_testCase = testCase; }
private:
    bool matches(const BoostTestTreeItem *item) const;

    Utils::FilePath m_projectFile;
    QString m_testSuite;
    QString m_testCase;
};

} // namespace Internal
} // namespace Autotest
