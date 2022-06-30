/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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
