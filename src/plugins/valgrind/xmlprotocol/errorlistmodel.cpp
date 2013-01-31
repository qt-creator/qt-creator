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

#include "errorlistmodel.h"
#include "error.h"
#include "frame.h"
#include "stack.h"
#include "modelhelpers.h"

#include <utils/qtcassert.h>

#include <QDir>
#include <QVector>

namespace Valgrind {
namespace XmlProtocol {

class ErrorListModel::Private
{
public:
    QVector<Error> errors;
    QVariant errorData(int row, int column, int role) const;
    QSharedPointer<const ErrorListModel::RelevantFrameFinder> relevantFrameFinder;
    Frame findRelevantFrame(const Error &error) const;
    QString formatAbsoluteFilePath(const Error &error) const;
    QString formatLocation(const Error &error) const;
};

ErrorListModel::ErrorListModel(QObject *parent)
    : QAbstractItemModel(parent)
    , d(new Private)
{
}

ErrorListModel::~ErrorListModel()
{
    delete d;
}

QSharedPointer<const ErrorListModel::RelevantFrameFinder> ErrorListModel::relevantFrameFinder() const
{
    return d->relevantFrameFinder;
}

void ErrorListModel::setRelevantFrameFinder(const QSharedPointer<const RelevantFrameFinder> &finder)
{
    d->relevantFrameFinder = finder;
}

Frame ErrorListModel::findRelevantFrame(const Error &error) const
{
    return d->findRelevantFrame(error);
}

QModelIndex ErrorListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        QTC_ASSERT(parent.model() == this, qt_noop());
        return QModelIndex();
    }
    return createIndex(row, column);
}

QModelIndex ErrorListModel::parent(const QModelIndex &child) const
{
    QTC_ASSERT(!child.isValid() || child.model() == this, return QModelIndex());
    return QModelIndex();
}

Frame ErrorListModel::Private::findRelevantFrame(const Error &error) const
{
    if (relevantFrameFinder)
        return relevantFrameFinder->findRelevant(error);
    const QVector<Stack> stacks = error.stacks();
    if (stacks.isEmpty())
        return Frame();
    const Stack &stack = stacks[0];
    const QVector<Frame> frames = stack.frames();
    if (!frames.isEmpty())
        return frames.first();
    return Frame();
}

QString ErrorListModel::Private::formatAbsoluteFilePath(const Error &error) const
{
    const Frame f = findRelevantFrame(error);
    if (!f.directory().isEmpty() && !f.file().isEmpty())
        return QString(f.directory() + QDir::separator() + f.file());
    return QString();
}

QString ErrorListModel::Private::formatLocation(const Error &error) const
{
    const Frame frame = findRelevantFrame(error);
    const QString file = frame.file();
    if (!frame.functionName().isEmpty())
        return frame.functionName();
    if (!file.isEmpty()) {
        const qint64 line = frame.line();
        if (line > 0)
            return QString::fromLatin1("%1:%2").arg(file, QString::number(frame.line()));
        return file;
    }
    return frame.object();
}

QVariant ErrorListModel::Private::errorData(int row, int column, int role) const
{
    // A dummy entry.
    if (row == 0 && errors.isEmpty()) {
        if (role == Qt::DisplayRole)
            return tr("No errors found");
        if (role == ErrorRole)
            return tr("No errors found");
        return QVariant();
    }

    if (row < 0 || row >= errors.size())
        return QVariant();

    const Error &error = errors.at(row);

    if (error.stacks().count())
    switch (role) {
    case Qt::DisplayRole: {
        switch (column) {
        case WhatColumn:
            return error.what();
        case LocationColumn:
            return formatLocation(error);
        case AbsoluteFilePathColumn:
            return formatAbsoluteFilePath(error);
        case LineColumn: {
            const qint64 line = findRelevantFrame(error).line();
            return line > 0 ? line : QVariant();
        }
        case UniqueColumn:
            return error.unique();
        case TidColumn:
            return error.tid();
        case KindColumn:
            return error.kind();
        case LeakedBlocksColumn:
            return error.leakedBlocks();
        case LeakedBytesColumn:
            return error.leakedBytes();
        case HelgrindThreadIdColumn:
            return error.helgrindThreadId();
        default:
            break;
        }
    }
    case Qt::ToolTipRole:
        return toolTipForFrame(findRelevantFrame(error));
    case ErrorRole:
        return QVariant::fromValue<Error>(error);
    case AbsoluteFilePathRole:
        return formatAbsoluteFilePath(error);
    case FileRole:
        return findRelevantFrame(error).file();
    case LineRole: {
        const qint64 line = findRelevantFrame(error).line();
        return line > 0 ? line : QVariant();
    }
    }
    return QVariant();
}

QVariant ErrorListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (!index.parent().isValid())
        return d->errorData(index.row(), index.column(), role);

    return QVariant();
}

QVariant ErrorListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case WhatColumn:
        return tr("What");
    case LocationColumn:
        return tr("Location");
    case AbsoluteFilePathColumn:
        return tr("File");
    case LineColumn:
        return tr("Line");
    case UniqueColumn:
        return tr("Unique");
    case TidColumn:
        return tr("Thread ID");
    case KindColumn:
        return tr("Kind");
    case LeakedBlocksColumn:
        return tr("Leaked Blocks");
    case LeakedBytesColumn:
        return tr("Leaked Bytes");
    case HelgrindThreadIdColumn:
        return tr("Helgrind Thread ID");
    }

    return QVariant();
}

int ErrorListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return qMax(1, d->errors.count());
}

int ErrorListModel::columnCount(const QModelIndex &parent) const
{
    QTC_ASSERT(!parent.isValid() || parent.model() == this, return 0);
    return ColumnCount;
}

bool ErrorListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    QTC_ASSERT(!parent.isValid() || parent.model() == this, return false);

    if (row < 0 || row + count > d->errors.size() || parent.isValid())
        return false;

    beginRemoveRows(parent, row, row + count);
    d->errors.remove(row, count);
    endRemoveRows();
    return true;
}

void ErrorListModel::addError(const Error &error)
{
    beginInsertRows(QModelIndex(), d->errors.size(), d->errors.size());
    d->errors.push_back(error);
    endInsertRows();
}

Error ErrorListModel::error(const QModelIndex &index) const
{
    if (!index.isValid())
        return Error();

    QTC_ASSERT(index.model() == this, return Error());

    const int r = index.row();
    if (r < 0 || r >= d->errors.size())
        return Error();
    return d->errors.at(r);
}

void ErrorListModel::clear()
{
    beginResetModel();
    d->errors.clear();
    endResetModel();
}

} // namespace XmlProtocol
} // namespace Valgrind
