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
    CallModel();
    ~CallModel() override;

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

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

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
