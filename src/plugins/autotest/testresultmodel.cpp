// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testresultmodel.h"

#include "autotesticons.h"
#include "testresultspane.h"
#include "testrunner.h"
#include "testsettings.h"
#include "testtreeitem.h"
#include "testtreemodel.h"

#include <projectexplorer/projectexplorericons.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QFontMetrics>
#include <QIcon>
#include <QToolButton>

using namespace Utils;

namespace Autotest {
namespace Internal {

/********************************* TestResultItem ******************************************/

TestResultItem::TestResultItem(const TestResult &testResult)
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

static QIcon testSummaryIcon(const std::optional<TestResultItem::SummaryEvaluation> &summary)
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
        if (!m_testResult.isValid())
            return {};
        const ResultType result = m_testResult.result();
        if (result == ResultType::MessageLocation && parent())
            return parent()->data(column, role);
        if (result == ResultType::TestStart)
            return testSummaryIcon(m_summaryResult);
        return testResultIcon(result);
    }
    case Qt::DisplayRole:
        return m_testResult.isValid() ? m_testResult.outputString(true) : QVariant();
    default:
        return TreeItem::data(column, role);
    }
}

void TestResultItem::updateDescription(const QString &description)
{
    QTC_ASSERT(m_testResult.isValid(), return);
    m_testResult.setDescription(description);
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
        QTC_ASSERT_STRING("Got unexpected type in isSignificant check");
        return false;
    default:
        return true;
    }
}

void TestResultItem::updateResult(bool &changed, ResultType addedChildType,
                                  const std::optional<SummaryEvaluation> &summary)
{
    changed = false;
    if (m_testResult.result() != ResultType::TestStart)
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
    case ResultType::MessageError:
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
    const TestResult otherResult = item->testResult();
    for (int row = childCount() - 1; row >= 0; --row) {
        TestResultItem *child = childAt(row);
        const TestResult testResult = child->testResult();
        if (testResult.result() != ResultType::TestStart)
            continue;
        if (testResult.isIntermediateFor(otherResult))
            return child;
    }
    return nullptr;
}

TestResultItem *TestResultItem::createAndAddIntermediateFor(const TestResultItem *child)
{
    TestResult result = child->testResult().intermediateResult();
    QTC_ASSERT(result.isValid(), return nullptr);
    result.setResult(ResultType::TestStart);
    TestResultItem *intermediate = new TestResultItem(result);
    appendChild(intermediate);
    // FIXME: make the expand button's state easier accessible
    auto widgets = TestResultsPane::instance()->toolBarWidgets();
    if (!widgets.empty()) {
        if (QToolButton *expand = qobject_cast<QToolButton *>(widgets.at(0))) {
            if (expand->isChecked()) {
                QMetaObject::invokeMethod(TestResultsPane::instance(),
                                          [intermediate] { intermediate->expand(); },
                                          Qt::QueuedConnection);
            }
        }
    }
    return intermediate;
}

QString TestResultItem::resultString() const
{
    if (testResult().result() != ResultType::TestStart)
        return TestResult::resultToString(testResult().result());
    if (!m_summaryResult)
        return {};
    return m_summaryResult->failed ? QString("FAIL") : QString("PASS");
}

//! \return true if descendant types have changed, false otherwise
bool TestResultItem::updateDescendantTypes(ResultType t)
{
    if (t == ResultType::TestStart || t == ResultType::TestEnd) // these are special
        return false;

    return Utils::insert(m_descendantsTypes, t);
}

bool TestResultItem::descendantTypesContainsAnyOf(const QSet<ResultType> &types) const
{
    return !m_descendantsTypes.isEmpty() && m_descendantsTypes.intersects(types);
}

/********************************* TestResultModel *****************************************/

TestResultModel::TestResultModel(QObject *parent)
    : TreeModel<TestResultItem>(new TestResultItem({}), parent)
{
    connect(TestRunner::instance(), &TestRunner::reportSummary,
            this, [this](const QString &id, const QHash<ResultType, int> &summary){
        m_reportedSummary.insert(id, summary);
    });
}

void TestResultModel::updateParent(const TestResultItem *item)
{
    QTC_ASSERT(item, return);
    QTC_ASSERT(item->testResult().isValid(), return);
    TestResultItem *parentItem = item->parent();
    if (parentItem == rootItem()) // do not update invisible root item
        return;
    bool changed = false;
    parentItem->updateResult(changed, item->testResult().result(), item->summaryResult());
    bool changedType = parentItem->updateDescendantTypes(item->testResult().result());
    if (!changed && !changedType)
        return;
    emit dataChanged(parentItem->index(), parentItem->index());
    updateParent(parentItem);
}

static bool isFailed(ResultType type)
{
    switch (type) {
    case ResultType::Fail: case ResultType::UnexpectedPass: case ResultType::MessageFatal:
        return true;
    default:
        return false;
    }
}

