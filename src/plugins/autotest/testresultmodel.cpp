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

#include "testresultmodel.h"
#include "autotesticons.h"
#include "autotestplugin.h"
#include "testresultdelegate.h"
#include "testrunner.h"
#include "testsettings.h"

#include <projectexplorer/projectexplorericons.h>
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

static QIcon testResultIcon(ResultType result) {
    const static QIcon icons[] = {
        Icons::RESULT_PASS.icon(),
        Icons::RESULT_FAIL.icon(),
        Icons::RESULT_XFAIL.icon(),
        Icons::RESULT_XPASS.icon(),
        Icons::RESULT_SKIP.icon(),
        Icons::RESULT_BLACKLISTEDPASS.icon(),
        Icons::RESULT_BLACKLISTEDFAIL.icon(),
        Icons::RESULT_BLACKLISTEDXPASS.icon(),
        Icons::RESULT_BLACKLISTEDXFAIL.icon(),
        Icons::RESULT_BENCHMARK.icon(),
        Icons::RESULT_MESSAGEDEBUG.icon(),
        Icons::RESULT_MESSAGEDEBUG.icon(), // Info gets the same handling as Debug for now
        Icons::RESULT_MESSAGEWARN.icon(),
        Icons::RESULT_MESSAGEFATAL.icon(),
        Icons::RESULT_MESSAGEFATAL.icon(), // System gets same handling as Fatal for now
        Icons::RESULT_MESSAGEFATAL.icon(), // Error gets same handling as Fatal for now
        ProjectExplorer::Icons::DESKTOP_DEVICE.icon(),  // for now
    }; // provide an icon for unknown??

    if (result < ResultType::FIRST_TYPE || result >= ResultType::MessageInternal) {
        switch (result) {
        case ResultType::Application:
            return icons[16];
        default:
            return QIcon();
        }
    }
    return icons[int(result)];
}

static QIcon testSummaryIcon(const Utils::optional<TestResultItem::SummaryEvaluation> &summary)
{
    if (!summary)
        return QIcon();
    if (summary->failed)
        return summary->warnings ? Icons::RESULT_MESSAGEFAILWARN.icon() : Icons::RESULT_FAIL.icon();
    return summary->warnings ? Icons::RESULT_MESSAGEPASSWARN.icon() : Icons::RESULT_PASS.icon();
}

QVariant TestResultItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DecorationRole: {
        if (!m_testResult)
            return QVariant();
        const ResultType result = m_testResult->result();
        if (result == ResultType::MessageLocation && parent())
            return parent()->data(column, role);
        if (result == ResultType::TestStart)
            return testSummaryIcon(m_summaryResult);
        return testResultIcon(result);
    }
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

static bool isSignificant(ResultType type)
{
    switch (type) {
    case ResultType::Benchmark:
    case ResultType::MessageInfo:
    case ResultType::MessageInternal:
    case ResultType::TestEnd:
        return false;
    case ResultType::MessageLocation:
    case ResultType::MessageCurrentTest:
    case ResultType::Application:
    case ResultType::Invalid:
        QTC_ASSERT_STRING("Got unexpedted type in isSignificant check");
        return false;
    default:
        return true;
    }
}

void TestResultItem::updateResult(bool &changed, ResultType addedChildType,
                                  const Utils::optional<SummaryEvaluation> &summary)
{
    changed = false;
    if (m_testResult->result() != ResultType::TestStart)
        return;

    if (!isSignificant(addedChildType) || (addedChildType == ResultType::TestStart && !summary))
        return;

    if (m_summaryResult.has_value() && m_summaryResult->failed && m_summaryResult->warnings)
        return; // can't become worse

    SummaryEvaluation newResult = m_summaryResult.value_or(SummaryEvaluation());
    switch (addedChildType) {
    case ResultType::Fail:
    case ResultType::MessageFatal:
    case ResultType::UnexpectedPass:
        if (newResult.failed)
            return;
        newResult.failed = true;
        break;
    case ResultType::ExpectedFail:
    case ResultType::MessageWarn:
    case ResultType::MessageSystem:
    case ResultType::Skip:
    case ResultType::BlacklistedFail:
    case ResultType::BlacklistedPass:
    case ResultType::BlacklistedXFail:
    case ResultType::BlacklistedXPass:
        if (newResult.warnings)
            return;
        newResult.warnings = true;
        break;
    case ResultType::TestStart:
        if (summary) {
            newResult.failed |= summary->failed;
            newResult.warnings |= summary->warnings;
        }
        break;
    default:
        break;
    }
    changed = !m_summaryResult.has_value() || m_summaryResult.value() != newResult;

    if (changed)
        m_summaryResult.emplace(newResult);
}

