// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishresultmodel.h"

#include "squishtr.h"

namespace Squish {
namespace Internal {

enum Role { Type = Qt::UserRole };

/************************** SquishResultItem ********************************/

SquishResultItem::SquishResultItem(const TestResult &result)
    : m_testResult(result)
{}

QVariant SquishResultItem::data(int column, int role) const
{
    switch (role) {
    case Qt::ToolTipRole:
        return m_testResult.text();
    case Qt::DisplayRole:
        switch (column) {
        case 0:
            return TestResult::typeToString(m_testResult.type());
        case 1:
            return m_testResult.text();
        case 2:
            return m_testResult.timeStamp();
        }
        break;
    case Qt::ForegroundRole:
        if (column == 0)
            return TestResult::colorForType(m_testResult.type());
        break;
    case Type:
        return m_testResult.type();
    }
    return QVariant();
}

/************************** SquishResultModel *******************************/

SquishResultModel::SquishResultModel(QObject *parent)
    : Utils::TreeModel<>(parent)
    , m_rootItem(new Utils::TreeItem)
{
    setRootItem(m_rootItem);
    setHeader(QStringList({Tr::tr("Result"), Tr::tr("Message"), Tr::tr("Time")}));

    connect(this,
            &QAbstractItemModel::rowsInserted,
            this,
            &SquishResultModel::updateResultTypeCount);
}

int SquishResultModel::resultTypeCount(Result::Type type)
{
    return m_resultsCounter.value(type, 0);
}

void SquishResultModel::clearResults()
{
    clear();
    m_resultsCounter.clear();
    emit resultTypeCountUpdated();
}

void SquishResultModel::expandVisibleRootItems()
{
    m_rootItem->forChildrenAtLevel(1, [](Utils::TreeItem *item) { item->expand(); });
}

void SquishResultModel::updateResultTypeCount(const QModelIndex &parent, int first, int last)
{
    bool countUpdated = false;
    for (int i = first; i <= last; ++i) {
        SquishResultItem *resultItem = static_cast<SquishResultItem *>(
            parent.isValid() ? itemForIndex(parent)->childAt(i) : m_rootItem->childAt(i));
        QHash<Result::Type, int> results;
        ++results[resultItem->result().type()];

        resultItem->forAllChildren([&results](Utils::TreeItem *it) {
            SquishResultItem *item = static_cast<SquishResultItem *>(it);

            Result::Type type = item->result().type();
            ++results[type];
        });

        auto cend = results.constEnd();
        for (auto pair = results.constBegin(); pair != cend; ++pair) {
            Result::Type type = pair.key();
            switch (type) {
            case Result::Pass:
            case Result::Fail:
            case Result::ExpectedFail:
            case Result::UnexpectedPass:
            case Result::Warn:
            case Result::Error:
            case Result::Fatal:
                if (int value = pair.value()) {
                    m_resultsCounter.insert(type, m_resultsCounter.value(type, 0) + value);
                    countUpdated = true;
                }
                break;
            default:
                break;
            }
        }
    }
    if (countUpdated)
        emit resultTypeCountUpdated();
}

void SquishResultModel::addResultItem(SquishResultItem *item)
{
    m_rootItem->appendChild(item);
}

/*********************** SquishResultFilerModel *****************************/

SquishResultFilterModel::SquishResultFilterModel(SquishResultModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_sourceModel(sourceModel)
{
    setSourceModel(sourceModel);
    enableAllResultTypes();
}

void SquishResultFilterModel::enableAllResultTypes()
{
    m_enabled << Result::Log << Result::Pass << Result::Fail << Result::ExpectedFail
              << Result::UnexpectedPass << Result::Warn << Result::Error << Result::Fatal
              << Result::Start << Result::End << Result::Detail;
    invalidateFilter();
}

void SquishResultFilterModel::toggleResultType(Result::Type type)
{
    if (!m_enabled.remove(type))
        m_enabled.insert(type);
    invalidateFilter();
}

void SquishResultFilterModel::clearResults()
{
    m_sourceModel->clearResults();
}

bool SquishResultFilterModel::hasResults()
{
    return m_sourceModel->hasResults();
}

TestResult SquishResultFilterModel::testResult(const QModelIndex &idx) const
{
    if (auto item = static_cast<SquishResultItem *>(m_sourceModel->itemForIndex(mapToSource(idx))))
        return item->result();
    return {};
}

bool SquishResultFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex idx = m_sourceModel->index(sourceRow, 0, sourceParent);
    if (!idx.isValid())
        return false;

    return m_enabled.contains(Result::Type(idx.data(Type).toInt()));
}

} // namespace Internal
} // namespace Squish
