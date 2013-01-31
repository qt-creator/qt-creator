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

#include "stackmodel.h"
#include "error.h"
#include "frame.h"
#include "stack.h"
#include "modelhelpers.h"

#include <utils/qtcassert.h>

#include <QDir>
#include <QVector>

namespace Valgrind {
namespace XmlProtocol {

class StackModel::Private
{
public:
    Error error;

    Stack stack(int i) const
    {
        if (i < 0 || i >= error.stacks().size())
            return Stack();
        else
            return error.stacks().at(i);
    }
};

static QString makeName(const Frame &frame)
{
    const QString d = frame.directory();
    const QString f = frame.file();
    const QString fn = frame.functionName();
    if (!fn.isEmpty())
        return fn;
    if (!d.isEmpty() && !f.isEmpty())
        return frame.line() > 0 ? QString::fromLatin1("%1%2%3:%4").arg(d, QDir::separator(), f, QString::number(frame.line()))
                                : QString::fromLatin1("%1%2%3").arg(d, QDir::separator(), f);
    return frame.object();
}

StackModel::StackModel(QObject *parent)
    : QAbstractItemModel(parent)
    , d(new Private)
{
}

StackModel::~StackModel()
{
    delete d;
}

QVariant StackModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QTC_ASSERT(index.model() == this, return QVariant());

    if (!index.parent().isValid()) {
        const Stack stack = d->stack(index.row());
        if (role == Qt::DisplayRole) {
            switch (index.column()) {
            case NameColumn:
                if (!stack.auxWhat().isEmpty())
                    return stack.auxWhat();
                else
                    return d->error.what();
            default:
                return QVariant();
            }
        }
    } else {
        const Stack stack = d->stack(index.parent().row());
        const QVector<Frame> frames = stack.frames();
        const int fidx = index.row();
        if (fidx < 0 || fidx >= frames.size())
            return QVariant();
        const Frame &frame = frames[fidx];
        switch (role) {
        case Qt::DisplayRole:
        {
            switch (index.column()) {
            case NameColumn:
                return makeName(frame);
            case InstructionPointerColumn:
                return QString::fromLatin1("0x%1").arg(frame.instructionPointer(), 0, 16);
            case ObjectColumn:
                return frame.object();
            case FunctionNameColumn:
                return frame.functionName();
            case DirectoryColumn:
                return frame.directory();
            case FileColumn:
                return frame.file();
            case LineColumn:
                if (frame.line() > 0)
                    return frame.line();
                else
                    return QVariant();
            }
            break;
        }
        case Qt::ToolTipRole:
            return toolTipForFrame(frame);
        case ObjectRole:
            return frame.object();
        case FunctionNameRole:
            return frame.functionName();
        case FileRole:
            return frame.file();
        case DirectoryRole:
            return frame.directory();
        case LineRole:
            if (frame.line() > 0)
                return frame.line();
            else
                return QVariant();
        }
    }

    return QVariant();
}

QVariant StackModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case NameColumn:
        return tr("Description");
    case InstructionPointerColumn:
        return tr("Instruction Pointer");
    case ObjectColumn:
        return tr("Object");
    case FunctionNameColumn:
        return tr("Function");
    case DirectoryColumn:
        return tr("Directory");
    case FileColumn:
        return tr("File");
    case LineColumn:
        return tr("Line");
    }

    return QVariant();
}

QModelIndex StackModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        QTC_ASSERT(parent.model() == this, return QModelIndex());
        return createIndex(row, column, parent.row());
    }
    return createIndex(row, column, -1);
}

QModelIndex StackModel::parent(const QModelIndex &child) const
{
    QTC_ASSERT(!child.isValid() || child.model() == this, return QModelIndex());

    if (quintptr(child.internalId()) == quintptr(-1))
        return QModelIndex();
    return createIndex(child.internalId(), 0, -1);
}

int StackModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return d->error.stacks().size();

    QTC_ASSERT(parent.model() == this, return 0);

    const QModelIndex gp = parent.parent();

    if (!gp.isValid())
        return d->stack(parent.row()).frames().size();
    return 0;
}

int StackModel::columnCount(const QModelIndex &parent) const
{
    QTC_ASSERT(!parent.isValid() || parent.model() == this, return 0);
    return ColumnCount;
}

void StackModel::setError(const Error &error)
{
    if (d->error == error)
        return;
    beginResetModel();
    d->error = error;
    endResetModel();
}

void StackModel::clear()
{
    beginResetModel();
    d->error = Error();
    endResetModel();
}

} // namespace XmlProtocol
} // namespace Valgrind
