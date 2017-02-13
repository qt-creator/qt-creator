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

#include "autotesticons.h"
#include "testresultdelegate.h"
#include "testresultmodel.h"

#include <utils/qtcassert.h>

#include <QFontMetrics>
#include <QIcon>

namespace Autotest {
namespace Internal {

/********************************* TestResultItem ******************************************/

TestResultItem::TestResultItem(const TestResultPtr &testResult)
    : m_testResult(testResult)
{
}

TestResultItem::~TestResultItem()
{
}

static QIcon testResultIcon(Result::Type result) {
    const static QIcon icons[] = {
        Icons::RESULT_PASS.icon(),
        Icons::RESULT_FAIL.icon(),
        Icons::RESULT_XFAIL.icon(),
        Icons::RESULT_XPASS.icon(),
        Icons::RESULT_SKIP.icon(),
        Icons::RESULT_BLACKLISTEDPASS.icon(),
        Icons::RESULT_BLACKLISTEDFAIL.icon(),
        Icons::RESULT_BENCHMARK.icon(),
        Icons::RESULT_MESSAGEDEBUG.icon(),
        Icons::RESULT_MESSAGEDEBUG.icon(), // Info gets the same handling as Debug for now
        Icons::RESULT_MESSAGEWARN.icon(),
        Icons::RESULT_MESSAGEFATAL.icon(),
        Icons::RESULT_MESSAGEFATAL.icon(), // System gets same handling as Fatal for now
    }; // provide an icon for unknown??

    if (result < 0 || result >= Result::MessageInternal) {
        switch (result) {
        case Result::MessageTestCaseSuccess:
            return icons[Result::Pass];
        case Result::MessageTestCaseFail:
            return icons[Result::Fail];
        case Result::MessageTestCaseWarn:
            return icons[Result::MessageWarn];
        default:
            return QIcon();
        }
    }
    return icons[result];
}

QVariant TestResultItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DecorationRole:
        return m_testResult ? testResultIcon(m_testResult->result()) : QVariant();
    case Qt::DisplayRole:
        return m_testResult ? m_testResult->outputString(true) : QVariant();
    default:
        return Utils::TreeItem::data(column, role);
    }
}

void TestResultItem::updateDescription(const QString &description)
{
    QTC_ASSERT(m_testResult, return);

    m_testResult->setDescription(description);
}

void TestResultItem::updateResult()
{
    if (!TestResult::isMessageCaseStart(m_testResult->result()))
        return;

    Result::Type newResult = Result::MessageTestCaseSuccess;
    for (Utils::TreeItem *child : *this) {
        const TestResult *current = static_cast<TestResultItem *>(child)->testResult();
        if (current) {
            switch (current->result()) {
            case Result::Fail:
            case Result::MessageFatal:
            case Result::UnexpectedPass:
            case Result::MessageTestCaseFail:
                m_testResult->setResult(Result::MessageTestCaseFail);
                return;
            case Result::ExpectedFail:
            case Result::MessageWarn:
            case Result::Skip:
            case Result::BlacklistedFail:
            case Result::BlacklistedPass:
            case Result::MessageTestCaseWarn:
                newResult = Result::MessageTestCaseWarn;
                break;
            default: {}
            }
        }
    }
    m_testResult->setResult(newResult);
}

void TestResultItem::updateIntermediateChildren()
{
    for (Utils::TreeItem *child : *this) {
        TestResultItem *childItem = static_cast<TestResultItem *>(child);
        if (childItem->testResult()->result() == Result::MessageIntermediate)
            childItem->updateResult();
    }
}

TestResultItem *TestResultItem::intermediateFor(const TestResultItem *item) const
{
    QTC_ASSERT(item, return nullptr);
    const TestResult *otherResult = item->testResult();
    for (int row = childCount() - 1; row >= 0; --row) {
        TestResultItem *child = static_cast<TestResultItem *>(childAt(row));
        const TestResult *testResult = child->testResult();
        if (testResult->result() != Result::MessageIntermediate)
            continue;
        if (testResult->isIntermediateFor(otherResult))
            return child;
    }
    return nullptr;
}

TestResultItem *TestResultItem::createAndAddIntermediateFor(const TestResultItem *child)
{
    TestResultPtr result(m_testResult->createIntermediateResultFor(child->testResult()));
    QTC_ASSERT(!result.isNull(), return nullptr);
    result->setResult(Result::MessageIntermediate);
    TestResultItem *intermediate = new TestResultItem(result);
    appendChild(intermediate);
    return intermediate;
}

