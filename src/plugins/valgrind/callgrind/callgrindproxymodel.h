// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSortFilterProxyModel>

namespace Valgrind::Callgrind {

class DataModel;
class Function;

class DataProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit DataProxyModel(QObject *parent = nullptr);

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    QString filterBaseDir() const { return m_baseDir; }
    const Function *filterFunction() const;
    int filterMaximumRows() const { return m_maxRows; }

    /// Only functions with an inclusive cost ratio above this minimum will be shown in the model
    double minimumInclusiveCostRatio() const { return m_minimumInclusiveCostRatio; }

    /// This will filter out all entries that are not located within \param baseDir
    void setFilterBaseDir(const QString& baseDir);
    void setFilterFunction(const Function *call);
    void setFilterMaximumRows(int rows);

    /// Only rows with a inclusive cost ratio above @p minimumInclusiveCost will be shown
    /// by this model. If @c 0 is passed as argument, all rows will be shown.
    void setMinimumInclusiveCostRatio(double minimumInclusiveCost);

signals:
    void filterFunctionChanged(const Function *previous, const Function *current);
    void filterMaximumRowsChanged(int rows);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    DataModel *dataModel() const;

    QString m_baseDir;
    const Function *m_function = nullptr;
    int m_maxRows = 0;
    double m_minimumInclusiveCostRatio = 0;
};

} // namespace Valgrind::Callgrind
