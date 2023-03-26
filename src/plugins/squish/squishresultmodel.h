// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "testresult.h"

#include <utils/treemodel.h>

#include <QSet>
#include <QSortFilterProxyModel>

namespace Squish {
namespace Internal {

class SquishResultItem : public Utils::TreeItem
{
public:
    SquishResultItem(const TestResult &result);
    QVariant data(int column, int role) const override;
    TestResult result() const { return m_testResult; }

private:
    TestResult m_testResult;
};

class SquishResultModel : public Utils::TreeModel<>
{
    Q_OBJECT

public:
    SquishResultModel(QObject *parent = nullptr);
    bool hasResults() const { return m_rootItem ? m_rootItem->hasChildren() : false; }
    int resultTypeCount(Result::Type type);
    void clearResults();
    void expandVisibleRootItems();
    void updateResultTypeCount(const QModelIndex &parent, int first, int last);

    void addResultItem(SquishResultItem *item);

signals:
    void resultTypeCountUpdated();

private:
    Utils::TreeItem *m_rootItem;
    QHash<Result::Type, int> m_resultsCounter;
};

class SquishResultFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    SquishResultFilterModel(SquishResultModel *sourceModel, QObject *parent = nullptr);

    void enableAllResultTypes();
    void toggleResultType(Result::Type type);
    void clearResults();
    bool hasResults();
    TestResult testResult(const QModelIndex &idx) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    SquishResultModel *m_sourceModel;
    QSet<Result::Type> m_enabled;
};

} // namespace Internal
} // namespace Squish
