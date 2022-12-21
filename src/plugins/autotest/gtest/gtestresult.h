// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testresult.h"

namespace Autotest {

class TestTreeItem;

namespace Internal {

class GTestResult : public TestResult
{
public:
    GTestResult(const QString &id, const Utils::FilePath &projectFile, const QString &name);
    const QString outputString(bool selected) const override;

    void setTestCaseName(const QString &testSetName) { m_testCaseName = testSetName; }
    void setIteration(int iteration) { m_iteration = iteration; }
    bool isDirectParentOf(const TestResult *other, bool *needsIntermediate) const override;
    virtual const ITestTreeItem *findTestTreeItem() const override;

private:
    bool isTestSuite() const { return m_testCaseName.isEmpty(); }
    bool isTestCase() const { return !m_testCaseName.isEmpty(); }

    bool matches(const TestTreeItem *item) const;
    bool matchesTestCase(const TestTreeItem *treeItem) const;
    bool matchesTestSuite(const TestTreeItem *treeItem) const;

    QString m_testCaseName;
    Utils::FilePath m_projectFile;
    int m_iteration = 1;
};

} // namespace Internal
} // namespace Autotest