void TestResultModel::addTestResult(const TestResult &testResult, bool autoExpand)
{
    const int lastRow = rootItem()->childCount() - 1;
    if (testResult.result() == ResultType::MessageCurrentTest) {
        // MessageCurrentTest should always be the last top level item
        if (lastRow >= 0) {
            TestResultItem *current = rootItem()->childAt(lastRow);
            const TestResult result = current->testResult();
            if (result.isValid() && result.result() == ResultType::MessageCurrentTest) {
                current->updateDescription(testResult.description());
                emit dataChanged(current->index(), current->index());
                return;
            }
        }

        rootItem()->appendChild(new TestResultItem(testResult));
        return;
    }

    m_testResultCount[testResult.id()][testResult.result()]++;

    TestResultItem *newItem = new TestResultItem(testResult);
    TestResultItem *root = nullptr;
    if (testSettings().displayApplication()) {
        const QString application = testResult.id();
        if (!application.isEmpty()) {
            root = rootItem()->findFirstLevelChild([&application](TestResultItem *child) {
                QTC_ASSERT(child, return false);
                return child->testResult().id() == application;
            });

            if (!root) {
                TestResult tmpAppResult(application, application);
                tmpAppResult.setResult(ResultType::Application);
                root = new TestResultItem(tmpAppResult);
                if (lastRow >= 0)
                    rootItem()->insertChild(lastRow, root);
                else
                    rootItem()->appendChild(root);
            }
        }
    }

    TestResultItem *parentItem = findParentItemFor(newItem, root);
    addFileName(testResult.fileName().fileName()); // ensure we calculate the results pane correctly
    if (parentItem) {
        parentItem->appendChild(newItem);
        if (autoExpand) {
            QMetaObject::invokeMethod(this, [parentItem]{ parentItem->expand(); },
                                      Qt::QueuedConnection);
        }
        updateParent(newItem);
    } else {
        if (lastRow >= 0) {
            TestResultItem *current = rootItem()->childAt(lastRow);
            const TestResult result = current->testResult();
            if (result.isValid() && result.result() == ResultType::MessageCurrentTest) {
                rootItem()->insertChild(current->index().row(), newItem);
                return;
            }
        }
        // there is no MessageCurrentTest at the last row, but we have a toplevel item - just add it
        rootItem()->appendChild(newItem);
    }

    if (isFailed(testResult.result())) {
        if (const ITestTreeItem *it = testResult.findTestTreeItem()) {
            TestTreeModel *model = TestTreeModel::instance();
            model->setData(model->indexForItem(it), true, FailedRole);
        }
    }
}

void TestResultModel::removeCurrentTestMessage()
{
    TestResultItem *currentMessageItem = rootItem()->findFirstLevelChild([](TestResultItem *it) {
            return (it->testResult().result() == ResultType::MessageCurrentTest);
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

TestResult TestResultModel::testResult(const QModelIndex &idx)
{
    if (idx.isValid())
        return itemForIndex(idx)->testResult();
    return {};
}

void TestResultModel::recalculateMaxWidthOfFileName(const QFont &font)
{
    const QFontMetrics fm(font);
    m_maxWidthOfFileName = 0;
    for (const QString &fileName : std::as_const(m_fileNames)) {
        m_maxWidthOfFileName = qMax(m_maxWidthOfFileName, fm.horizontalAdvance(fileName));
    }
}

void TestResultModel::addFileName(const QString &fileName)
{
    const QFontMetrics fm(m_measurementFont);
    m_maxWidthOfFileName = qMax(m_maxWidthOfFileName, fm.horizontalAdvance(fileName));
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
    for (auto it = m_testResultCount.cbegin(); it != m_testResultCount.cend(); ++it) {
        // if we got a result count from the framework prefer that over our counted results
        int reported = m_reportedSummary[it.key()].value(type);
        result += reported != 0 ? reported : it.value().value(type);
    }
    return result;
}

TestResultItem *TestResultModel::findParentItemFor(const TestResultItem *item,
                                                   const TestResultItem *startItem) const
{
    QTC_ASSERT(item, return nullptr);
    TestResultItem *root = startItem ? const_cast<TestResultItem *>(startItem) : nullptr;
    const TestResult result = item->testResult();
    const QString &name = result.name();
    const QString &id = result.id();

    if (root == nullptr && !name.isEmpty()) {
        for (int row = rootItem()->childCount() - 1; row >= 0; --row) {
            TestResultItem *tmp = rootItem()->childAt(row);
            const TestResult tmpTestResult = tmp->testResult();
            if (tmpTestResult.id() == id && tmpTestResult.name() == name) {
                root = tmp;
                break;
            }
        }
    }
    if (root == nullptr)
        return root;

    bool needsIntermediate = false;
    auto predicate = [result, &needsIntermediate](TreeItem *it) {
        TestResultItem *currentItem = static_cast<TestResultItem *>(it);
        return currentItem->testResult().isDirectParentOf(result, &needsIntermediate);
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
                  << ResultType::MessageCurrentTest
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
    if (m_enabled.remove(type)) {
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

TestResult TestResultFilterModel::testResult(const QModelIndex &index) const
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

    const ResultType resultType = m_sourceModel->testResult(index).result();
    if (resultType == ResultType::TestStart) {
        auto item = m_sourceModel->itemForIndex(index);
        return item && item->descendantTypesContainsAnyOf(m_enabled);
    } else if (resultType == ResultType::TestEnd) {
        auto item = m_sourceModel->itemForIndex(index);
        if (!item)
            return false;
        auto parent = item->parent();
        return parent && parent->descendantTypesContainsAnyOf(m_enabled);
    }

    return m_enabled.contains(resultType);
}

} // namespace Internal
} // namespace Autotest
