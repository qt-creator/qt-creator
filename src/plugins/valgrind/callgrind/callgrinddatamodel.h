// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "callgrindabstractmodel.h"

#include <QAbstractItemModel>

namespace Valgrind::Callgrind {

class Function;
class ParseData;

class DataModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    DataModel();
    ~DataModel() override;

    virtual void setParseData(const ParseData *data);
    virtual const ParseData *parseData() const;

    void setVerboseToolTipsEnabled(bool enabled);
    bool verboseToolTipsEnabled() const;

    /// Only one cost event column will be shown, this decides which one it is.
    /// By default it is the first event in the @c ParseData, i.e. 0.
    virtual int costEvent() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const override;

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

} // namespace Valgrind::Callgrind
