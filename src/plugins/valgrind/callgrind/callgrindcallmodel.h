// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "callgrindabstractmodel.h"

#include <QAbstractItemModel>

namespace Valgrind::Callgrind {

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

    void setCalls(const QList<const FunctionCall *> &calls, const Function *function);
    QList<const FunctionCall *> calls() const;
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

} // Valgrind::Callgrind
