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
    DataModel();
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
