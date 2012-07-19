/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef VALGRIND_CALLGRIND_CALLGRINDDATAMODEL_H
#define VALGRIND_CALLGRIND_CALLGRINDDATAMODEL_H

#include "callgrindabstractmodel.h"

#include <QAbstractItemModel>

namespace Valgrind {
namespace Callgrind {

class Function;
class ParseData;

class DataModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit DataModel(QObject *parent);
    virtual ~DataModel();

    virtual void setParseData(const ParseData *data);
    virtual const ParseData *parseData() const;

    void setVerboseToolTipsEnabled(bool enabled);
    bool verboseToolTipsEnabled() const;

    /// Only one cost event column will be shown, this decides which one it is.
    /// By default it is the first event in the @c ParseData, i.e. 0.
    virtual int costEvent() const;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;

    QModelIndex indexForObject(const Function *function) const;

    enum Columns {
        NameColumn,
        LocationColumn,
        CalledColumn,
        SelfCostColumn,
        InclusiveCostColumn,
        ColumnCount
    };

    enum Roles {
        FunctionRole = NextCustomRole,
        LineNumberRole,
        FileNameRole
    };

public slots:
    /// enable/disable cycle detection
    void enableCycleDetection(bool enabled);

    /// Only one cost event column will be shown, this decides which one it is.
    /// By default it is the first event in the @c ParseData, i.e. 0.
    virtual void setCostEvent(int event);

private:
    class Private;
    Private *d;
};

} // namespace Callgrind
} // namespace Valgrind

#endif // VALGRIND_CALLGRIND_CALLGRINDDATAMODEL_H