/********************************* TestResultModel *****************************************/

TestResultModel::TestResultModel(QObject *parent)
    : Utils::TreeModel<>(parent)
{
}

void TestResultModel::addTestResult(const TestResultPtr &testResult, bool autoExpand)
{
    const int lastRow = rootItem()->childCount() - 1;
    if (testResult->result() == Result::MessageCurrentTest) {
        // MessageCurrentTest should always be the last top level item
        if (lastRow >= 0) {
            TestResultItem *current = static_cast<TestResultItem *>(rootItem()->childAt(lastRow));
            const TestResult *result = current->testResult();
            if (result && result->result() == Result::MessageCurrentTest) {
                current->updateDescription(testResult->description());
                emit dataChanged(current->index(), current->index());
                return;
            }
        }

        rootItem()->appendChild(new TestResultItem(testResult));
        return;
    }

    if (testResult->result() == Result::MessageDisabledTests)
        m_disabled += testResult->line();
    m_testResultCount[testResult->result()]++;

    TestResultItem *newItem = new TestResultItem(testResult);
    TestResultItem *parentItem = findParentItemFor(newItem);
    addFileName(testResult->fileName()); // ensure we calculate the results pane correctly
    if (parentItem) {
        parentItem->appendChild(newItem);
        if (autoExpand)
            parentItem->expand();
        if (testResult->result() == Result::MessageTestCaseEnd) {
            if (parentItem->childCount()) {
                parentItem->updateIntermediateChildren();
                emit dataChanged(parentItem->firstChild()->index(),
                                 parentItem->lastChild()->index());
            }
            parentItem->updateResult();
            emit dataChanged(parentItem->index(), parentItem->index());
        }
    } else {
        if (lastRow >= 0) {
            TestResultItem *current = static_cast<TestResultItem *>(rootItem()->childAt(lastRow));
            const TestResult *result = current->testResult();
            if (result && result->result() == Result::MessageCurrentTest) {
                rootItem()->insertChild(current->index().row(), newItem);
                return;
            }
        }
        // there is no MessageCurrentTest at the last row, but we have a toplevel item - just add it
        rootItem()->appendChild(newItem);
    }
}

void TestResultModel::removeCurrentTestMessage()
{
    std::vector<Utils::TreeItem *> topLevelItems(rootItem()->begin(), rootItem()->end());
    for (int row = topLevelItems.size() - 1; row >= 0; --row) {
        TestResultItem *current = static_cast<TestResultItem *>(topLevelItems[row]);
        if (current->testResult()->result() == Result::MessageCurrentTest) {
            destroyItem(current);
            break;
        }
    }
}

void TestResultModel::clearTestResults()
{
    clear();
    m_testResultCount.clear();
    m_disabled = 0;
    m_fileNames.clear();
    m_maxWidthOfFileName = 0;
    m_widthOfLineNumber = 0;
}

const TestResult *TestResultModel::testResult(const QModelIndex &idx)
{
    if (idx.isValid())
        return static_cast<TestResultItem *>(itemForIndex(idx))->testResult();

    return 0;
}

void TestResultModel::recalculateMaxWidthOfFileName(const QFont &font)
{
    const QFontMetrics fm(font);
    m_maxWidthOfFileName = 0;
    for (const QString &fileName : m_fileNames) {
        int pos = fileName.lastIndexOf('/');
        m_maxWidthOfFileName = qMax(m_maxWidthOfFileName, fm.width(fileName.mid(pos + 1)));
    }
}

void TestResultModel::addFileName(const QString &fileName)
{
    const QFontMetrics fm(m_measurementFont);
    int pos = fileName.lastIndexOf('/');
    m_maxWidthOfFileName = qMax(m_maxWidthOfFileName, fm.width(fileName.mid(pos + 1)));
    m_fileNames.insert(fileName);
}

int TestResultModel::maxWidthOfFileName(const QFont &font)
{
    if (font != m_measurementFont)
        recalculateMaxWidthOfFileName(font);
    return m_maxWidthOfFileName;
}

int TestResultModel::maxWidthOfLineNumber(const QFont &font)
{
    if (m_widthOfLineNumber == 0 || font != m_measurementFont) {
        QFontMetrics fm(font);
        m_measurementFont = font;
        m_widthOfLineNumber = fm.width("88888");
    }
    return m_widthOfLineNumber;
}

