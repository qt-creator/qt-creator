// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestparser.h"

#include "qttest_utils.h"
#include "qttesttreeitem.h"

#include <optional>

namespace Autotest::Internal {

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

class QtTestParseContext : public CppParseContext
{
public:
    QHash<Utils::FilePath, TestCases> cachedTestCases;
    QMultiHash<Utils::FilePath, Utils::FilePath> alternativeFiles;
    QSet<Utils::FilePath> prefilteredFiles;
};

class QtTestParser : public CppParser
{
public:
    explicit QtTestParser(ITestFramework *framework) : CppParser(framework) {}

    DocumentProcessor init(const QSet<Utils::FilePath> &filesToParse, bool fullParse) override;

private:
    bool processDocument(QPromise<TestParseResultPtr> &promise,
                         const QtTestParseContext &context,
                         const Utils::FilePath &fileName);
    TestCases testCases(const QtTestParseContext &context, const Utils::FilePath &fileName) const;
    QHash<QString, QtTestCodeLocationList> checkForDataTags(const QtTestParseContext &context,
                                                            const Utils::FilePath &fileName) const;
    struct TestCaseData {
        Utils::FilePath fileName;
        int line = 0;
        int column = 0;
        QMap<QString, QtTestCodeLocationAndType> testFunctions;
        QHash<QString, QtTestCodeLocationList> dataTags;
        bool multipleTestCases = false;
        bool valid = false;
    };

    std::optional<bool> fillTestCaseData(const QtTestParseContext &context,
                                         const QString &testCaseName,
                                         const CPlusPlus::Document::Ptr &doc,
                                         TestCaseData &data) const;
    QtTestParseResult *createParseResult(
        const QString &testCaseName,
        const TestCaseData &data,
        const Utils::FilePath &projectFile) const;
};

} // namespace Autotest::Internal
