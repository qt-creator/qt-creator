/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef VALGRIND_CALLGRIND_CALLGRINDPROXYMODEL_H
#define VALGRIND_CALLGRIND_CALLGRINDPROXYMODEL_H

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

#endif // VALGRIND_CALLGRIND_CALLGRINDPROXYMODEL_H
