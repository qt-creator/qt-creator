/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
    void setShortenTemplates(bool enabled);

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
