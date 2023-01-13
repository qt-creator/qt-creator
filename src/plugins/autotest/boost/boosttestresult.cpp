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
        const Id id = Id(Constants::FRAMEWORK_PREFIX).withSuffix(BoostTest::Constants::FRAMEWORK_NAME);
        ITestFramework *framework = TestFrameworkManager::frameworkForId(id);
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

BoostTestResult::BoostTestResult(const QString &id, const QString &name,
                                 const FilePath &projectFile, const QString &testCaseName,
                                 const QString &testSuiteName)
    : TestResult(id, name, {outputStringHook(testCaseName),
                            findTestItemHook(projectFile, testCaseName, testSuiteName)})
    , m_projectFile(projectFile)
    , m_testCaseName(testCaseName)
    , m_testSuiteName(testSuiteName) {}

bool BoostTestResult::isDirectParentOf(const TestResult *other, bool *needsIntermediate) const
{
    if (!TestResult::isDirectParentOf(other, needsIntermediate))
        return false;

    if (result() != ResultType::TestStart)
        return false;

    bool weAreModule = (m_testCaseName.isEmpty() && m_testSuiteName.isEmpty());
    bool weAreSuite = (m_testCaseName.isEmpty() && !m_testSuiteName.isEmpty());
    bool weAreCase = (!m_testCaseName.isEmpty());

    const BoostTestResult *boostOther = static_cast<const BoostTestResult *>(other);
    bool otherIsSuite = boostOther->m_testCaseName.isEmpty() && !boostOther->m_testSuiteName.isEmpty();
    bool otherIsCase = !boostOther->m_testCaseName.isEmpty();

    if (otherIsSuite)
        return weAreSuite ? boostOther->m_testSuiteName.startsWith(m_testSuiteName + '/') : weAreModule;

    if (otherIsCase) {
        if (weAreCase)
            return boostOther->m_testCaseName == m_testCaseName && boostOther->m_testSuiteName == m_testSuiteName;
        if (weAreSuite)
            return boostOther->m_testSuiteName == m_testSuiteName;
        if (weAreModule)
            return boostOther->m_testSuiteName.isEmpty();
    }
    return false;
}

} // namespace Internal
} // namespace Autotest

