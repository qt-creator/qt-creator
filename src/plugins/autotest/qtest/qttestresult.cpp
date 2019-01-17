/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qttestresult.h"
#include "../testframeworkmanager.h"
#include "../testtreeitem.h"
#include "../quick/quicktestframework.h" // FIXME BAD! - but avoids declaring QuickTestResult

#include <coreplugin/id.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

QtTestResult::QtTestResult(const QString &projectFile, TestType type, const QString &className)
    : TestResult(className), m_projectFile(projectFile), m_type(type)
{
}

QtTestResult::QtTestResult(const QString &id, const QString &projectFile, TestType type,
                           const QString &className)
    : TestResult(id, className), m_projectFile(projectFile), m_type(type)
{
}

const QString QtTestResult::outputString(bool selected) const
{
    const QString &desc = description();
    const QString &className = name();
    QString output;
    switch (result()) {
    case Result::Pass:
    case Result::Fail:
    case Result::ExpectedFail:
    case Result::UnexpectedPass:
    case Result::BlacklistedFail:
    case Result::BlacklistedPass:
        output = className + "::" + m_function;
        if (!m_dataTag.isEmpty())
            output.append(QString(" (%1)").arg(m_dataTag));
        if (selected && !desc.isEmpty()) {
            output.append('\n').append(desc);
        }
        break;
    case Result::Benchmark:
        output = className + "::" + m_function;
        if (!m_dataTag.isEmpty())
            output.append(QString(" (%1)").arg(m_dataTag));
        if (!desc.isEmpty()) {
            int breakPos = desc.indexOf('(');
            output.append(": ").append(desc.left(breakPos));
            if (selected)
                output.append('\n').append(desc.midRef(breakPos));
        }
        break;
    default:
        output = desc;
        if (!selected)
            output = output.split('\n').first();
    }
    return output;
}

bool QtTestResult::isDirectParentOf(const TestResult *other, bool *needsIntermediate) const
{
    if (!TestResult::isDirectParentOf(other, needsIntermediate))
        return false;
    const QtTestResult *qtOther = static_cast<const QtTestResult *>(other);

    if (TestResult::isMessageCaseStart(result())) {
        if (qtOther->isDataTag()) {
            if (qtOther->m_function == m_function) {
                if (m_dataTag.isEmpty()) {
                    // avoid adding function's TestCaseEnd to the data tag
                    *needsIntermediate = qtOther->result() != Result::MessageTestCaseEnd;
                    return true;
                }
                return qtOther->m_dataTag == m_dataTag;
            }
        } else if (qtOther->isTestFunction()) {
            return isTestCase() || m_function == qtOther->m_function;
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
    QtTestResult *intermediate = new QtTestResult(qtOther->id(), qtOther->m_projectFile,
                                                  m_type, qtOther->name());
    intermediate->m_function = qtOther->m_function;
    intermediate->m_dataTag = qtOther->m_dataTag;
    // intermediates will be needed only for data tags
    intermediate->setDescription("Data tag: " + qtOther->m_dataTag);
    const auto correspondingItem = intermediate->findTestTreeItem();
    if (correspondingItem && correspondingItem->line()) {
        intermediate->setFileName(correspondingItem->filePath());
        intermediate->setLine(static_cast<int>(correspondingItem->line()));
    }
    return intermediate;
}

const TestTreeItem *QtTestResult::findTestTreeItem() const
{
    Core::Id id;
    if (m_type == TestType::QtTest)
        id = Core::Id(Constants::FRAMEWORK_PREFIX).withSuffix(QtTest::Constants::FRAMEWORK_NAME);
    else
        id = Core::Id(Constants::FRAMEWORK_PREFIX).withSuffix(QuickTest::Constants::FRAMEWORK_NAME);
    const TestTreeItem *rootNode = TestFrameworkManager::instance()->rootNodeForTestFramework(id);
    QTC_ASSERT(rootNode, return nullptr);

    const auto item = rootNode->findAnyChild([this](const Utils::TreeItem *item) {
        const TestTreeItem *treeItem = static_cast<const TestTreeItem *>(item);
        return treeItem && matches(treeItem);
    });
    return static_cast<const TestTreeItem *>(item);
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
    case TestTreeItem::TestFunctionOrSet:
    case TestTreeItem::TestSpecialFunction:
        if (!isTestFunction())
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
