/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#include "testresultmodel.h"

#include <QDebug>
#include <QFontMetrics>
#include <QIcon>
#include <QSortFilterProxyModel>

namespace Autotest {
namespace Internal {

TestResultModel::TestResultModel(QObject *parent) :
    QAbstractItemModel(parent),
    m_widthOfLineNumber(0),
    m_maxWidthOfFileName(0),
    m_lastMaxWidthIndex(0)
{
}

TestResultModel::~TestResultModel()
{
    QWriteLocker lock(&m_rwLock);
    m_testResults.clear();
}

QModelIndex TestResultModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex TestResultModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

int TestResultModel::rowCount(const QModelIndex &parent) const
{
    // do not use the QReadLocker here or this will produce a deadlock
    return parent.isValid() ? 0 : m_testResults.size();
}

int TestResultModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

static QIcon testResultIcon(ResultType result) {
    static QIcon icons[11] = {
        QIcon(QLatin1String(":/images/pass.png")),
        QIcon(QLatin1String(":/images/fail.png")),
        QIcon(QLatin1String(":/images/xfail.png")),
        QIcon(QLatin1String(":/images/xpass.png")),
        QIcon(QLatin1String(":/images/skip.png")),
        QIcon(QLatin1String(":/images/blacklisted_pass.png")),
        QIcon(QLatin1String(":/images/blacklisted_fail.png")),
        QIcon(QLatin1String(":/images/benchmark.png")),
        QIcon(QLatin1String(":/images/debug.png")),
        QIcon(QLatin1String(":/images/warn.png")),
        QIcon(QLatin1String(":/images/fatal.png")),
    }; // provide an icon for unknown??

    if (result < 0 || result >= MESSAGE_INTERNAL)
        return QIcon();
    return icons[result];
}

QVariant TestResultModel::data(const QModelIndex &index, int role) const
{
    QReadLocker lock(&m_rwLock);
    if (!index.isValid() || index.row() >= m_testResults.count() || index.column() != 0)
        return QVariant();
    if (role == Qt::DisplayRole) {
        const TestResult &tr = m_testResults.at(index.row());
        switch (tr.result()) {
        case ResultType::PASS:
        case ResultType::FAIL:
        case ResultType::EXPECTED_FAIL:
        case ResultType::UNEXPECTED_PASS:
        case ResultType::SKIP:
        case ResultType::BLACKLISTED_PASS:
        case ResultType::BLACKLISTED_FAIL:
        case ResultType::BENCHMARK:
            return QString::fromLatin1("%1::%2 (%3) - %4").arg(tr.className(), tr.testCase(),
                                                               tr.dataTag(), tr.fileName());
        default:
            return tr.description();
        }
    }
    if (role == Qt::DecorationRole) {
        const TestResult &tr = m_testResults.at(index.row());
        return testResultIcon(tr.result());
    }

    return QVariant();
}

void TestResultModel::addTestResult(const TestResult &testResult)
{
    const bool isCurrentTestMssg = testResult.result() == ResultType::MESSAGE_CURRENT_TEST;
    const bool hasCurrentTestMssg = m_availableResultTypes.contains(ResultType::MESSAGE_CURRENT_TEST);

    QReadLocker rLock(&m_rwLock);
    int position = m_testResults.size();
    rLock.unlock();

    QWriteLocker wLock(&m_rwLock);
    if (hasCurrentTestMssg && isCurrentTestMssg) {
        beginRemoveRows(QModelIndex(), position, position);
        m_testResults.replace(position - 1, testResult);
        endRemoveRows();
    } else {
        if (!isCurrentTestMssg && position) // decrement only if at least one other item
            --position;
        beginInsertRows(QModelIndex(), position, position);
        m_testResults.insert(position, testResult);
        endInsertRows();
    }
    wLock.unlock();

    if (!isCurrentTestMssg) {
        int count = m_testResultCount.value(testResult.result(), 0);
        m_testResultCount.insert(testResult.result(), ++count);
    }

    m_availableResultTypes.insert(testResult.result());
}

void TestResultModel::removeCurrentTestMessage()
{
    QReadLocker rLock(&m_rwLock);
    if (m_availableResultTypes.contains(ResultType::MESSAGE_CURRENT_TEST)) {
        rLock.unlock();
        QWriteLocker wLock(&m_rwLock);
        beginRemoveRows(QModelIndex(), m_testResults.size() - 1, m_testResults.size() - 1);
        m_testResults.removeLast();
        endRemoveRows();
        m_availableResultTypes.remove(ResultType::MESSAGE_CURRENT_TEST);
    }
}

void TestResultModel::clearTestResults()
{
    QReadLocker rLock(&m_rwLock);
    if (m_testResults.size() == 0)
        return;
    beginRemoveRows(QModelIndex(), 0, m_testResults.size() - 1);
    rLock.unlock();
    QWriteLocker wLock(&m_rwLock);
    m_testResults.clear();
    wLock.unlock();
    m_testResultCount.clear();
    m_lastMaxWidthIndex = 0;
    m_maxWidthOfFileName = 0;
    m_widthOfLineNumber = 0;
    endRemoveRows();
    m_availableResultTypes.clear();
}

TestResult TestResultModel::testResult(const QModelIndex &index) const
{
    if (!index.isValid())
        return TestResult(QString(), QString());
    QReadLocker lock(&m_rwLock);
    return m_testResults.at(index.row());
}

int TestResultModel::maxWidthOfFileName(const QFont &font)
{
    QReadLocker lock(&m_rwLock);
    int count = m_testResults.size();
    lock.unlock();
    if (count == 0)
        return 0;
    if (m_maxWidthOfFileName > 0 && font == m_measurementFont && m_lastMaxWidthIndex == count - 1)
        return m_maxWidthOfFileName;

    QFontMetrics fm(font);
    m_measurementFont = font;

    for (int i = m_lastMaxWidthIndex; i < count; ++i) {
        lock.relock();
        QString filename = m_testResults.at(i).fileName();
        lock.unlock();
        const int pos = filename.lastIndexOf(QLatin1Char('/'));
        if (pos != -1)
            filename = filename.mid(pos +1);

        m_maxWidthOfFileName = qMax(m_maxWidthOfFileName, fm.width(filename));
    }
    m_lastMaxWidthIndex = count - 1;
    return m_maxWidthOfFileName;
}

int TestResultModel::maxWidthOfLineNumber(const QFont &font)
{
    if (m_widthOfLineNumber == 0 || font != m_measurementFont) {
        QFontMetrics fm(font);
        m_measurementFont = font;
        m_widthOfLineNumber = fm.width(QLatin1String("88888"));
    }
    return m_widthOfLineNumber;
}

int TestResultModel::resultTypeCount(ResultType type)
{
    return m_testResultCount.value(type, 0);
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
    m_enabled << ResultType::PASS << ResultType::FAIL << ResultType::EXPECTED_FAIL
              << ResultType::UNEXPECTED_PASS << ResultType::SKIP << ResultType::MESSAGE_DEBUG
              << ResultType::MESSAGE_WARN << ResultType::MESSAGE_INTERNAL
              << ResultType::MESSAGE_FATAL << ResultType::UNKNOWN << ResultType::BLACKLISTED_PASS
              << ResultType::BLACKLISTED_FAIL << ResultType::BENCHMARK
              << ResultType::MESSAGE_CURRENT_TEST;
    invalidateFilter();
}

void TestResultFilterModel::toggleTestResultType(ResultType type)
{
    if (m_enabled.contains(type)) {
        m_enabled.remove(type);
    } else {
        m_enabled.insert(type);
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

bool TestResultFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = m_sourceModel->index(sourceRow, 0, sourceParent);
    if (!index.isValid())
        return false;
    return m_enabled.contains(m_sourceModel->testResult(index).result());
}

} // namespace Internal
} // namespace Autotest
