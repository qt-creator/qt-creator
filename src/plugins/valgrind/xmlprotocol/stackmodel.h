// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractItemModel>

namespace Valgrind::XmlProtocol {

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

    explicit StackModel(QObject *parent = nullptr);
    ~StackModel() override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void clear();
    void setError(const Valgrind::XmlProtocol::Error &error);

private:
    class Private;
    Private *const d;
};

} // namespace Valgrind::XmlProtocol
