// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "boosttestresult.h"
#include "boosttestconstants.h"
#include "boosttesttreeitem.h"
#include "../testframeworkmanager.h"

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

static bool matches(const FilePath &fileName, const FilePath &projectFile, const QString &testCaseName,
             const QString &testSuiteName, const BoostTestTreeItem *item)
{
    // due to lacking information on the result side and a not fully appropriate tree we
    // might end up here with a differing set of tests, but it's the best we can do
    if (!item)
        return false;
    if (testCaseName.isEmpty()) // a top level module node
        return item->proFile() == projectFile;
    if (item->proFile() != projectFile)
        return false;
    if (!fileName.isEmpty() && fileName != item->filePath())
        return false;

    QString fullName = "::" + testCaseName;
    fullName.prepend(testSuiteName.isEmpty() ? QString(BoostTest::Constants::BOOST_MASTER_SUITE)
                                              : testSuiteName);

    BoostTestTreeItem::TestStates states = item->state();
    if (states & BoostTestTreeItem::Templated) {
        const QRegularExpression regex(
            QRegularExpression::wildcardToRegularExpression(item->fullName() + "<*>"));
        return regex.match(fullName).hasMatch();
    } else if (states & BoostTestTreeItem::Parameterized) {
        const QRegularExpression regex(
            QRegularExpression::anchoredPattern(item->fullName() + "_\\d+"));
        return regex.isValid() && regex.match(fullName).hasMatch();
    }
    return item->fullName() == fullName;
}

static ResultHooks::FindTestItemHook findTestItemHook(const FilePath &projectFile,
                                                      const QString &testCaseName,
                                                      const QString &testSuiteName)
{
    return [=](const TestResult &result) -> ITestTreeItem * {
        ITestFramework *framework =
            TestFrameworkManager::frameworkForId(BoostTest::Constants::FRAMEWORK_ID);
        QTC_ASSERT(framework, return nullptr);
        const TestTreeItem *rootNode = framework->rootNode();
        if (!rootNode)
            return nullptr;

        return rootNode->findAnyChild([&](const TreeItem *item) {
            const auto testTreeItem = static_cast<const BoostTestTreeItem *>(item);
            return testTreeItem && matches(result.fileName(), projectFile, testCaseName,
                                           testSuiteName, testTreeItem);
        });
    };
}

struct BoostTestData
{
    QString m_testCaseName;
    QString m_testSuiteName;
};

static ResultHooks::DirectParentHook directParentHook(const QString &testCaseName,
                                                      const QString &testSuiteName)
{
    return [=](const TestResult &result, const TestResult &other, bool *) -> bool {
        if (!other.extraData().canConvert<BoostTestData>())
            return false;
        const BoostTestData otherData = other.extraData().value<BoostTestData>();

        if (result.result() != ResultType::TestStart)
            return false;

        bool thisModule = (testCaseName.isEmpty() && testSuiteName.isEmpty());
        bool thisSuite = (testCaseName.isEmpty() && !testSuiteName.isEmpty());
        bool thisCase = (!testCaseName.isEmpty());

        bool otherSuite = otherData.m_testCaseName.isEmpty() && !otherData.m_testSuiteName.isEmpty();
        bool otherCase = !otherData.m_testCaseName.isEmpty();

        if (otherSuite)
            return thisSuite ? otherData.m_testSuiteName.startsWith(testSuiteName + '/') : thisModule;

        if (otherCase) {
            if (thisCase)
                return otherData.m_testCaseName == testCaseName && otherData.m_testSuiteName == testSuiteName;
            if (thisSuite)
                return otherData.m_testSuiteName == testSuiteName;
            if (thisModule)
                return otherData.m_testSuiteName.isEmpty();
        }
        return false;
    };
}

BoostTestResult::BoostTestResult(const QString &id, const QString &name,
                                 const FilePath &projectFile, const QString &testCaseName,
                                 const QString &testSuiteName)
    : TestResult(id, name, {QVariant::fromValue(BoostTestData{testCaseName, testSuiteName}),
                            outputStringHook(testCaseName),
                            findTestItemHook(projectFile, testCaseName, testSuiteName),
                            directParentHook(testCaseName, testSuiteName)})
{}

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::BoostTestData);
