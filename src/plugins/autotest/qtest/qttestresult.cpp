// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qttestresult.h"
#include "../testframeworkmanager.h"
#include "../testtreeitem.h"
#include "../quick/quicktestframework.h" // FIXME BAD! - but avoids declaring QuickTestResult

#include <utils/id.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

static ResultHooks::OutputStringHook outputStringHook(const QString &function, const QString &dataTag)
{
    return [function, dataTag](const TestResult &result, bool selected) {
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

QtTestResult::QtTestResult(const QString &id, const QString &name,
                           const Utils::FilePath &projectFile, TestType type,
                           const QString &functionName, const QString &dataTag)
    : TestResult(id, name, {outputStringHook(functionName, dataTag)})
    , m_projectFile(projectFile)
    , m_type(type)
    , m_function(functionName)
    , m_dataTag(dataTag) {}

bool QtTestResult::isDirectParentOf(const TestResult *other, bool *needsIntermediate) const
{
    if (!TestResult::isDirectParentOf(other, needsIntermediate))
        return false;
    const QtTestResult *qtOther = static_cast<const QtTestResult *>(other);

    if (result() == ResultType::TestStart) {
        if (qtOther->isDataTag()) {
            if (qtOther->m_function == m_function) {
                if (m_dataTag.isEmpty()) {
                    // avoid adding function's TestCaseEnd to the data tag
                    *needsIntermediate = qtOther->result() != ResultType::TestEnd;
                    return true;
                }
                return qtOther->m_dataTag == m_dataTag;
            }
        } else if (qtOther->isTestFunction()) {
            return isTestCase() || (m_function == qtOther->m_function
                                    && qtOther->result() != ResultType::TestStart);
        }
    }
    return false;
}

bool QtTestResult::isIntermediateFor(const TestResult *other) const
{
    QTC_ASSERT(other, return false);
    const QtTestResult *qtOther = static_cast<const QtTestResult *>(other);
    return m_dataTag == qtOther->m_dataTag && m_function == qtOther->m_function
            && name() == qtOther->name() && id() == qtOther->id()
            && m_projectFile == qtOther->m_projectFile;
}

TestResult *QtTestResult::createIntermediateResultFor(const TestResult *other)
{
    QTC_ASSERT(other, return nullptr);
    const QtTestResult *qtOther = static_cast<const QtTestResult *>(other);
    QtTestResult *intermediate = new QtTestResult(qtOther->id(), qtOther->name(), qtOther->m_projectFile,
                                                  m_type);
    intermediate->m_function = qtOther->m_function;
    intermediate->m_dataTag = qtOther->m_dataTag;
    // intermediates will be needed only for data tags
    intermediate->setDescription("Data tag: " + qtOther->m_dataTag);
    const auto correspondingItem = intermediate->findTestTreeItem();
    if (correspondingItem && correspondingItem->line()) {
        intermediate->setFileName(correspondingItem->filePath());
        intermediate->setLine(correspondingItem->line());
    }
    return intermediate;
}

const ITestTreeItem *QtTestResult::findTestTreeItem() const
{
    Utils::Id id;
    if (m_type == TestType::QtTest)
        id = Utils::Id(Constants::FRAMEWORK_PREFIX).withSuffix(QtTest::Constants::FRAMEWORK_NAME);
    else
        id = Utils::Id(Constants::FRAMEWORK_PREFIX).withSuffix(QuickTest::Constants::FRAMEWORK_NAME);
    ITestFramework *framework = TestFrameworkManager::frameworkForId(id);
    QTC_ASSERT(framework, return nullptr);
    const TestTreeItem *rootNode = framework->rootNode();
    QTC_ASSERT(rootNode, return nullptr);

    return rootNode->findAnyChild([this](const Utils::TreeItem *item) {
        const TestTreeItem *treeItem = static_cast<const TestTreeItem *>(item);
        return treeItem && matches(treeItem);
    });
}

bool QtTestResult::matches(const TestTreeItem *item) const
{
    QTC_ASSERT(item, return false);
    TestTreeItem *parentItem = item->parentItem();
    QTC_ASSERT(parentItem, return false);

    TestTreeItem::Type type = item->type();
    switch (type) {
    case TestTreeItem::TestCase:
        if (!isTestCase())
            return false;
        if (item->proFile() != m_projectFile)
            return false;
        return matchesTestCase(item);
    case TestTreeItem::TestFunction:
    case TestTreeItem::TestSpecialFunction:
        // QuickTest data tags have no dedicated TestTreeItem, so treat these results as function
        if (!isTestFunction() && !(m_type == TestType::QuickTest && isDataTag()))
            return false;
        if (parentItem->proFile() != m_projectFile)
            return false;
        return matchesTestFunction(item);
    case TestTreeItem::TestDataTag: {
        if (!isDataTag())
            return false;
        TestTreeItem *grandParentItem = parentItem->parentItem();
        QTC_ASSERT(grandParentItem, return false);
        if (grandParentItem->proFile() != m_projectFile)
            return false;
        return matchesTestFunction(item);
    }
    default:
        break;
    }

    return false;
}

bool QtTestResult::matchesTestCase(const TestTreeItem *item) const
{
    // FIXME this will never work for Quick Tests
    if (item->name() == name())
        return true;
    return false;
}

bool QtTestResult::matchesTestFunction(const TestTreeItem *item) const
{
    TestTreeItem *parentItem = item->parentItem();
    TestTreeItem::Type type = item->type();
    if (m_type == TestType::QuickTest) { // Quick tests have slightly different layout // BAD/WRONG!
        const QStringList tmp = m_function.split("::");
        return tmp.size() == 2 && item->name() == tmp.last() && parentItem->name() == tmp.first();
    }
    if (type == TestTreeItem::TestDataTag) {
        TestTreeItem *grandParentItem = parentItem->parentItem();
        return parentItem->name() == m_function && grandParentItem->name() == name()
                && item->name() == m_dataTag;
    }
    return item->name() == m_function && parentItem->name() == name();
}

} // namespace Internal
} // namespace Autotest
