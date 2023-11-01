// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qttestresult.h"
#include "../testframeworkmanager.h"
#include "../testtreeitem.h"
#include "../quick/quicktestframework.h" // FIXME BAD! - but avoids declaring QuickTestResult

#include <utils/id.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace Autotest {
namespace Internal {

static ResultHooks::OutputStringHook outputStringHook(const QString &function, const QString &dataTag)
{
    return [=](const TestResult &result, bool selected) {
        const QString &desc = result.description();
        const QString &className = result.name();
        QString output;
        switch (result.result()) {
        case ResultType::Pass:
        case ResultType::Fail:
        case ResultType::ExpectedFail:
        case ResultType::UnexpectedPass:
        case ResultType::BlacklistedFail:
        case ResultType::BlacklistedPass:
            output = className;
            if (!function.isEmpty())
                output.append("::" + function);
            if (!dataTag.isEmpty())
                output.append(QString(" (%1)").arg(dataTag));
            if (selected && !desc.isEmpty()) {
                output.append('\n').append(desc);
            }
            break;
        case ResultType::Benchmark:
            output = className;
            if (!function.isEmpty())
                output.append("::" + function);
            if (!dataTag.isEmpty())
                output.append(QString(" (%1)").arg(dataTag));
            if (!desc.isEmpty()) {
                int breakPos = desc.indexOf('(');
                output.append(": ").append(desc.left(breakPos));
                if (selected)
                    output.append('\n').append(desc.mid(breakPos));
            }
            break;
        default:
            output = desc;
            if (!selected)
                output = output.split('\n').first();
        }
        return output;
    };
}

static bool matchesTestCase(const QString &name, const TestTreeItem *item)
{
    // FIXME this will never work for Quick Tests
    return item->name() == name;
}

static bool matchesTestFunction(const QString &name, TestType testType, const QString &functionName,
                                const QString &dataTag, const TestTreeItem *item)
{
    TestTreeItem *parentItem = item->parentItem();
    const TestTreeItem::Type type = item->type();
    if (testType == TestType::QuickTest) { // Quick tests have slightly different layout // BAD/WRONG!
        const QStringList tmp = functionName.split("::");
        return tmp.size() == 2 && item->name() == tmp.last() && parentItem->name() == tmp.first();
    }
    if (type == TestTreeItem::TestDataTag) {
        TestTreeItem *grandParentItem = parentItem->parentItem();
        return parentItem->name() == functionName && grandParentItem->name() == name
                && item->name() == dataTag;
    }
    return item->name() == functionName && parentItem->name() == name;
}

static bool matches(const QString &name, const FilePath &projectFile, TestType testType,
                    const QString &functionName, const QString &dataTag, const TestTreeItem *item)
{
    const auto isTestCase = [&] { return functionName.isEmpty()  && dataTag.isEmpty(); };
    const auto isTestFunction = [&] { return !functionName.isEmpty() && dataTag.isEmpty(); };
    const auto isDataTag = [&] { return !functionName.isEmpty() && !dataTag.isEmpty(); };

    QTC_ASSERT(item, return false);
    TestTreeItem *parentItem = item->parentItem();
    QTC_ASSERT(parentItem, return false);

    TestTreeItem::Type type = item->type();
    switch (type) {
    case TestTreeItem::TestCase:
        if (!isTestCase())
            return false;
        if (item->proFile() != projectFile)
            return false;
        return matchesTestCase(name, item);
    case TestTreeItem::TestFunction:
    case TestTreeItem::TestSpecialFunction:
        // QuickTest data tags have no dedicated TestTreeItem, so treat these results as function
        if (!isTestFunction() && !(testType == TestType::QuickTest && isDataTag()))
            return false;
        if (parentItem->proFile() != projectFile)
            return false;
        return matchesTestFunction(name, testType, functionName, dataTag, item);
    case TestTreeItem::TestDataTag: {
        if (!isDataTag())
            return false;
        TestTreeItem *grandParentItem = parentItem->parentItem();
        QTC_ASSERT(grandParentItem, return false);
        if (grandParentItem->proFile() != projectFile)
            return false;
        return matchesTestFunction(name, testType, functionName, dataTag, item);
    }
    default:
        break;
    }

    return false;
}

static ResultHooks::FindTestItemHook findTestItemHook(const FilePath &projectFile, TestType type,
                                                      const QString &functionName,
                                                      const QString &dataTag)
{
    return [=](const TestResult &result) -> ITestTreeItem * {
        const Id id = type == TestType::QtTest
                ? Id(QtTest::Constants::FRAMEWORK_ID)
                : Id(QuickTest::Constants::FRAMEWORK_ID);
        ITestFramework *framework = TestFrameworkManager::frameworkForId(id);
        QTC_ASSERT(framework, return nullptr);
        const TestTreeItem *rootNode = framework->rootNode();
        QTC_ASSERT(rootNode, return nullptr);

        return rootNode->findAnyChild([&](const TreeItem *item) {
            const TestTreeItem *testTreeItem = static_cast<const TestTreeItem *>(item);
            return testTreeItem && matches(result.name(), projectFile, type, functionName, dataTag,
                                           testTreeItem);
        });
    };
}

struct QtTestData
{
    FilePath m_projectFile;
    TestType m_type;
    QString m_function;
    QString m_dataTag;
    bool isTestFunction() const { return !m_function.isEmpty() && m_dataTag.isEmpty(); }
    bool isDataTag() const { return !m_function.isEmpty() && !m_dataTag.isEmpty(); }
};

static ResultHooks::DirectParentHook directParentHook(const QString &functionName,
                                                      const QString &dataTag)
{
    return [=](const TestResult &result, const TestResult &other, bool *needsIntermediate) -> bool {
        if (!other.extraData().canConvert<QtTestData>())
            return false;
        const QtTestData otherData = other.extraData().value<QtTestData>();

        if (result.result() == ResultType::TestStart) {
            if (otherData.isDataTag()) {
                if (otherData.m_function == functionName) {
                    if (dataTag.isEmpty()) {
                        // avoid adding function's TestCaseEnd to the data tag
                        *needsIntermediate = other.result() != ResultType::TestEnd;
                        return true;
                    }
                    return otherData.m_dataTag == dataTag;
                }
            } else if (otherData.isTestFunction()) {
                return (functionName.isEmpty() && dataTag.isEmpty())
                        || (functionName == otherData.m_function
                            && other.result() != ResultType::TestStart);
            } else if (other.result() == ResultType::MessageInternal) {
                return other.name() == result.name();
            }
        }
        return false;
    };
}

static ResultHooks::IntermediateHook intermediateHook(const FilePath &projectFile,
                                                      const QString &functionName,
                                                      const QString &dataTag)
{
    return [=](const TestResult &result, const TestResult &other) -> bool {
        if (!other.extraData().canConvert<QtTestData>())
            return false;
        const QtTestData otherData = other.extraData().value<QtTestData>();
        return dataTag == otherData.m_dataTag && functionName == otherData.m_function
                && result.name() == other.name() && result.id() == other.id()
                && projectFile == otherData.m_projectFile;
    };
}

static ResultHooks::CreateResultHook createResultHook(const FilePath &projectFile, TestType type,
                                                      const QString &functionName,
                                                      const QString &dataTag)
{
    return [=](const TestResult &result) -> TestResult {
        TestResult newResult =  QtTestResult(result.id(), result.name(), projectFile, type,
                                             functionName, dataTag);
        // intermediates will be needed only for data tags
        newResult.setDescription("Data tag: " + dataTag);
        const auto correspondingItem = newResult.findTestTreeItem();
        if (correspondingItem && correspondingItem->line()) {
            newResult.setFileName(correspondingItem->filePath());
            newResult.setLine(correspondingItem->line());
        }
        return newResult;
    };
}

QtTestResult::QtTestResult(const QString &id, const QString &name, const FilePath &projectFile,
                           TestType type, const QString &functionName, const QString &dataTag)
    : TestResult(id, name, {QVariant::fromValue(QtTestData{projectFile, type, functionName, dataTag}),
                            outputStringHook(functionName, dataTag),
                            findTestItemHook(projectFile, type, functionName, dataTag),
                            directParentHook(functionName, dataTag),
                            intermediateHook(projectFile, functionName, dataTag),
                            createResultHook(projectFile, type, functionName, dataTag)})
{}

} // namespace Internal
} // namespace Autotest

Q_DECLARE_METATYPE(Autotest::Internal::QtTestData);
