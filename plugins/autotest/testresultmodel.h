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

#ifndef TESTRESULTMODEL_H
#define TESTRESULTMODEL_H

#include "testresult.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QFont>
#include <QReadWriteLock>
#include <QSet>

namespace Autotest {
namespace Internal {

class TestResultModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit TestResultModel(QObject *parent = 0);
    ~TestResultModel();
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    void addTestResult(const TestResult &testResult);
    void removeCurrentTestMessage();
    void clearTestResults();

    bool hasResults() const { return m_testResults.size() > 0; }
    TestResult testResult(const QModelIndex &index) const;

    int maxWidthOfFileName(const QFont &font);
    int maxWidthOfLineNumber(const QFont &font);

    bool hasResultType(ResultType type) { return m_availableResultTypes.contains(type); }
    int resultTypeCount(ResultType type);

signals:

public slots:

private:
    QList<TestResult> m_testResults;
    QMap<ResultType, int> m_testResultCount;
    int m_widthOfLineNumber;
    int m_maxWidthOfFileName;
    int m_lastMaxWidthIndex;
    QFont m_measurementFont;
    QSet<ResultType> m_availableResultTypes;
    mutable QReadWriteLock m_rwLock;
};

class TestResultFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    TestResultFilterModel(TestResultModel *sourceModel, QObject *parent = 0);

    void enableAllResultTypes();
    void toggleTestResultType(ResultType type);
    void clearTestResults();
    bool hasResults();
    TestResult testResult(const QModelIndex &index) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    TestResultModel *m_sourceModel;
    QSet<ResultType> m_enabled;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTRESULTMODEL_H
