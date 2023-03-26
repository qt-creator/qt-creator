// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSortFilterProxyModel>

namespace Utils {

class SortFilterModel : public QSortFilterProxyModel
{
public:
    using FilterColumnFunction = std::function<bool(int, const QModelIndex &source_parent)>;
    using FilterRowFunction = std::function<bool(int, const QModelIndex &source_parent)>;
    using LessThanFunction = std::function<bool(const QModelIndex &left, const QModelIndex &right)>;

    using QSortFilterProxyModel::QSortFilterProxyModel;

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        if (!m_filterRowFunction)
            return true;

        return m_filterRowFunction(source_row, source_parent);
    }

    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override
    {
        if (!m_filterColumnFunction)
            return true;

        return m_filterColumnFunction(source_column, source_parent);
    }

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override
    {
        if (!m_lessThanFunction)
            return QSortFilterProxyModel::lessThan(left, right);
        return m_lessThanFunction(left, right);
    }

    void setFilterRowFunction(FilterRowFunction filterRowFunction)
    {
        m_filterRowFunction = filterRowFunction;
        invalidate();
    }

    void setFilterColumnFunction(FilterColumnFunction filterColumnFunction)
    {
        m_filterColumnFunction = filterColumnFunction;
        invalidate();
    }

    void setLessThanFunction(LessThanFunction lessThanFunction)
    {
        m_lessThanFunction = lessThanFunction;
        invalidate();
    }

private:
    FilterRowFunction m_filterRowFunction;
    FilterColumnFunction m_filterColumnFunction;
    LessThanFunction m_lessThanFunction;
};
} // namespace Utils
