// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gtestresult.h"
#include "gtestconstants.h"
#include "../testframeworkmanager.h"
#include "../testtreeitem.h"

#include <utils/id.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

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

GTestResult::GTestResult(const QString &id, const QString &name, const Utils::FilePath &projectFile,
                         const QString &testCaseName, int iteration)
    : TestResult(id, name, {outputStringHook(testCaseName)})
    , m_projectFile(projectFile)
    , m_testCaseName(testCaseName)
    , m_iteration(iteration)
{
}

bool GTestResult::isDirectParentOf(const TestResult *other, bool *needsIntermediate) const
{
    if (!TestResult::isDirectParentOf(other, needsIntermediate))
        return false;

    const GTestResult *gtOther = static_cast<const GTestResult *>(other);
    if (m_testCaseName == gtOther->m_testCaseName) {
        const ResultType otherResult = other->result();
        if (otherResult == ResultType::MessageInternal || otherResult == ResultType::MessageLocation)
            return result() != ResultType::MessageInternal && result() != ResultType::MessageLocation;
    }
    if (m_iteration != gtOther->m_iteration)
        return false;
    return isTestSuite() && gtOther->isTestCase();
}

static QString normalizeName(const QString &name)
{
    static QRegularExpression parameterIndex("/\\d+");

    QString nameWithoutParameterIndices = name;
    nameWithoutParameterIndices.remove(parameterIndex);

    return nameWithoutParameterIndices.split('/').last();
}

static QString normalizeTestName(const QString &testname)
{
    QString nameWithoutTypeParam = testname.split(',').first();

    return normalizeName(nameWithoutTypeParam);
}

const ITestTreeItem *GTestResult::findTestTreeItem() const
{
    auto id = Utils::Id(Constants::FRAMEWORK_PREFIX).withSuffix(GTest::Constants::FRAMEWORK_NAME);
    ITestFramework *framework = TestFrameworkManager::frameworkForId(id);
    QTC_ASSERT(framework, return nullptr);
    const TestTreeItem *rootNode = framework->rootNode();
    if (!rootNode)
        return nullptr;

    return rootNode->findAnyChild([this](const Utils::TreeItem *item) {
        const auto treeItem = static_cast<const TestTreeItem *>(item);
        return treeItem && matches(treeItem);
    });
}

bool GTestResult::matches(const TestTreeItem *treeItem) const
{
    if (treeItem->proFile() != m_projectFile)
        return false;

    if (isTestSuite())
        return matchesTestSuite(treeItem);

    return matchesTestCase(treeItem);
}

bool GTestResult::matchesTestCase(const TestTreeItem *treeItem) const
{
    if (treeItem->type() != TestTreeItem::TestCase)
        return false;

    const QString testItemTestCase = treeItem->parentItem()->name() + '.' + treeItem->name();
    return testItemTestCase == normalizeName(m_testCaseName);
}

bool GTestResult::matchesTestSuite(const TestTreeItem *treeItem) const
{
    if (treeItem->type() != TestTreeItem::TestSuite)
        return false;

    return treeItem->name() == normalizeTestName(name());
}

} // namespace Internal
} // namespace Autotest
