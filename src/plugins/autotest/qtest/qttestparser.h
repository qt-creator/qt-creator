// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestparser.h"

#include "qttest_utils.h"
#include "qttesttreeitem.h"

#include <optional>

namespace Autotest {
namespace Internal {

class QtTestParseResult : public TestParseResult
{
public:
    explicit QtTestParseResult(ITestFramework *framework) : TestParseResult(framework) {}
    void setInherited(bool inherited) { m_inherited = inherited; }
    bool inherited() const { return m_inherited; }
    void setRunsMultipleTestcases(bool multi) { m_multiTest = multi; }
    bool runsMultipleTestcases() const { return m_multiTest; }
    TestTreeItem *createTestTreeItem() const override;
private:
    bool m_inherited = false;
    bool m_multiTest = false;
};

class QtTestParser : public CppParser
{
public:
    explicit QtTestParser(ITestFramework *framework) : CppParser(framework) {}

    void init(const QSet<Utils::FilePath> &filesToParse, bool fullParse) override;
    void release() override;
    bool processDocument(QPromise<TestParseResultPtr> &promise,
                         const Utils::FilePath &fileName) override;

private:
    TestCases testCases(const Utils::FilePath &fileName) const;
    QHash<QString, QtTestCodeLocationList> checkForDataTags(const Utils::FilePath &fileName) const;
    struct TestCaseData {
        Utils::FilePath fileName;
        int line = 0;
        int column = 0;
        QMap<QString, QtTestCodeLocationAndType> testFunctions;
        QHash<QString, QtTestCodeLocationList> dataTags;
        bool multipleTestCases = false;
        bool valid = false;
    };

    std::optional<bool> fillTestCaseData(const QString &testCaseName,
                                           const CPlusPlus::Document::Ptr &doc,
                                           TestCaseData &data) const;
    QtTestParseResult *createParseResult(const QString &testCaseName, const TestCaseData &data,
                                         const QString &projectFile) const;
    QHash<Utils::FilePath, TestCases> m_testCases;
    QMultiHash<Utils::FilePath, Utils::FilePath> m_alternativeFiles;
    QSet<Utils::FilePath> m_prefilteredFiles;
};

} // namespace Internal
} // namespace Autotest
