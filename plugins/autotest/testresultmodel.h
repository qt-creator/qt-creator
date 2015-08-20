/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef TESTRESULTMODEL_H
#define TESTRESULTMODEL_H

#include "testresult.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QFont>
#include <QSet>

#include <utils/treemodel.h>

namespace Autotest {
namespace Internal {

class TestResultItem : public Utils::TreeItem
{
public:
    explicit TestResultItem(TestResult *testResult);
    ~TestResultItem();
    QVariant data(int column, int role) const;
    const TestResult *testResult() const { return m_testResult; }
    void updateDescription(const QString &description);
    void updateResult();

private:
    TestResult *m_testResult;
};

class TestResultModel : public Utils::TreeModel
{
public:
    explicit TestResultModel(QObject *parent = 0);
    QVariant data(const QModelIndex &idx, int role) const;

    void addTestResult(TestResult *testResult, bool autoExpand = false);
    void removeCurrentTestMessage();
    void clearTestResults();

    TestResult testResult(const QModelIndex &idx);

    int maxWidthOfFileName(const QFont &font);
    int maxWidthOfLineNumber(const QFont &font);

    int resultTypeCount(Result::Type type) const { return m_testResultCount.value(type, 0); }

private:
    QMap<Result::Type, int> m_testResultCount;
    int m_widthOfLineNumber;
    int m_maxWidthOfFileName;
    QList<int> m_processedIndices;
    QFont m_measurementFont;
};

class TestResultFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    TestResultFilterModel(TestResultModel *sourceModel, QObject *parent = 0);

    void enableAllResultTypes();
    void toggleTestResultType(Result::Type type);
    void clearTestResults();
    bool hasResults();
    TestResult testResult(const QModelIndex &index) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    TestResultModel *m_sourceModel;
    QSet<Result::Type> m_enabled;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTRESULTMODEL_H
