// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stackmodel.h"
#include "error.h"
#include "frame.h"
#include "stack.h"
#include "../valgrindtr.h"

#include <utils/qtcassert.h>

namespace Valgrind::XmlProtocol {

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
    const QString f = frame.fileName();
    const QString fn = frame.functionName();
    if (!fn.isEmpty())
        return fn;
    if (!d.isEmpty() && !f.isEmpty())
        return frame.line() > 0 ? QString::fromLatin1("%1/%2:%3").arg(d, f).arg(frame.line())
                                : QString::fromLatin1("%1/%2").arg(d, f);
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
        const QList<Frame> frames = stack.frames();
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
                return frame.fileName();
            case LineColumn:
                if (frame.line() > 0)
                    return frame.line();
                else
                    return QVariant();
            }
            break;
        }
        case Qt::ToolTipRole:
            return frame.toolTip();
        case ObjectRole:
            return frame.object();
        case FunctionNameRole:
            return frame.functionName();
        case FileRole:
            return frame.fileName();
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
        return Tr::tr("Description");
    case InstructionPointerColumn:
        return Tr::tr("Instruction Pointer");
    case ObjectColumn:
        return Tr::tr("Object");
    case FunctionNameColumn:
        return Tr::tr("Function");
    case DirectoryColumn:
        return Tr::tr("Directory");
    case FileColumn:
        return Tr::tr("File");
    case LineColumn:
        return Tr::tr("Line");
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

} // namespace Valgrind::XmlProtocol
