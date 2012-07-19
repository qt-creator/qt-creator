/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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

#ifndef LIBVALGRIND_PROTOCOL_STACKMODEL_H
#define LIBVALGRIND_PROTOCOL_STACKMODEL_H

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

#endif // LIBVALGRIND_PROTOCOL_STACKMODEL_H