TestResultItem *TestResultModel::findParentItemFor(const TestResultItem *item,
                                                   const TestResultItem *startItem) const
{
    QTC_ASSERT(item, return nullptr);
    TestResultItem *root = startItem ? const_cast<TestResultItem *>(startItem) : nullptr;
    const TestResult *result = item->testResult();
    const QString &name = result->name();

    if (root == nullptr && !name.isEmpty()) {
        for (int row = rootItem()->childCount() - 1; row >= 0; --row) {
            TestResultItem *tmp = static_cast<TestResultItem *>(rootItem()->childAt(row));
            if (tmp->testResult()->name() == name) {
                root = tmp;
                break;
            }
        }
    }
    if (root == nullptr)
        return root;

    bool needsIntermediate = false;
    auto predicate = [result, &needsIntermediate](Utils::TreeItem *it) {
        TestResultItem *currentItem = static_cast<TestResultItem *>(it);
        return currentItem->testResult()->isDirectParentOf(result, &needsIntermediate);
    };
    TestResultItem *parent = static_cast<TestResultItem *>(root->findAnyChild(predicate));
    if (parent) {
        if (needsIntermediate) {
            // check if the intermediate is present already
            if (TestResultItem *intermediate = parent->intermediateFor(item))
                return intermediate;
            return parent->createAndAddIntermediateFor(item);
        }
        return parent;
    }
    return root;
}

/********************************** Filter Model **********************************/

TestResultFilterModel::TestResultFilterModel(TestResultModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent),
      m_sourceModel(sourceModel)
{
    setSourceModel(sourceModel);
    enableAllResultTypes();
}

void TestResultFilterModel::enableAllResultTypes()
{
    m_enabled << Result::Pass << Result::Fail << Result::ExpectedFail
              << Result::UnexpectedPass << Result::Skip << Result::MessageDebug
              << Result::MessageWarn << Result::MessageInternal
              << Result::MessageFatal << Result::Invalid << Result::BlacklistedPass
              << Result::BlacklistedFail << Result::Benchmark << Result::MessageIntermediate
              << Result::MessageCurrentTest << Result::MessageTestCaseStart
              << Result::MessageTestCaseSuccess << Result::MessageTestCaseWarn
              << Result::MessageTestCaseFail << Result::MessageTestCaseEnd
              << Result::MessageInfo << Result::MessageSystem;
    invalidateFilter();
}

void TestResultFilterModel::toggleTestResultType(Result::Type type)
{
    if (m_enabled.contains(type)) {
        m_enabled.remove(type);
        if (type == Result::MessageInternal)
            m_enabled.remove(Result::MessageTestCaseEnd);
        if (type == Result::MessageDebug)
            m_enabled.remove(Result::MessageInfo);
    } else {
        m_enabled.insert(type);
        if (type == Result::MessageInternal)
            m_enabled.insert(Result::MessageTestCaseEnd);
        if (type == Result::MessageDebug)
            m_enabled.insert(Result::MessageInfo);
    }
    invalidateFilter();
}

void TestResultFilterModel::clearTestResults()
{
    m_sourceModel->clearTestResults();
}

bool TestResultFilterModel::hasResults()
{
    return rowCount(QModelIndex());
}

const TestResult *TestResultFilterModel::testResult(const QModelIndex &index) const
{
    return m_sourceModel->testResult(mapToSource(index));
}

bool TestResultFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = m_sourceModel->index(sourceRow, 0, sourceParent);
    if (!index.isValid())
        return false;
    Result::Type resultType = m_sourceModel->testResult(index)->result();
    switch (resultType) {
    case Result::MessageTestCaseSuccess:
        return m_enabled.contains(Result::Pass);
    case Result::MessageTestCaseFail:
    case Result::MessageTestCaseWarn:
        return acceptTestCaseResult(index);
    default:
        return m_enabled.contains(resultType);
    }
}

bool TestResultFilterModel::acceptTestCaseResult(const QModelIndex &srcIndex) const
{
    for (int row = 0, count = m_sourceModel->rowCount(srcIndex); row < count; ++row) {
        const QModelIndex &child = srcIndex.child(row, 0);
        Result::Type type = m_sourceModel->testResult(child)->result();
        if (type == Result::MessageTestCaseSuccess)
            type = Result::Pass;
        if (type == Result::MessageTestCaseFail || type == Result::MessageTestCaseWarn) {
            if (acceptTestCaseResult(child))
                return true;
        } else if (m_enabled.contains(type)) {
            return true;
        }
    }
    return false;
}

} // namespace Internal
} // namespace Autotest
