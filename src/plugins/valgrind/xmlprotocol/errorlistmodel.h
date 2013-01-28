/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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

#ifndef LIBVALGRIND_PROTOCOL_ERRORLISTMODEL_H
#define LIBVALGRIND_PROTOCOL_ERRORLISTMODEL_H

#include <QAbstractItemModel>
#include <QSharedPointer>

namespace Valgrind {
namespace XmlProtocol {

class Error;
class Frame;

class ErrorListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Column {
        WhatColumn = 0,
        LocationColumn,
        AbsoluteFilePathColumn,
        LineColumn,
        UniqueColumn,
        TidColumn,
        KindColumn,
        LeakedBlocksColumn,
        LeakedBytesColumn,
        HelgrindThreadIdColumn,
        ColumnCount
    };

    enum Role {
        ErrorRole = Qt::UserRole,
        AbsoluteFilePathRole,
        FileRole,
        LineRole
    };

    class RelevantFrameFinder
    {
    public:
        virtual ~RelevantFrameFinder() {}
        virtual Frame findRelevant(const Error &error) const = 0;
    };

    explicit ErrorListModel(QObject *parent = 0);
    ~ErrorListModel();

    QSharedPointer<const RelevantFrameFinder> relevantFrameFinder() const;
    void setRelevantFrameFinder(const QSharedPointer<const RelevantFrameFinder> &finder);

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

    Error error(const QModelIndex &index) const;

    Frame findRelevantFrame(const Error &error) const;

    void clear();

public slots:
    void addError(const Valgrind::XmlProtocol::Error &error);

private:
    class Private;
    Private *const d;
};

} // namespace XmlProtocol
} // namespace Valgrind

#endif // LIBVALGRIND_PROTOCOL_ERRORLISTMODEL_H
