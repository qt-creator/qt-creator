/**************************************************************************
**
** This file is part of Qt Creator Analyzer Tools
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef VALGRIND_CALLGRIND_CALLGRINDPROXYMODEL_H
#define VALGRIND_CALLGRIND_CALLGRINDPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "../valgrind_global.h"

namespace Valgrind {
namespace Callgrind {

class DataModel;
class Function;

class VALGRINDSHARED_EXPORT DataProxyModel : public QSortFilterProxyModel
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

}
}

#endif // VALGRIND_CALLGRIND_CALLGRINDPROXYMODEL_H
