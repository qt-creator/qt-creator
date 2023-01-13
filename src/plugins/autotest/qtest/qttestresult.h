// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../testresult.h"
#include "qttestconstants.h"

namespace Autotest {

class TestTreeItem;

namespace Internal {

class QtTestResult : public TestResult
{
public:
    QtTestResult(const QString &id, const QString &name, const Utils::FilePath &projectFile,
                 TestType type, const QString &functionName = {}, const QString &dataTag = {});

    bool isDirectParentOf(const TestResult *other, bool *needsIntermediate) const override;
    bool isIntermediateFor(const TestResult *other) const override;
    TestResult *createIntermediateResultFor(const TestResult *other) override;
    const ITestTreeItem *findTestTreeItem() const override;
private:
    bool isTestCase() const     { return m_function.isEmpty()  && m_dataTag.isEmpty(); }
    bool isTestFunction() const { return !m_function.isEmpty() && m_dataTag.isEmpty(); }
    bool isDataTag() const      { return !m_function.isEmpty() && !m_dataTag.isEmpty(); }

    bool matches(const TestTreeItem *item) const;
    bool matchesTestCase(const TestTreeItem *item) const;
    bool matchesTestFunction(const TestTreeItem *item) const;

    Utils::FilePath m_projectFile;
    TestType m_type;
    QString m_function;
    QString m_dataTag;
};

} // namespace Internal
} // namespace Autotest
