// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gtestresult.h"
#include "gtestconstants.h"
#include "../testframeworkmanager.h"
#include "../testtreeitem.h"

#include <utils/id.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

using namespace Utils;

namespace Autotest {
namespace Internal {

static ResultHooks::OutputStringHook outputStringHook(const QString &testCaseName)
{
    return [testCaseName](const TestResult &result, bool selected) {
        const QString &desc = result.description();
        QString output;
        switch (result.result()) {
        case ResultType::Pass:
        case ResultType::Fail:
            output = testCaseName;
            if (selected && !desc.isEmpty())
                output.append('\n').append(desc);
            break;
        default:
            output = desc;
            if (!selected)
                output = output.split('\n').first();
        }
        return output;
    };
}

static QString normalizeName(const QString &name)
{
    static const QRegularExpression parameterIndex("/\\d+");

    QString nameWithoutParameterIndices = name;
    nameWithoutParameterIndices.remove(parameterIndex);

    return nameWithoutParameterIndices.split('/').last();
}

static QString normalizeTestName(const QString &testname)
{
    return normalizeName(testname.split(',').first());
}

static bool matchesTestCase(const QString &testCaseName, const TestTreeItem *treeItem)
{
    if (treeItem->type() != TestTreeItem::TestCase)
        return false;

    const QString testItemTestCase = treeItem->parentItem()->name() + '.' + treeItem->name();
    return testItemTestCase == normalizeName(testCaseName);
}

static bool matchesTestSuite(const QString &name, const TestTreeItem *treeItem)
{
    if (treeItem->type() != TestTreeItem::TestSuite)
        return false;

    return treeItem->name() == normalizeTestName(name);
}

static bool matches(const QString &name, const FilePath &projectFile,
                    const QString &testCaseName, const TestTreeItem *treeItem)
{
    if (treeItem->proFile() != projectFile)
        return false;

    if (!testCaseName.isEmpty())
        return matchesTestCase(testCaseName, treeItem);
    return matchesTestSuite(name, treeItem);
}

static ResultHooks::FindTestItemHook findTestItemHook(const FilePath &projectFile,
                                                      const QString &testCaseName)
{
    return [=](const TestResult &result) -> ITestTreeItem * {
        ITestFramework *framework = TestFrameworkManager::frameworkForId(GTest::Constants::FRAMEWORK_ID);
        QTC_ASSERT(framework, return nullptr);
        const TestTreeItem *rootNode = framework->rootNode();
        if (!rootNode)
            return nullptr;

        return rootNode->findAnyChild([&](const TreeItem *item) {
            const auto testTreeItem = static_cast<const TestTreeItem *>(item);
            return testTreeItem && matches(result.name(), projectFile, testCaseName, testTreeItem);
        });
    };
}

struct GTestData
{
    QString m_testCaseName;
    int m_iteration = 1;
};

static ResultHooks::DirectParentHook directParentHook(const QString &testCaseName, int iteration)
{
    return [=](const TestResult &result, const TestResult &other, bool *) -> bool {
        if (!other.extraData().canConvert<GTestData>())
            return false;
        const GTestData otherData = other.extraData().value<GTestData>();

        if (testCaseName == otherData.m_testCaseName) {
            const ResultType thisResult = result.result();
            const ResultType otherResult = other.result();
            if (otherResult == ResultType::MessageInternal || otherResult == ResultType::MessageLocation)
                return thisResult != ResultType::MessageInternal && thisResult != ResultType::MessageLocation;
        }
        if (iteration != otherData.m_iteration)
            return false;
        return testCaseName.isEmpty() && !otherData.m_testCaseName.isEmpty();
    };
}

GTestResult::GTestResult(const QString &id, const QString &name, const FilePath &projectFile,
                         const QString &testCaseName, int iteration)
    : TestResult(id, name, {QVariant::fromValue(GTestData{testCaseName, iteration}),
                            outputStringHook(testCaseName),
                            findTestItemHook(projectFile, testCaseName),
                            directParentHook(testCaseName, iteration)})
{}

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::GTestData);

