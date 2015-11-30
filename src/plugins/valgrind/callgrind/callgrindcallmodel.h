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

#ifndef VALGRIND_CALLGRIND_CALLGRINDCALLMODEL_H
#define VALGRIND_CALLGRIND_CALLGRINDCALLMODEL_H

#include <QAbstractItemModel>

#include "callgrindabstractmodel.h"

namespace Valgrind {
namespace Callgrind {

class FunctionCall;
class Function;

/**
 * Model to display list of function calls.
 */
class CallModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit CallModel(QObject *parent);
    virtual ~CallModel();

    void clear();

    /// Only one cost event column will be shown, this decides which one it is.
    /// By default it is the first event in the @c ParseData, i.e. 0.
    virtual int costEvent() const;
    virtual void setCostEvent(int event);

    virtual void setParseData(const ParseData *data);
    virtual const ParseData *parseData() const;

    void setCalls(const QVector<const FunctionCall *> &calls, const Function *function);
    QVector<const FunctionCall *> calls() const;
    const Function *function() const;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    enum Columns {
        CallerColumn,
        CalleeColumn,
        CallsColumn,
        CostColumn,
        ColumnCount
    };

    enum Roles {
        FunctionCallRole = NextCustomRole
    };

private:
    class Private;
    Private *d;
};

} // namespace Callgrind
} // namespace Valgrind

#endif // VALGRIND_CALLGRIND_CALLGRINDCALLMODEL_H
