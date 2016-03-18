/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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

namespace Valgrind {
namespace XmlProtocol {

class Error;

class StackModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Column {
        NameColumn = 0,
        FunctionNameColumn,
        DirectoryColumn,
        FileColumn,
        LineColumn,
        InstructionPointerColumn,
        ObjectColumn,
        ColumnCount
    };

    enum Role {
        ObjectRole = Qt::UserRole,
        FunctionNameRole,
        DirectoryRole,
        FileRole,
        LineRole
    };

    explicit StackModel(QObject *parent = 0);
    ~StackModel();

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void clear();

public Q_SLOTS:
    void setError(const Valgrind::XmlProtocol::Error &error);

private:
    class Private;
    Private *const d;
};

} // namespace XmlProtocol
} // namespace Valgrind