TestResultItem *TestResultItem::intermediateFor(const TestResultItem *item) const
{
    QTC_ASSERT(item, return nullptr);
    const TestResult *otherResult = item->testResult();
    for (int row = childCount() - 1; row >= 0; --row) {
        TestResultItem *child = childAt(row);
        const TestResult *testResult = child->testResult();
        if (testResult->result() != ResultType::TestStart)
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
    result->setResult(ResultType::TestStart);
    TestResultItem *intermediate = new TestResultItem(result);
    appendChild(intermediate);
    return intermediate;
}

QString TestResultItem::resultString() const
{
    if (testResult()->result() != ResultType::TestStart)
        return TestResult::resultToString(testResult()->result());
    if (!m_summaryResult)
        return QString();
    return m_summaryResult->failed ? QString("FAIL") : QString("PASS");
}

/********************************* TestResultModel *****************************************/

TestResultModel::TestResultModel(QObject *parent)
    : Utils::TreeModel<TestResultItem>(new TestResultItem(TestResultPtr()), parent)
{
    connect(TestRunner::instance(), &TestRunner::reportSummary,
            this, [this](const QString &id, const QHash<ResultType, int> &summary){
        m_reportedSummary.insert(id, summary);
    });
}

void TestResultModel::updateParent(const TestResultItem *item)
{
    QTC_ASSERT(item, return);
    QTC_ASSERT(item->testResult(), return);
    TestResultItem *parentItem = item->parent();
    if (parentItem == rootItem()) // do not update invisible root item
        return;
    bool changed = false;
    parentItem->updateResult(changed, item->testResult()->result(), item->summaryResult());
    if (!changed)
        return;
    emit dataChanged(parentItem->index(), parentItem->index());
    updateParent(parentItem);
}

void TestResultModel::addTestResult(const TestResultPtr &testResult, bool autoExpand)
{
    const int lastRow = rootItem()->childCount() - 1;
    if (testResult->result() == ResultType::MessageCurrentTest) {
        // MessageCurrentTest should always be the last top level item
        if (lastRow >= 0) {
            TestResultItem *current = rootItem()->childAt(lastRow);
            const TestResult *result = current->testResult();
            if (result && result->result() == ResultType::MessageCurrentTest) {
                current->updateDescription(testResult->description());
                emit dataChanged(current->index(), current->index());
                return;
            }
        }

        rootItem()->appendChild(new TestResultItem(testResult));
        return;
    }

    m_testResultCount[testResult->id()][testResult->result()]++;

    TestResultItem *newItem = new TestResultItem(testResult);
    TestResultItem *root = nullptr;
    if (AutotestPlugin::settings()->displayApplication) {
        const QString application = testResult->id();
        if (!application.isEmpty()) {
            root = rootItem()->findFirstLevelChild([&application](TestResultItem *child) {
                QTC_ASSERT(child, return false);
                return child->testResult()->id() == application;
            });

            if (!root) {
                TestResult *tmpAppResult = new TestResult(application, application);
                tmpAppResult->setResult(ResultType::Application);
                root = new TestResultItem(TestResultPtr(tmpAppResult));
                if (lastRow >= 0)
                    rootItem()->insertChild(lastRow, root);
                else
                    rootItem()->appendChild(root);
            }
        }
    }

    TestResultItem *parentItem = findParentItemFor(newItem, root);
    addFileName(testResult->fileName()); // ensure we calculate the results pane correctly
    if (parentItem) {
        parentItem->appendChild(newItem);
        if (autoExpand)
            parentItem->expand();
        updateParent(newItem);
    } else {
        if (lastRow >= 0) {
            TestResultItem *current = rootItem()->childAt(lastRow);
            const TestResult *result = current->testResult();
            if (result && result->result() == ResultType::MessageCurrentTest) {
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
    TestResultItem *currentMessageItem = rootItem()->findFirstLevelChild([](TestResultItem *it) {
            return (it->testResult()->result() == ResultType::MessageCurrentTest);
    });
    if (currentMessageItem)
        destroyItem(currentMessageItem);
}

void TestResultModel::clearTestResults()
{
    clear();
    m_testResultCount.clear();
    m_reportedSummary.clear();
    m_disabled = 0;
    m_fileNames.clear();
    m_maxWidthOfFileName = 0;
    m_widthOfLineNumber = 0;
}

const TestResult *TestResultModel::testResult(const QModelIndex &idx)
{
    if (idx.isValid())
        return itemForIndex(idx)->testResult();

    return nullptr;
}

void TestResultModel::recalculateMaxWidthOfFileName(const QFont &font)
{
    const QFontMetrics fm(font);
    m_maxWidthOfFileName = 0;
    for (const QString &fileName : m_fileNames) {
        int pos = fileName.lastIndexOf('/');
        m_maxWidthOfFileName = qMax(m_maxWidthOfFileName, fm.horizontalAdvance(fileName.mid(pos + 1)));
    }
}

void TestResultModel::addFileName(const QString &fileName)
{
    const QFontMetrics fm(m_measurementFont);
    int pos = fileName.lastIndexOf('/');
    m_maxWidthOfFileName = qMax(m_maxWidthOfFileName, fm.horizontalAdvance(fileName.mid(pos + 1)));
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
        m_widthOfLineNumber = fm.horizontalAdvance("88888");
    }
    return m_widthOfLineNumber;
}

int TestResultModel::resultTypeCount(ResultType type) const
{
    int result = 0;

    for (const auto &resultsForId : m_testResultCount.values())
        result += resultsForId.value(type, 0);

    for (const auto &id : m_reportedSummary.keys()) {
        if (int counted = m_testResultCount.value(id).value(type))
            result -= counted;
        result += m_reportedSummary[id].value(type);
    }
    return result;
}

TestResultItem *TestResultModel::findParentItemFor(const TestResultItem *item,
                                                   const TestResultItem *startItem) const
{
    QTC_ASSERT(item, return nullptr);
    TestResultItem *root = startItem ? const_cast<TestResultItem *>(startItem) : nullptr;
    const TestResult *result = item->testResult();
    const QString &name = result->name();
    const QString &id = result->id();

    if (root == nullptr && !name.isEmpty()) {
        for (int row = rootItem()->childCount() - 1; row >= 0; --row) {
            TestResultItem *tmp = rootItem()->childAt(row);
            auto tmpTestResult = tmp->testResult();
            if (tmpTestResult->id() == id && tmpTestResult->name() == name) {
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
    TestResultItem *parent = root->reverseFindAnyChild(predicate);
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
    enableAllResultTypes(true);
}

void TestResultFilterModel::enableAllResultTypes(bool enabled)
{
    if (enabled) {
        m_enabled << ResultType::Pass << ResultType::Fail << ResultType::ExpectedFail
                  << ResultType::UnexpectedPass << ResultType::Skip << ResultType::MessageDebug
                  << ResultType::MessageWarn << ResultType::MessageInternal << ResultType::MessageLocation
                  << ResultType::MessageFatal << ResultType::Invalid << ResultType::BlacklistedPass
                  << ResultType::BlacklistedFail << ResultType::BlacklistedXFail << ResultType::BlacklistedXPass
                  << ResultType::Benchmark
                  << ResultType::MessageCurrentTest << ResultType::TestStart << ResultType::TestEnd
                  << ResultType::MessageInfo << ResultType::MessageSystem << ResultType::Application
                  << ResultType::MessageError;
    } else {
        m_enabled.clear();
        m_enabled << ResultType::MessageFatal << ResultType::MessageSystem
                  << ResultType::MessageError;
    }
    invalidateFilter();
}

void TestResultFilterModel::toggleTestResultType(ResultType type)
{
    if (m_enabled.contains(type)) {
        m_enabled.remove(type);
        if (type == ResultType::MessageInternal)
            m_enabled.remove(ResultType::TestEnd);
        if (type == ResultType::MessageDebug)
            m_enabled.remove(ResultType::MessageInfo);
        if (type == ResultType::MessageWarn)
            m_enabled.remove(ResultType::MessageSystem);
    } else {
        m_enabled.insert(type);
        if (type == ResultType::MessageInternal)
            m_enabled.insert(ResultType::TestEnd);
        if (type == ResultType::MessageDebug)
            m_enabled.insert(ResultType::MessageInfo);
        if (type == ResultType::MessageWarn)
            m_enabled.insert(ResultType::MessageSystem);
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

TestResultItem *TestResultFilterModel::itemForIndex(const QModelIndex &index) const
{
    return index.isValid() ? m_sourceModel->itemForIndex(mapToSource(index)) : nullptr;
}

bool TestResultFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = m_sourceModel->index(sourceRow, 0, sourceParent);
    if (!index.isValid())
        return false;
    ResultType resultType = m_sourceModel->testResult(index)->result();
    if (resultType == ResultType::TestStart) {
        TestResultItem *item = m_sourceModel->itemForIndex(index);
        QTC_ASSERT(item, return false);
        if (!item->summaryResult())
            return true;
        return acceptTestCaseResult(index);
    }
    return m_enabled.contains(resultType);
}

bool TestResultFilterModel::acceptTestCaseResult(const QModelIndex &srcIndex) const
{
    for (int row = 0, count = m_sourceModel->rowCount(srcIndex); row < count; ++row) {
        const QModelIndex &child = m_sourceModel->index(row, 0, srcIndex);
        TestResultItem *item = m_sourceModel->itemForIndex(child);
        ResultType type = item->testResult()->result();

        if (type == ResultType::TestStart) {
            if (!item->summaryResult())
                return true;
            if (acceptTestCaseResult(child))
                return true;
        } else if (m_enabled.contains(type))
            return true;
    }
    return false;
}

} // namespace Internal
} // namespace Autotest
