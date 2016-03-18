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

#pragma once

#include <QSortFilterProxyModel>

namespace Valgrind {
namespace Callgrind {

class DataModel;
class Function;

class DataProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit DataProxyModel(QObject *parent = 0);

    virtual void setSourceModel(QAbstractItemModel *sourceModel);

    QString filterBaseDir() const { return m_baseDir; }
    const Function *filterFunction() const;
    int filterMaximumRows() const { return m_maxRows; }

    /// Only functions with an inclusive cost ratio above this minimum will be shown in the model
    double minimumInclusiveCostRatio() const { return m_minimumInclusiveCostRatio; }

public Q_SLOTS:
    /// This will filter out all entries that are not located within \param baseDir
    void setFilterBaseDir(const QString& baseDir);
    void setFilterFunction(const Function *call);
    void setFilterMaximumRows(int rows);

    /// Only rows with a inclusive cost ratio above @p minimumInclusiveCost will be shown
    /// by this model. If @c 0 is passed as argument, all rows will be shown.
    void setMinimumInclusiveCostRatio(double minimumInclusiveCost);

Q_SIGNALS:
    void filterFunctionChanged(const Function *previous, const Function *current);
    void filterMaximumRowsChanged(int rows);

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    DataModel *dataModel() const;

    QString m_baseDir;
    const Function *m_function;
    int m_maxRows;
    double m_minimumInclusiveCostRatio;
};

} // namespace Callgrind
} // namespace Valgrind
